unit_files = [
    'xyz.openbmc_project.Software.Psu.Updater.service',
    'psu-update@.service'
]

foreach unit : unit_files
configure_file(input: unit,
               output: unit,
               copy: true,
               install_dir: servicedir)
endforeach
