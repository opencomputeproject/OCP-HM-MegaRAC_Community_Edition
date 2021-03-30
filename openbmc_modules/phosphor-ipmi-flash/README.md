# Secure Flash Update Mechanism

This document describes the OpenBmc software implementing the secure flash
update mechanism.

The primary details are [here](https://github.com/openbmc/docs/blob/master/designs/firmware_update_via_blobs.md).

## Building and using the host-tool

This repo contains a host-tool implementation for talking to the corresponding
BMC blob handler.

### Building the host-tool

The host-tool depends on ipmi-blob-tool and pciutils.

#### Building libpciaccess

Check out the [xorg-macros source](https://gitlab.freedesktop.org/xorg/util/macros).

Then run these commands in the source directory.

```
./autogen.sh --prefix=/usr
make install
```

Check out the [libpciaccess source](https://gitlab.freedesktop.org/xorg/lib/libpciaccess).

Then run these commands in the source directory.

```
./autogen.sh
make
make install
```

#### Building ipmi-blob-tool

Check out the [ipmi-blob-tool source](https://github.com/openbmc/ipmi-blob-tool).

Then run these commands in the source directory.

```
./bootstrap.sh
./configure
make
make install
```

#### Building fmtlib

Check out the [fmtlib source](https://github.com/fmtlib/fmt).

Then run these commands in the source directory.

```
mkdir build && cd build
cmake ..
make
make install
```

#### Building stdplus

Check out the [stdplus source](https://github.com/openbmc/stdplus).

Then run these commands in the source directory.

```
meson setup -Dexamples=false -Dtests=disabled builddir
ninja -C builddir
ninja -C builddir install
```

#### Building burn_my_bmc (the host-tool)

Check out the [phosphor-ipmi-flash source](https://github.com/openbmc/phosphor-ipmi-flash).

Then run these commands in the source directory.

```
./bootstrap.sh
./configure --disable-build-bmc-blob-handler
make
make install
```

*NOTE:* When building from the OpenBMC SDK your configuration call will be:

```
./configure --enable-oe-sdk --host "$(uname -m)" --disable-build-bmc-blob-handler AR=x86_64-openbmc-linux-gcc-ar RANLIB=x86_64-openbmc-linux-gcc-ranlib
```

### Parameters

The host-tool has parameters that let the caller specify every required detail.

The required parameters are:

 Parameter  | Options  | Meaning
----------- | -------- | -------
`command`   | `update` | The tool should try to update the BMC firmware.
`interface` | `ipmibt`, `ipmilpc`, `ipmipci`, `ipminet` | The data transport mechanism, typically `ipmilpc`
`image`     | path     | The BMC firmware image file (or tarball)
`sig`       | path     | The path to a signature file to send to the BMC along with the image file.
`type`      | blob ending | The ending of the blob id.  For instance `/flash/image` becomes a type of `image`.

If you're using an LPC data transfer mechanism, you'll need two additional
parameters: `address` and `length`.  These values indicate where on the host
you've reserved memory to be used for the transfer window.

If you're using a net data transfer mechanism, you'll also need two additional
parameters: `hostname` and `port`. These specify which address and port the tool
should attempt to connect to the BMC using. If unspecified, the `port` option
defaults to 623, the same port as IPMI LAN+.

## Introduction

This supports three methods of providing the image to stage. You can send the
file over IPMI packets, which is a very slow process. A 32-MiB image can take
~3 hours to send via this method.  This can be done in <1 minutes via the PCI or
net bridge, or just a few minutes via LPC depending on the size of the mapped
area.

This is implemented as a phosphor blob handler.

The image must be signed via the production or development keys, the former
being required for production builds. The image itself and the image signature
are separately sent to the BMC for verification. The verification package
source is beyond the scope of this design.

Basically the IPMI OEM handler receives the image in one fashion or another and
then triggers the `verify_image` service. Then, the user polls until the result
is reported. This is because the image verification process can exceed 10
seconds.

### Using Legacy Images

The image flashing mechanism itself is the initramfs stage during reboot. It
will check for files named "`image-*`" and flash them appropriately for each
name to section. The IPMI command creates a file `/run/initramfs/bmc-image` and
writes the contents there. It was found that writing it in /tmp could cause OOM
errors moving it on low memory systems, whereas renaming a file within the same
folder seems to only update the directory inode's contents.

### Using UBI

The staging file path can be controlled via software configuration.  The image
is assumed to be the tarball contents and is written into `/tmp/{tarball_name}.gz`

TODO: Flesh out the UBI approach.

## Configuration

To use `phosphor-ipmi-flash` a platform must provide a configuration.  A
platform can configure multiple interfaces, such as both lpc and pci.  However,
a platform should only configure either static layout updates, or ubi.  If
enabling lpc, the platform must specify either aspeed or nuvoton.  The system
also supports receiving BIOS updates.

The following are the two primary configuration options, which control how the
update is treated.

Option                   | Meaning
------------------------ | -------
`--enable-static-layout` | Enable treating the update as a static layout update.
`--enable-tarball-ubi`   | Enable treating the update as a tarball for UBI update.
`--enable-host-bios`     | Enable receiving the update for a host bios update.

The following are configuration options for how the host and BMC are meant to
transfer the data.  By default, the data-in-IPMI mechanism is enabled.

There are three configurable data transport mechanisms, either staging the bytes
via the LPC memory region, the PCI-to-AHB memory region, or sending over a
network connection.  Because there is only one `MAPPED_ADDRESS` variable at
present, a platform should not configure LPC and P2A at the same time.  The
platform's device-tree may have the region locked to a specific driver
(lpc-aspeed-ctrl), preventing the region from other use.

***NOTE:*** It will likely be possible to configure both LPC and P2A in the near
future.

Variable              | Default | Meaning
--------------------- | ------- | -------
`MAPPED_ADDRESS`      | 0       | The address used for mapping P2A or LPC into the BMC's memory-space.

If a platform enables p2a as the transport mechanism, a specific vendor must be
selected via the following configuration option.  Currently, only one is
supported.

Option                 | Meaning
-----------------------| -------
`--enable-aspeed-p2a`  | Use with ASPEED parts.

If a platform enables lpc as the transport mechanism, a specific vendor must be
selected via the following configuration option.  Currently, only two are
supported.

Option                 | Meaning
---------------------- | -------
`--enable-aspeed-lpc`  | Use with ASPEED parts.
`--enable-nuvoton-lpc` | Use with Nuvoton parts.

A platform may also enable the network transport mechanism.

NOTE: This mechanism is only intended to be used in-band and not exposed
externally, as it doesn't implement any encryption or integrity verification.

Option                | Meaning
----------------------| -------
`--enable-net-bridge` | Enable net transport bridge

There are also options to control an optional clean up mechanism.

Option                    | Meaning
------------------------- | -------
`--enable-cleanup-delete` | Provide a simple blob id that deletes artifacts.

If the update mechanism desired is simply a BMC reboot, a platform can just
enable that directly.

Option                   | Meaning
------------------------ | -------
`--enable-reboot-update` | Enable use of reboot update mechanism.

If you would like the update status to be tracked with a status file, this
option can be enabled. Note that `--enable-update-status` option and the above
`--enable-reboot-update` option cannot be enabled at the same time.

Option                   | Meaning
------------------------ | -------
`--enable-update-status` | Enable use of update status file.

If you would like to use host memory access to update on a PPC platform, this configuration option needs to be enabled.

Option          | Meaning
--------------- | -------
`--enable-ppc`  | Enable PPC host memory access.

### Internal Configuration Details

The following variables can be set to whatever you wish, however they have
usable default values.

Variable                      | Default                    | Meaning
----------------------------- | -------------------------- | -------------------------------------------------------------------------
`STATIC_HANDLER_STAGED_NAME`  | `/run/initramfs/bmc-image` | The filename where to write the staged firmware image for static updates.
`TARBALL_STAGED_NAME`         | `/tmp/image-update.tar`    | The filename where to write the UBI update tarball.
`HASH_FILENAME`               | `/tmp/bmc.sig`             | The file to use for the hash provided.
`PREPARATION_DBUS_SERVICE`    | `phosphor-ipmi-flash-bmc-prepare.target` | The systemd target started when the host starts to send an update.
`VERIFY_STATUS_FILENAME`      | `/tmp/bmc.verify`          | The file checked for the verification status.
`VERIFY_DBUS_SERVICE`         | `phosphor-ipmi-flash-bmc-verify.target`  | The systemd target started for verification.
`UPDATE_DBUS_SERVICE`         | `phosphor-ipmi-flash-bmc-update.target`  | The systemd target started for updating the BMC.
`UPDATE_STATUS_FILENAME`      | `/tmp/bmc.update`  | The file checked for the update status.
`BIOS_STAGED_NAME`            | `/tmp/bios-image`  | The file to use for staging the bios firmware update.
`BIOS_VERIFY_STATUS_FILENAME` | `/tmp/bios.verify` | The file checked for the verification status.
`PREPARATION_BIOS_TARGET`     | `phosphor-ipmi-flash-bios-prepare.target` | The systemd target when the host starts to send an update.
`VERIFY_BIOS_TARGET`          | `phosphor-ipmi-flash-bios-verify.target`  | The systemd target started for verification.
`UPDATE_BIOS_TARGET`          | `phosphor-ipmi-flash-bios-update.target`  | The systemd target started for updating the BIOS.

## JSON Configuration

Read the [details](bmc_json_config.md) of the json configuration.  The json configurations are used to configure the BMC's flash handler behaviors.

## Flash State Machine Details

[This document](ipmi_flash.md) describes the details of the state machine
implemented and how different interactions with it will respond.  This also
describes how a host-side tool is expected to talk to it (triggering different
states and actions).
