cpp_args = boost_args + ['-DPACKAGE_DIR="' + packagedir + '/"']

executable(
    'entity-manager',
    'EntityManager.cpp',
    'Overlay.cpp',
    'Utils.cpp',
    cpp_args: cpp_args + ['-DBOOST_ASIO_DISABLE_THREADS'],
    dependencies: [
        boost,
        nlohmann_json,
        sdbusplus,
        valijson,
    ],
    implicit_include_directories: false,
    include_directories: '../include',
    install: true,
)

if get_option('fru-device')
    executable(
        'fru-device',
        'FruDevice.cpp',
        'Utils.cpp',
        cpp_args: cpp_args,
        dependencies: [
            boost,
            i2c,
            nlohmann_json,
            sdbusplus,
            threads,
            valijson,
        ],
        implicit_include_directories: false,
        include_directories: '../include',
        install: true,
    )
endif
