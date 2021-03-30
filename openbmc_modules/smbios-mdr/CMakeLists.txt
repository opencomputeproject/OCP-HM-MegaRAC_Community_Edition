cmake_minimum_required (VERSION 3.2 FATAL_ERROR)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set (DBUS_OBJECT_NAME "xyz/openbmc_project/Smbios/MDR_V2")
set (DBUS_INTF_NAME "xyz.openbmc_project.Smbios.MDR_V2")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

include_directories (
    ${CMAKE_CURRENT_SOURCE_DIR}/include/phosphor-dbus-interfaces
)
include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories (${DBUSINTERFACE_LIBRARIES})

find_package (PkgConfig REQUIRED)
pkg_check_modules (SYSTEMD libsystemd REQUIRED)
include_directories (${SYSTEMD_INCLUDE_DIRS})
link_directories (${SYSTEMD_LIBRARY_DIRS})

# import sdbusplus
find_package (PkgConfig REQUIRED)
pkg_check_modules (SDBUSPLUSPLUS sdbusplus REQUIRED)
include_directories (${SDBUSPLUSPLUS_INCLUDE_DIRS})
link_directories (${SDBUSPLUSPLUS_LIBRARY_DIRS})

# phosphor-dbus-interfaces
find_package (PkgConfig REQUIRED)
pkg_check_modules (DBUSINTERFACE phosphor-dbus-interfaces REQUIRED)
include_directories (${DBUSINTERFACE_INCLUDE_DIRS})
link_directories (${DBUSINTERFACE_LIBRARY_DIRS})

set (SRC_FILES src/mdrv2.cpp src/mdrv2_main.cpp src/cpu.cpp src/dimm.cpp
     src/system.cpp)

include_directories (${CMAKE_CURRENT_BINARY_DIR})

find_package (PkgConfig REQUIRED)
pkg_check_modules (SDBUSPLUSPLUS sdbusplus REQUIRED)
include_directories (${SDBUSPLUSPLUS_INCLUDE_DIRS})
link_directories (${SDBUSPLUSPLUS_LIBRARY_DIRS})
find_program (SDBUSPLUSPLUS sdbus++)

add_executable (smbiosmdrv2app ${SRC_FILES})
target_link_libraries (smbiosmdrv2app ${SYSTEMD_LIBRARIES})
target_link_libraries (smbiosmdrv2app ${DBUSINTERFACE_LIBRARIES})
target_link_libraries (smbiosmdrv2app ${SDBUSPLUSPLUS_LIBRARIES})
target_link_libraries (smbiosmdrv2app phosphor_logging)

install (TARGETS smbiosmdrv2app DESTINATION bin)
