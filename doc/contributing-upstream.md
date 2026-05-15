# Cómo proponer los parches navius a UBports

Este documento describe los pasos para abrir Merge Requests en el repositorio
upstream de `lomiri-location-service` y proponer la inclusión de los parches
del branch `navius`.

---

## Qué proponer y qué no

No todos los cambios son adecuados para upstream:

| Cambio | ¿Proponer? | Motivo |
|---|---|---|
| Fix mutex/EDEADLK en `register_callbacks()` | **Sí** — prioritario | Bug real con crash confirmado, afecta a cualquier dispositivo con Waydroid en HALIUM_10 |
| `GetVisibleSpaceVehicles` D-Bus method | **Sí** — con discusión | Feature nueva de API pública; requiere revisión del equipo |
| `build-deb.sh` | No | Herramienta personal de build, no pertenece al árbol upstream |

Abre **dos MRs separados**: facilita la revisión y permite que uno se acepte
independientemente del otro.

---

## Paso 1 — Preparar el entorno GitLab

1. Crea cuenta en **`gitlab.com`** si no tienes una.
2. Busca el repositorio: `gitlab.com/ubports/development/core/lomiri-location-service`
3. Pulsa **Fork** → elige tu espacio de nombres personal.

---

## Paso 2 — Añadir el fork como remote y subir las ramas

```bash
cd /ext_r/ext_edi/src/prog_ia/navius/lomiri-location-service

# Añadir remote con tu fork
git remote add myfork https://gitlab.com/<tu-usuario>/lomiri-location-service.git

# Crear rama solo con el fix del mutex/EDEADLK
git checkout -b fix/gps-register-callbacks-deadlock bc3b7bd
git push myfork fix/gps-register-callbacks-deadlock

# Crear rama solo con GetVisibleSpaceVehicles
# (parte del commit bc3b7bd, habría que separar con git cherry-pick o format-patch)
git checkout -b feature/dbus-get-visible-space-vehicles
# cherry-pick solo los cambios de interface.h / skeleton.cpp / implementation.cpp
git push myfork feature/dbus-get-visible-space-vehicles
```

> **Nota:** Los dos fixes están mezclados en el commit `bc3b7bd`. Para separarlos
> de forma limpia antes de proponer el MR, usa `git add -p` y crea dos commits
> distintos, o usa `git format-patch` para extraer parches individuales.

---

## Paso 3 — Abrir los Merge Requests

En GitLab, desde tu fork → **Merge Requests → New merge request**.

### MR 1: fix mutex/EDEADLK (bug fix)

**Título:**
```
gps: fix race condition and EDEADLK in android HAL register_callbacks()
```

**Descripción sugerida:**
```
## Problem

On devices running Waydroid in HALIUM_10 mode, lomiri-location-service and
Waydroid both register GPS HAL callbacks via host_hwbinder. Waydroid can
overwrite our callbacks at any time, causing register_callbacks() to be
called asynchronously.

register_callbacks() calls u_hardware_gps_delete() + u_hardware_gps_new().
If a GPS callback fires between these two calls it dereferences the freed
handle → SIGSEGV (confirmed crash on eutlan device, 2026-05-07).

A first attempt at a fix held a std::unique_lock across the entire
delete+new sequence, but u_hardware_gps_new() can invoke callbacks
(e.g. on_set_capabilities) synchronously on the calling thread, which
then tries to acquire a shared_lock on the same mutex → EDEADLK.

## Fix

- Add std::shared_mutex callback_mutex to Impl.
- Each of the 7 GPS callbacks acquires a shared_lock (concurrent-safe).
- register_callbacks() splits into 3 phases:
  1. unique_lock → delete old handle → unlock
  2. u_hardware_gps_new() without lock (re-entrant callbacks welcome)
  3. unique_lock → install new handle → dispatch_updated_modes → unlock

## Tested on

- UBports device (Snapdragon, arm64) running Waydroid HALIUM_10
- Observed: service starts cleanly, no crash after several hours with
  Waydroid running concurrently
```

### MR 2: GetVisibleSpaceVehicles (feature)

**Título:**
```
service: add GetVisibleSpaceVehicles D-Bus method
```

**Descripción sugerida:**
```
## Motivation

Clients that cannot subscribe to D-Bus property-change signals (e.g. QML
via QtDBus without a persistent connection) need a way to poll the current
set of visible GNSS satellites.

## Changes

- interface.h: add GetVisibleSpaceVehicles method struct and
  VisibleSpaceVehicles read-only property.
- skeleton.cpp: install method handler that replies with the current
  property value synchronously.
- implementation.cpp: forward engine SVS updates to the property via
  the D-Bus dispatcher thread.

## Usage

    dbus-send --system \
      --dest=com.lomiri.location.Service \
      --print-reply \
      /com/lomiri/location/Service \
      com.lomiri.location.Service.Interface.GetVisibleSpaceVehicles
```

---

## Paso 4 — Antes de abrir el MR: anunciarlo en los canales de UBports

Es habitual avisar antes de abrir el MR para evitar duplicar trabajo:

- **Foro de desarrollo:** `forums.ubports.com` → sección Development
- **Telegram:** grupo `UBports Core Apps` o `UBports Development`
- **Matrix:** `#ubports-devel:matrix.org`

Describe brevemente el problema, el hardware afectado y el fix propuesto.
Alguien del equipo puede orientarte sobre si ya hay trabajo en curso o si
prefieren que el MR siga algún formato concreto.

---

## Paso 5 — Lo que probablemente pedirán en la revisión

- **Separación de commits:** un commit por cambio lógico, mensaje en
  imperativo (`fix:`, `feat:`, etc.)
- **Test unitario:** difícil aquí porque requiere libhybris; menciona en el
  MR por qué no es viable y que el fix fue validado en hardware real.
- **CLA / DCO:** comprobar si UBports exige Developer Certificate of Origin
  (`git commit -s`) o algún CLA de Canonical.
- **CI verde:** el pipeline de GitLab debe pasar antes del merge.

---

## Referencia rápida de commits navius

| Commit | Contenido |
|---|---|
| `bc3b7bd` | mutex fix + EDEADLK (primera versión — EDEADLK al arrancar) + GetVisibleSpaceVehicles |
| `6f270d3` | build-deb.sh (no proponer) |
| `4369d8b` | fix definitivo EDEADLK (split 3 fases) + deps build-deb.sh |
| `4b83610` | doc/navius-patches.md (documentación interna, no proponer) |

Para el MR del bug fix, el commit limpio a proponer es la lógica de
`4369d8b` aplicada sobre el upstream original (sin los cambios de
`build-deb.sh` ni `navius-patches.md`).
