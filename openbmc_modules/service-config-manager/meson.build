project(
    'phosphor-srvcfg-manager',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

# Wno-psabi reduces the number of "Note:" messages when cross-compiling some STL
# stuff for ARM. See https://stackoverflow.com/questions/48149323/strange-gcc-warning-when-compiling-qt-project
# Basically, gcc 6 and gcc 7 are not ABI compatible, but since the whole OpenBMC
# project uses the same compiler, we can safely ignmore these info notes.
add_project_arguments('-Wno-psabi', language: 'cpp')

boost_args = ['-DBOOST_ALL_NO_LIB',
              '-DBOOST_ASIO_DISABLE_THREADS',
              '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
              '-DBOOST_ERROR_CODE_HEADER_ONLY',
              '-DBOOST_NO_RTTI',
              '-DBOOST_NO_TYPEID',
              '-DBOOST_SYSTEM_NO_DEPRECATED']

deps = [dependency('boost'),
        dependency('boost', modules : ['coroutine']),
        dependency('phosphor-dbus-interfaces'),
        dependency('phosphor-logging'),
        dependency('sdbusplus'),
        dependency('systemd'),
]

executable('phosphor-srvcfg-manager',
           'src/main.cpp',
           'src/srvcfg_manager.cpp',
           'src/utils.cpp',
        implicit_include_directories: false,
        include_directories: ['inc'],
        dependencies: deps,
        cpp_args : boost_args,
        install: true,
        install_dir: get_option('bindir'))

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])

configure_file(
    copy: true,
    input: 'srvcfg-manager.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'srvcfg-manager.service'
)
