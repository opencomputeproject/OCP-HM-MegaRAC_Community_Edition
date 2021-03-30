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

tests = [
  'chip',
  'event',
  'handle',
  'internal/fd',
  'mocks',
  'utility/aspeed',
]

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     implicit_include_directories: false,
                     dependencies: [gpioplus, gtest, gmock]))
endforeach
