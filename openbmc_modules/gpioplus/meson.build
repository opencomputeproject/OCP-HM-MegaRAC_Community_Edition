project('gpioplus', 'cpp',
        version: '0.1', meson_version: '>=0.53.2',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++17',
          'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
          'examples=' + (meson.is_subproject() ? 'false' : 'true'),
        ])

subdir('src')

build_tests = get_option('tests')
build_examples = get_option('examples')

if build_examples
  subdir('example')
endif
if not build_tests.disabled()
  subdir('test')
endif
