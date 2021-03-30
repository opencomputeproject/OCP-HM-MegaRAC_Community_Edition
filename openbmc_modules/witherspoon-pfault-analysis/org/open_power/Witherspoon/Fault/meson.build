error_hpp = custom_target(
    'error.hpp',
    capture: true,
    command: [
        sdbuspp,
        '-r', meson.source_root(),
        'error',
        'exception-header',
        'org.open_power.Witherspoon.Fault',
    ],
    input: '../Fault.errors.yaml',
    install: true,
    install_dir: get_option('includedir') / 'org/open_power/Witherspoon/Fault',
    output: 'error.hpp',
)

error_cpp = custom_target(
    'error.cpp',
    capture: true,
    command: [
        sdbuspp,
        '-r', meson.source_root(),
        'error',
        'exception-cpp',
        'org.open_power.Witherspoon.Fault',
    ],
    input: '../Fault.errors.yaml',
    output: 'error.cpp',
)
