#
# INTEL CONFIDENTIAL
#
# Copyright 2019 Intel Corporation.
#
# This software and the related documents are Intel copyrighted materials, and
# your use of them is governed by the express license under which they were
# provided to you ("License"). Unless the License provides otherwise, you may
# not use, modify, copy, publish, distribute, disclose or transmit this software
# or the related documents without Intel's prior written permission.
#
# This software and the related documents are provided as is, with no express or
# implied warranties, other than those that are expressly stated in the License.
#

cmake_minimum_required (VERSION 3.6)
option (YOCTO_DEPENDENCIES "Use YOCTO dependencies system" OFF)
option (CRASHDUMP_BUILD_UT "Enable Building Unit tests" OFF)

project (crashdump)
include (ExternalProject)
if (${YOCTO_DEPENDENCIES})
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (SAFEC REQUIRED libsafec)
    include_directories (${SAFEC_INCLUDE_DIRS})
    link_directories (${SAFEC_LIBRARY_DIRS})
endif ()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (EXTERNAL_PACKAGES Boost cjson-project sdbusplus-project safeclib-project
                       libpeci-project
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") # require at least gcc 8.0
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        message (FATAL_ERROR "GCC version must be at least 8.0")
    endif ()
endif ()

if (NOT ${YOCTO_DEPENDENCIES})

    externalproject_add (
        Boost
        URL https://dl.bintray.com/boostorg/release/1.73.0/source/boost_1_73_0.tar.gz
        URL_MD5 4036cd27ef7548b8d29c30ea10956196
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )
    externalproject_get_property (Boost SOURCE_DIR)
    include_directories (SYSTEM ${SOURCE_DIR})
    set (CMAKE_PREFIX_PATH ${SOURCE_DIR} ${CMAKE_PREFIX_PATH})

    # requires apt install autoconf-archive and autoconf
    externalproject_add (
        sdbusplus-project
        PREFIX ${CMAKE_BINARY_DIR}/sdbusplus-project
        GIT_REPOSITORY https://github.com/openbmc/sdbusplus.git
        GIT_TAG 0f19c87276e46b56edbae70a71749353d401ed39
        SOURCE_DIR ${CMAKE_BINARY_DIR}/sdbusplus-src
        BINARY_DIR ${CMAKE_BINARY_DIR}/sdbusplus-build
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/sdbusplus-src && meson build
                      -Ddefault_library=static && ninja -C build libsdbusplus.a
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
    )
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/sdbusplus-src/include)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/build)

    externalproject_add (
        safeclib-project
        GIT_REPOSITORY https://github.com/rurban/safeclib.git
        GIT_TAG 60786283fd61cd621a5d1df00e083a1c1e3cf52a
        SOURCE_DIR "${CMAKE_BINARY_DIR}/safeclib-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/safeclib-build"
        INSTALL_DIR "${CMAKE_BINARY_DIR}/safeclib-install"
        CONFIGURE_COMMAND
            cd ${CMAKE_BINARY_DIR}/safeclib-src/ && ./build-aux/autogen.sh &&
            ./configure --prefix=${CMAKE_BINARY_DIR}/safeclib-install
        BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/safeclib-src && make -j 10
        INSTALL_COMMAND cd ${CMAKE_BINARY_DIR}/safeclib-src && make install
        LOG_DOWNLOAD ON
    )
    include_directories (
        SYSTEM ${CMAKE_BINARY_DIR}/safeclib-install/include/libsafec
    )
    externalproject_get_property (safeclib-project INSTALL_DIR)
    add_library (safec SHARED IMPORTED)
    set_target_properties (
        safec PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libsafec-3.5.1.so
    )

    externalproject_add (
        libpeci-project
        PREFIX ${CMAKE_BINARY_DIR}/libpeci-project
        GIT_REPOSITORY https://github.com/openbmc/libpeci.git
        GIT_TAG a2ceec2aa139277cebb62e1eda449ef60fa4c962
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
    )
    externalproject_get_property (libpeci-project SOURCE_DIR)
    include_directories (${SOURCE_DIR})
    externalproject_get_property (libpeci-project BINARY_DIR)
    add_library (peci SHARED IMPORTED)
    set_target_properties (
        peci PROPERTIES IMPORTED_LOCATION ${BINARY_DIR}/libpeci.so
    )

    externalproject_add (
        cjson-project
        GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
        GIT_TAG v1.7.12
        SOURCE_DIR "${CMAKE_BINARY_DIR}/cjson-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/cjson-build"
        INSTALL_DIR "${CMAKE_BINARY_DIR}/cjson-install"
        CONFIGURE_COMMAND
            cd ${CMAKE_BINARY_DIR}/cjson-src/ && cmake . -DENABLE_CJSON_UTILS=On
            -DENABLE_CJSON_TEST=Off
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/cjson-install
        BUILD_COMMAND cd ${CMAKE_BINARY_DIR}/cjson-src && make -j 10
        INSTALL_COMMAND cd ${CMAKE_BINARY_DIR}/cjson-src && make install
        LOG_DOWNLOAD ON
    )
    include_directories (SYSTEM ${CMAKE_BINARY_DIR}/cjson-install/include/)
    externalproject_get_property (cjson-project INSTALL_DIR)
    add_library (cjson SHARED IMPORTED)
    set_target_properties (
        cjson PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libcjson.so
    )

endif ()

if (NOT ${YOCTO_DEPENDENCIES})
    set (
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wno-cpp -Wno-write-strings -Werror \
-Wall \
-Wuninitialized \
-Wunused-parameter \
-Wshift-negative-value \
-Wuninitialized \
-Wtype-limits \
-Wsign-compare \
-Wmissing-field-initializers \
-Wimplicit-fallthrough=3 \
-Wempty-body \
-Wignored-qualifiers \
-Wclobbered \
-Wcast-function-type \
"
    )
