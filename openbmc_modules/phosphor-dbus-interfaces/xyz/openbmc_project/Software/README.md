# Software Version Management and Image Update

## Overview

There are two types of processes involved in software version management and
code update:

1. *ImageManager* - This is a process which manages a collection of, likely
                    temporary, images located somewhere in a file system.
                    These are images which are available on the BMC for update.
2. *ItemUpdater* - This is a process which manages specific storage elements,
                   likely for an inventory item, to determine which software
                   versions are installed onto that item.  A specific example of
                   this would be a process that controls and updates the BIOS
                   flash module for a managed host.

A simple system design would be to include a single *ImageManager* and two
*ItemUpdater*s: one for the BMC itself and one for the Host.

### ImageManager

The *ImageManager* would provide interfaces at `/xyz/openbmc_project/software`
to allow additional images to be added to the BMC, such as Object.Add() for
REST and DownloadViaTFTP() for TFTP.  The standard Object.Delete() interface
would also be provided to facilitate removing images which are no longer
needed.  Images maintained in the file system would be presented as a
corresponding `/xyz/openbmc_project/software/<id>` object. In addition, the
`xyz.openbmc_project.Common.FilePath` interface would be provided to specify
the location of the image.

It is assumed that the *ImageManager* has [at least] a bare minimum amount of
parsing knowledge, perhaps due to a common image format, to allow it to
populate all of the properties of `xyz.openbmc_project.Software.Version`.
*ItemUpdater*s will likely listen for standard D-Bus signals to identify new
images being created.

### *ItemUpdater*

The *ItemUpdater* is responsible for monitoring for new `Software.Version` elements
being created to identify versions that are applicable to the inventory
element(s) it is managing.  The *ItemUpdater* should dynamically create
an `xyz.openbmc_project.Software.Activation` interface under
`/xyz/openbmc_project/software/`, an association of type
`{active_image,software_version}` between the `Software.Version` and
`Software.Activation` under `/xyz/openbmc_project/software/`, and an
association of type `{activation,item}` between the `Inventory.Item`
and `Software.Activation` under `/xyz/openbmc_project/software/<id>`.
Application of the software image is then handled through the
`RequestedActivation` property of the `Software.Activation` interface.

The *ItemUpdater* should, if possible, also create its own
`xyz.openbmc_project.Software.Version` objects, and appropriate associations
for software versions that are currently present on the managed inventory
element(s).  This provides a mechanism for interrogation of the
software versions when the *ImageManager* no longer contains a copy.

## Details

### Image Identifier

The *ImageManager* and *ItemUpdater* should use a common, though perhaps
implementation specific, algorithm for the `<id>` portion of a D-Bus path for
each `Software.Version`.  This allows the same software version to be contained
in multiple locations but represented by the same object path.

A reasonable algorithm might be:
`echo <Version.Version> <Version.Purpose> | sha512sum | cut -b 1-8`

> TODO: May need an issue against the REST server to 'merge' two copies of
>       a single D-Bus object into a single REST object.

### Activation States

`xyz.openbmc_project.Software.Activation` has a property Activation that can
be in the following states:

1. *NotReady* - Indicating that the *ItemUpdater* is still processing the
                version and it is therefore not ready for activation.  This
                might be used on an image that has a security header while
                verification is being performed.
2. *Invalid* - Indicating that, while the `Software.Version.Purpose` suggested
               the image was valid for a managed element, a detailed analysis
               by the *ItemUpdater* showed that it was not.  Reasons may
               include image corruption detected via CRC or security
               verification failure.  An event may be recorded with additional
               details.
3. *Ready* - Indicating that the `Software.Version` can be activated.
4. *Activating* - Indicating that the `Software.Version` is in the process of
                  being activated.
5. *Active* - The `Software.Version` is active on the managed element.  Note
              that on systems with redundant storage devices a version might
              be *Active* but not the primary version.
6. *Failed* - The `Software.Version` or the storage medium on which it is stored
              has failed.  An event may be recorded with additional details.
7. *Staged* - The `Software.Version` is in staged flash region. This will be
              moved from staged flash region to active flash region upon reset.
              Staged flash region is the designated flash area which is used to
              store the integrity validated firmware image that comes in during
              firmware update process. Note that the staged image is not
              necessarily a functional firmware.

### Image Apply Time

`xyz.openbmc_project.Software.ApplyTime` has a property called
RequestedApplyTime that indicates when the newly applied software image will
be activated.  RequestedApplyTime is a D-Bus property that maps to the
"ApplyTime" property in the Redfish UpdateService schema. Below are the
currently supported values and the value can be supplied through
HttpPushUriApplyTime object:

1. *Immediate* - Indicating that the `Software.Version` needs to be activated
                 immediately.
2. *OnReset* - Indicating that the `Software.Version` needs to be activated
               on the next reset.

### Blocking State Transitions

It is sometimes useful to block a system state transition while activations
are being performed.  For example, we do not want to boot a managed host while
its BIOS is being updated.  In order to facilitate this, the interface
`xyz.openbmc_project.Software.ActivationBlocksTransition` may be added to any
object with `Software.Activation` to indicate this behavior.  See that
interface for more details.

It is strongly suggested that any activations are completed prior to a managed
BMC reboot.  This could be facilitated with systemd service specifiers.

### Software Versions

All version identifiers are implementation specific strings.  No format
should be assumed.

Some software versions are a collection of images, each with their own version
identifiers.  The `xyz.openbmc_project.Software.ExtendedVersion` interface
can be added to any `Software.Version` to express the versioning of the
aggregation.

### Activation Progress

The `xyz.openbmc_project.Software.ActivationProgress` interface is provided
to show current progress while a software version is *Activating*.  It is
expected that an *ItemUpdater* will dynamically create this interface while
the version is *Activating* and dynamically remove it when the activation is
complete (or failed).

### Handling Redundancy

The `xyz.openbmc_project.Software.RedundancyPriority` interface is provided to
express the relationship between two (or more) software versions activated for
a single managed element.  It is expected that all installed versions are listed
as *Active* and the `Priority` shows which version is the primary and which are
available for redundancy.

Prior to `Activation`, it can be useful to indicate a desired
`RedundancyPriority`.  This can be done by setting the `Priority` on the
`RequestedRedundancyPriority` interface.  Some *ItemUpdater* implementations
may not honor this field or be unable to comply with the request, in which
case the resulting `Activation` may result in one of two conditions: a
`ActivationState = Failed` or an `ActivateState = Active`` with a
`RedundancyPriority = 0 (High)`.

## REST use-cases

### Find all software versions on the system, either active or available.

List `/xyz/openbmc_project/software/`.  This list can be filtered to just
active listing `.../software/active/` and following the `software_version`
association to retrieve version information.  To list just "functional" or
running versions list `/xyz/openbmc_project/software/functional/`.

### Find all software versions on a managed element.

List `/xyz/openbmc_project/inventory/.../<item>/activation` association.

### Upload new version via REST

HTTP PUT to `/xyz/openbmc_project/software/`.  *ImageManager* will assign the
`<id>` when called for Object.Add().

### Upload new version via ???

Need additional interfaces defined for alternative upload methods.

### Activate a version.

Modify `RequestedActivation` to *Active* on the desired `Activation`.

### Switch primary image.

Set `Priority` to 0 on the desired `RedundancyPriority` interface.

