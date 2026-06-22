# Navius patches

This document describes changes made in the `navius` branch on top of the
upstream `lomiri-location-service` tree.  All patches are in the `main` branch
and the resulting binary is versioned `3.4.1+navius6`.

---

## 1. Waydroid GPS callback race condition fix (`shared_mutex`)

**Commit:** `bc3b7bd`  
**File:** `src/location_service/com/lomiri/location/providers/gps/android_hardware_abstraction_layer.cpp` (and `.h`)

### Background

On devices running Waydroid in HALIUM\_10 mode two Android containers coexist:

- `lxc.payload.android` – the Halium layer that owns the real GNSS HAL.
- `lxc.payload.waydroid` – the Waydroid container, which mounts
  `/dev/hwbinder` from the host as `dev/host_hwbinder` and uses it to
  register its own GPS HAL callbacks through `host_hwbinder`.

Waydroid calls `register_callbacks()` asynchronously at any time.
`register_callbacks()` deletes the current GPS handle and creates a new one.
If a GPS callback (e.g. `on_location_update`) fires between
`u_hardware_gps_delete()` and `u_hardware_gps_new()` it dereferences the
freed handle → **SIGSEGV**.

### Fix

A `std::shared_mutex callback_mutex` was added to `Impl`:

```cpp
// android_hardware_abstraction_layer.h  (inside struct Impl)
std::shared_mutex callback_mutex;
```

`register_callbacks()` acquires an **exclusive** (`unique_lock`) so that
all in-flight callbacks finish before the handle is replaced.  Each of the
seven GPS callbacks acquires a **shared** (`shared_lock`) so they can run
concurrently with each other but not with `register_callbacks()`:

```cpp
// callbacks: on_location_update, on_sv_status_update, on_status_update,
//            on_set_capabilities, on_agps_status_update,
//            on_xtra_download_request, on_request_utc_time
static void on_location_update(UHardwareGpsLocation* location, void* context)
{
    auto thiz = static_cast<android::HardwareAbstractionLayer*>(context);
    if (thiz) {
        std::shared_lock<std::shared_mutex> lock(thiz->impl.callback_mutex);
        // ... use thiz->impl fields safely
    }
}
```

---

## 2. EDEADLK fix in `register_callbacks()` on service start

**Commit:** `4369d8b`  
**File:** `src/location_service/com/lomiri/location/providers/gps/android_hardware_abstraction_layer.cpp`

### Problem

The initial implementation of fix #1 held the exclusive lock across the
entire `u_hardware_gps_delete + u_hardware_gps_new` sequence.
`u_hardware_gps_new()` can invoke callbacks (e.g. `on_set_capabilities`)
**synchronously on the calling thread**.  That thread already held the write
lock, so the callback's attempt to take a shared lock triggered:

```
terminate called after throwing an instance of 'std::system_error'
  what():  Resource deadlock avoided
```

POSIX `pthread_rwlock_rdlock` returns `EDEADLK` when the calling thread
already holds the write lock on the same rwlock.

### Fix

The lock is split into three phases so the handle creation runs lock-free:

```cpp
void android::HardwareAbstractionLayer::Impl::register_callbacks()
{
    // Phase 1: wait for in-flight callbacks, then delete the old handle.
    {
        std::unique_lock<std::shared_mutex> lock(callback_mutex);
        if (gps_handle)
            u_hardware_gps_delete(gps_handle);
        gps_handle = nullptr;
    }

    // Phase 2: create the new handle WITHOUT the lock.
    // u_hardware_gps_new() may invoke callbacks synchronously on this thread;
    // holding the write lock here would cause EDEADLK.
    UHardwareGps new_handle = u_hardware_gps_new(std::addressof(gps_params));

    // Phase 3: install the new handle and push configuration.
    {
        std::unique_lock<std::shared_mutex> lock(callback_mutex);
        gps_handle = new_handle;
        dispatch_updated_modes_to_driver();
    }
}
```

Between phase 1 and phase 2 `gps_handle` is `nullptr`.  Callbacks do not
use `gps_handle` directly (they use other `Impl` fields such as `updates`),
so setting it to null before the lock is released is safe.

---

## 3. `GetVisibleSpaceVehicles` D-Bus method

**Commit:** `bc3b7bd`  
**Files:**
- `include/location_service/com/lomiri/location/service/interface.h`
- `src/location_service/com/lomiri/location/service/skeleton.cpp`
- `src/location_service/com/lomiri/location/service/implementation.cpp`

### What it does

Adds a synchronous D-Bus method to the service interface that returns the
current set of visible GNSS space vehicles (satellites).  Clients that
cannot subscribe to property-change signals (e.g. QML via `lomiri-location`
bindings) can call this method to get a one-shot snapshot.

### Interface (`interface.h`)

