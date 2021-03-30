project(
    'obmc-ikvm',
    'cpp',
    version: '1.0',
    license: 'GPLv2',
    default_options: [
        'cpp_std=c++17',
        'werror=true',
    ],
)

install_data(
    'create_usbhid.sh',
    install_mode: 'rwxr-xr-x',
    install_dir: get_option('bindir')
)

executable(
    'obmc-ikvm',
    [
        'ikvm_args.cpp',
        'ikvm_input.cpp',
        'ikvm_manager.cpp',
        'ikvm_server.cpp',
        'ikvm_video.cpp',
        'obmc-ikvm.cpp',
    ],
    dependencies: [
        dependency('libvncserver'),
        dependency('phosphor-logging'),
        dependency('phosphor-dbus-interfaces'),
        dependency('sdbusplus'),
        dependency('threads'),
    ],
    install: true
)

configure_file(
    input: 'start-ipkvm.service',
    output: 'start-ipkvm.service',
    copy: true,
    install_dir:
        dependency('systemd').get_pkgconfig_variable(
            'systemdsystemunitdir')
)
