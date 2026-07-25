Título: Una aplicación no puede consultar la lista de satélites visibles (VisibleSpaceVehicles) vía Properties.Get: la propiedad no se puebla y el policy group AppArmor la bloquea

## Problema

Una aplicación —típicamente confinada, p. ej. vía el plugin Qt `lomiri-location`—
no puede obtener la lista de satélites visibles. La vía estándar es leer la
propiedad `VisibleSpaceVehicles` con `org.freedesktop.DBus.Properties.Get`
(sondeo, sin depender de `PropertiesChanged`), pero se topa con **dos** barreras.

## Causa

**1. LLS — la propiedad nunca se puebla.** El engine escucha los satélites en
`provider->updates().svs`, pero cada provider se envuelve en un
`StateTrackingProvider`
(`src/location_service/com/lomiri/location/state_tracking_provider.h`), y ese
wrapper reenvía del provider interno solo `position`, `heading` y `velocity`
— **no `svs`**:

```cpp
connections
{
    impl_->updates().position.connect([this](...){ mutable_updates().position(u); }),
    impl_->updates().heading.connect(...),
    impl_->updates().velocity.connect(...),
    // falta svs
}
```

Como el wrapper nunca emite `svs`, el engine nunca recibe satélites, la propiedad
`VisibleSpaceVehicles` nunca se puebla y `Properties.Get` devuelve `[]`.

**2. AppArmor — las apps confinadas no pueden llamar `Properties.Get`.** El policy
group `location` (de `apparmor-easyprof-ubuntu`) solo permite la interfaz
`com.lomiri.location.Service`, no `org.freedesktop.DBus.Properties`. Una app
confinada recibe `AccessDenied` al hacer `Properties.Get`, aunque LLS funcione.
(Por eso un método propietario sobre la interfaz del servicio "parecía" funcionar
para apps confinadas mientras que `Properties.Get` no.)

## Solución

**1. LLS (este repo):** reenviar también `svs` en `StateTrackingProvider`
(y añadir su `core::ScopedConnection svs_updates;` al struct `connections`):

```cpp
impl_->updates().svs.connect([this](const Update<std::set<SpaceVehicle>>& u)
{
    mutable_updates().svs(u);
}),
```

Con eso la propiedad se puebla y es legible con el `Properties.Get` estándar
— sin necesidad de un método D-Bus a medida.

**2. `apparmor-easyprof-ubuntu` (repo aparte):** añadir al policy group `location`
la regla que permite `Get` de `org.freedesktop.DBus.Properties` hacia el servicio:

```
dbus (send)
     bus=system
     path="/com/lomiri/location/Service"
     interface="org.freedesktop.DBus.Properties"
     member="Get"
     peer=(name="com.lomiri.location.Service",label=unconfined),
```

Con las dos, una app confinada puede sondear los satélites con `Properties.Get`
estándar.

## Notas

- Emitir `PropertiesChanged` con el mapa SVS provoca SIGBUS al codificarlo como
  `Variant` en esta plataforma; por eso los clientes deben **sondear** con
  `Properties.Get`, no suscribirse. `Properties.Get` (lectura puntual) sí funciona
  una vez la propiedad está poblada.
- `azimuth` y `elevation` están intercambiados en `on_sv_status_update` del HAL
  Android — fix aparte.

## Relacionado

La parte 2 (AppArmor) va en otro proyecto y tiene su propio issue:
`apparmor_location_properties_get.md` (proyecto **apparmor-easyprof-ubuntu**). El
fix completo necesita las dos piezas: la de este repo (poblar la propiedad) y la
del policy group.
