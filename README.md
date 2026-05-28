# lomiri-location-service — navius fork

A fork of [UBports' lomiri-location-service](https://gitlab.com/ubports/development/core/lomiri-location-service)
with patches developed for [Navius](https://github.com/woodyst/navius), a GPS
navigation app for Ubuntu Touch.

The upstream service is the system daemon that multiplexes access to GNSS
hardware on Ubuntu Touch devices. This fork adds bug fixes and features needed
for reliable positioning with Navius, particularly on devices that run
[Waydroid](https://waydroid.io) alongside the host system.

---

## Patches

### navius1 — Waydroid GPS callback race condition (SIGSEGV fix)

On devices running Waydroid in HALIUM\_10 mode, two Android containers coexist:
the Halium layer that owns the real GNSS HAL and the Waydroid container, which
registers its own GPS callbacks through `host_hwbinder`.

`register_callbacks()` deletes the current GPS handle and creates a new one.
If a GPS callback (e.g. `on_location_update`) fires between
`u_hardware_gps_delete()` and `u_hardware_gps_new()` it dereferences the freed
handle → **SIGSEGV**.

**Fix:** a `std::shared_mutex callback_mutex` protects all seven GPS callbacks
(shared lock, concurrent-safe) against `register_callbacks()` (exclusive lock).

---

### navius2 — EDEADLK fix in `register_callbacks()`

The initial mutex fix held an exclusive lock across the entire
`delete + new` sequence. `u_hardware_gps_new()` can invoke callbacks
synchronously on the calling thread — that thread already held the write lock,
so the re-entrant shared lock triggered `EDEADLK` at service start.

**Fix:** the lock is split into three phases so handle creation runs lock-free:

1. `unique_lock` → delete old handle → release
2. `u_hardware_gps_new()` **without** lock (re-entrant callbacks are fine here)
3. `unique_lock` → install new handle → dispatch position mode → release

---

### navius2 — `GetVisibleSpaceVehicles` D-Bus method

QML clients that cannot maintain a persistent D-Bus signal subscription (e.g.
via the `lomiri-location` Qt plugin) need a way to poll the current set of
visible GNSS satellites.

**Added:**
- `Interface::GetVisibleSpaceVehicles` — synchronous D-Bus method that returns
  the current satellite map.
- `Interface::VisibleSpaceVehicles` — read-only D-Bus property kept up to date
  by the engine's SVS update signal.

```bash
dbus-send --system --dest=com.lomiri.location.Service \
          --print-reply \
          /com/lomiri/location/Service \
          com.lomiri.location.Service.Interface.GetVisibleSpaceVehicles
```

---

### navius3 — Non-blocking `start_positioning()` via `Qt::QueuedConnection`

`startUpdates()` must be called on the main Qt thread that owns the D-Bus event
loop. If called from a background thread the internal `QEventLoop` blocks
forever and no GPS fixes ever arrive.

**Fix:** `startUpdates()` is dispatched via `QMetaObject::invokeMethod` with
`Qt::QueuedConnection`, guaranteeing it runs on the correct thread regardless
of the caller.

---

### navius4 — GPS watchdog + position mode in fast path

Two problems remained after navius3:

1. **Waydroid calls `u_hardware_gps_stop()` on close.** When the Waydroid
   container shuts down it stops GPS globally. LLS's handle remained non-null
   so the stall was never detected — GPS stayed frozen until LLS was restarted
   manually.

2. **No position fix despite 37+ satellites.** `dispatch_updated_modes_to_driver()`
   was missing from the fast path: the chipset tracked satellites but never
   computed fixes.

**Fix 1 — watchdog thread:** a detached thread (5 s tick, 10 s stale threshold)
monitors `last_gps_ms` (updated by every GPS callback). On stale detection it
re-runs `register_callbacks()` and restarts the chipset, recovering GPS within
10 s without restarting LLS.

**Fix 2 — position mode in fast path:** `dispatch_updated_modes_to_driver()` is
now called before `u_hardware_gps_start()` so the chipset always receives the
correct position mode on every start.

**Verified:** Navius and Waydroid position simultaneously. After Waydroid opens
and steals the HAL, the watchdog reclaims callbacks within 10 s and GPS accuracy
converges back to ~4 m.

---

### navius5 — Shared debug header (`lls_trace.h`)

All GPS trace logging is gated behind an `LLS_DEBUG` constant defined in
`include/location_service/com/lomiri/location/lls_trace.h`. Set it to `true`
and rebuild to enable verbose per-fix and per-SV logs; no traces are emitted in
production builds.

---

## Building

The included `build-deb.sh` builds a deployable `.deb` inside an isolated
Ubuntu 24.04 Docker container with the UBports repository. The host system is
not modified. Requires Docker and an ARM64 host (or emulation).

```bash
bash build-deb.sh              # output to ./debs/
bash build-deb.sh /path/to/out # output to a custom directory
```

The script builds `liblomiri-location-service3`, `lomiri-location-service-bin`,
`lomiri-location-service-qt5-plugin`, and `lomiri-location-service-qt6-plugin`.

---

## Installing on device

```bash
# Copy the relevant packages to the device
scp debs/liblomiri-location-service3_*_arm64.deb \
    debs/lomiri-location-service-bin_*_arm64.deb \
    debs/lomiri-location-service-qt5-plugin_*_arm64.deb \
    phablet@<device>:/tmp/

# Install (requires rw remount on Ubuntu Touch)
ssh root@<device> "mount -o remount,rw / && \
    dpkg -i /tmp/*lomiri*.deb && \
    mount -o remount,ro / && \
    systemctl restart lomiri-location-service"
```

To revert to the UBports stock package:

```bash
ssh phablet@<device> "sudo apt install --reinstall lomiri-location-service"
```

---

## Enabling debug traces

Edit `include/location_service/com/lomiri/location/lls_trace.h`:

```cpp
constexpr bool LLS_DEBUG = true;   // false in production
```

Rebuild and reinstall, then watch logs:

```bash
ssh phablet@<device> "journalctl -f -u lomiri-location-service"
```

---

## Upstream

The canonical repository is maintained by UBports:
https://gitlab.com/ubports/development/core/lomiri-location-service

The bug fixes in navius1–2 (mutex/EDEADLK) and the `GetVisibleSpaceVehicles`
D-Bus method are candidates for upstream contribution. See
[`doc/contributing-upstream.md`](doc/contributing-upstream.md) for details.
