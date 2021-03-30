cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(buttons CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

set(POWER_DBUS_OBJECT_NAME "xyz/openbmc_project/Chassis/Buttons/Power")
set(RESET_DBUS_OBJECT_NAME "xyz/openbmc_project/Chassis/Buttons/Reset")
set(ID_DBUS_OBJECT_NAME "xyz/openbmc_project/Chassis/Buttons/ID")
set(GPIO_BASE_LABEL_NAME "1e780000.gpio")
set(LONG_PRESS_TIME_MS 3000)
set(CHASSIS_STATE_OBJECT_NAME "xyz/openbmc_project/state/chassis")
set(HOST_STATE_OBJECT_NAME "xyz/openbmc_project/state/host")
set(ID_LED_GROUP "enclosure_identify" CACHE STRING "The identify LED group name")

add_definitions(-DPOWER_DBUS_OBJECT_NAME="/${POWER_DBUS_OBJECT_NAME}0")
add_definitions(-DRESET_DBUS_OBJECT_NAME="/${RESET_DBUS_OBJECT_NAME}0")
add_definitions(-DID_DBUS_OBJECT_NAME="/${ID_DBUS_OBJECT_NAME}0")
add_definitions(-DGPIO_BASE_LABEL_NAME="${GPIO_BASE_LABEL_NAME}")
add_definitions(-DLONG_PRESS_TIME_MS=${LONG_PRESS_TIME_MS})
add_definitions(-DHOST_STATE_OBJECT_NAME="/${HOST_STATE_OBJECT_NAME}0")
add_definitions(-DCHASSIS_STATE_OBJECT_NAME="/${CHASSIS_STATE_OBJECT_NAME}0")

set(SRC_FILES src/power_button.cpp
    src/reset_button.cpp
    src/id_button.cpp
    src/main.cpp
    src/gpio.cpp
)

set(HANDLER_SRC_FILES
    src/button_handler_main.cpp
    src/button_handler.cpp
)

option (LOOKUP_GPIO_BASE
    "Look up the GPIO base value in /sys/class/gpio. Otherwise use a base of 0." ON
)

configure_file (settings.hpp.in ${CMAKE_BINARY_DIR}/inc/settings.hpp)
include_directories(${CMAKE_BINARY_DIR}/inc)

# import sdbusplus
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSPLUSPLUS sdbusplus REQUIRED)
include_directories(${SDBUSPLUSPLUS_INCLUDE_DIRS})
link_directories(${SDBUSPLUSPLUS_LIBRARY_DIRS})
find_program(SDBUSPLUSPLUS sdbus++)

# import phosphor-logging
find_package(PkgConfig REQUIRED)
pkg_check_modules(LOGGING phosphor-logging REQUIRED)
include_directories(${LOGGING_INCLUDE_DIRS})
link_directories(${LOGGING_LIBRARY_DIRS})

# phosphor-dbus-interfaces
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUSINTERFACE phosphor-dbus-interfaces REQUIRED)
include_directories(${DBUSINTERFACE_INCLUDE_DIRS})
link_directories(${DBUSINTERFACE_LIBRARY_DIRS})

add_executable(${PROJECT_NAME} ${SRC_FILES} )
target_link_libraries(${PROJECT_NAME} "${SDBUSPLUSPLUS_LIBRARIES} -lphosphor_dbus  -lstdc++fs")

add_executable(button-handler ${HANDLER_SRC_FILES})
target_link_libraries(button-handler "${SDBUSPLUSPLUS_LIBRARIES} -lphosphor_dbus")

set (
    SERVICE_FILES
    ${PROJECT_SOURCE_DIR}/service_files/xyz.openbmc_project.Chassis.Buttons.service
    ${PROJECT_SOURCE_DIR}/service_files/phosphor-button-handler.service
)

install (FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
install (TARGETS button-handler DESTINATION ${CMAKE_INSTALL_BINDIR})
