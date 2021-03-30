project('phosphor-bmc-code-mgmt', 'cpp',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true'
    ],
    license: 'Apache-2.0',
    version: '1.0')

conf = configuration_data()

# DBus information
conf.set_quoted('MAPPER_BUSNAME', 'xyz.openbmc_project.ObjectMapper')
conf.set_quoted('MAPPER_INTERFACE', 'xyz.openbmc_project.ObjectMapper')
conf.set_quoted('MAPPER_PATH', '/xyz/openbmc_project/object_mapper')
conf.set_quoted('CPLD_OBJPATH', '/xyz/openbmc_project/cpld')
conf.set_quoted('CPLD_VERSION_BUSNAME', 'xyz.openbmc_project.Cpld.Version')
conf.set_quoted('CPLD_VERSION_IFACE', 'xyz.openbmc_project.Cpld.Version')


# Filesystem files and directories
# The name of the BMC table of contents file
conf.set_quoted('OS_RELEASE_FILE', '/etc/os-release')

# Configurable variables
conf.set_quoted('CPLD_IMG_UPLOAD_DIR', get_option('cpld-img-upload-dir'))
conf.set_quoted('MANIFEST_FILE_NAME', get_option('manifest-file-name'))

configure_file(output: 'config.h', configuration: conf)

deps = [
    dependency('phosphor-dbus-interfaces'),
    dependency('phosphor-logging'),
    dependency('sdbusplus'),
    dependency('libpthread'),
]

ssl = dependency('openssl')

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

unit_files = [
    'xyz.openbmc_project.Cpld.Version.service.in'
]

sdbuspp = find_program('sdbus++')
subdir('xyz/openbmc_project/Cpld/Image')


executable(
    'phosphor-upload-cpldimage',
    image_error_cpp,
    image_error_hpp,
    'cpldimage_manager.cpp',
    'cpldimage_manager_main.cpp',
    'cpldversion.cpp',
    'cpldwatch.cpp',
    'lattice.cpp', 
    'ast-jtag.cpp',
    dependencies: [deps, ssl],
    install: true
)


install_data('cpld.conf',
    install_dir: '/usr/lib/tmpfiles.d/'
)

foreach u : unit_files
    configure_file(
        input: u,
        output: '@BASENAME@',
        configuration: conf,
        install: true,
        install_dir: systemd_system_unit_dir,
    )
endforeach

