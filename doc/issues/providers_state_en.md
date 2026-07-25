Title: Location indicator intermittently missing while GPS is active (the "any provider active" check only considers the last provider)

## Problem

The location indicator in the top bar does not appear even though GPS is actively
positioning. It is intermittent: on some boots or devices it shows up, on others
it doesn't. Apps still receive positions normally; what fails is the visual cue,
because the D-Bus `State` property stays at `"enabled"` instead of switching to
`"active"`.

LLS exposes the D-Bus `State` property, which the shell uses to light up the
location indicator in the top bar. The engine should report `State = active`
when **at least one** provider is delivering position, regardless of which one
(for example, I have a GPS fix even though the network provider is idle), and
`enabled` when none is.

## Solution

In src/location_service/com/lomiri/location/engine.cpp, line 217:

```cpp
is_any_active = pair.first->state() == StateTrackingProvider::State::active;
```

is reassigned on every iteration of the loop (once per provider), so it ends up
returning only the value of the last provider instead of `active` when any of
them is active.

The fix is this:

```cpp
is_any_active |= pair.first->state() == StateTrackingProvider::State::active;
```

because we start from false and switch to true if ***any*** of the providers
(GPS, geoclue, etc.) has a fix.
