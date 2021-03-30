cmake_minimum_required(VERSION 3.6)
project(libpeci)

add_library(peci SHARED peci.c)

set_property(TARGET peci PROPERTY C_STANDARD 99)
target_include_directories(peci PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(peci PROPERTIES VERSION "1.0" SOVERSION "1")

set(
  CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} \
-Wall \
-Wextra \
-Wcast-align \
-Wunused \
-Wconversion \
-Wsign-conversion \
-Wnull-dereference \
-Wdouble-promotion \
-Wformat=2 \
-Wno-unused-parameter \
-Werror \
-Wduplicated-cond \
-Wduplicated-branches \
-Wlogical-op \
"
  )

install(TARGETS peci DESTINATION lib)
install(FILES peci.h DESTINATION include)

add_executable(peci_cmds peci_cmds.c)
add_dependencies(peci_cmds peci)
target_link_libraries(peci_cmds peci)

install(TARGETS peci_cmds
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)
