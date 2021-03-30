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
conf.set_quoted('BMC_INVENTORY_INTERFACE', 'xyz.openbmc_project.Inventory.Item.Bmc')
conf.set_quoted('BUSNAME_UPDATER', 'xyz.openbmc_project.Software.BMC.Updater')
conf.set_quoted('DOWNLOAD_BUSNAME', 'xyz.openbmc_project.Software.Download')
conf.set_quoted('FILEPATH_IFACE', 'xyz.openbmc_project.Common.FilePath')
conf.set_quoted('INVENTORY_PATH', '/xyz/openbmc_project/inventory/')
conf.set_quoted('MAPPER_BUSNAME', 'xyz.openbmc_project.ObjectMapper')
conf.set_quoted('MAPPER_INTERFACE', 'xyz.openbmc_project.ObjectMapper')
conf.set_quoted('MAPPER_PATH', '/xyz/openbmc_project/object_mapper')
conf.set_quoted('SOFTWARE_OBJPATH', '/xyz/openbmc_project/software')
conf.set_quoted('SYSTEMD_BUSNAME', 'org.freedesktop.systemd1')
conf.set_quoted('SYSTEMD_PATH', '/org/freedesktop/systemd1')
conf.set_quoted('SYSTEMD_INTERFACE', 'org.freedesktop.systemd1.Manager')
conf.set_quoted('VERSION_BUSNAME', 'xyz.openbmc_project.Software.Version')
conf.set_quoted('VERSION_IFACE', 'xyz.openbmc_project.Software.Version')

# Names of the forward and reverse associations
conf.set_quoted('ACTIVATION_FWD_ASSOCIATION', 'inventory')
conf.set_quoted('ACTIVATION_REV_ASSOCIATION', 'activation')
conf.set_quoted('ACTIVE_FWD_ASSOCIATION', 'active')
conf.set_quoted('ACTIVE_REV_ASSOCIATION', 'software_version')
conf.set_quoted('FUNCTIONAL_FWD_ASSOCIATION', 'functional')
conf.set_quoted('FUNCTIONAL_REV_ASSOCIATION', 'software_version')
conf.set_quoted('UPDATEABLE_FWD_ASSOCIATION', 'updateable')
conf.set_quoted('UPDATEABLE_REV_ASSOCIATION', 'software_version')

# Filesystem files and directories
# The path of the alt rwfs overlay
conf.set_quoted('ALT_RWFS', '/media/alt/var/persist')
# The prefix path for the versioned read-only bmc partitions
conf.set_quoted('BMC_ROFS_PREFIX', get_option('media-dir') + '/rofs-')
# The name of the BMC table of contents file
conf.set_quoted('OS_RELEASE_FILE', '/etc/os-release')
# The dir where activation data is stored in files
conf.set_quoted('PERSIST_DIR', '/var/lib/phosphor-bmc-code-mgmt/')

# Supported BMC layout types
conf.set('STATIC_LAYOUT', get_option('bmc-layout').contains('static'))
conf.set('UBIFS_LAYOUT', get_option('bmc-layout').contains('ubi'))
conf.set('MMC_LAYOUT', get_option('bmc-layout').contains('mmc'))

# Configurable features
conf.set('HOST_BIOS_UPGRADE', get_option('host-bios-upgrade').enabled())
conf.set('WANT_SIGNATURE_VERIFY', get_option('verify-signature').enabled())

# Configurable variables
conf.set('ACTIVE_BMC_MAX_ALLOWED', get_option('active-bmc-max-allowed'))
conf.set_quoted('HASH_FILE_NAME', get_option('hash-file-name'))
conf.set_quoted('IMG_UPLOAD_DIR', get_option('img-upload-dir'))
conf.set_quoted('MANIFEST_FILE_NAME', get_option('manifest-file-name'))
conf.set_quoted('MEDIA_DIR', get_option('media-dir'))
optional_array = get_option('optional-images')
optional_images = ''
foreach optiona_image : optional_array
    optional_images = ' '.join([optional_images, optiona_image])
endforeach
conf.set_quoted('OPTIONAL_IMAGES', optional_images)
conf.set_quoted('PUBLICKEY_FILE_NAME', get_option('publickey-file-name'))
conf.set_quoted('SIGNATURE_FILE_EXT', get_option('signature-file-ext'))
conf.set_quoted('SIGNED_IMAGE_CONF_PATH', get_option('signed-image-conf-path'))
conf.set_quoted('SYNC_LIST_DIR_PATH', get_option('sync-list-dir-path'))
conf.set_quoted('SYNC_LIST_FILE_NAME', get_option('sync-list-file-name'))

configure_file(output: 'config.h', configuration: conf)

deps = [
    dependency('phosphor-dbus-interfaces'),
    dependency('phosphor-logging'),
    dependency('sdbusplus')
]

ssl = dependency('openssl')

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

