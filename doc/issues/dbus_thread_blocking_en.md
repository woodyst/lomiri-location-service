Title: The service stops answering D-Bus when the GPS HAL stalls, because start_positioning() makes blocking calls on the dispatch thread

## Problem

Under certain conditions (GPS HAL busy or stalled; reliably with Waydroid, which
holds the HAL), the service stops answering **all** D-Bus calls: it appears frozen
to every client until the HAL returns.

## Cause

`start_positioning()` runs on the thread that handles D-Bus calls (the
`StartPositionUpdates` handler: `skeleton` → … →
`gps::Provider::start_position_updates` → `hal->start_positioning()`), and it makes
**synchronous, potentially blocking** calls into the Android HAL:

```cpp
bool ...::start_positioning()
{
    impl.register_callbacks();                     // u_hardware_gps_delete + u_hardware_gps_new
    return u_hardware_gps_start(impl.gps_handle);  // starts the chipset
}
```

`register_callbacks()` (delete the handle + create a new one) and
`u_hardware_gps_start()` can take a long time or stall inside the HAL. Since they
run on the dispatch thread, while they are in progress the service handles no other
D-Bus request → it appears frozen. A D-Bus method handler should not make
unbounded blocking calls into the HAL.

## Fix

Move the blocking calls off the dispatch thread: run `register_callbacks()` +
`u_hardware_gps_start()` in a separate (*detached*) thread, so that
`start_positioning()` returns immediately and the D-Bus dispatch is never blocked.
When the handle is already valid, `u_hardware_gps_start()` can be called directly
without re-registering (fast path). The same applies to `stop_positioning()`.
