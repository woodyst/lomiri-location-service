Título: El servicio deja de responder a D-Bus cuando el HAL GPS se atasca, porque start_positioning() hace llamadas bloqueantes en el hilo de dispatch

## Problema

En ciertas condiciones (HAL GPS ocupado o atascado; de forma fiable con Waydroid,
que retiene el HAL), el servicio deja de responder a **todas** las llamadas D-Bus:
aparece congelado para todos los clientes hasta que el HAL devuelve.

## Causa

`start_positioning()` se ejecuta en el hilo que atiende las llamadas D-Bus (el
handler de `StartPositionUpdates`: `skeleton` → … →
`gps::Provider::start_position_updates` → `hal->start_positioning()`), y hace
llamadas **síncronas y potencialmente bloqueantes** al HAL de Android:

```cpp
bool ...::start_positioning()
{
    impl.register_callbacks();                     // u_hardware_gps_delete + u_hardware_gps_new
    return u_hardware_gps_start(impl.gps_handle);  // arranque del chipset
}
```

`register_callbacks()` (borrar el handle + crear uno nuevo) y
`u_hardware_gps_start()` pueden tardar o quedarse atascadas dentro del HAL. Como
corren en el hilo de dispatch, mientras duran el servicio no atiende ninguna otra
petición D-Bus → se percibe congelado. Un handler de método D-Bus no debería
hacer llamadas de duración no acotada al HAL.

## Solución

Sacar las llamadas bloqueantes del hilo de dispatch: ejecutar
`register_callbacks()` + `u_hardware_gps_start()` en un hilo aparte (*detached*),
de modo que `start_positioning()` retorne de inmediato y el dispatch de D-Bus
nunca se bloquee. Cuando el handle ya es válido, se puede llamar directamente a
`u_hardware_gps_start()` sin re-registrar (fast path). Lo mismo aplica a
`stop_positioning()`.
