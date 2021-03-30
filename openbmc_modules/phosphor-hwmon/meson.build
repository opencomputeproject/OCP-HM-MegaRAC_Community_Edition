project(
    'phosphor-hwmon',
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

gmock = dependency('gmock')
gpioplus = dependency('gpioplus')
gtest = dependency('gtest', main: true)
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')
stdplus = dependency('stdplus')
fmt = dependency('fmt')
threads = dependency('threads')

conf = configuration_data()
conf.set_quoted('BUSNAME_PREFIX', get_option('busname-prefix'))
conf.set_quoted('SENSOR_ROOT', get_option('sensor-root'))
conf.set10('NEGATIVE_ERRNO_ON_FAIL', get_option('negative-errno-on-fail'))
conf.set10('UPDATE_FUNCTIONAL_ON_FAIL', get_option('update-functional-on-fail'))

configure_file(output: 'config.h', configuration: conf)

libaverage = static_library(
    'average',
    'average.cpp',
)

libfan_pwm = static_library(
    'fan_pwm',
    'fan_pwm.cpp',
)

libhwmon = static_library(
    'hwmon',
    'hwmon.cpp',
)

libhwmonio = static_library(
    'hwmonio',
    'hwmonio.cpp',
)

libsensor = static_library(
    'sensor',
    'sensor.cpp',
    dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ],
    link_with: [
        libhwmon,
    ],
)

libsysfs = static_library(
    'sysfs',
    'sysfs.cpp',
    dependencies: [
        sdbusplus,
    ],
)

libhwmon_all = static_library(
    'hwmon_all',
    'env.cpp',
    'fan_speed.cpp',
    'gpio_handle.cpp',
    'mainloop.cpp',
    'sensorset.cpp',
    dependencies: [
        fmt,
        gpioplus,
        phosphor_dbus_interfaces,
        phosphor_logging,
    ],
    link_with: [
        libaverage,
        libfan_pwm,
        libhwmon,
        libhwmonio,
        libsensor,
        libsysfs,
    ],
)

executable(
    'phosphor-hwmon-readd',
    'readd.cpp',
    dependencies: [
        sdeventplus,
    ],
    install: true,
    link_with: [
        libhwmon_all,
    ],
)

subdir('msl')
subdir('test')
subdir('tools')
