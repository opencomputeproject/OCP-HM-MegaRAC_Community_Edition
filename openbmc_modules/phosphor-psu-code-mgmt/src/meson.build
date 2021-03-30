configure_file(output: 'config.h',
  configuration: cdata,
)
psu_inc = include_directories('.')

executable(
  'phosphor-psu-code-manager',
  'activation.cpp',
  'item_updater.cpp',
  'main.cpp',
  'version.cpp',
  'utils.cpp',
  include_directories: psu_inc,
  dependencies: [
    phosphor_logging,
    phosphor_dbus_interfaces,
    sdbusplus,
    ssl,
  ],
  install: true,
  install_dir: get_option('bindir'))