endif ()

if (${YOCTO_DEPENDENCIES})
    set (
        CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wno-cpp -Wno-write-strings -Werror \
-Wall \
-Wuninitialized \
-Wunused-parameter \
-Wshift-negative-value \
-Wuninitialized \
-Wtype-limits \
-Wredundant-move \
-Wstring-compare \
-Wsign-compare \
-Wdeprecated-copy \
-Wmissing-field-initializers \
-Wimplicit-fallthrough=3 \
-Wempty-body \
-Wignored-qualifiers \
-Wclobbered \
-Wcast-function-type"
    )
endif ()

set (
    SRC_FILES
    utils_dbusplus.cpp
    CrashdumpSections/AddressMap.c
    CrashdumpSections/TorDump.c
    CrashdumpSections/BigCore.c
    CrashdumpSections/MetaData.c
    CrashdumpSections/CoreMca.c
    CrashdumpSections/UncoreMca.c
    CrashdumpSections/Uncore.c
    CrashdumpSections/PowerManagement.c
    CrashdumpSections/OemData.c
    CrashdumpSections/utils.c
    CrashdumpSections/crashdump.c
)

add_executable (crashdump crashdump.cpp ${SRC_FILES})

if (NOT ${YOCTO_DEPENDENCIES})
    add_dependencies (crashdump ${EXTERNAL_PACKAGES})
    target_link_libraries (crashdump safec sdbusplus peci cjson)
endif ()

option (CRASHDUMP_PRINT_UNFORMATTED
        "Use cJSON_PrintUnformatted() to print the crashdump contents without
         whitespace to reduce the file size." OFF
)

option (OEMDATA_SECTION
        "Adds oemdata Section to the crashdump contents, for OEM usage." OFF
)

target_compile_definitions (
    crashdump
    PRIVATE $<$<BOOL:${CRASHDUMP_PRINT_UNFORMATTED}>:
            -DCRASHDUMP_PRINT_UNFORMATTED> $<$<BOOL:${OEMDATA_SECTION}>:
            -DOEMDATA_SECTION>
)

target_include_directories (crashdump PRIVATE ${CMAKE_SOURCE_DIR})
set_property (TARGET crashdump PROPERTY C_STANDARD 99)
target_link_libraries (
    crashdump
    cjson
    peci
    -lsystemd
    pthread
    -lstdc++fs
    sdbusplus
    ${SAFEC_LIBRARIES}
)

install (
    TARGETS crashdump
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
)

install (
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/crashdump_input_icx.json
          ${CMAKE_CURRENT_SOURCE_DIR}/telemetry_input_icx.json
          ${CMAKE_CURRENT_SOURCE_DIR}/crashdump_input_spr.json
    PERMISSIONS OWNER_READ
    DESTINATION /usr/share/crashdump/input/
)

find_package (Boost 1.73 REQUIRED)
include_directories (${BOOST_SRC_DIR})

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)

# Unit Tests
if (NOT ${YOCTO_DEPENDENCIES})
    if (${CRASHDUMP_BUILD_UT})
        externalproject_add (
            googletest-project
            GIT_REPOSITORY "https://github.com/google/googletest.git"
            GIT_TAG 2134e3fd857d952e03ce76064fad5ac6e9036104
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/googletest-build
            SOURCE_DIR "${CMAKE_BINARY_DIR}/googletest-src"
            BINARY_DIR "${CMAKE_BINARY_DIR}/googletest-build"
        )
        externalproject_get_property (googletest-project BINARY_DIR)
        include_directories (${BINARY_DIR}/include)
        externalproject_get_property (googletest-project BINARY_DIR)
        add_library (gmock UNKNOWN IMPORTED)
        set_target_properties (
            gmock
            PROPERTIES "IMPORTED_LOCATION" "${BINARY_DIR}/lib/libgmock.a"
                       "IMPORTED_LINK_INTERFACE_LIBRARIES"
                       "${CMAKE_THREAD_LIBS_INIT}"
        )
        add_library (gtest UNKNOWN IMPORTED)
        set_target_properties (
            gtest
            PROPERTIES "IMPORTED_LOCATION" "${BINARY_DIR}/lib/libgtest.a"
                       "IMPORTED_LINK_INTERFACE_LIBRARIES"
                       "${CMAKE_THREAD_LIBS_INIT}"
        )

        enable_testing ()

        include (CTest)
        include_directories (${CMAKE_SOURCE_DIR})

        set (
            UT_FILES
            tests/CrashdumpSections/SqDump_test.cpp
            tests/CrashdumpSections/BigCore_test.cpp tests/test_utils.cpp
            tests/test_main.cpp tests/CrashdumpSections/Uncore_test.cpp
        )

        add_definitions (-DCOMPILE_UNIT_TESTS)

        add_executable (crashdump_ut ${SRC_FILES} ${UT_FILES})
        add_dependencies (crashdump_ut ${EXTERNAL_PACKAGES} googletest-project)

        target_link_libraries (crashdump_ut cjson peci -lsystemd pthread)
        target_link_libraries (crashdump_ut -lstdc++fs sdbusplus)
        target_link_libraries (crashdump_ut safec ${SAFEC_LIBRARIES})
        target_link_libraries (crashdump_ut gmock gtest)

        add_test (test_crashdump_ut crashdump_ut
                  "--gtest_output=xml:crashdump_test.xml"
        )

    endif ()
endif ()
