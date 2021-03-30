unit_files = [
    ['xyz.openbmc_project.FruDevice.service', 'fru-device'],
    ['xyz.openbmc_project.EntityManager.service', ''],
]

foreach u : unit_files
    if u[1] == '' or get_option(u[1])
        configure_file(
            copy: true,
            input: u[0],
            install: true,
            install_dir: systemd_system_unit_dir,
            output: u[0],
        )
    endif
endforeach
