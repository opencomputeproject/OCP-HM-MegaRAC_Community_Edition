gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
gmock = dependency('gmock', disabler: true, required: build_tests)
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
sdbusplus = dependency('sdbusplus')

tests = [
  'post_reporter_test',
]

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     include_directories: postd_headers,
                     implicit_include_directories: false,
                     dependencies: [
                       gtest,
                       gmock,
                       phosphor_dbus_interfaces,
                       sdbusplus,
                    ]))
endforeach
