Título: El policy group `location` no permite a las apps confinadas leer propiedades del servicio de localización (org.freedesktop.DBus.Properties.Get)

> Proyecto: **apparmor-easyprof-ubuntu** (no lomiri-location-service).
> Issue compañero (en LLS): `svs_properties_get.md` — el fix completo necesita las dos piezas.

## Problema

Una app confinada con el policy group `location` no puede leer propiedades del
servicio de localización mediante `org.freedesktop.DBus.Properties.Get`: recibe
`AccessDenied`. En concreto, no puede sondear `VisibleSpaceVehicles` (la lista de
satélites visibles), que es la vía estándar para obtenerla.

## Causa

El policy group `location` (paquete `apparmor-easyprof-ubuntu`,
`/usr/share/apparmor/easyprof/policygroups/ubuntu/1.0/location`) solo permite la
interfaz `com.lomiri.location.Service`, pero **no** `org.freedesktop.DBus.Properties`.
Por eso las llamadas estándar `Properties.Get`/`GetAll` sobre el objeto del
servicio quedan bloqueadas para las apps confinadas.

Verificado: `aa-exec -p <perfil-app> -- dbus-send ... Properties.Get` → `AccessDenied`;
tras añadir la regla → OK.

## Solución

Añadir al policy group `location` la regla que permite `Get` de
`org.freedesktop.DBus.Properties` hacia el servicio:

```
dbus (send)
     bus=system
     path="/com/lomiri/location/Service"
     interface="org.freedesktop.DBus.Properties"
     member="Get"
     peer=(name="com.lomiri.location.Service",label=unconfined),
```

(Y análogamente `member="GetAll"` si se quiere permitir leer todas las
propiedades de una vez.)

## Relacionado

Este arreglo es una de las dos piezas necesarias para que una app confinada
obtenga la lista de satélites. La otra está en **lomiri-location-service**: la
propiedad `VisibleSpaceVehicles` no se puebla porque `StateTrackingProvider` no
reenvía los `svs` al engine. Ver el issue compañero `svs_properties_get.md`.
