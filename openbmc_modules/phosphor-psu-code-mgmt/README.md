# phosphor-psu-code-mgmt

phosphor-psu-code-mgmt is a service to provide management for PSU code,
including:

* PSU code version
* PSU code update


## Building

```
meson build/ && ninja -C build
```

## Unit test

* Run it in OpenBMC CI, refer to [local-ci-build.md][1]
* Run it in [OE SDK][2], run below commands in a x86-64 SDK env:
   ```
   meson -Doe-sdk=enabled -Dtests=enabled build/
   ninja -C build/ test  # Meson skips running the case due to it thinks it's cross compiling
   # Manually run the tests
   for t in `find build/test/ -maxdepth 1 -name "test_*"`; do ./$t || break ; done
   ```

## Vendor-specific tools

This repo contains generic code to handle the PSU versions and
updates. It depends on vendor-specific tools to provide the below
functions on the real PSU hardware:
* Get PSU firmware version
* Compare the firmware version
* Update the PSU firmware

It provides configure options for vendor-specific tools for the above functions:
* `PSU_VERSION_UTIL`: It shall be defined as a command-line tool that
accepts the PSU inventory path as input, and outputs the PSU version
string to stdout.
* `PSU_VERSION_COMPARE_UTIL`: It shall be defined as a command-line
tool that accepts one or more PSU version strings, and outputs the
latest version string to stdout.
* `PSU_UPDATE_SERVICE`: It shall be defined as a systemd service that
accepts two arguments:
   * The PSU inventory DBus object;
   * The path of the PSU image(s).

For example:
```
meson -Dtests=disabled \
    '-DPSU_VERSION_UTIL=/usr/bin/psutils --raw --get-version' \
    '-DPSU_VERSION_COMPARE_UTIL=/usr/bin/psutils --raw --compare' \
    '-DPSU_UPDATE_SERVICE=psu-update@.service' \
    build
```

The above configures the vendor-specific tools to use `psutils` from
[phosphor-power][3] to get and compare the PSU versions, and use
`psu-update@.service` to perform the PSU firmware update, where
internally it invokes `psutils` as well.


## Usage

### PSU version

When the service starts, it queries the inventory to get all the PSU inventory
paths, invokes the vendor-specific tool to get the versions, and creates
version objects under `/xyz/openbmc_project/software` that are associated with
the PSU inventory path.
If multiple PSUs are using the same version, multiple PSU inventory paths are
associated.

E.g.
* Example of system with two PSUs that have different versions:
   ```
    "/xyz/openbmc_project/software/02572429": {
      "Activation": "xyz.openbmc_project.Software.Activation.Activations.Active",
      "Associations": [
        [
          "inventory",
          "activation",
          "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply1"
        ]
      ],
      "ExtendedVersion": "",
      "Path": "",
      "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.PSU",
      "RequestedActivation": "xyz.openbmc_project.Software.Activation.RequestedActivations.None",
      "Version": "01120114"
    },
    "/xyz/openbmc_project/software/7094f612": {
      "Activation": "xyz.openbmc_project.Software.Activation.Activations.Active",
      "Associations": [
        [
          "inventory",
          "activation",
          "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0"
        ]
      ],
      "ExtendedVersion": "",
      "Path": "",
      "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.PSU",
      "RequestedActivation": "xyz.openbmc_project.Software.Activation.RequestedActivations.None",
      "Version": "00000110"
    },
   ```
* Example of system with two PSUs that have the same version:
   ```
    "/xyz/openbmc_project/software/9463c2ad": {
      "Activation": "xyz.openbmc_project.Software.Activation.Activations.Active",
      "Associations": [
        [
          "inventory",
          "activation",
          "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0"
        ],
        [
          "inventory",
          "activation",
          "/xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply1"
        ]
      ],
      "ExtendedVersion": "",
      "Path": "",
      "Purpose": "xyz.openbmc_project.Software.Version.VersionPurpose.PSU",
      "RequestedActivation": "xyz.openbmc_project.Software.Activation.RequestedActivations.None",
      "Version": "01100110"
    },
   ```

### PSU update

1. Generate a tarball of PSU firmware image by [generate-psu-tar tool][4].
   ```
   ./generate-psu-tar --image <psu-image> --version <version> --model <model> --manufacture \
   <manufacture> --machineName <machineName> --outfile <psu.tar> --sign
   ```
2. To update the PSU firmware, follow the same steps as described in
   [code-update.md][5]:
   * Upload a PSU image tarball and get the version ID;
   * Set the RequestedActivation state of the uploaded image's version ID.
   * Check the state and wait for the activation to be completed.
3. After a successful update, the PSU image and the manifest is stored
   in BMC's persistent storage defined by `IMG_DIR_PERSIST`. When a PSU
   is replaced, the PSU's firmware version will be checked and updated if
   it's older than the one stored in BMC.
4. It is possible to put a PSU image and MANIFEST in the built-bin
   OpenBMC image in BMC's read-only filesystem defined by
   `IMG_DIR_BUILTIN`. When the service starts, it will compare the
   versions of the built-in image, the stored image (after PSU update),
   and the existing PSUs, if there is any PSU that has an older firmware,
   it will be updated to the newest one.


[1]: https://github.com/openbmc/docs/blob/master/testing/local-ci-build.md
[2]: https://github.com/openbmc/docs/blob/master/cheatsheet.md#building-the-openbmc-sdk
[3]: https://github.com/openbmc/phosphor-power/tree/master/tools/power-utils
[4]: https://github.com/openbmc/phosphor-psu-code-mgmt/blob/master/tools/generate-psu-tar
[5]: https://github.com/openbmc/docs/blob/master/code-update/code-update.md
