deps = [ libpldm, dependency('sdeventplus') ]

executable('set-state-effecter', 'requester/set_state_effecter.cpp',
           implicit_include_directories: false,
           dependencies: deps,
           install: true,
           install_dir: get_option('bindir'))

executable('set-state-effecter-async',
           'requester/set_state_effecter_async.cpp',
           implicit_include_directories: false,
           dependencies: deps,
           install: true,
           install_dir: get_option('bindir'))
