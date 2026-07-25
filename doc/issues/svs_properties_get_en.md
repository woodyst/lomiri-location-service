Title: An application cannot query the list of visible satellites (VisibleSpaceVehicles) via Properties.Get: the property is never populated and the AppArmor policy group blocks it

## Problem

An application —typically confined, e.g. via the `lomiri-location` Qt plugin—
cannot obtain the list of visible satellites. The standard way is to read the
`VisibleSpaceVehicles` property with `org.freedesktop.DBus.Properties.Get`
(polling, without relying on `PropertiesChanged`), but it runs into **two**
barriers.

## Cause

**1. LLS — the property is never populated.** The engine listens for satellites
on `provider->updates().svs`, but each provider is wrapped in a
`StateTrackingProvider`
(`src/location_service/com/lomiri/location/state_tracking_provider.h`), and that
wrapper forwards only `position`, `heading` and `velocity` from the inner
provider — **not `svs`**:

```cpp
connections
{
    impl_->updates().position.connect([this](...){ mutable_updates().position(u); }),
    impl_->updates().heading.connect(...),
    impl_->updates().velocity.connect(...),
    // svs missing
}
```

Since the wrapper never emits `svs`, the engine never receives satellites, the
`VisibleSpaceVehicles` property is never populated, and `Properties.Get` returns
`[]`.

**2. AppArmor — confined apps cannot call `Properties.Get`.** The `location`
policy group (from `apparmor-easyprof-ubuntu`) only allows the
`com.lomiri.location.Service` interface, not `org.freedesktop.DBus.Properties`. A
confined app gets `AccessDenied` on `Properties.Get`, even when LLS works.
(This is why a proprietary method on the service interface "seemed" to work for
confined apps while `Properties.Get` did not.)

## Fix

**1. LLS (this repo):** also forward `svs` in `StateTrackingProvider` (and add its
`core::ScopedConnection svs_updates;` to the `connections` struct):

```cpp
impl_->updates().svs.connect([this](const Update<std::set<SpaceVehicle>>& u)
{
    mutable_updates().svs(u);
}),
```

This populates the property and makes it readable via the standard
`Properties.Get` — with no need for a custom D-Bus method.

**2. `apparmor-easyprof-ubuntu` (separate repo):** add to the `location` policy
group the rule that allows `Get` of `org.freedesktop.DBus.Properties` towards the
service:

```
dbus (send)
     bus=system
     path="/com/lomiri/location/Service"
     interface="org.freedesktop.DBus.Properties"
     member="Get"
     peer=(name="com.lomiri.location.Service",label=unconfined),
```

With both, a confined app can poll the satellites via standard `Properties.Get`.

## Notes

- Emitting `PropertiesChanged` with the SVS map causes a SIGBUS when encoding it
  as a `Variant` on this platform; that's why clients must **poll** via
  `Properties.Get` rather than subscribe. `Properties.Get` (a one-shot read) does
  work once the property is populated.
- `azimuth` and `elevation` are swapped in the HAL's `on_sv_status_update` —
  separate fix.

## Related

Part 2 (AppArmor) belongs to another project and has its own issue:
`apparmor_location_properties_get.md` (project **apparmor-easyprof-ubuntu**). The
full fix needs both pieces: the one in this repo (populate the property) and the
policy group one.
