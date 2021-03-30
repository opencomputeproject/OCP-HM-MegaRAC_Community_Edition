project(
    'phosphor-gpio-monitor',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

build_tests = get_option('tests')

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
libevdev = dependency('libevdev')
libsystemd = dependency('libsystemd')
libgpiod = dependency('libgpiod')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
systemd = dependency('systemd')

boost_args = ['-DBOOST_ASIO_DISABLE_THREADS',
              '-DBOOST_ERROR_CODE_HEADER_ONLY',
              '-DBOOST_SYSTEM_NO_DEPRECATED']

systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])

configure_file(
    copy: true,
    input: 'phosphor-gpio-monitor@.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-gpio-monitor@.service'
)

configure_file(
    copy: true,
    input: 'phosphor-multi-gpio-monitor.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-multi-gpio-monitor.service'
)

configure_file(
    copy: true,
    input: 'phosphor-gpio-presence@.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-gpio-presence@.service'
)

udev = dependency('udev')
udev_rules_dir = join_paths(
    udev.get_pkgconfig_variable(
        'udevdir',
        define_variable: ['prefix', get_option('prefix')],
    ),
    'rules.d',
)

configure_file(
    copy: true,
    input: '99-gpio-keys.rules',
    install: true,
    install_dir: udev_rules_dir,
    output: '99-gpio-keys.rules'
)


configure_file(
    copy: true,
    input: 'phosphor-multi-gpio-monitor.json',
    install: true,
    install_dir: '/usr/share/phosphor-gpio-monitor',
    output: 'phosphor-multi-gpio-monitor.json'
)

libevdev_o = static_library(
    'libevdev_o',
    'evdev.cpp',
    dependencies: [
        libevdev,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ]
)

libmonitor_o = static_library(
    'libmonitor_o',
     'monitor.cpp',
    dependencies: [
        libevdev,
        libsystemd,
        phosphor_logging,
    ],
    link_with: [
        libevdev_o,
    ],
)

phosphor_gpio_monitor = executable(
    'phosphor-gpio-monitor',
    'argument.cpp',
    'mainapp.cpp',
    dependencies: [
        libevdev,
        libsystemd,
        phosphor_logging,
    ],
    install: true,
    link_with: [
        libevdev_o,
        libmonitor_o,
    ],
)

executable(
   'phosphor-multi-gpio-monitor',
   'gpioMonMain.cpp',
   'gpioMon.cpp',
   dependencies: [
        phosphor_logging,
        sdbusplus,
        libgpiod,
    ],
    cpp_args: boost_args,
    install: true,
)

subdir('gpio-util')
subdir('presence')
subdir('test')
