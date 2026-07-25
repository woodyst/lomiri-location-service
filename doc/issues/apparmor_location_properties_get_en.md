Title: The `location` policy group does not let confined apps read properties of the location service (org.freedesktop.DBus.Properties.Get)

> Project: **apparmor-easyprof-ubuntu** (not lomiri-location-service).
> Companion issue (in LLS): `svs_properties_get.md` — the full fix needs both pieces.

## Problem

A confined app using the `location` policy group cannot read properties of the
location service via `org.freedesktop.DBus.Properties.Get`: it gets
`AccessDenied`. In particular, it cannot poll `VisibleSpaceVehicles` (the list of
visible satellites), which is the standard way to obtain it.

## Cause

The `location` policy group (package `apparmor-easyprof-ubuntu`,
`/usr/share/apparmor/easyprof/policygroups/ubuntu/1.0/location`) only allows the
`com.lomiri.location.Service` interface, but **not**
`org.freedesktop.DBus.Properties`. So the standard `Properties.Get`/`GetAll` calls
on the service object are blocked for confined apps.

Verified: `aa-exec -p <app-profile> -- dbus-send ... Properties.Get` →
`AccessDenied`; after adding the rule → OK.

## Fix

Add to the `location` policy group the rule that allows `Get` of
`org.freedesktop.DBus.Properties` towards the service:

```
dbus (send)
     bus=system
     path="/com/lomiri/location/Service"
     interface="org.freedesktop.DBus.Properties"
     member="Get"
     peer=(name="com.lomiri.location.Service",label=unconfined),
```

(And likewise `member="GetAll"` if reading all properties at once should be
allowed.)

## Related

This fix is one of the two pieces needed for a confined app to obtain the list of
satellites. The other one is in **lomiri-location-service**: the
`VisibleSpaceVehicles` property is never populated because `StateTrackingProvider`
does not forward `svs` to the engine. See the companion issue
`svs_properties_get.md`.
