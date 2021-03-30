# Rules for building tests

test(
  'hostlogger',
  executable(
    'hostlogger_test',
    [
      'config_test.cpp',
      'file_storage_test.cpp',
      'host_console_test.cpp',
      'log_buffer_test.cpp',
      'zlib_file_test.cpp',
      '../src/config.cpp',
      '../src/file_storage.cpp',
      '../src/host_console.cpp',
      '../src/log_buffer.cpp',
      '../src/zlib_exception.cpp',
      '../src/zlib_file.cpp',
    ],
    dependencies: [
      dependency('gtest', main: true, disabler: true, required: build_tests),
      dependency('zlib'),
    ],
    include_directories: '../src',
  )
)
