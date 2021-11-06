cmake_minimum_required (VERSION 3.6)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

project (rsyslog-rotate CXX)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

set (SOURCES
        ./src/main.cpp
)


add_executable (rsyslog-rotate ${SOURCES})

install (TARGETS rsyslog-rotate DESTINATION /usr/bin/)
