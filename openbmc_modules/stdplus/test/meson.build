gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)
if not gtest.found() or not gmock.found()
  gtest_proj = import('cmake').subproject('googletest', required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
    gmock = gtest_proj.dependency('gmock')
  else
    assert(not build_tests.enabled(), 'Googletest is required')
  endif
endif

gtests = [
  'handle/copyable',
  'handle/managed',
  'util/cexec',
  'util/string',
]

foreach t : gtests
  test(t, executable(t.underscorify(), t + '.cpp',
                     build_by_default: false,
                     implicit_include_directories: false,
                     dependencies: [stdplus, gtest, gmock]))
endforeach

# catch2 might not have a pkg-config. It is header only so just make
# sure we can access the needed symbols from the header.
catch2_dep = dependency('Catch2', required: false)
has_catch2 = meson.get_compiler('cpp').has_header_symbol(
  'catch2/catch.hpp',
  'Approx',
  dependencies: catch2_dep,
  required: false)
if not has_catch2
  catch2_proj = import('cmake').subproject(
    'Catch2',
    cmake_options: [
      '-DBUILD_TESTING=OFF'
    ],
    required: false)
  if catch2_proj.found()
    catch2_dep = catch2_proj.dependency('Catch2')
    has_catch2 = true
  else
    assert(not build_tests.enabled(), 'Catch2 is required')
  endif
endif

catch2_tests = [
  'signal',
  'raw',
]

if has_catch2
  libcatch2 = static_library(
    'catch2', 'catch2_main.cpp',
    implicit_include_directories: false,
    build_by_default: false,
    dependencies: catch2_dep)

  foreach t : catch2_tests
    test(t, executable(t.underscorify(), t + '.cpp',
                       build_by_default: false,
                       implicit_include_directories: false,
                       link_with: libcatch2,
                       dependencies: [stdplus, span_dep, catch2_dep]))
  endforeach
endif
