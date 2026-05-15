#!/bin/bash
# Construye liblomiri-location-service3 como paquete .deb usando un contenedor Docker
# con Ubuntu 24.04 + repo UBports. No modifica el sistema host.
#
# Uso: ./build-deb.sh [destino]
#   destino: directorio donde copiar los .deb (por defecto: ./debs/)
#
# Requisitos: docker en ejecución, acceso a repo.ubports.com y hub.docker.com

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${1:-$SCRIPT_DIR/debs}"
IMAGE_NAME="lomiri-location-service-builder"
UBPORTS_REPO="http://repo.ubports.com/"
UBPORTS_DIST="24.04-1.x"
LOG_FILE="/tmp/build-location-service.log"

mkdir -p "$OUTPUT_DIR"

# Redirigir toda la salida al log con tee
exec > >(tee -a "$LOG_FILE") 2>&1
echo "=== Build iniciado: $(date) → log en $LOG_FILE ==="

echo "=== Construyendo imagen Docker de compilación ==="
docker build -t "$IMAGE_NAME" - <<'DOCKERFILE'
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Repo UBports
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl gnupg && \
    curl -fsSL http://repo.ubports.com/keyring.gpg | \
        gpg --dearmor -o /etc/apt/trusted.gpg.d/ubports.gpg 2>/dev/null || \
    curl -fsSL http://repo.ubports.com/pool/main/u/ubports-keyring/ubports-keyring_2026.02.02_all.deb \
        -o /tmp/ubports-keyring.deb && \
    dpkg -i /tmp/ubports-keyring.deb 2>/dev/null || true

RUN echo "deb http://repo.ubports.com/ 24.04-1.x main" > /etc/apt/sources.list.d/ubports.list && \
    apt-get update

# Dependencias de compilación
RUN apt-get install -y --no-install-recommends \
    build-essential cmake cmake-extras debhelper devscripts fakeroot \
    googletest libapparmor-dev libboost-filesystem-dev libboost-system-dev \
    libboost-program-options-dev libdbus-1-dev libdbus-cpp-dev \
    libgoogle-glog-dev libgps-dev libgtest-dev libiw-dev libjson-c-dev \
    libnet-cpp-dev libprocess-cpp-dev libproperties-cpp-dev libtrust-store-dev \
    libubuntu-platform-hardware-api-headers libubuntu-platform-hardware-api-dev \
    lsb-release pkg-config qtbase5-dev qtlocation5-dev qtpositioning5-dev \
    qt6-base-dev qt6-declarative-dev qt6-declarative-private-dev \
    qt6-location-dev qt6-positioning-dev \
    systemd-dev trust-store-bin && \
    apt-get clean

WORKDIR /src
DOCKERFILE

echo "=== Ejecutando dpkg-buildpackage dentro del contenedor ==="
docker run --rm \
    -v "$SCRIPT_DIR:/src:ro" \
    -v "$OUTPUT_DIR:/output" \
    "$IMAGE_NAME" \
    bash -c "
        set -e
        cp -a /src /build
        cd /build

        # Versión: añadir sufijo navius4 si no está ya
        if ! head -1 debian/changelog | grep -q 'navius4'; then
            CURRENT_VER=\$(dpkg-parsechangelog -S Version)
            BASE_VER=\$(echo "\$CURRENT_VER" | sed 's/+navius.*//')
            dch --newversion "\${BASE_VER}+navius4" \
                --distribution noble --force-distribution \
                'navius4: GPS watchdog + dispatch_updated_modes in fast path + location trace'
        fi

        nice -n 15 dpkg-buildpackage -b -uc -us \
            --build-profiles=nocheck,nodoc \
            -j\$(nproc)

        cp /build/../*.deb /output/
        echo '=== .deb generados ==='
        ls -lh /output/*.deb
    "

echo ""
echo "=== Paquetes en $OUTPUT_DIR ==="
ls -lh "$OUTPUT_DIR"/*.deb
