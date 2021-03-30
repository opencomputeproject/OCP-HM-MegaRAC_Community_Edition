project('pldm', ['c', 'cpp'],
        version: '0.1', meson_version: '>=0.53.2',
        default_options: [
          'warning_level=3',
          'default_library=shared',
          'werror=true',
          'cpp_std=c++17'
        ])

# Wno-psabi reduces the number of "Note:" messages when cross-compiling some STL
# stuff for ARM. See https://stackoverflow.com/questions/48149323/strange-gcc-warning-when-compiling-qt-project
# Basically, gcc 6 and gcc 7 are not ABI compatible, but since the whole OpenBMC
# project uses the same compiler, we can safely ignmore these info notes.
add_project_arguments('-Wno-psabi', language: 'cpp')

conf_data = configuration_data()
conf_data.set_quoted('BIOS_JSONS_DIR', '/usr/share/pldm/bios')
conf_data.set_quoted('BIOS_TABLES_DIR', '/var/lib/pldm/bios')
conf_data.set_quoted('PDR_JSONS_DIR', '/usr/share/pldm/pdr')
conf_data.set_quoted('FRU_JSONS_DIR', '/usr/share/pldm/fru')
conf_data.set_quoted('HOST_JSONS_DIR', '/usr/share/pldm/host')
conf_data.set_quoted('EVENTS_JSONS_DIR', '/usr/share/pldm/events')
if get_option('oem-ibm').enabled()
  conf_data.set_quoted('FILE_TABLE_JSON', '/usr/share/pldm/fileTable.json')
  conf_data.set_quoted('LID_RUNNING_DIR', '/var/lib/phosphor-software-manager/hostfw/running')
  conf_data.set_quoted('LID_ALTERNATE_DIR', '/var/lib/phosphor-software-manager/hostfw/alternate')
  conf_data.set_quoted('LID_PATCH_DIR', '/usr/local/share/hostfw/')
  conf_data.set('DMA_MAXSIZE', get_option('oem-ibm-dma-maxsize'))
  add_global_arguments('-DOEM_IBM', language : 'c')
  add_global_arguments('-DOEM_IBM', language : 'cpp')
endif
configure_file(output: 'config.h',
  configuration: conf_data
)

subdir('libpldm')

if get_option('libpldm-only').disabled()

libpldmutils_headers = ['.']
libpldmutils = library(
  'pldmutils',
  'common/utils.cpp',
  version: meson.project_version(),
  dependencies: [
      libpldm,
      dependency('phosphor-dbus-interfaces'),
      dependency('sdbusplus'),
  ],
  install: true,
  include_directories: include_directories(libpldmutils_headers),
)

libpldmutils = declare_dependency(
  include_directories: include_directories(libpldmutils_headers),
  link_with: libpldmutils)
subdir('libpldmresponder')

deps = [
  libpldm,
  libpldmutils,
  libpldmresponder,
  libpldmutils,
  dependency('sdbusplus'),
  dependency('sdeventplus'),
  dependency('phosphor-dbus-interfaces')
]

executable(
  'pldmd',
  'pldmd/pldmd.cpp',
  'pldmd/dbus_impl_requester.cpp',
  'pldmd/instance_id.cpp',
  'pldmd/dbus_impl_pdr.cpp',
  'host-bmc/dbus_to_host_effecters.cpp',
  implicit_include_directories: false,
  dependencies: deps,
  install: true,
  install_dir: get_option('bindir'))

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])
configure_file(
  copy: true,
  input: 'pldmd/pldmd.service',
  install: true,
  install_dir: systemd_system_unit_dir,
  output: 'pldmd.service',
)

subdir('pldmtool')

subdir('configurations')

if get_option('tests').enabled()
  subdir('test')
endif

if get_option('utilities').enabled()
  subdir('utilities')
endif

endif # pldm-only
