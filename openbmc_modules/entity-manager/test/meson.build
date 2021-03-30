gtest = dependency('gtest', main: true, disabler: true, required: false)
if not gtest.found() and build_tests.enabled()
    cmake = import('cmake')
    gtest_subproject = cmake.subproject('gtest')
    cm_gtest = gtest_subproject.dependency('gtest')
    cm_gtest_main = gtest_subproject.dependency('gtest_main')
    gtest = declare_dependency(dependencies: [cm_gtest, cm_gtest_main, threads])
endif

test(
    'test_entity_manager',
    executable(
        'test_entity_manager',
        'test_entity-manager.cpp',
        '../src/Utils.cpp',
        cpp_args: boost_args + ['-DBOOST_ASIO_DISABLE_THREADS'],
        dependencies: [
            boost,
            gtest,
            nlohmann_json,
            sdbusplus,
            valijson,
        ],
        implicit_include_directories: false,
        include_directories: '../include',
    )
)
