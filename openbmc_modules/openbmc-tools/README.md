# The OpenBMC Tools Collection

The goal of this repository is to collect the two-minute hacks you write to
automate interactions with OpenBMC systems.

It's highly likely the scripts don't meet your needs - they could be
undocumented, dysfunctional or utterly broken. Please help us improve!

## Repository Rules

* _Always_ inspect what you will be executing
* Some hacking on your part is to be expected

## If you're still with us

Then this repository aims to be the default destination for your otherwise
un-homed scripts. As such we are setting the bar for submission pretty low,
and we aim to make the process as easy as possible.

## Catalogue of scripts

### Users

* [`openbmc-events`](geissonator/openbmc-events/): Query error events on the target server
* [`openbmc-sensors`](geissonator/openbmc-events/): Query sensors on the target server
* [`openbmc-sfw`](geissonator/openbmc-events/): Manage host and BMC firmware images on the target server
* [`openbmctool`](thalerj/): A general purpose tool for user interactions with OpenBMC
* [`pretty-journal`](post-process/): Convert journalctl's 'pretty' output to regular output
* [`upload_and_update`](leiyu/): Upload a tarball to TFTP server and update BMC with it

### Developers

* [`netboot`](amboar/obmc-scripts/netboot/): Painless netboot of BMC kernels
* [`obmc-gerrit`](amboar/obmc-scripts/maintainers/): Automagically add reviewers to changes pushed to Gerrit
* [`reboot`](amboar/obmc-scripts/reboot/): Endlessly reboot OpenPOWER hosts
* [`tracing`](amboar/obmc-scripts/tracing/): Enable and clean up kernel tracepoints remotely
* [`witherspoon-debug`](amboar/obmc-scripts/witherspoon-debug/): Deploy the debug tools tarball to Witherspoon BMCs

### Maintainers

* [`cla-signers`](emilyshaffer/cla-signers): Check if a contributor has signed the OpenBMC CLA

### Project Administrators

* [`openbmc-autobump.py`](infra/): Update commit IDs in bitbake recipes to bring in new changes

## Sending patches

Please use gerrit for all patches to this repository:

* [Gerrit](https://gerrit.openbmc-project.xyz/) Repository

Do note that you will need to be party to the OpenBMC CLA before your
contributions can be accepted. See [Gerrit Setup and CLA][1]
for more information.

## What we will do once we have your patches

So long as your patches look sane with a cursory glance you can expect them to
be applied. We may push back in the event that similar tools already exist or
there are egregious issues.

## What you must have in your patches

We don't ask for much, but you need to give us at least a
[Signed-off-by](https://developercertificate.org/), and put your work under the
Apache 2.0 license. Licensing everything under Apache 2.0 will just hurt our
heads less. Lets keep the lawyers off our backs. ^

^Any exceptions must be accompanied by a LICENSE file in the relevant
subdirectory, and be compatible with Apache 2.0. You thought you would get away
without any fine print?

## How you consume the repository

There's no standard way to install the scripts housed in the here, so adding
parts of the repository to your PATH might be a bit of a dice-roll. We may also
move or remove scripts from time to time as part of housekeeping. It's probably
best to copy things out if you need stability.

[1]: https://github.com/openbmc/docs/blob/master/CONTRIBUTING.md#submitting-changes-via-gerrit-server
