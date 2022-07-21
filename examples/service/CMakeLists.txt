message(STATUS ${Boost_LIBRARIES})
include_directories(
    ${Boost_INCLUDE_DIRS}
    ${DBUS_INCLUDE_DIRS}
    ${DBUS_CPP_INCLUDE_DIRS}

)

link_directories(
  /tmp/wpsapi-4.7.6_05-linux-x86_64/lib
)

add_executable(
    client
    client.cpp
)

add_executable(
    service
    service.cpp
)

target_link_libraries(
    client

    lomiri-location-service

    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${DBUS_CPP_LDFLAGS}
)

target_link_libraries(
    service

    lomiri-location-service

    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
    ${DBUS_LIBRARIES}
    ${DBUS_CPP_LDFLAGS}
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service.1"
    COMMAND ronn --roff --pipe --organization=ubports ${CMAKE_CURRENT_SOURCE_DIR}/service.1.ronn > ${CMAKE_CURRENT_BINARY_DIR}/lomiri-location-service.1
)

add_custom_target(
    lomiri-location-service-man-page
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/lomiri-location-service.1"
)

install(
    TARGETS service client
    DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/examples/
)

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/lomiri-location-service.1
    DESTINATION share/doc/${CMAKE_PROJECT_NAME}/man
)
