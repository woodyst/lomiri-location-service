Título: El indicador de ubicación no aparece de forma intermitente aunque el GPS esté activo (la comprobación de "proveedor activo" solo tiene en cuenta el último proveedor)

## Problema

El indicador de ubicación de la barra no aparece aunque el GPS esté posicionando
activamente. Es intermitente: en unos arranques o dispositivos aparece y en
otros no. Las apps sí reciben posición con normalidad; lo que falla es la señal
visual, porque la propiedad D-Bus `State` se queda en `"enabled"` en vez de
pasar a `"active"`.

LLS expone en D-Bus la propiedad `State`, que la shell usa para encender el
indicador de ubicación de la barra superior. El engine debería marcar
`State = active` cuando **al menos un** proveedor está dando posición, sin
importar cuál (por ejemplo, tengo fix por GPS aunque el proveedor de red esté
inactivo), y `enabled` cuando ninguno lo está.

## Solución

En src/location_service/com/lomiri/location/engine.cpp, línea 217:

    ```
    is_any_active = pair.first->state() == StateTrackingProvider::State::active;
    ```

se va actualizando en cada interacción del bucle (por cada proveedor), por lo que
devuelve sólo el valor del último proveedor en vez de `active` si hay alguno activo.

La solución es esta:

    ```
    is_any_active |= pair.first->state() == StateTrackingProvider::State::active;
    ```

porque partimos de false, y pasamos a true si **_alguno_** de los proveedores (GPS,
geoclue, etc) tiene fix.