```cpp
struct GetVisibleSpaceVehicles
{
    typedef com::lomiri::location::service::Interface Interface;
    inline static const std::string& name() {
        static const std::string s{"GetVisibleSpaceVehicles"};
        return s;
    }
    typedef std::map<com::lomiri::location::SpaceVehicle::Key,
                     com::lomiri::location::SpaceVehicle> ResultType;
    inline static const std::chrono::milliseconds default_timeout() {
        return std::chrono::seconds{5};
    }
};
```

A matching read-only D-Bus property `VisibleSpaceVehicles` was also added so
that the current value is always accessible without calling the method:

```cpp
struct VisibleSpaceVehicles {
    typedef std::map<SpaceVehicle::Key, SpaceVehicle> ValueType;
    static const bool readable = true;
    static const bool writable = false;
};
```

### Skeleton (`skeleton.cpp`)

The method handler replies immediately with the current property value
(poll-based, no subscription required):

```cpp
object->install_method_handler<clls::Interface::GetVisibleSpaceVehicles>(
    [this](const dbus::Message::Ptr& msg)
    {
        auto reply = dbus::Message::make_method_return(msg);
        reply->writer() << visible_space_vehicles().get();
        this->configuration.incoming->send(reply);
    });
```

### Implementation (`implementation.cpp`)

The property is kept up to date by forwarding engine SVS updates through
the D-Bus dispatcher thread:

```cpp
configuration.engine->updates.visible_space_vehicles.changed().connect(
    [this](const std::map<cll::SpaceVehicle::Key, cll::SpaceVehicle>& svs)
    {
        configuration.dispatcher([this, svs]() {
            visible_space_vehicles() = svs;
        });
    });
```

### D-Bus call example

```bash
dbus-send --system --dest=com.lomiri.location.Service \
          --print-reply \
          /com/lomiri/location/Service \
          com.lomiri.location.Service.Interface.GetVisibleSpaceVehicles
```

---

## 4. Docker-based ARM64 `.deb` build (`build-deb.sh`)

**Commit:** `6f270d3` (dependencies fixed in `4369d8b`)

### Purpose

Builds a deployable `liblomiri-location-service3_<version>_arm64.deb` inside
an isolated Ubuntu 24.04 container with the UBports repository.  The host
system is not modified.  The script works from any ARM64 machine that has
Docker running.

### Usage

```bash
cd lomiri-location-service
bash build-deb.sh              # output to ./debs/
bash build-deb.sh /path/to/out # output to custom directory
```

The script:

1. Builds (or reuses a cached) Docker image `lomiri-location-service-builder`
   with all build dependencies installed from Ubuntu 24.04 and
   `http://repo.ubports.com/ 24.04-1.x`.
2. Mounts the source tree read-only at `/src` and the output directory at
   `/output`.
3. Bumps the changelog version to `<upstream>+navius1` via `dch` if not
   already present.
4. Runs `dpkg-buildpackage -b -uc -us --build-profiles=nocheck,nodoc`.
5. Copies all generated `.deb` files to the output directory.

### Deploy to device

```bash
scp debs/liblomiri-location-service3_*_arm64.deb phablet@<device>:/tmp/
ssh root@<device> "mount -o remount,rw / && \
    dpkg -i /tmp/liblomiri-location-service3_*_arm64.deb && \
    systemctl restart lomiri-location-service"
```

---

## 5. GPS watchdog + position mode in fast path (navius4)

**Commits:** `8c74e5d`  
**Files:** `android_hardware_abstraction_layer.cpp`, `android_hardware_abstraction_layer.h`

### Background

Two problems remained after navius3:

1. **Waydroid calls `u_hardware_gps_stop()` on close.** When the Waydroid
   container shuts down it calls `u_hardware_gps_stop()` on the shared GPS HAL,
   stopping GPS globally. LLS's `gps_handle` remains non-null so the navius3
   fast path never detected the stall. GPS stayed frozen until LLS was manually
   restarted.

2. **No position fix despite 37+ satellites.** `dispatch_updated_modes_to_driver()`
   (which sets position mode via `u_hardware_gps_set_position_mode()`) was called
   in `register_callbacks()` phase 3 but **not** in the navius3 fast path. The
   chipset tracked satellites but never computed fixes.

### Fix 1 — GPS watchdog thread

A detached watchdog thread is started in the `Impl` constructor:

```cpp
// 5-second tick; 10-second stale threshold
std::thread([this]() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        uint64_t last = last_gps_ms.load(std::memory_order_relaxed);
        if (last == 0) continue;                      // GPS not started
        if (now_ms() - last <= 10000) continue;       // still fresh
        // ... acquire positioning_active flag, re-register + restart GPS
    }
}).detach();
```

`last_gps_ms` (a new `std::atomic<uint64_t>` field in `Impl`) is updated by
`on_location_update` and `on_sv_status_update`, and reset to 0 by
`stop_positioning()`. When more than 10 s pass without any GPS data, the
watchdog re-runs `register_callbacks()` and restarts the chipset — recovering
GPS without restarting LLS.

