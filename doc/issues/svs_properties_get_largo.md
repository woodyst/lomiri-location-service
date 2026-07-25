# Satélites vacíos vía Properties.Get(VisibleSpaceVehicles) — explicación larga

Notas de trabajo. El issue público es la versión corta (`svs_properties_get.md`).
Enfoque acordado: en vez de añadir el método a medida `GetVisibleSpaceVehicles`,
proponer el arreglo que hace funcionar el `Properties.Get` estándar.

## La cadena de los satélites (svs)

```
HAL Android (on_sv_status_update)
  → gps::Provider           emite updates().svs
  → StateTrackingProvider   [WRAPPER] — el engine añade cada provider envuelto aquí
  → Engine                  escucha wrapper->updates().svs
  → Implementation          actualiza la propiedad VisibleSpaceVehicles
  → Skeleton                expone la propiedad en D-Bus
  → Cliente                 Properties.Get(VisibleSpaceVehicles)
```

## El bug raíz (upstream)

`Engine::add_provider()` no registra el provider crudo, sino un
`StateTrackingProvider` que lo envuelve. Ese wrapper reenvía del provider interno
solo `position`, `heading` y `velocity`. **No reenvía `svs`.** En upstream
(`gitlab/main`), el struct `connections` solo tiene `position_updates`,
`heading_updates`, `velocity_updates`.

Resultado: `wrapper->updates().svs` nunca emite → el engine nunca recibe
satélites → la propiedad `VisibleSpaceVehicles` está siempre vacía → cualquier
`Properties.Get` sobre ella devuelve `[]`. Los satélites se pierden entre el
provider y el engine, y ha sido así desde siempre en upstream.

Verificado con trazas: el HAL entrega N SVs, `gps::Provider` los emite, y el
engine no recibe ninguno.

## Por qué el método a medida era un parche, no la solución

Se había añadido un método D-Bus `GetVisibleSpaceVehicles` que lee el mismo valor
y lo devuelve. Pero:

- No arregla la causa: la propiedad sigue vacía, así que el método también
  devolvía `[]` hasta que se arregló el svs-forwarding.
- Es no estándar: obliga a los clientes a usar un método propietario en vez del
  `org.freedesktop.DBus.Properties.Get` de siempre.

La solución correcta es poblar la propiedad (reenviar `svs`); entonces
`Properties.Get` funciona y el método a medida sobra.

## El fix

En `state_tracking_provider.h`, añadir al initializer de `connections`:

```cpp
impl_->updates().svs.connect([this](const Update<std::set<SpaceVehicle>>& u)
{
    mutable_updates().svs(u);
}),
```

y su `core::ScopedConnection svs_updates;` al struct. Una conexión, simétrica a
las de position/heading/velocity.

## Por qué se sondea con Get y no se usa el signal PropertiesChanged

Emitir `PropertiesChanged` con el mapa SVS provoca **SIGBUS** al codificarlo. El
codec: `SpaceVehicle` es `(uudbbbdd)` y el mapa es `a(uudbbbdd)` (array de
structs, la key va embebida). dbus-cpp trae además un codec genérico de
`std::map` que codifica dict `a{KV}`; con una key struct eso daría una firma
D-Bus ilegal, y hay riesgo de ODR según el orden de includes — candidato al
SIGBUS en la ruta del signal.

Sin embargo, `Properties.Get` (lectura puntual) **sí funciona** una vez la
propiedad está poblada: `handle_get` (dbus-cpp `impl/property.h`) lee el mismo
valor que leía el método a medida, solo que envuelto en `TypedVariant`, y eso se
serializa bien. Por eso el plan es: poblar la propiedad + que los clientes
**sondeen** con `Properties.Get` (sin suscribirse a `PropertiesChanged`).

## Fixes relacionados (issues/MRs aparte)

- **azimuth/elevation intercambiados** en `on_sv_status_update` del HAL
  (`sv.azimuth` recibe la elevation del HAL y viceversa; verificado contra el
  `gps.h` del HAL y `space_vehicle.h`). Fix: asignación directa. Comprobable
  cuando fluyen satélites (elevation > 90° = swap activo).
- **AppArmor**: el policy group `location` solo permite la interfaz
  `com.lomiri.location.Service`, no `org.freedesktop.DBus.Properties`. Las apps
  confinadas no pueden llamar `Properties.Get` aunque LLS funcione. Fix: MR al
  policy group `location` de `apparmor-easyprof-ubuntu` (UBports). Este es el
  motivo por el que el método a medida "parecía" funcionar para apps confinadas y
  `Properties.Get` no.

## Encaje con upstream

Es el más sólido de los cuatro: bug raíz real (los satélites nunca llegan a la
propiedad en upstream), fix mínimo y simétrico, y elimina la necesidad de un
método D-Bus propietario. La parte de AppArmor va a otro proyecto
(`apparmor-easyprof-ubuntu`), no a LLS.
