project('phosphor-power-monitor','cpp',
        version: '0.1', meson_version: '>=0.49.0',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++17'
        ])

subdir('src')

build_tests = get_option('tests')

if not build_tests.disabled()
  subdir('test')
endif
