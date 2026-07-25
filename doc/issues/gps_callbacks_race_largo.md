# El crash por race condition en los callbacks GPS — explicación larga

Notas de trabajo para entender el bug a fondo. El issue público es la versión
corta (`gps_callbacks_race.md`); esto es el material de fondo.

## Qué hace `register_callbacks()`

Es la función que (re)inicializa el driver GPS. Hace tres cosas seguidas:

1. libera el handle viejo — `u_hardware_gps_delete(gps_handle)`,
2. crea uno nuevo — `gps_handle = u_hardware_gps_new(&gps_params)`,
3. le empuja la configuración — `dispatch_updated_modes_to_driver()`.

En upstream es literalmente esto, sin ningún candado:

```cpp
void ...::register_callbacks()
{
    if (gps_handle)
        u_hardware_gps_delete(gps_handle);
    gps_handle = u_hardware_gps_new(std::addressof(gps_params));
    dispatch_updated_modes_to_driver();
}
```

Se llama al construir la HAL y en reconfiguraciones.

## Quién corre en paralelo

El HAL de Android entrega los datos GPS por **callbacks asíncronos** desde su
propio hilo: `on_location_update`, `on_status_update`, `on_sv_status_update`,
`on_set_capabilities`, `on_nmea_update`, `on_agps_status_update`, etc. (7+).
Todos reciben un `context` que apunta a la HAL y acaban tocando `impl.gps_handle`
y el estado de `impl`. En upstream tampoco toman ningún lock.

## La race → use-after-free → SIGSEGV

`register_callbacks()` y esos callbacks no comparten ninguna sincronización. Hay
dos ventanas de peligro:

1. **Entre `u_hardware_gps_delete(gps_handle)` y la reasignación**: durante ese
   instante `gps_handle` apunta a memoria ya liberada (puntero colgante). Si un
   callback en vuelo lo dereferencia → SIGSEGV.
2. **Durante `u_hardware_gps_new()`**: esa llamada tarda y puede disparar
   callbacks *síncronamente* mientras el objeto interno del driver está a medio
   construir.

Es un use-after-free clásico: un hilo libera/recrea un recurso mientras otro lo
usa sin candado.

## Por qué Waydroid lo dispara de forma fiable

En modo HALIUM_10 conviven dos contenedores Android: el de Halium (dueño del HAL
GNSS real) y el de Waydroid, que registra sus propios callbacks GPS por
`host_hwbinder`. Eso hace que `register_callbacks()` se invoque mientras hay
callbacks del otro contenedor en vuelo → la ventana de la race se abre una y otra
vez → crash en bucle. En UT “normal” (un solo contenedor) la ventana es minúscula
y casi nunca se toca, por eso upstream no lo ha visto; pero el bug está en su
código: Waydroid solo es el reproductor fiable.

## El fix y su sutileza (el EDEADLK)

La solución es un `std::shared_mutex callback_mutex` que proteja `gps_handle`:

- **Todos los callbacks** toman un **shared_lock** (lectura concurrente: varios a
  la vez, sin bloquearse entre sí).
- **`register_callbacks()`** toma el **unique_lock** (exclusivo) mientras
  manipula el handle.

Pero el intento naïve —coger el lock exclusivo y mantenerlo durante todo el
`delete + new + dispatch`— se auto-bloquea: `u_hardware_gps_new()` (y
`set_position_mode()`) pueden invocar callbacks *síncronamente en el mismo hilo*;
ese callback pide el shared_lock mientras el hilo ya tiene el exclusivo → como
`std::shared_mutex` **no es reentrante**, salta EDEADLK. Por eso el fix parte la
función en fases, soltando el lock antes de las llamadas que pueden reentrar:

1. `unique_lock` → borrar handle viejo → `gps_handle = nullptr` → soltar.
2. `u_hardware_gps_new()` **sin** lock (los callbacks reentrantes ya pueden coger
   su shared_lock).
3. `unique_lock` → instalar el handle nuevo → soltar.
4. `dispatch_updated_modes_to_driver()` **sin** lock (por lo mismo).

Así los callbacks nunca ven un handle colgante (fuera de las fases con lock el
handle es `nullptr` o válido, nunca liberado-pero-no-nulo) y no hay reentrada
bajo el lock exclusivo.

## Qué NO entra en el issue

Es específico de navius (recuperación del GPS cuando Waydroid lo roba), no del
bug de upstream, y mezclarlo daría a los revisores la excusa de “esto es cosa de
Waydroid”:

- El **watchdog** que reclama el GPS cuando deja de emitir.
- El átomo `positioning_active` y el `try_to_lock` de `start/stop_positioning()`.
- La maquinaria de arranque/recuperación no-bloqueante.

Eso, si acaso, iría en un issue aparte (“GPS robado por Waydroid”).