**Verified:** Waydroid and navius now both position simultaneously. After
Waydroid opens and steals the HAL, the watchdog reclaims callbacks within 10 s
and GPS accuracy converges back to ~4 m while Waydroid continues positioning.

### Fix 2 — `dispatch_updated_modes_to_driver()` in fast path

```cpp
if (handle) {
    impl.dispatch_updated_modes_to_driver();   // ← added
    u_hardware_gps_start(handle);
    return true;
}
```

### Additional: `on_location_update` trace

Each position fix now logs:

```
[lls] on_location_update: flags=0x77 lat=37.71388 lon=-2.99852 acc=3.8
```

### UBports-specific build dependencies

These packages are only available from `repo.ubports.com` and must be
present in the Docker image:

| Package | Purpose |
|---|---|
| `libdbus-cpp-dev` | D-Bus C++ bindings |
| `libprocess-cpp-dev` | Process/threading utilities |
| `libproperties-cpp-dev` | Property/signal library |
| `libtrust-store-dev` / `trust-store-bin` | Permission management |
| `libubuntu-platform-hardware-api-dev` | Android HAL bridge (libhybris) |
| `qt6-declarative-private-dev` | Qt6 QML private headers |
| `qt6-location-dev` / `qt6-positioning-dev` | Qt6 location stack |

---

## 6. Location indicator fix + non-blocking `start_positioning()` (navius6)

**Commits:** `8a78b42`, `bd0ccef`  
**Files:** `engine.cpp`, `android_hardware_abstraction_layer.cpp`, `build-deb.sh`

### Bug 1 — Location indicator never appears (`engine.cpp`)

**Symptom:** The location indicator in the UBports notification bar never
appeared while Navius was using GPS.

**Root cause:** In `Engine::add_provider()`, the callback that checks whether
any provider is active contained a logic error:

```cpp
// Before (broken): only the LAST provider in the map determines is_any_active
for (const auto& pair : providers)
    is_any_active = pair.first->state() == StateTrackingProvider::State::active;
```

With two providers (`gps::Provider` and `remote::Provider`), iteration order
over the `std::map` is deterministic but the wrong result occurs whenever
`remote::Provider` (which stays in `enabled` state when not used) is last:
`is_any_active` becomes `false` even though `gps::Provider` is `active` →
`Engine::Status` never reaches `active` → the D-Bus `State` property stays
`"enabled"` → the indicator is never shown.

**Fix:**

```cpp
// After: accumulate with OR so any active provider wins
for (const auto& pair : providers)
    is_any_active |= (pair.first->state() == StateTrackingProvider::State::active);
```

### Bug 2 — LLS hangs intermittently (`android_hardware_abstraction_layer.cpp`)

**Symptom:** `lomiri-location-serviced` appeared frozen — Navius could not
get GPS updates, and the D-Bus session became unresponsive for tens of
seconds.

**Root cause:** `start_positioning()` fast path acquired a **blocking**
`shared_lock` on `callback_mutex`:

```cpp
// Before (broken): blocks the D-Bus thread if watchdog holds write lock
std::shared_lock<std::shared_mutex> lock(impl.callback_mutex);  // ← blocks
handle = impl.gps_handle;
```

The watchdog thread holds the **write** lock (exclusive) during
`register_callbacks()` phase 1 while calling `u_hardware_gps_delete()`.
That HAL call can itself stall for several seconds on some devices.
Any D-Bus method that reached `start_positioning()` during this window
blocked the entire D-Bus dispatch thread until the watchdog released
the lock.

**Fix:** use `std::try_to_lock` so the D-Bus thread is never blocked:

```cpp
// After: non-blocking; if watchdog holds the lock, return immediately
std::shared_lock<std::shared_mutex> lock(impl.callback_mutex, std::try_to_lock);
if (!lock.owns_lock()) {
    // Watchdog is reclaiming GPS; it will call u_hardware_gps_start() when done.
    return true;
}
```

**Additional fix — `register_callbacks()` phase 3:**

`dispatch_updated_modes_to_driver()` (which calls
`u_hardware_gps_set_position_mode()`) was previously called while holding the
write lock in phase 3.  Some HAL implementations invoke `on_set_capabilities`
**synchronously** from within `u_hardware_gps_set_position_mode()`; that
callback tries to acquire a `shared_lock` on the same mutex → deadlock.

```cpp
// After: install handle under lock, then configure without it
{
    std::unique_lock<std::shared_mutex> lock(callback_mutex);
    gps_handle = new_handle;
}
if (gps_handle)
    dispatch_updated_modes_to_driver();  // no lock held — callbacks can run
```

**Additional fix — `stop_positioning()` null handle guard:**

`u_hardware_gps_stop()` was called unconditionally even when `gps_handle`
was `nullptr` (possible during watchdog recovery). Fixed with a
`try_to_lock` guard consistent with `start_positioning()`.
