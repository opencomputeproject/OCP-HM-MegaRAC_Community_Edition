cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(mail-alert-manager CXX)
set(CMAKE_CXX_STANDARD 20)
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

set(SRC_FILES src/mail_alert_manager_main.cpp src/mail_alert_manager.cpp)

# import libsystemd
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD libsystemd REQUIRED)

# import sdbusplus
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDBUSPLUSPLUS sdbusplus REQUIRED)

add_executable(mail-alert-manager ${SRC_FILES})

target_link_libraries(mail-alert-manager systemd)
target_link_libraries(mail-alert-manager boost_coroutine)
target_link_libraries(mail-alert-manager sdbusplus -lstdc++fs)
target_link_libraries(mail-alert-manager esmtp)
target_link_libraries(mail-alert-manager ssl)
target_link_libraries(mail-alert-manager ${Boost_LIBRARIES})

link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)

set(SERVICE_FILES
    ${PROJECT_SOURCE_DIR}/service_files/mail-alert-manager.service)

install(FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
install(TARGETS mail-alert-manager DESTINATION bin)
