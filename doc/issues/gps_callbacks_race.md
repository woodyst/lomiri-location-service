Título: El servicio crashea (SIGSEGV) por una race condition entre register_callbacks() y los callbacks GPS

## Problema

El servicio crashea con SIGSEGV. Es reproducible de forma fiable con Waydroid
(modo HALIUM_10), donde conviven dos contenedores Android que registran callbacks
GPS, pero se trata de una race condition genérica del código.

## Causa

En `src/location_service/com/lomiri/location/providers/gps/android_hardware_abstraction_layer.cpp`,
`register_callbacks()` libera y recrea el handle del GPS **sin ninguna
sincronización**:

```cpp
if (gps_handle)
    u_hardware_gps_delete(gps_handle);          // libera el handle
gps_handle = u_hardware_gps_new(&gps_params);   // crea uno nuevo
```

Los callbacks del HAL (`on_location_update`, `on_sv_status_update`, etc.) corren
en otro hilo y dereferencian `gps_handle` sin candado. Si un callback se dispara
entre el `delete` y la reasignación —o durante `u_hardware_gps_new()`, que puede
invocar callbacks— toca memoria ya liberada → use-after-free → SIGSEGV.

## Solución

Proteger `gps_handle` con un `std::shared_mutex`: `shared_lock` en los callbacks
(lectura concurrente) y `unique_lock` en `register_callbacks()` mientras se
manipula el handle.

Importante: **no** mantener el lock exclusivo durante `u_hardware_gps_new()` ni
`set_position_mode()`, porque pueden invocar callbacks síncronamente en el mismo
hilo; como `std::shared_mutex` no es reentrante, el callback pediría el
`shared_lock` teniendo ya el exclusivo → EDEADLK. Por eso se parte en fases:

1. `unique_lock` → borrar el handle viejo → `gps_handle = nullptr` → soltar.
2. `u_hardware_gps_new()` **sin** lock.
3. `unique_lock` → instalar el handle nuevo → soltar.
4. Empujar la configuración **sin** lock.

Así los callbacks nunca ven un handle colgante (fuera de las fases con lock el
handle es `nullptr` o válido, nunca liberado-pero-no-nulo) y no hay reentrada
bajo el lock exclusivo.
