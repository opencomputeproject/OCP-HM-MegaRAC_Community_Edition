image_error_hpp = custom_target(
    'error.hpp',
    capture: true,
    command: [
        sdbuspp,
        '-r', meson.source_root(),
        'error',
        'exception-header',
        'xyz.openbmc_project.Cpld.Image',
    ],
    input: '../Image.errors.yaml',
    install: true,
    install_dir: get_option('includedir') / 'xyz/openbmc_project/Cpld/Image',
    output: 'error.hpp',
)

image_error_cpp = custom_target(
    'error.cpp',
    capture: true,
    command: [
        sdbuspp,
        '-r', meson.source_root(),
        'error',
        'exception-cpp',
        'xyz.openbmc_project.Cpld.Image',
    ],
    input: '../Image.errors.yaml',
    output: 'error.cpp',
)
