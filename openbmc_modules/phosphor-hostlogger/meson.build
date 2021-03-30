# Rules for building with Meson

project(
  'hostlogger',
  'cpp',
  default_options: [
   'warning_level=3',
   'werror=true',
   'cpp_std=c++17',
  ],
  license: 'Apache-2.0',
)

# version info from git
version = vcs_tag(command: [ 'git', 'describe', '--always', '--dirty', '--long' ],
                  input: 'src/version.hpp.in',
                  output: 'version.hpp')

# unit tests
build_tests = get_option('tests')
subdir('test')

# install systemd unit template file
systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
  'systemdsystemunitdir',
  define_variable: ['prefix', get_option('prefix')],
)
configure_file(
  copy: true,
  input: 'hostlogger@.service',
  install: true,
  install_dir: systemd_system_unit_dir,
  output: 'hostlogger@.service'
)

executable(
  'hostlogger',
  [
    version,
    'src/config.cpp',
    'src/dbus_loop.cpp',
    'src/file_storage.cpp',
    'src/host_console.cpp',
    'src/log_buffer.cpp',
    'src/main.cpp',
    'src/service.cpp',
    'src/zlib_exception.cpp',
    'src/zlib_file.cpp',
  ],
  dependencies: [
    systemd,
    dependency('phosphor-logging'),
    dependency('zlib'),
  ],
  install: true
)
