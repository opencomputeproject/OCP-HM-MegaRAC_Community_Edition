if get_option('oe-sdk').enabled()
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

gtest = dependency('gtest', main: true, disabler: true, required: true)
gmock = dependency('gmock', disabler: true, required: true)
pldmd = declare_dependency(
          sources: [
            '../pldmd/instance_id.cpp',
            '../pldmd/dbus_impl_requester.cpp',
            '../host-bmc/host_pdr_handler.cpp',
            '../host-bmc/dbus_to_host_effecters.cpp'])

tests = [
  'libpldmresponder_base_test',
  'libpldmresponder_bios_test',
  'libpldmresponder_bios_attribute_test',
  'libpldmresponder_bios_string_attribute_test',
  'libpldmresponder_bios_config_test',
  'libpldmresponder_bios_integer_attribute_test',
  'libpldmresponder_bios_enum_attribute_test',
  'libpldmresponder_pdr_effecter_test',
  'libpldmresponder_bios_table_test',
  'libpldmresponder_platform_test',
  'pldmd_instanceid_test',
  'pldmd_registration_test',
  'pldm_utils_test',
  'libpldmresponder_fru_test',
  'libpldmresponder_dbus_to_host_effecter_test',
]

if get_option('oem-ibm').enabled()
  tests += [
    '../oem/ibm/test/libpldmresponder_fileio_test'
  ]
endif

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     implicit_include_directories: false,
                     link_args: dynamic_linker,
                     build_rpath: get_option('oe-sdk').enabled() ? rpath : '',
                     dependencies: [
                         libpldm,
                         libpldmresponder,
                         libpldmutils,
                         gtest,
                         gmock,
                         pldmd,
                         dependency('phosphor-dbus-interfaces'),
                         dependency('sdeventplus'),
                         dependency('sdbusplus')]),
       workdir: meson.current_source_dir())
endforeach
