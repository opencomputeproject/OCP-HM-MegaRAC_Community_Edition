project('phosphor-psu-code-mgmt', 'cpp',
        version: '0.1', meson_version: '>=0.49.0',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++17',
          'b_lto=true',
        ])

systemd = dependency('systemd')
servicedir = systemd.get_pkgconfig_variable('systemdsystemunitdir')
subdir('services')

# Common configurations for src and test
cdata = configuration_data()
cdata.set_quoted('ITEM_IFACE', 'xyz.openbmc_project.Inventory.Item')
cdata.set_quoted('ASSET_IFACE', 'xyz.openbmc_project.Inventory.Decorator.Asset')
cdata.set_quoted('VERSION_IFACE', 'xyz.openbmc_project.Software.Version')
cdata.set_quoted('FILEPATH_IFACE', 'xyz.openbmc_project.Common.FilePath')
cdata.set_quoted('BUSNAME_UPDATER', 'xyz.openbmc_project.Software.Psu.Updater')
cdata.set_quoted('PSU_INVENTORY_IFACE', 'xyz.openbmc_project.Inventory.Item.PowerSupply')
cdata.set_quoted('ACTIVATION_FWD_ASSOCIATION', 'inventory')
cdata.set_quoted('ACTIVATION_REV_ASSOCIATION', 'activation')
cdata.set_quoted('ACTIVE_FWD_ASSOCIATION', 'active')
cdata.set_quoted('ACTIVE_REV_ASSOCIATION', 'software_version')
cdata.set_quoted('FUNCTIONAL_FWD_ASSOCIATION', 'functional')
cdata.set_quoted('FUNCTIONAL_REV_ASSOCIATION', 'software_version')
cdata.set_quoted('UPDATEABLE_FWD_ASSOCIATION', 'updateable')
cdata.set_quoted('UPDATEABLE_REV_ASSOCIATION', 'software_version')
cdata.set_quoted('VERSION', 'Version')
cdata.set_quoted('PRESENT', 'Present')
cdata.set_quoted('MANUFACTURER', 'Manufacturer')
cdata.set_quoted('MODEL', 'Model')

cdata.set_quoted('SOFTWARE_OBJPATH', get_option('SOFTWARE_OBJPATH'))
cdata.set_quoted('MANIFEST_FILE', get_option('MANIFEST_FILE'))
cdata.set_quoted('PSU_INVENTORY_PATH_BASE', get_option('PSU_INVENTORY_PATH_BASE'))
cdata.set_quoted('PSU_VERSION_UTIL', get_option('PSU_VERSION_UTIL'))
cdata.set_quoted('PSU_VERSION_COMPARE_UTIL', get_option('PSU_VERSION_COMPARE_UTIL'))
cdata.set_quoted('PSU_UPDATE_SERVICE', get_option('PSU_UPDATE_SERVICE'))
cdata.set_quoted('IMG_DIR', get_option('IMG_DIR'))
cdata.set_quoted('IMG_DIR_PERSIST', get_option('IMG_DIR_PERSIST'))
cdata.set_quoted('IMG_DIR_BUILTIN', get_option('IMG_DIR_BUILTIN'))

phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
ssl = dependency('openssl')

add_project_link_arguments(['-lstdc++fs'], language: 'cpp')

subdir('src')

build_tests = get_option('tests')
build_examples = get_option('examples')

if build_examples
  subdir('vendor-example')
endif

if not build_tests.disabled()
  subdir('test')
endif
