headers = [
  'base.h',
  'pldm_types.h',
  'platform.h',
  'bios.h',
  'bios_table.h',
  'entity.h',
  'states.h',
  'state_set.h',
  'fru.h',
  'utils.h',
  'pdr.h'
]

sources = [
  'base.c',
  'platform.c',
  'bios.c',
  'bios_table.c',
  'fru.c',
  'utils.c',
  'pdr.c'
]

libpldm_headers = ['.', '..']

if get_option('oem-ibm').enabled()
  headers += [
    '../oem/ibm/libpldm/file_io.h',
    '../oem/ibm/libpldm/host.h'
  ]
  sources += [
    '../oem/ibm/libpldm/file_io.c',
    '../oem/ibm/libpldm/host.c'
  ]
  libpldm_headers += ['../oem/ibm']
endif

if get_option('requester-api').enabled()
  headers += [
    'requester/pldm.h'
  ]
  sources += [
    'requester/pldm.c'
  ]
  libpldm_headers += ['requester']
endif

install_headers(
  headers,
  subdir: 'libpldm')

libpldm = library(
  'pldm',
  sources,
  implicit_include_directories: false,
  include_directories: include_directories(libpldm_headers),
  version: meson.project_version(),
  install: true)

libpldm = declare_dependency(
  include_directories: include_directories(libpldm_headers),
  link_with: libpldm)

import('pkgconfig').generate(
  name: 'libpldm',
  description: 'PLDM protocol encode/decode C lib',
  version: meson.project_version(),
  libraries: libpldm)

if get_option('tests').enabled()
  subdir('tests')
endif