unit_files = [
    'obmc-flash-bmc-setenv@.service.in',
    'reboot-guard-disable.service.in',
    'reboot-guard-enable.service.in',
    'force-reboot.service.in',
    'usr-local.mount.in',
    'xyz.openbmc_project.Software.BMC.Updater.service.in',
    'xyz.openbmc_project.Software.Download.service.in',
    'xyz.openbmc_project.Software.Sync.service.in',
    'xyz.openbmc_project.Software.Version.service.in'
]

sdbuspp = find_program('sdbus++')
subdir('xyz/openbmc_project/Software/Image')

image_updater_sources = files(
    'activation.cpp',
    'images.cpp',
    'item_updater.cpp',
    'item_updater_main.cpp',
    'serialize.cpp',
    'version.cpp',
    'utils.cpp'
)

if get_option('bmc-layout').contains('static')
    image_updater_sources += files(
        'static/flash.cpp',
        'static/item_updater_helper.cpp'
    )
elif get_option('bmc-layout').contains('ubi')
    image_updater_sources += files(
        'ubi/flash.cpp',
        'ubi/item_updater_helper.cpp'
    )

    unit_files += [
        'ubi/obmc-flash-bmc-cleanup.service.in',
        'ubi/obmc-flash-bmc-mirroruboot.service.in',
        'ubi/obmc-flash-bmc-ubiremount.service.in',
        'ubi/obmc-flash-bmc-ubiro@.service.in',
        'ubi/obmc-flash-bmc-ubiro-remove@.service.in',
        'ubi/obmc-flash-bmc-ubirw.service.in',
        'ubi/obmc-flash-bmc-ubirw-remove.service.in',
        'ubi/obmc-flash-bmc-updateubootvars@.service.in'
    ]
elif get_option('bmc-layout').contains('mmc')
    image_updater_sources += files(
        'mmc/flash.cpp',
        'mmc/item_updater_helper.cpp'
    )

    unit_files += [
        'mmc/obmc-flash-mmc@.service.in',
        'mmc/obmc-flash-mmc-mount.service.in',
        'mmc/obmc-flash-mmc-remove@.service.in',
        'mmc/obmc-flash-mmc-setprimary@.service.in',
        'mmc/obmc-flash-mmc-umount.service.in',
    ]
endif

if get_option('host-bios-upgrade').enabled()
    unit_files += 'obmc-flash-host-bios@.service.in'
endif

if get_option('sync-bmc-files').enabled()
    executable(
        'phosphor-sync-software-manager',
        'sync_manager.cpp',
        'sync_manager_main.cpp',
        'sync_watch.cpp',
        dependencies: deps,
        install: true
    )

    install_data('synclist',
        install_dir:  get_option('sysconfdir')
    )
endif

if get_option('verify-signature').enabled()
    image_updater_sources += files(
        'image_verify.cpp',
        'openssl_alloc.cpp'
    )
endif

executable(
    'phosphor-download-manager',
    'download_manager.cpp',
    'download_manager_main.cpp',
    dependencies: deps,
    install: true
)

executable(
    'phosphor-image-updater',
    image_error_cpp,
    image_error_hpp,
    image_updater_sources,
    dependencies: [deps, ssl],
    install: true
)

executable(
    'phosphor-version-software-manager',
    image_error_cpp,
    image_error_hpp,
    'image_manager.cpp',
    'image_manager_main.cpp',
    'version.cpp',
    'watch.cpp',
    dependencies: [deps, ssl],
    install: true
)

install_data('obmc-flash-bmc',
    install_mode: 'rwxr-xr-x',
    install_dir: get_option('bindir')
)

install_data('software.conf',
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

# If test coverage of source files within the root directory are wanted,
# need to define and build the tests from here
build_tests = get_option('tests')
if not build_tests.disabled()
    oe_sdk = get_option('oe-sdk')
    if oe_sdk.enabled()
        # Setup OE SYSROOT
        OECORE_TARGET_SYSROOT = run_command('sh', '-c', 'echo $OECORE_TARGET_SYSROOT').stdout().strip()
        if OECORE_TARGET_SYSROOT == ''
            error('Unable to get $OECORE_TARGET_SYSROOT, check your environment.')
        endif
        message('OE_SYSROOT: ' + OECORE_TARGET_SYSROOT)
        rpath = ':'.join([OECORE_TARGET_SYSROOT + '/lib', OECORE_TARGET_SYSROOT + '/usr/lib'])
        ld_so = run_command('sh', '-c', 'find ' + OECORE_TARGET_SYSROOT + '/lib/ld-*.so | sort -r -n | head -n1').stdout().strip()
        dynamic_linker = ['-Wl,-dynamic-linker,' + ld_so]
    else
        dynamic_linker = []
    endif

    gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
    include_srcs = declare_dependency(sources: [
        'image_verify.cpp',
        'images.cpp',
        'version.cpp']
    )

    test('utest',
        executable(
            'utest',
            './test/utest.cpp',
            link_args: dynamic_linker,
            build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
            dependencies: [deps, gtest, include_srcs, ssl]
        )
)
endif
