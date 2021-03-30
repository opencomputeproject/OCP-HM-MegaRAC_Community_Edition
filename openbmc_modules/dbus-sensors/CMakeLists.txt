cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)
set (BUILD_SHARED_LIBRARIES OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
    -Werror \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wmisleading-indentation \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wnull-dereference \
    -Wuseless-cast \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-sign-compare \
    -Wno-reorder \
"
)
# todo: get rid of nos, add the below:
#  -Wshadow \
#  -Wconversion \

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

option (YOCTO "Enable Building in Yocto" OFF)
option (HUNTER_ENABLED "Enable hunter package pulling" OFF)

option (DISABLE_ADC "Disable installing ADC sensor" OFF)
option (DISABLE_CPU "Disable installing CPU sensor" OFF)
option (DISABLE_EXIT_AIR "Disable installing Exit Air Temp sensor" OFF)
option (DISABLE_FAN "Disable installing fan sensor" OFF)
option (DISABLE_HWMON_TEMP "Disable installing hwmon temp sensor" OFF)
option (DISABLE_INTRUSION "Disable installing intrusion sensor" OFF)
option (DISABLE_IPMB "Disable installing IPMB sensor" OFF)
option (DISABLE_MCUTEMP "Disable installing MCU temperature sensor" OFF)
option (DISABLE_PSU "Disable installing PSU sensor" OFF)
option (DISABLE_NVME "Disable installing NVME sensor" ON)

include ("cmake/HunterGate.cmake")

huntergate (URL "https://github.com/ruslo/hunter/archive/v0.18.64.tar.gz" SHA1
            "baf9c8cc4f65306f0e442b5419967b4c4c04589a")

project (sensors CXX)

set (ADC_SRC_FILES src/Utils.cpp src/ADCSensor.cpp src/Thresholds.cpp)

set (CPU_SRC_FILES src/Utils.cpp src/CPUSensor.cpp src/Thresholds.cpp)

set (EXIT_AIR_SRC_FILES src/Utils.cpp src/Thresholds.cpp)

set (FAN_SRC_FILES src/TachSensor.cpp src/PwmSensor.cpp src/Utils.cpp
     src/Thresholds.cpp)

set (HWMON_TEMP_SRC_FILES src/Utils.cpp src/HwmonTempSensor.cpp
     src/Thresholds.cpp)

set (INTRUSION_SRC_FILES src/Utils.cpp src/ChassisIntrusionSensor.cpp)

set (IPMB_SRC_FILES src/Utils.cpp src/Thresholds.cpp)

set (MCUTEMP_SRC_FILES src/Utils.cpp src/Thresholds.cpp)

set (PSU_SRC_FILES src/Utils.cpp src/PSUSensor.cpp src/Thresholds.cpp
     src/PwmSensor.cpp src/PSUEvent.cpp)

set (NVME_SRC_FILES src/Utils.cpp src/NVMeSensorMain.cpp src/NVMeSensor.cpp src/Thresholds.cpp)

set (EXTERNAL_PACKAGES Boost sdbusplus-project nlohmann-json)
set (SENSOR_LINK_LIBS -lsystemd stdc++fs sdbusplus)

