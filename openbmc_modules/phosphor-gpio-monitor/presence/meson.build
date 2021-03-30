executable(
    'phosphor-gpio-presence',
    'argument.cpp',
    'main.cpp',
    'gpio_presence.cpp',
    dependencies: [
        cppfs,
        libevdev,
        phosphor_logging,
    ],
    include_directories: '..',
    implicit_include_directories: false,
    install: true,
    link_with: [
        libevdev_o,
    ],
)
