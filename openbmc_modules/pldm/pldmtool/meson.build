libpldm_headers = ['.', './oem/ibm']

sources = [
  'pldm_cmd_helper.cpp',
  'pldm_base_cmd.cpp',
  'pldm_platform_cmd.cpp',
  'pldm_bios_cmd.cpp',
  'pldm_fru_cmd.cpp',
  'pldmtool.cpp',
]

if get_option('oem-ibm').enabled()
sources += [
    'oem/ibm/pldm_oem_ibm.cpp',
  ]
endif

executable(
  'pldmtool',
  sources,
  implicit_include_directories: false,
  include_directories: include_directories(libpldm_headers),
  dependencies: [libpldm, libpldmutils, dependency('sdbusplus')],
  install: true,
  install_dir: get_option('bindir'))
