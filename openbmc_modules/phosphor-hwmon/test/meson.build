average_unittest = executable(
    'average_unittest',
    'average_unittest.cpp',
    dependencies: [
        gmock,
        gtest,
    ],
    include_directories: '..',
    link_with: [
        libaverage,
    ],
)

env_unittest = executable(
    'env_unittest',
    'env_unittest.cpp',
    dependencies: [
        gmock,
        gtest,
    ],
    include_directories: '..',
)

fanpwm_unittest = executable(
    'fanpwm_unittest',
    'fanpwm_unittest.cpp',
    dependencies: [
        gmock,
        gtest,
        phosphor_logging,
        fmt,
    ],
    include_directories: '..',
    link_with: [
        libfan_pwm,
    ],
)

hwmon_unittest = executable(
    'hwmon_unittest',
    'hwmon_unittest.cpp',
    dependencies: [
        gmock,
        gtest,
        fmt,
    ],
    include_directories: '..',
    link_with: [
        libhwmon,
    ],
)

hwmonio_default_unittest = executable(
    'hwmonio_default_unittest',
    'hwmonio_default_unittest.cpp',
    dependencies: [
        gtest,
        gmock,
        fmt,
    ],
    include_directories: '..',
    link_with: [
        libhwmonio,
    ],
)

sensor_unittest = executable(
    'sensor_unittest',
    'gpio.cpp',
    'sensor_unittest.cpp',
    dependencies: [
        gtest,
        gmock,
        fmt,
    ],
    include_directories: '..',
    link_with: [
        libsensor,
    ],
)
