Title: Service crashes (SIGSEGV) due to a race condition between register_callbacks() and the GPS callbacks

## Problem

The service crashes with SIGSEGV. It is reliably reproducible with Waydroid
(HALIUM_10 mode), where two Android containers coexist and both register GPS
callbacks, but it is a generic race condition in the code.

## Cause

In `src/location_service/com/lomiri/location/providers/gps/android_hardware_abstraction_layer.cpp`,
`register_callbacks()` frees and recreates the GPS handle **with no
synchronization**:

```cpp
if (gps_handle)
    u_hardware_gps_delete(gps_handle);          // frees the handle
gps_handle = u_hardware_gps_new(&gps_params);   // creates a new one
```

The HAL callbacks (`on_location_update`, `on_sv_status_update`, etc.) run on
another thread and dereference `gps_handle` without a lock. If a callback fires
between the `delete` and the reassignment —or during `u_hardware_gps_new()`,
which can invoke callbacks— it touches already-freed memory → use-after-free →
SIGSEGV.

## Fix

Protect `gps_handle` with a `std::shared_mutex`: a `shared_lock` in the callbacks
(concurrent reads) and a `unique_lock` in `register_callbacks()` while the handle
is being manipulated.

Important: do **not** hold the exclusive lock across `u_hardware_gps_new()` or
`set_position_mode()`, because they can invoke callbacks synchronously on the
same thread; since `std::shared_mutex` is not reentrant, the callback would ask
for the `shared_lock` while already holding the exclusive one → EDEADLK. Hence
it is split into phases:

1. `unique_lock` → delete the old handle → `gps_handle = nullptr` → release.
2. `u_hardware_gps_new()` **without** the lock.
3. `unique_lock` → install the new handle → release.
4. Push the configuration **without** the lock.

This way the callbacks never see a dangling handle (outside the locked phases the
handle is either `nullptr` or valid, never freed-but-not-null) and there is no
reentrancy under the exclusive lock.
