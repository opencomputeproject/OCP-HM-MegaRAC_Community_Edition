cmake_minimum_required (VERSION 3.1 FATAL_ERROR)
set (BUILD_SHARED_LIBRARIES OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
if (NOT YOCTO) # to download gtest
    include ("cmake/HunterGate.cmake")
    huntergate (URL "https://github.com/ruslo/hunter/archive/v0.18.64.tar.gz"
                SHA1 "baf9c8cc4f65306f0e442b5419967b4c4c04589a")
endif ()

project (entity-manager CXX)

set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
    -Werror \
    -Wall \
    -Wextra \
    -Wshadow \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wconversion \
    -Wmisleading-indentation \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wnull-dereference \
    -Wuseless-cast \
    -Wdouble-promotion \
    -Wformat=2 \
"
)

# todo: fix these warnings
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing -Wno-cast-align")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-rtti")

option (YOCTO "Enable Building in Yocto" OFF)

if (NOT YOCTO)
    externalproject_add (
        Boost URL
        https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz
        URL_MD5 b50944c0c13f81ce2c006802a1186f5a SOURCE_DIR
        "${CMAKE_BINARY_DIR}/boost-src" BINARY_DIR
        "${CMAKE_BINARY_DIR}/boost-build" CONFIGURE_COMMAND "" BUILD_COMMAND ""
        INSTALL_COMMAND mkdir -p "${CMAKE_BINARY_DIR}/prefix/include/" && cp -R
        ${CMAKE_BINARY_DIR}/boost-src/boost ${CMAKE_BINARY_DIR}/prefix/include
    )

    externalproject_add (
        nlohmann-json GIT_REPOSITORY "https://github.com/nlohmann/json.git"
        GIT_TAG 456478b3c50d60100dbb1fb9bc931f370a2c1c28 SOURCE_DIR
        "${CMAKE_BINARY_DIR}/nlohmann-json-src" BINARY_DIR
        "${CMAKE_BINARY_DIR}/nlohmann-json-build" CONFIGURE_COMMAND ""
        BUILD_COMMAND "" INSTALL_COMMAND mkdir -p
        "${CMAKE_BINARY_DIR}/nlohmann/include/nlohmann" && cp -r
        "${CMAKE_BINARY_DIR}/nlohmann-json-src/single_include/nlohmann"
        "${CMAKE_BINARY_DIR}/nlohmann/include"
    )

    externalproject_add (valijson GIT_REPOSITORY
                         "https://github.com/tristanpenman/valijson.git"
                         GIT_TAG c2f22fddf599d04dc33fcd7ed257c698a05345d9
                         SOURCE_DIR "${CMAKE_BINARY_DIR}/valijson-src"
                         BINARY_DIR "${CMAKE_BINARY_DIR}/valijson-build"
                         CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND
                         mkdir -p
                         "${CMAKE_BINARY_DIR}/valijson/include/vaijson" && cp
                         -r "${CMAKE_BINARY_DIR}/valijson-src/include"
                         "${CMAKE_BINARY_DIR}/valijson")

    # requires apt install autoconf-archive and autoconf
    externalproject_add (sdbusplus-project PREFIX
                         ${CMAKE_BINARY_DIR}/sdbusplus-project GIT_REPOSITORY
                         https://github.com/openbmc/sdbusplus.git GIT_TAG
                         72f24b0773530b0404c2a20d06272c31d85923d8 SOURCE_DIR
                         ${CMAKE_BINARY_DIR}/sdbusplus-src BINARY_DIR
                         ${CMAKE_BINARY_DIR}/sdbusplus-build CONFIGURE_COMMAND
                         "" BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/sdbusplus-src
                         && meson build -Ddefault_library=static && ninja -C
                         build libsdbusplus.a INSTALL_COMMAND ""
                         LOG_DOWNLOAD ON)

    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/sdbusplus-src)
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/nlohmann/include)
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/nlohmann/include/nlohmann)
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/valijson/include)
    include_directories (SYSTEM
                         ${CMAKE_BINARY_DIR}/phosphor-dbus-interfaces/include)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/build)

    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/boost-src)
    set (CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/boost-src ${CMAKE_PREFIX_PATH})

    option (HUNTER_ENABLED "Enable hunter package pulling" ON)
    hunter_add_package (GTest)

    find_package (GTest CONFIG REQUIRED)

    enable_testing ()

    add_executable (entityManagerTests test/test_entity-manager.cpp
                    src/Utils.cpp)
    add_test (NAME test_entitymanager COMMAND entityManagerTests)
    target_link_libraries (entityManagerTests GTest::main GTest::gtest)
    target_link_libraries (entityManagerTests -lsystemd)
    target_link_libraries (entityManagerTests stdc++fs)
    target_link_libraries (entityManagerTests ${Boost_LIBRARIES})
    target_link_libraries (entityManagerTests sdbusplus)

    find_package (PythonInterp REQUIRED)
    find_package (Git REQUIRED)
    execute_process (COMMAND ${PYTHON_EXECUTABLE}
                             ${CMAKE_CURRENT_SOURCE_DIR}/scripts/autojson.py
                             ${CMAKE_CURRENT_SOURCE_DIR}/configurations)
    execute_process (COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_CURRENT_SOURCE_DIR}
                             diff
                             --quiet configurations
                     RESULT_VARIABLE ret)
    if (ret EQUAL "1")
        message (FATAL_ERROR
                     "Invalid JSON Format, Please rerun scripts/autojson.")
    endif ()
