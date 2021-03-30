deps = [
  dependency('phosphor-dbus-interfaces'),
  dependency('sdbusplus'),
  dependency('sdeventplus'),
  libpldm,
  libpldmutils
]

sources = [
  'base.cpp',
  'bios.cpp',
  'bios_table.cpp',
  'bios_attribute.cpp',
  'bios_string_attribute.cpp',
  'bios_integer_attribute.cpp',
  'bios_enum_attribute.cpp',
  'bios_config.cpp',
  'pdr_utils.cpp',
  'pdr.cpp',
  'platform.cpp',
  'fru_parser.cpp',
  'fru.cpp',
  '../host-bmc/host_pdr_handler.cpp',
  'event_parser.cpp'
]

if get_option('oem-ibm').enabled()
  sources += [
    '../oem/ibm/libpldmresponder/file_io.cpp',
    '../oem/ibm/libpldmresponder/file_table.cpp',
    '../oem/ibm/libpldmresponder/file_io_by_type.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_pel.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_dump.cpp',
    '../oem/ibm/libpldmresponder/file_io_type_cert.cpp'
  ]
endif

libpldmresponder = library(
  'pldmresponder',
  sources,
  version: meson.project_version(),
  dependencies: deps,
  install: true)

libpldmresponder = declare_dependency(
  link_with: libpldmresponder)