if (NOT YOCTO)
    set (DISABLE_NVME ON) # todo allow this to build out of tree
    option (ENABLE_TEST "Enable Google Test" OFF)

    externalproject_add (
        Boost URL
        https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
        URL_MD5 d275cd85b00022313c171f602db59fc5 SOURCE_DIR
        "${CMAKE_BINARY_DIR}/boost-src" BINARY_DIR
        "${CMAKE_BINARY_DIR}/boost-build" CONFIGURE_COMMAND "" BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/boost-src)
    set (CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/boost-src ${CMAKE_PREFIX_PATH})

    # requires apt install autoconf-archive and autoconf
    externalproject_add (sdbusplus-project PREFIX
                         ${CMAKE_BINARY_DIR}/sdbusplus-project GIT_REPOSITORY
                         https://github.com/openbmc/sdbusplus.git SOURCE_DIR
                         ${CMAKE_BINARY_DIR}/sdbusplus-src BINARY_DIR
                         ${CMAKE_BINARY_DIR}/sdbusplus-build CONFIGURE_COMMAND
                         "" BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/sdbusplus-src
                         && meson build -Ddefault_library=static && ninja -C
                         build libsdbusplus.a INSTALL_COMMAND ""
                         LOG_DOWNLOAD ON)
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/sdbusplus-src)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/build)

    externalproject_add (nlohmann-json PREFIX
                         ${CMAKE_CURRENT_BINARY_DIR}/nlohmann-json
                         GIT_REPOSITORY https://github.com/nlohmann/json.git
                         SOURCE_DIR ${CMAKE_BINARY_DIR}/nlohmann-json-src
                         BINARY_DIR ${CMAKE_BINARY_DIR}/nlohmann-json-build
                         CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND
                         "" LOG_DOWNLOAD ON)
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/nlohmann-json-src/include)

    if (ENABLE_TEST)
        option (HUNTER_ENABLED "Enable hunter package pulling" ON)
        hunter_add_package (GTest)

        find_package (GTest CONFIG REQUIRED)

        enable_testing ()

        add_executable (runTachTests tests/test_TachSensor.cpp ${FAN_SRC_FILES})
        add_test (NAME test_fansensor COMMAND runTachTests)
        target_link_libraries (runTachTests GTest::main GTest::gtest pthread
                               ${DBUS_LIBRARIES} stdc++fs)
        add_dependencies (runTachTests nlohmann-json)

        add_executable (runHwmonTempTests tests/test_HwmonTempSensor.cpp
                        ${HWMON_TEMP_SRC_FILES})
        add_test (NAME test_hwmontempsensor COMMAND runHwmonTempTests)
        target_link_libraries (runHwmonTempTests GTest::main GTest::gtest
                               pthread ${DBUS_LIBRARIES} stdc++fs)
        add_dependencies (runHwmonTempTests nlohmann-json)
    endif ()

endif ()

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)

