cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED ON)

project (watchdog-reset C)

set (SOURCES
	./src/watchdog-reset.c
)

add_executable (watchdog-reset ${SOURCES})

install (TARGETS watchdog-reset DESTINATION bin)
