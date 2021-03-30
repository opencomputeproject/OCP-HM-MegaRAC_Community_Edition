# Function2 might not have a pkg-config. It is header only so just make
# sure we can access the needed symbols from the header.
function2_dep = dependency('function2', required: false)
has_function2 = meson.get_compiler('cpp').has_header_symbol(
  'function2/function2.hpp',
  'fu2::unique_function',
  dependencies: function2_dep,
  required: false)
if not has_function2
  function2_proj = import('cmake').subproject(
    'function2',
    cmake_options: [
	  '-DBUILD_TESTING=OFF',
    ],
    required: false)
  assert(function2_proj.found(), 'function2 is required')
  if function2_proj.found()
    function2_dep = function2_proj.dependency('function2')
  endif
endif

sdeventplus_deps = [
  dependency('libsystemd', version: '>=240'),
  dependency('stdplus', fallback: ['stdplus', 'stdplus']),
  function2_dep,
]

sdeventplus_headers = include_directories('.')

sdeventplus_lib = library(
  'sdeventplus',
  [
    'sdeventplus/clock.cpp',
    'sdeventplus/event.cpp',
    'sdeventplus/exception.cpp',
    'sdeventplus/internal/sdevent.cpp',
    'sdeventplus/source/base.cpp',
    'sdeventplus/source/child.cpp',
    'sdeventplus/source/event.cpp',
    'sdeventplus/source/io.cpp',
    'sdeventplus/source/signal.cpp',
    'sdeventplus/source/time.cpp',
    'sdeventplus/utility/timer.cpp',
  ],
  include_directories: sdeventplus_headers,
  implicit_include_directories: false,
  version: meson.project_version(),
  dependencies: sdeventplus_deps,
  install: true)

sdeventplus = declare_dependency(
  dependencies: sdeventplus_deps,
  include_directories: sdeventplus_headers,
  link_with: sdeventplus_lib)

sdeventplus_reqs = []
foreach dep : sdeventplus_deps
  if dep.type_name() == 'pkgconfig'
	sdeventplus_reqs += dep
  endif
endforeach

import('pkgconfig').generate(
  name: 'sdeventplus',
  description: 'C++ systemd event wrapper',
  version: meson.project_version(),
  libraries: sdeventplus,
  requires: sdeventplus_reqs)

install_headers(
  'sdeventplus/clock.hpp',
  'sdeventplus/event.hpp',
  'sdeventplus/exception.hpp',
  'sdeventplus/types.hpp',
  subdir: 'sdeventplus')

install_headers(
  'sdeventplus/internal/sdevent.hpp',
  'sdeventplus/internal/utils.hpp',
  subdir: 'sdeventplus/internal')

install_headers(
  'sdeventplus/source/base.hpp',
  'sdeventplus/source/child.hpp',
  'sdeventplus/source/event.hpp',
  'sdeventplus/source/io.hpp',
  'sdeventplus/source/signal.hpp',
  'sdeventplus/source/time.hpp',
  subdir: 'sdeventplus/source')

install_headers(
  'sdeventplus/test/sdevent.hpp',
  subdir: 'sdeventplus/test')

install_headers(
  'sdeventplus/utility/timer.hpp',
  subdir: 'sdeventplus/utility')
