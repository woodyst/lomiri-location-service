cmake_minimum_required(VERSION 3.16)

project(lomiri-location-service VERSION 3.4.1 LANGUAGES C CXX)

# This version is for the API version (i.e. library's SOVERSION).
set(LOMIRI_LOCATION_SERVICE_VERSION_MAJOR 3)
set(LOMIRI_LOCATION_SERVICE_VERSION_MINOR 0)
set(LOMIRI_LOCATION_SERVICE_VERSION_PATCH 0)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

message(STATUS "${CMAKE_PROJECT_NAME} ${LOMIRI_LOCATION_SERVICE_VERSION_MAJOR}.${LOMIRI_LOCATION_SERVICE_VERSION_MINOR}.${LOMIRI_LOCATION_SERVICE_VERSION_PATCH}")

set(LOMIRI_LOCATION_SERVICE_TRUST_STORE_SERVICE_NAME "LomiriLocationService")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -fno-strict-aliasing -pedantic -Wextra -fPIC -DBOOST_ASIO_DISABLE_EPOLL")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option (ENABLE_TRUST_STORE "Enable support for trust-store" ON)

option(ENABLE_QT6 "Enable Qt6 build" OFF)
if(ENABLE_QT6)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Location)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Location)
    set(QT_VERSION_MAJOR 5)
endif()

include(cmake/EnableCoverageReport.cmake)
include(cmake/FindGFlags.cmake)
include(CTest)

include(GNUInstallDirs)

find_package(PkgConfig)
find_package(Boost COMPONENTS filesystem system program_options)
find_package(glog REQUIRED)
find_package(GFlags)
find_package(Threads)
pkg_check_modules(DBUS dbus-1 REQUIRED)
pkg_check_modules(DBUS_CPP dbus-cpp REQUIRED)
pkg_check_modules(JSON_C json-c REQUIRED)
pkg_check_modules(LIBAPPARMOR libapparmor REQUIRED)
pkg_check_modules(NET_CPP net-cpp REQUIRED)
pkg_check_modules(PROCESS_CPP process-cpp REQUIRED)
pkg_check_modules(PROPERTIES_CPP properties-cpp REQUIRED)
if (ENABLE_TRUST_STORE)
    pkg_check_modules(TRUST_STORE trust-store REQUIRED)
endif (ENABLE_TRUST_STORE)

option(ENFORCE_SYSTEMD_SERVICE "Enable installation of systemd services" OFF)
pkg_check_modules(SYSTEMD systemd)
if(SYSTEMD_FOUND)
    pkg_get_variable(SYSTEMD_SYSTEM_UNITDIR systemd systemd_system_unit_dir)
    pkg_get_variable(SYSTEMD_USER_UNITDIR systemd systemd_user_unit_dir)
elseif(ENFORCE_SYSTEMD_SERVICE)
    set (SYSTEMD_SYSTEM_UNITDIR "/usr/lib/systemd/system")
    set (SYSTEMD_USER_UNITDIR "/usr/lib/systemd/user")
endif()
pkg_check_modules(UBUNTU_PLATFORM_HARDWARE_API ubuntu-platform-hardware-api)
pkg_check_modules(GPSD libgps)
#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

option (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS "Disable errors when local typedefs are unused" ON)
if (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-local-typedefs")
endif (DISABLE_ERROR_ON_LOCAL_TYPEDEFS_WARNINGS)

#
# Code style fixer. We rely on clang-format to adjust the formatting of source code.
#
find_program(CLANG_FORMAT_COMMAND NAMES clang-format-3.6 clang-format-3.5)
if (NOT CLANG_FORMAT_COMMAND)
  message(WARNING "Cannot find clang-format >= clang-format-3.5: formatcode target will not be available")
endif()

if (CLANG_FORMAT_COMMAND)
  file(GLOB_RECURSE LOMIRI_LOCATION_SERVICE_INTERFACE_HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.h)
  file(GLOB_RECURSE LOMIRI_LOCATION_SERVICE_IMPLEMENTATION_FILES ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/src/*.cpp)
  add_custom_target(formatcode ${CLANG_FORMAT_COMMAND} -i ${LOMIRI_LOCATION_SERVICE_INTERFACE_HEADER_FILES} ${LOMIRI_LOCATION_SERVICE_IMPLEMENTATION_FILES} SOURCES _clang-format)
endif()

include_directories(
  3rd-party/

  ${Boost_INCLUDE_DIRS}
  ${DBUS_INCLUDE_DIRS}
  ${DBUS_CPP_INCLUDE_DIRS}
  ${JSON_C_INCLUDE_DIRS}
  ${LIBAPPARMOR_INCLUDE_DIRS}
  ${NET_CPP_INCLUDE_DIRS}
  ${PROPERTIES_CPP_INCLUDE_DIRS}
  ${PROCESS_CPP_INCLUDE_DIRS}
  ${TRUST_STORE_INCLUDE_DIRS}
  ${GFlags_INCLUDE_DIR}

  ${CMAKE_SOURCE_DIR}/include/location_service
  ${CMAKE_SOURCE_DIR}/src/location_service
  ${CMAKE_BINARY_DIR}/src/location_service
)

set(LOMIRI_LOCATION_SERVICE_PUBLIC_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/boost_ptree_settings.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/cached_radio_cell.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/cached_wireless_network.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/dummy_connectivity_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/nm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/ofono.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/connectivity/ofono_nm_connectivity_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/engine.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/non_selecting_provider_selection_policy.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/dummy/delayed_provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/dummy/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/geoclue/geoclue.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/geoclue/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/android_hardware_abstraction_layer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/hardware_abstraction_layer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/net_cpp_gps_xtra_downloader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/null_gps_xtra_downloader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/sntp_client.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gps/sntp_reference_time_source.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/gpsd/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/remote/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/providers/skyhook/provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/always_granting_permission_manager.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/daemon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/dbus_connection_factory.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/default_configuration.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/demultiplexing_reporter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/harvester.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/ichnaea_reporter.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/implementation.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/program_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/provider_daemon.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/runtime.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/runtime_tests.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/session/interface_p.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/set_name_for_thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/settings.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/state_tracking_provider.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/time_based_update_policy.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/time_since_boot.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/update_policy.h
)
if(ENABLE_TRUST_STORE)
  set(LOMIRI_LOCATION_SERVICE_PUBLIC_HEADERS
    ${LOMIRI_LOCATION_SERVICE_PUBLIC_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/location_service/com/lomiri/location/service/trust_store_permission_manager.h
  )
endif(ENABLE_TRUST_STORE)

add_subdirectory(data)
add_subdirectory(doc)
add_subdirectory(examples)
add_subdirectory(include)
add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(qt)

# There's no nice way to format this. Thanks CMake.
add_test(LGPL-required
  /bin/sh -c "! grep -rl 'GNU General' ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/include"
)

enable_coverage_report(service)
