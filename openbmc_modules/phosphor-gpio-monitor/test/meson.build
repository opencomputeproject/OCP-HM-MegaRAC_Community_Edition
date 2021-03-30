gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)

test(
    'utest',
    executable(
        'utest', 'utest.cpp',
        dependencies: [
            gmock,
            gtest,
            libevdev,
        ],
        implicit_include_directories: false,
        include_directories: '..',
        link_with: [
            libevdev_o,
            libmonitor_o,
        ]
    )
)
