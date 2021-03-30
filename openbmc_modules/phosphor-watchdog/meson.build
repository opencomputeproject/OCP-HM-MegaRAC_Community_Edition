project('phosphor-watchdog', 'cpp',
        version: '0.1', meson_version: '>=0.49.0',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++17'
        ])

build_tests = get_option('tests')

phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')

libwatchdog = static_library(
  'watchdog',
  'watchdog.cpp',
  implicit_include_directories: false,
  dependencies: [
    phosphor_dbus_interfaces,
    phosphor_logging,
    sdbusplus,
    sdeventplus,
  ])

executable(
  'phosphor-watchdog',
  'mainapp.cpp',
  implicit_include_directories: false,
  link_with: libwatchdog,
  dependencies: [
    phosphor_logging,
    phosphor_dbus_interfaces,
    sdbusplus,
    sdeventplus,
  ],
  install: true,
  install_dir: get_option('bindir'))

if not build_tests.disabled()
  subdir('test')
endif
