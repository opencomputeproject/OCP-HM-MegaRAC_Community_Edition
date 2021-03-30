gpioplus_headers = include_directories('.')

gpioplus_lib = library(
  'gpioplus',
  [
    'gpioplus/chip.cpp',
    'gpioplus/event.cpp',
    'gpioplus/handle.cpp',
    'gpioplus/internal/fd.cpp',
    'gpioplus/internal/sys.cpp',
  ],
  include_directories: gpioplus_headers,
  implicit_include_directories: false,
  version: meson.project_version(),
  install: true)

gpioplus = declare_dependency(
  include_directories: gpioplus_headers,
  link_with: gpioplus_lib,
)

import('pkgconfig').generate(
  name: 'gpioplus',
  description: 'C++ systemd event wrapper',
  version: meson.project_version(),
  libraries: gpioplus)

install_headers(
  'gpioplus/chip.hpp',
  'gpioplus/event.hpp',
  'gpioplus/handle.hpp',
  subdir: 'gpioplus')

install_headers(
  'gpioplus/internal/fd.hpp',
  'gpioplus/internal/sys.hpp',
  subdir: 'gpioplus/internal')

install_headers(
  'gpioplus/test/event.hpp',
  'gpioplus/test/handle.hpp',
  'gpioplus/test/sys.hpp',
  subdir: 'gpioplus/test')

install_headers(
  'gpioplus/utility/aspeed.hpp',
  subdir: 'gpioplus/utility')
