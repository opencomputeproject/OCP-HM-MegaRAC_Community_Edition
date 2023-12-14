cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(pef-alert-manager CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_NO_RTTI)
add_definitions(-DBOOST_NO_TYPEID)
add_definitions(-DBOOST_ASIO_DISABLE_THREADS)

set(SOURCE_FILES src/pef_config.cpp)
set(SRC_FILES src/pef_action.cpp)

# import libsystemd
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD libsystemd REQUIRED)

# import sdbusplus
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSPLUSPLUS sdbusplus REQUIRED)

add_executable(pef-configuration ${SOURCE_FILES})
add_executable(pef-event-filtering ${SRC_FILES})

target_link_libraries(pef-configuration systemd)
target_link_libraries(pef-configuration boost_coroutine)
target_link_libraries(pef-configuration sdbusplus -lstdc++fs)
target_link_libraries(pef-configuration ${Boost_LIBRARIES})

target_link_libraries(pef-event-filtering systemd)
target_link_libraries(pef-event-filtering boost_coroutine)
target_link_libraries(pef-event-filtering sdbusplus -lstdc++fs)
target_link_libraries(pef-event-filtering ${Boost_LIBRARIES})

link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

set(SERVICE_FILES
    ${PROJECT_SOURCE_DIR}/service_files/pef-configuration.service
    ${PROJECT_SOURCE_DIR}/service_files/pef-event-filtering.service)
set(CONFIG_FILES
    ${PROJECT_SOURCE_DIR}/pef_configurations/pef-alert-manager.json)

install(TARGETS pef-configuration DESTINATION bin)
install(FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
install(FILES ${CONFIG_FILES} DESTINATION /usr/share/pef-alert-manager/)

install(TARGETS pef-event-filtering DESTINATION bin)
