systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_pkgconfig_variable(
    'systemdsystemunitdir',
    define_variable: ['prefix', get_option('prefix')])
packagedir = join_paths(
    get_option('prefix'),
    get_option('datadir'),
    meson.project_name(),
)

install_subdir('fru', install_dir: packagedir)
if get_option('oem-ibm').enabled()
    install_subdir('../oem/ibm/configurations/fru', install_dir: packagedir)
    install_data('../oem/ibm/configurations/fileTable.json',
        install_dir: packagedir)
    install_data('../oem/ibm/configurations/host_eid', install_dir: packagedir)
endif

install_subdir('pdr', install_dir: packagedir)

install_subdir('host', install_dir: packagedir)

install_subdir('events', install_dir: packagedir)
