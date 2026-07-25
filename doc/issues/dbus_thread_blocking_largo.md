# El servicio se congela por llamadas bloqueantes al HAL en el hilo de D-Bus — explicación larga

Notas de trabajo. El issue público es la versión corta
(`dbus_thread_blocking.md`); esto es el material de fondo.

## La cadena de llamada

Cuando un cliente pide posición, llega por D-Bus el método `StartPositionUpdates`.
El recorrido es:

```
StartPositionUpdates (D-Bus)
  → session/skeleton.cpp: on_start_position_updates()      [hilo de dispatch de D-Bus]
  → session/implementation.cpp: start_position_updates()
  → provider->state_controller()->start_position_updates()
  → gps/provider.cpp: Provider::start_position_updates()
  → hal->start_positioning()
```

Todo eso corre en el **hilo de dispatch de D-Bus** (el que ejecuta los handlers de
método). No se delega a ningún hilo de trabajo.

## Qué hace `start_positioning()` en upstream

```cpp
bool ...::start_positioning()
{
    // Re-register callbacks in case they have been overwritten (e.g. by Waydroid)
    impl.register_callbacks();
    return u_hardware_gps_start(impl.gps_handle);
}
```

Es decir, en **cada** `StartPositionUpdates`:

1. `register_callbacks()` → `u_hardware_gps_delete(gps_handle)` +
   `u_hardware_gps_new(...)` + `dispatch_updated_modes_to_driver()`. Re-inicializa
   el driver GPS entero.
2. `u_hardware_gps_start(gps_handle)` → arranca el chipset.

Las dos son llamadas **síncronas** al HAL de Android, de duración no acotada.

## El bloqueo

`u_hardware_gps_new()`, `u_hardware_gps_delete()` y `u_hardware_gps_start()` pueden
tardar o quedarse atascadas dentro del HAL (hardware ocupado, driver esperando,
etc.). Como se ejecutan en el hilo de dispatch de D-Bus, mientras no vuelvan:

- el servicio no procesa ninguna otra llamada D-Bus,
- todos los clientes ven el servicio como colgado/congelado,
- no se recupera hasta que el HAL devuelve (que puede ser segundos o nunca).

Es el antipatrón clásico de “trabajo bloqueante en el hilo de eventos”: el
dispatch de D-Bus debe volver rápido; cualquier operación que pueda bloquear tiene
que ir a un hilo de trabajo.

## Por qué Waydroid lo dispara de forma fiable

En HALIUM_10, Waydroid retiene/ocupa el HAL GNSS, así que las llamadas de arranque
se quedan esperando dentro del HAL con mucha más frecuencia. Pero el problema es de
diseño: cualquier chipset lento o HAL que se demore congela el servicio igual.
Waydroid es solo el reproductor fiable.

## El fix

Sacar el trabajo bloqueante del hilo de dispatch:

- Si el handle ya es válido (caso común tras el primer arranque): **fast path** —
  llamar directamente a `u_hardware_gps_start()` y volver, sin re-registrar
  callbacks.
- Si hay que (re)inicializar: lanzar `register_callbacks()` + `u_hardware_gps_start()`
  en un hilo **detached**, de modo que `start_positioning()` retorne de inmediato.
- Lo mismo en `stop_positioning()`.

Así el hilo de dispatch de D-Bus nunca se queda esperando al HAL.

## Qué NO entra en el issue

Es específico de navius y mezclarlo alargaría el issue y daría munición para
desviar el tema:

- El **watchdog** que reclama el GPS cuando Waydroid lo roba.
- El átomo `positioning_active` (guarda contra hilos de recuperación concurrentes).
- El `try_to_lock` sobre el `callback_mutex` — que además es del otro parche
  (el de la race/SIGSEGV), no de upstream. En upstream no existe ese mutex, así
  que ese sub-problema (bloquearse esperando el write lock) ni siquiera aplica.

## Aviso de encaje con upstream

De los tres, este es el más discutible:

1. El disparador fiable es Waydroid → riesgo de “esto es cosa vuestra”.
2. Es una mejora arquitectónica más que un bug puntual; podrían responder “el HAL
   no debería atascarse”.
3. El fix real de navius va muy entrelazado con maquinaria propia.

Conviene plantearlo como principio de framework: **un handler de método D-Bus no
debe hacer llamadas bloqueantes al HAL en el hilo de dispatch; muévanse a un hilo
de trabajo**. Con Waydroid solo como reproductor.