link_directories (${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable (adcsensor src/ADCSensorMain.cpp ${ADC_SRC_FILES})
add_dependencies (adcsensor sdbusplus-project)
target_link_libraries (adcsensor ${SENSOR_LINK_LIBS})
target_link_libraries (adcsensor gpiodcxx)

add_executable (cpusensor src/CPUSensorMain.cpp ${CPU_SRC_FILES})
add_dependencies (cpusensor sdbusplus-project)
target_link_libraries (cpusensor ${SENSOR_LINK_LIBS})
target_link_libraries (cpusensor gpiodcxx)

add_executable (exitairtempsensor src/ExitAirTempSensor.cpp
                ${EXIT_AIR_SRC_FILES})
add_dependencies (exitairtempsensor sdbusplus-project)
target_link_libraries (exitairtempsensor ${SENSOR_LINK_LIBS})

add_executable (fansensor src/FanMain.cpp ${FAN_SRC_FILES})
add_dependencies (fansensor sdbusplus-project)
target_link_libraries (fansensor ${SENSOR_LINK_LIBS})
target_link_libraries (fansensor gpiodcxx)

add_executable (hwmontempsensor src/HwmonTempMain.cpp ${HWMON_TEMP_SRC_FILES})
add_dependencies (hwmontempsensor sdbusplus-project)
target_link_libraries (hwmontempsensor ${SENSOR_LINK_LIBS})

add_executable (intrusionsensor src/IntrusionSensorMain.cpp
                ${INTRUSION_SRC_FILES})
add_dependencies (intrusionsensor sdbusplus-project)
target_link_libraries (intrusionsensor ${SENSOR_LINK_LIBS})
target_link_libraries (intrusionsensor i2c)
target_link_libraries (intrusionsensor gpiodcxx)

add_executable (ipmbsensor src/IpmbSensor.cpp ${IPMB_SRC_FILES})
add_dependencies (ipmbsensor sdbusplus-project)
target_link_libraries (ipmbsensor ${SENSOR_LINK_LIBS})

add_executable (mcutempsensor src/MCUTempSensor.cpp ${MCUTEMP_SRC_FILES})
add_dependencies (mcutempsensor sdbusplus-project)
target_link_libraries (mcutempsensor ${SENSOR_LINK_LIBS})
target_link_libraries (mcutempsensor i2c)

add_executable (psusensor src/PSUSensorMain.cpp ${PSU_SRC_FILES})
add_dependencies (psusensor sdbusplus-project)
target_link_libraries (psusensor ${SENSOR_LINK_LIBS})

if (NOT DISABLE_NVME)
    add_executable (nvmesensor ${NVME_SRC_FILES})
    add_dependencies (nvmesensor sdbusplus-project)
    target_link_libraries (nvmesensor liblibmctp.a i2c ${SENSOR_LINK_LIBS})
endif()

if (NOT YOCTO)
    add_dependencies (adcsensor ${EXTERNAL_PACKAGES})
    add_dependencies (cpusensor ${EXTERNAL_PACKAGES})
    add_dependencies (exitairtempsensor ${EXTERNAL_PACKAGES})
    add_dependencies (fansensor ${EXTERNAL_PACKAGES})
    add_dependencies (hwmontempsensor ${EXTERNAL_PACKAGES})
    add_dependencies (intrusionsensor ${EXTERNAL_PACKAGES})
    add_dependencies (ipmbsensor ${EXTERNAL_PACKAGES})
    add_dependencies (mcutempsensor ${EXTERNAL_PACKAGES})
    add_dependencies (psusensor ${EXTERNAL_PACKAGES})
endif ()

set (SERVICE_FILE_SRC_DIR ${PROJECT_SOURCE_DIR}/service_files)
set (SERVICE_FILE_INSTALL_DIR /lib/systemd/system/)

if (NOT DISABLE_ADC)
    install (TARGETS adcsensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.adcsensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()

if (NOT DISABLE_CPU)
    install (TARGETS cpusensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.cpusensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()

if (NOT DISABLE_EXIT_AIR)
    install (TARGETS exitairtempsensor DESTINATION bin)
    install (
        FILES ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.exitairsensor.service
              DESTINATION ${SERVICE_FILE_INSTALL_DIR}
    )
endif ()

if (NOT DISABLE_FAN)
    install (TARGETS fansensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.fansensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()

if (NOT DISABLE_HWMON_TEMP)
    install (TARGETS hwmontempsensor DESTINATION bin)
    install (
        FILES
            ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.hwmontempsensor.service
            DESTINATION ${SERVICE_FILE_INSTALL_DIR}
    )
endif ()

if (NOT DISABLE_INTRUSION)
    install (TARGETS intrusionsensor DESTINATION bin)
    install (
        FILES
            ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.intrusionsensor.service
            DESTINATION ${SERVICE_FILE_INSTALL_DIR}
    )
endif ()

if (NOT DISABLE_IPMB)
    install (TARGETS ipmbsensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.ipmbsensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()

if (NOT DISABLE_MCUTEMP)
    install (TARGETS mcutempsensor DESTINATION bin)
    install (
        FILES ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.mcutempsensor.service
              DESTINATION ${SERVICE_FILE_INSTALL_DIR}
    )
endif ()

if (NOT DISABLE_PSU)
    install (TARGETS psusensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.psusensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()

if (NOT DISABLE_NVME)
    install (TARGETS nvmesensor DESTINATION bin)
    install (FILES
                 ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.nvmesensor.service
                 DESTINATION ${SERVICE_FILE_INSTALL_DIR})
endif ()