endif ()

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable (fru-device src/FruDevice.cpp src/Utils.cpp)

target_link_libraries (fru-device pthread)
target_link_libraries (fru-device stdc++fs)
target_link_libraries (fru-device i2c)
target_link_libraries (fru-device ${Boost_LIBRARIES})
target_link_libraries (fru-device -lsystemd)
target_link_libraries (fru-device sdbusplus)

add_executable (entity-manager src/EntityManager.cpp src/Overlay.cpp
                src/Utils.cpp)

target_link_libraries (entity-manager -lsystemd)
target_link_libraries (entity-manager stdc++fs)
target_link_libraries (entity-manager ${Boost_LIBRARIES})
target_link_libraries (entity-manager sdbusplus)

if (NOT YOCTO)
    add_dependencies (entity-manager nlohmann-json)
    add_dependencies (entity-manager sdbusplus-project)
    add_dependencies (entity-manager valijson)
    add_dependencies (entityManagerTests nlohmann-json)
    add_dependencies (entityManagerTests sdbusplus-project)
    add_dependencies (entityManagerTests valijson)
    add_dependencies (fru-device nlohmann-json)
    add_dependencies (fru-device valijson)
    add_dependencies (fru-device sdbusplus-project)
    add_dependencies (valijson nlohmann-json)
endif ()

set (
    SERVICE_FILES
    ${PROJECT_SOURCE_DIR}/service_files/xyz.openbmc_project.EntityManager.service
    ${PROJECT_SOURCE_DIR}/service_files/xyz.openbmc_project.FruDevice.service
)

set (PACKAGE_DIR /usr/share/entity-manager/)
target_compile_definitions (entity-manager PRIVATE PACKAGE_DIR="${PACKAGE_DIR}"
                            -DBOOST_ASIO_DISABLE_THREADS)
target_compile_definitions (fru-device PRIVATE PACKAGE_DIR="${PACKAGE_DIR}")
install (TARGETS fru-device entity-manager DESTINATION bin)
install (DIRECTORY configurations DESTINATION ${PACKAGE_DIR})
install (DIRECTORY schemas DESTINATION ${PACKAGE_DIR}/configurations)
install (FILES ${SERVICE_FILES} DESTINATION /lib/systemd/system/)
install (FILES blacklist.json DESTINATION ${PACKAGE_DIR})
