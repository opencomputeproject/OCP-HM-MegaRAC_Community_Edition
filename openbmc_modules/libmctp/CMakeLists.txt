cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

add_definitions (-DMCTP_LOG_STDERR)
add_definitions (-DMCTP_HAVE_FILEIO)
add_definitions (-DMCTP_HAVE_STDIO)
add_definitions (-DMCTP_DEFAULT_ALLOC)

add_library (mctp STATIC alloc.c astlpc.c core.c log.c libmctp.h serial.c)

target_include_directories (mctp PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<INSTALL_INTERFACE:include/libmctp)

enable_testing ()

add_executable (test_eid tests/test_eid.c tests/test-utils.c)
target_link_libraries (test_eid mctp)
add_test (NAME eid COMMAND test_eid)

add_executable (test_seq tests/test_seq.c tests/test-utils.c)
target_link_libraries (test_seq mctp)
add_test (NAME seq COMMAND test_seq)

add_executable (test_bridge tests/test_bridge.c tests/test-utils.c)
target_link_libraries (test_bridge mctp)
add_test (NAME bridge COMMAND test_bridge)

add_executable (test_astlpc tests/test_astlpc.c tests/test-utils.c)
target_link_libraries (test_astlpc mctp)
add_test (NAME astlpc COMMAND test_astlpc)

add_executable (test_serial tests/test_serial.c tests/test-utils.c)
target_link_libraries (test_serial mctp)
add_test (NAME serial COMMAND test_serial)

add_executable (test_cmds tests/test_cmds.c tests/test-utils.c)
target_link_libraries (test_cmds mctp)
add_test (NAME control_commands COMMAND test_cmds)

install (TARGETS mctp DESTINATION lib)
install (FILES libmctp.h DESTINATION include)

