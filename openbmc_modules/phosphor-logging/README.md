# phosphor-logging
The phosphor logging repository provides mechanisms for event and journal
logging.

## Table Of Contents
* [Building](#to-build)
* [Event Logs](#event-logs)
* [Application Specific Error YAML](#adding-application-specific-error-yaml)
* [Event Log Extensions](#event-log-extensions)
* [Remote Logging](#remote-logging-via-rsyslog)
* [Boot Fail on Hardware Errors](#boot-fail-on-hardware-errors)

## To Build
```
To build this package, do the following steps:

    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make

To clean the repository run `./bootstrap.sh clean`.
```

## Event Logs
OpenBMC event logs are a collection of D-Bus interfaces owned by
phosphor-log-manager that reside at `/xyz/openbmc_project/logging/entry/X`,
where X starts at 1 and is incremented for each new log.

The interfaces are:
* [xyz.openbmc_project.Logging.Entry]
  * The main event log interface.
* [xyz.openbmc_project.Association.Definitions]
  * Used for specifying inventory items as the cause of the event.
  * For more information on associations, see [here][associations-doc].
* [xyz.openbmc_project.Object.Delete]
  * Provides a Delete method to delete the event.
* [xyz.openbmc_project.Software.Version]
  * Stores the code version that the error occurred on.

On platforms that make use of these event logs, the intent is that they are
the common event log representation that other types of event logs can be
created from.  For example, there is code to convert these into both Redfish
and IPMI event logs, in addition to the event log extensions mentioned
[below](#event-log-extensions).

The logging daemon has the ability to add `callout` associations to an event
log based on text in the AdditionalData property.  A callout is a link to the
inventory item(s) that were the cause of the event log. See [here][callout-doc]
for details.

### Creating Event Logs In Code
There are two approaches to creating event logs in OpenBMC code.  The first
makes use of the systemd journal to store metadata needed for the log, and the
second is a plain D-Bus method call.

#### Journal Based Event Log Creation
Event logs can be created by using phosphor-logging APIs to commit sdbusplus
exceptions.  These APIs write to the journal, and then call a `Commit`
D-Bus method on the logging daemon to create the event log using the information
it put in the journal.

The APIs are found in `<phosphor-logging/elog.hpp>`:
* `elog()`: Throw an sdbusplus error.
* `commit()`: Catch an error thrown by elog(), and commit it to create the
  event log.
* `report()`: Create an event log from an sdbusplus error without throwing the
  exception first.

Any errors passed into these APIs must be known to phosphor-logging, usually
by being defined in `<phosphor-logging/elog-errors.hpp>`.  The errors must
also be known by sdbusplus, and be defined in their corresponding error.hpp.
See below for details on how get errors into these headers.

Example:
```
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
...
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
...
if (somethingBadHappened)
{
    phosphor::logging::report<InternalFailure>();
}

```
Alternatively, to throw, catch, and then commit the error:
```
try
{
    phosphor::logging::elog<InternalFailure>();
}
catch (InternalFailure& e)
{
    phosphor::logging::commit<InternalFailure>();
}
```

Metadata can be added to event logs to add debug data captured at the time of
the event. It shows up in the AdditionalData property in the
`xyz.openbmc_project.Logging.Entry` interface.  Metadata is passed in via the
`elog()` or `report()` functions, which write it to the journal. The metadata
must be predefined for the error in the [metadata YAML](#event-log-definition)
so that the daemon knows to look for it in the journal when it creates the
event log.

Example:
```
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Control/Device/error.hpp>
...
using WriteFailure =
    sdbusplus::xyz::openbmc_project::Control::Device::Error::WriteFailure;
using metadata =
    xyz::openbmc_project::Control::Device::WriteFailure;
...
if (somethingBadHappened)
{
    phosphor::logging::report<WriteFailure>(metadata::CALLOUT_ERRNO(5),
                              metadata::CALLOUT_DEVICE_PATH("some path"));
}
```
In the above example, the AdditionalData property would look like:
```
["CALLOUT_ERRNO=5", "CALLOUT_DEVICE_PATH=some path"]
```
Note that the metadata fields must be all uppercase.

##### Event Log Definition
As mentioned above, both sdbusplus and phosphor-logging must know about the
event logs in their header files, or the code that uses them will not even
compile.  The standard way to do this to define the event in the appropriate
`<error-category>.errors.yaml` file, and define any metadata in the
`<error-category>.metadata.yaml` file in the appropriate `*-dbus-interfaces`
repository.  During the build, phosphor-logging generates the elog-errors.hpp
file for use by the calling code.

In much the same way, sdbusplus uses the event log definitions to generate an
error.hpp file that contains the specific exception.  The path of the error.hpp
matches the path of the YAML file.

For example, if in phosphor-dbus-interfaces there is
`xyz/openbmc_project/Control/Device.errors.yaml`, the errors that come from
that file will be in the include:
`xyz/openbmc_project/Control/Device/error.hpp`.

In rare cases, one may want one to define their errors in the same repository
that uses them.  To do that, one must:

1. Add the error and metadata YAML files to the repository.
2. Run the sdbus++ script within the makefile to create the error.hpp and .cpp
   files from the local YAML, and include the error.cpp file in the application
   that uses it.  See [openpower-occ-control] for an example.
3. Tell phosphor-logging about the error.  This is done by either:
   * Following the [directions](#adding-application-specific-error-yaml)
     defined in this README, or
   * Running the script yourself:
    1. Run phosphor-logging\'s `elog-gen.py` script on the local yaml to
       generate an elog-errors.hpp file that just contains the local errors,
       and check that into the repository and include it where the errors are
       needed.
    2. Create a recipe that copies the local YAML files to a place that
       phosphor-logging can find it during the build. See [here][led-link]
       for an example.

#### D-Bus Event Log Creation
There is also a [D-Bus method][log-create-link] to create event logs:
* Service: xyz.openbmc_project.Logging
* Object Path: /xyz/openbmc_project/logging
* Interface: xyz.openbmc_project.Logging.Create
* Method: Create
  * Method Arguments:
      * Message: The `Message` string property for the
                 `xyz.openbmc_project.Logging.Entry` interface.
      * Severity: The `severity` property for the
                 `xyz.openbmc_project.Logging.Entry` interface.
                 An `xyz.openbmc_project.Logging.Entry.Level` enum value.
      * AdditionalData: The `AdditionalData` property for the
                 `xyz.openbmc_project.Logging.Entry` interface, but in a map
                 instead of in a vector of "KEY=VALUE" strings.
                 Example:
```
                     std::map<std::string, std::string> additionalData;
                     additionalData["KEY"] = "VALUE";
```


Unlike the previous APIs where errors could also act as exceptions that could
be thrown across D-Bus, this API does not require that the error be defined in
the error YAML in the D-Bus interfaces repository so that sdbusplus knows about
it.  Additionally, as this method passes in everything needed to create the
event log, the logging daemon doesn't have to know about it ahead of time
either.

That being said, it is recommended that users of this API still follow some
guidelines for the message field, which is normally generated from a
combination of the path to the error YAML file and the error name itself.  For
example, the `Timeout` error in `xyz/openbmc_project/Common.errors.yaml` will
have a Message property of `xyz.openbmc_project.Common.Error.Timeout`.

The guidelines are:
1. When it makes sense, one can still use an existing error that has already
   been defined in an error YAML file, and use the same severity and metadata
   (AdditionalData) as in the corresponding metadata YAML file.

2. If creating a new error, use the same naming scheme as other errors, which
   starts with the domain, `xyz.openbmc_project`, `org.open_power`, etc,
   followed by the capitalized category values, followed by `Error`, followed
   by the capitalized error name itself, with everything  separated by "."s.
   For example: `xyz.openbmc_project.Some.Category.Error.Name`.

3. If creating a new common error, still add it to the appropriate error and
   metadata YAML files in the appropriate D-Bus interfaces repository so that
   others can know about it and use it in the future.  This can be done after
   the fact.

[xyz.openbmc_project.Logging.Entry]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Logging/Entry.interface.yaml
[xyz.openbmc_project.Association.Definitions]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Association/Definitions.interface.yaml
[associations-doc]: https://github.com/openbmc/docs/blob/master/architecture/object-mapper.md#associations
[callout-doc]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Common/Callout/README.md
[xyz.openbmc_project.Object.Delete]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Object/Delete.interface.yaml
[xyz.openbmc_project.Software.Version]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Software/Version.errors.yaml
[elog-errors.hpp]: https://github.com/openbmc/phosphor-logging/blob/master/phosphor-logging/elog.hpp
[openpower-occ-control]: https://github.com/openbmc/openpower-occ-control
[led-link]: https://github.com/openbmc/openbmc/tree/master/meta-phosphor/recipes-phosphor/leds
[log-create-link]: https://github.com/openbmc/phosphor-dbus-interfaces/blob/master/xyz/openbmc_project/Logging/Create.interface.yaml

## Adding application specific error YAML
* This document captures steps for adding application specific error YAML files
  and generating local elog-errors.hpp header file for application use.
* Should cater for continuous integration (CI) build, bitbake image build, and
  local repository build.

#### Continuous Integration (CI) build
 * Make is called on the repository that is modified.
 * Dependent packages are pulled based on the dependency list specified in the
   configure.ac script.

#### Recipe build
 * Native recipes copy error YAML files to shared location.
 * phosphor-logging builds elog-errors.hpp by parsing the error YAML files from
   the shared location.

#### Local repository build
 * Copies local error YAML files to the shared location in SDK
 * Make generates elog-errors.hpp by parsing the error YAML files from the
   shared location.

#### Makefile changes
**Reference**
 * https://github.com/openbmc/openpower-debug-collector/blob/master/Makefile.am

###### Export error YAML to shared location
*Modify Makefile.am to export newly added error YAML to shared location*
```
yamldir = ${datadir}/phosphor-dbus-yaml/yaml
nobase_yaml_DATA = \
    org/open_power/Host.errors.yaml
```

###### Generate elog-errors.hpp using elog parser from SDK location
 * Add a conditional check "GEN_ERRORS"
 * Disable the check for recipe bitbake image build
 * Enable it for local repository build
 * If "GEN_ERRORS" is enabled, build generates elog-errors.hpp header file.
```
  # Generate phosphor-logging/elog-errors.hpp
  if GEN_ERRORS
  ELOG_MAKO ?= elog-gen-template.mako.hpp
  ELOG_DIR ?= ${OECORE_NATIVE_SYSROOT}${datadir}/phosphor-logging/elog
  ELOG_GEN_DIR ?= ${ELOG_DIR}/tools/
  ELOG_MAKO_DIR ?= ${ELOG_DIR}/tools/phosphor-logging/templates/
  YAML_DIR ?= ${OECORE_NATIVE_SYSROOT}${datadir}/phosphor-dbus-yaml/yaml
  phosphor-logging/elog-errors.hpp:
      @mkdir -p ${YAML_DIR}/org/open_power/
      @cp ${top_srcdir}/org/open_power/Host.errors.yaml \
        ${YAML_DIR}/org/open_power/Host.errors.yaml
      @mkdir -p `dirname $@`
      @chmod 777 $(ELOG_GEN_DIR)/elog-gen.py
      $(AM_V_at)$(PYTHON) $(ELOG_GEN_DIR)/elog-gen.py -y ${YAML_DIR} \
        -t ${ELOG_MAKO_DIR} -m ${ELOG_MAKO} -o $@
  endif
```

###### Update BUILT_SOURCES
 * Append elog-errors.hpp to BUILT_SOURCES list and put it in conditional check
   GEN_ERRORS so that the elog-errors.hpp is generated only during local
   repository build.
```
    if GEN_ERRORS
    nobase_nodist_include_HEADERS += \
                phosphor-logging/elog-errors.hpp
    endif
    if GEN_ERRORS
    BUILT_SOURCES += phosphor-logging/elog-errors.hpp
    endif
```

###### Conditional check for native build
 * As the same Makefile is used both for recipe image build and native recipe
   build, add a conditional to ensure that only installation of error yaml files
   happens during native build. It is not required to build repository during
   native build.
```
   if !INSTALL_ERROR_YAML
   endif
```

#### Autotools changes
**Reference**
 * https://github.com/openbmc/openpower-debug-collector/blob/master/configure.ac

###### Add option(argument) to enable/disable installing error yaml file
 * Install error yaml option(argument) is enabled for native recipe build
   and disabled for bitbake build.

 * When install error yaml option is disabled do not check for target specific
   packages in autotools configure script.

###### Add option(argument) to install error yaml files
```
AC_ARG_ENABLE([install_error_yaml],
    AS_HELP_STRING([--enable-install_error_yaml],
    [Enable installing error yaml file]),[], [install_error_yaml=no])
AM_CONDITIONAL([INSTALL_ERROR_YAML],
    [test "x$enable_install_error_yaml" = "xyes"])
AS_IF([test "x$enable_install_error_yaml" != "xyes"], [
..
..
])
```

###### Add option(argument) to enable/disable generating elog-errors header file
```
AC_ARG_ENABLE([gen_errors],
    AS_HELP_STRING([--enable-gen_errors], [Enable elog-errors.hpp generation ]),
    [],[gen_errors=yes])
AM_CONDITIONAL([GEN_ERRORS], [test "x$enable_gen_errors" != "xno"])
```

#### Recipe changes
**Reference**
* https://github.com/openbmc/openbmc/blob/master/meta-openbmc-machines\
/meta-openpower/common/recipes-phosphor/debug/openpower-debug-collector.bb

###### Extend recipe for native and nativesdk
* Extend the recipe for native and native SDK builds
```
BBCLASSEXTEND += "native nativesdk"
```
###### Remove dependencies for native and native SDK build
* Native recipe caters only for copying error yaml files to shared location.
* For native and native SDK build remove dependency on packages that recipe
  build depends

###### Remove dependency on phosphor-logging for native build
```
DEPENDS_remove_class-native = "phosphor-logging"
```

###### Remove dependency on phosphor-logging for native SDK build
```
DEPENDS_remove_class-nativesdk = "phosphor-logging"
```

###### Add install_error_yaml argument during native build
* Add package config to enable/disable install_error_yaml feature.

###### Add package config to enable/disable install_error_yaml feature
```
PACKAGECONFIG ??= "install_error_yaml"
PACKAGECONFIG[install_error_yaml] = " \
        --enable-install_error_yaml, \
        --disable-install_error_yaml, ,\
        "
```
###### Enable install_error_yaml check for native build
```
PACKAGECONFIG_add_class-native = "install_error_yaml"
PACKAGECONFIG_add_class-nativesdk = "install_error_yaml"
```
###### Disable install_error_yaml during target build
```
PACKAGECONFIG_remove_class-target = "install_error_yaml"
```

###### Disable generating elog-errors.hpp for bitbake build
* Disable gen_errors argument for bitbake image build as the application uses
  the elog-errors.hpp generated by phosphor-logging
* Argument is enabled by default for local repository build in the configure
  script of the local repository.
```
 XTRA_OECONF += "--disable-gen_errors"
```

#### Local build
* During local build use --prefix=/usr for the configure script.

**Reference**
* https://github.com/openbmc/openpower-debug-collector/blob/master/README.md

## Event Log Extensions

The extension concept is a way to allow code that creates other formats of
error logs besides phosphor-logging's event logs to still reside in the
phosphor-log-manager application.

The extension code lives in the `extensions/<extension>` subdirectories,
and is enabled with a `--enable-<extension>` configure flag.  The
extension code won't compile unless enabled with this flag.

Extensions can register themselves to have functions called at the following
points using the REGISTER_EXTENSION_FUNCTION macro.
* On startup
   * Function type void(internal::Manager&)
* After an event log is created
   * Function type void(args)
   * The args are:
     * const std::string& - The Message property
     * uin32_t - The event log ID
     * uint64_t - The event log timestamp
     * Level - The event level
     * const AdditionalDataArg& - the additional data
     * const AssociationEndpointsArg& - Association endpoints (callouts)
* Before an event log is deleted, to check if it is allowed.
   * Function type void(std::uint32_t, bool&) that takes the event ID
* After an event log is deleted
   * Function type void(std::uint32_t) that takes the event ID

Using these callback points, they can create their own event log for each
OpenBMC event log that is created, and delete these logs when the corresponding
OpenBMC event log is deleted.

In addition, an extension has the option of disabling phosphor-logging's
default error log capping policy so that it can use its own.  The macro
DISABLE_LOG_ENTRY_CAPS() is used for that.

### Motivation

The reason for adding support for extensions inside the phosphor-log-manager
daemon as opposed to just creating new daemons that listen for D-Bus signals is
to allow interactions that would be complicated or expensive if just done over
D-Bus, such as:
* Allowing for custom old log retention algorithms.
* Prohibiting manual deleting of certain logs based on an extension's
  requirements.

### Creating extensions

1. Add a new flag to configure.ac to enable the extension:
```
AC_ARG_ENABLE([foo-extension],
              AS_HELP_STRING([--enable-foo-extension],
                             [Create Foo logs]))
AM_CONDITIONAL([ENABLE_FOO_EXTENSION],
               [test "x$enable_foo_extension" == "xyes"])
```
2. Add the code in `extensions/<extension>/`.
3. Create a makefile include to add the new code to phosphor-log-manager:
```
phosphor_log_manager_SOURCES += \
        extensions/foo/foo.cpp
```
4. In `extensions/extensions.mk`, add the makefile include:
```
if ENABLE_FOO_EXTENSION
include extensions/foo/foo.mk
endif
```
5. In the extension code, register the functions to call and optionally disable
   log capping using the provided macros:
```
DISABLE_LOG_ENTRY_CAPS();

void fooStartup(internal::Manager& manager)
{
    // Initialize
}

REGISTER_EXTENSION_FUNCTION(fooStartup);

void fooCreate(const std::string& message, uint32_t id, uint64_t timestamp,
               Entry::Level severity, const AdditionalDataArg& additionalData,
               const AssociationEndpointsArg& assocs)
{
    // Create a different type of error log based on 'entry'.
}

REGISTER_EXTENSION_FUNCTION(fooCreate);

void fooRemove(uint32_t id)
{
    // Delete the extension error log that corresponds to 'id'.
}

REGISTER_EXTENSION_FUNCTION(fooRemove);
```
### Extension List

The supported extensions are:

* OpenPower PELs
    * Enabled with --enable-openpower-pel-extension
    * Detailed information can be found
        [here](extensions/openpower-pels/README.md)

## Remote Logging via Rsyslog
The BMC has the ability to stream out local logs (that go to the systemd journal)
via rsyslog (https://www.rsyslog.com/).

The BMC will send everything. Any kind of filtering and appropriate storage
will have to be managed on the rsyslog server. Various examples are available
on the internet. Here are few pointers :
https://www.rsyslog.com/storing-and-forwarding-remote-messages/
https://www.rsyslog.com/doc/rsyslog%255Fconf%255Ffilter.html
https://www.thegeekdiary.com/understanding-rsyslog-filter-options/

#### Configuring rsyslog server for remote logging
The BMC is an rsyslog client. To stream out logs, it needs to talk to an rsyslog
server, to which there's connectivity over a network. REST API can be used to
set the remote server's IP address and port number.

The following presumes a user has logged on to the BMC (see
https://github.com/openbmc/docs/blob/master/rest-api.md).

Set the IP:
```
curl -b cjar -k -H "Content-Type: application/json" -X PUT \
    -d '{"data": <IP address>}' \
    https://<BMC IP address>/xyz/openbmc_project/logging/config/remote/attr/Address
```

Set the port:
```
curl -b cjar -k -H "Content-Type: application/json" -X PUT \
    -d '{"data": <port number>}' \
    https://<BMC IP address>/xyz/openbmc_project/logging/config/remote/attr/Port
```

#### Querying the current configuration
```
curl -b cjar -k \
    https://<BMC IP address>/xyz/openbmc_project/logging/config/remote
```

#### Setting the hostname
Rsyslog can store logs separately for each host. For this reason, it's useful to
provide a unique hostname to each managed BMC. Here's how that can be done via a
REST API :
```
curl -b cjar -k -H "Content-Type: application/json" -X PUT \
    -d '{"data": "myHostName"}' \
    https://<BMC IP address>//xyz/openbmc_project/network/config/attr/HostName
```

#### Disabling remote logging
Remote logging can be disabled by writing 0 to the port, or an empty string("")
to the IP.

#### Changing the rsyslog server
When switching to a new server from an existing one (i.e the address, or port,
or both change), it is recommended to disable the existing configuration first.

## Boot Fail on Hardware Errors

phosphor-logging supports a setting, which when set, will result in the
software looking at new phosphor-logging entries being created, and if a
CALLOUT* is found within the entry, ensuring the system will not power
on.

The full design for this can be found
[here](https://github.com/openbmc/docs/blob/master/designs/fail-boot-on-hw-error.md)

To enable this function:

```
busctl set-property xyz.openbmc_project.Settings /xyz/openbmc_project/logging/settings xyz.openbmc_project.Logging.Settings QuiesceOnHwError b true
```

To check if an entry is blocking the boot:
```
obmcutil listbootblock
```

Resolve or clear the corresponding entry to allow the system to boot.
