cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)
project(power-control CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option (
    CHASSIS_SYSTEM_RESET
    "Enable chassis system power reset to allow removing
     power and restoring back"
    OFF
)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
add_definitions(-DBOOST_ASIO_DISABLE_THREADS)

set(SRC_FILES src/power_control.cpp)

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} -lstdc++fs)
target_link_libraries(${PROJECT_NAME} chassisi2c)
target_link_libraries(${PROJECT_NAME} i2c)
target_link_libraries(${PROJECT_NAME} gpiodcxx)
target_link_libraries(${PROJECT_NAME} systemd)
target_link_libraries(${PROJECT_NAME} sdbusplus)

target_compile_definitions (
    ${PROJECT_NAME} PRIVATE $<$<BOOL:${CHASSIS_SYSTEM_RESET}>:
    -DCHASSIS_SYSTEM_RESET>
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

# Temporarily disable rtti
# See https://github.com/chriskohlhoff/asio/issues/533
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

set(
  SERVICE_FILES
  ${PROJECT_SOURCE_DIR}/service_files/chassis-system-reset.service
  ${PROJECT_SOURCE_DIR}/service_files/chassis-system-reset.target
  ${PROJECT_SOURCE_DIR}/service_files/xyz.openbmc_project.Chassis.Control.Power.service
  )

set(
  CONFIG_FILE
  ${PROJECT_SOURCE_DIR}/config/power-config-host0.json
  )

install(FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
install(FILES ${CONFIG_FILE} DESTINATION /usr/share/x86-power-control/)
