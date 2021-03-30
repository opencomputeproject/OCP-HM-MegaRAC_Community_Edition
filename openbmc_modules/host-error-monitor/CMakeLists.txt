cmake_minimum_required (VERSION 3.6)
project (host-error-monitor CXX)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable (host-error-monitor src/host_error_monitor.cpp)

option (HOST_ERROR_CRASHDUMP_ON_SMI_TIMEOUT "Enable crashdump on SMI Timeout."
        OFF)

option (YOCTO "Enable Building in Yocto" OFF)

if (NOT YOCTO)
    add_dependencies (host-error-monitor libpeci)
    externalproject_add (libpeci PREFIX ${CMAKE_BINARY_DIR}/libpeci
                         GIT_REPOSITORY https://github.com/openbmc/libpeci.git
                         GIT_TAG a2ceec2aa139277cebb62e1eda449ef60fa4c962
                         INSTALL_COMMAND "")

    externalproject_get_property (libpeci SOURCE_DIR)
    include_directories (${SOURCE_DIR})

    externalproject_get_property (libpeci BINARY_DIR)
    add_library (peci SHARED IMPORTED)
    set_target_properties (peci
                           PROPERTIES IMPORTED_LOCATION
                                      ${BINARY_DIR}/libpeci.so)
endif ()

target_compile_definitions (host-error-monitor PRIVATE
                            $<$<BOOL:${HOST_ERROR_CRASHDUMP_ON_SMI_TIMEOUT}>:
                            -DHOST_ERROR_CRASHDUMP_ON_SMI_TIMEOUT>)

target_include_directories (host-error-monitor PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries (host-error-monitor sdbusplus -lsystemd gpiodcxx peci)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

install (TARGETS host-error-monitor
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib/static)

find_package (Boost 1.66 REQUIRED)
include_directories (${BOOST_SRC_DIR})

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

set (
    SERVICE_FILES
    ${PROJECT_SOURCE_DIR}/service_files/xyz.openbmc_project.HostErrorMonitor.service
)
install (FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
