cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Set project name here
project(testmctp C)

option(BUILD_STANDALONE "Use outside of YOCTO depedencies system" OFF)

set(BUILD_SHARED_LIBRARIES OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DMCTP_ASTPCIE_RESPONSE_WA)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

add_executable(testmctp amiregister.c )
add_dependencies(testmctp ${PROJECT_NAME})
target_link_libraries(testmctp mctpw mctpami)

find_package (Boost 1.71 REQUIRED)
include_directories (SYSTEM ${BOOST_SRC_DIR})

include_directories (${PROJECT_SOURCE_DIR})

target_link_libraries (${PROJECT_NAME} sdbusplus -lsystemd -lboost_coroutine
                        -lpthread -lstdc++fs -lmctp_intel)

INSTALL(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION /usr/lib
        PUBLIC_HEADER DESTINATION /usr/include
)
