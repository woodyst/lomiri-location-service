# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

configure_file(
  lomiri-location-service.pc.in lomiri-location-service.pc @ONLY
)

configure_file(
  lomiri-location-service-connectivity.pc.in lomiri-location-service-connectivity.pc @ONLY
)

configure_file(
  lomiri-location-service.service.in lomiri-location-service.service @ONLY
)

configure_file(
  lomiri-location-service-trust-stored.service.in lomiri-location-service-trust-stored.service @ONLY
)

configure_file(
  lomiri-location-service-trust-stored-wayland.service.in lomiri-location-service-trust-stored-wayland.service @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service-connectivity.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/com.lomiri.location.Service.conf
  DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d/
)

if(SYSTEMD_FOUND OR ENFORCE_SYSTEMD_SERVICE)
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service.service
      DESTINATION ${SYSTEMD_SYSTEM_UNITDIR}
    )

    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service-trust-stored.service
      DESTINATION ${SYSTEMD_USER_UNITDIR}
    )

    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service-trust-stored-wayland.service
      DESTINATION ${SYSTEMD_USER_UNITDIR}
    )
endif()
