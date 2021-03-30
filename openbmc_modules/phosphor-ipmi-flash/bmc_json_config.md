# BMC Handler Configuration

The BMC BLOB handler for the flash objects is configured at run-time via a json
file or multiple json files. The json fields detail every aspect of how the
handler should behave for a given blob id.

There are three json configurations available by default:

*   config-bios.json
*   config-static-bmc-reboot.json
*   config-static-bmc.json

Let's break them down, what they contain and what it means, and then how to
write your own. It's helpful to start here.

## config-bios.json

This file is generated from configuration and therefore some values can be
replaced, here we're using the defaults.

```
[{
    "blob": "/flash/bios",
    "handler": {
        "type": "file",
        "path": "/tmp/bios-image"
    },
    "actions": {
        "preparation": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bios-prepare.target"
        },
        "verification": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bios-verify.target"
        },
        "update": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bios-update.target"
        }
    }
}]
```

Each json file is expected to be an array of flash handler configurations. You
can define more than one handler inside a json file, or you can have one per
file. It will make no difference. The file though, must contain an array.

Beyond this, the flash handler configuration is a dictionary or object with
three fields.

*   `blob`
*   `handler`
*   `actions`

### `blob`

The `blob` field expects to hold a string. This string is the blob id that'll be
presented to the host and registered with the blob manager. It must be unique
among all blobs on a system and must start with `/flash/`. The uniqueness is not
presently verified at any stage. If there is another blob id that matches, the
first one loaded will be the first one hit during any calls from the host. It's
generally unlikely that there will be a name collision.

### `handler`

The `handler` field expects to hold a dictionary or object with at least one
field.

*   `type`

The `type` field expects a string and will control what other parameters are
required. The type here refers to the handler for the incoming data associated
with the flash image contents. The hash data is always done as a file, but the
image data is configurable via the json.

In the above configuration the `type` is set to `file`. The `file` type handler
requires one parameter: `path`. The `path` here is the full file system path to
where to write the bytes received into a file. In this case specifically the
byte received will be written to `/tmp/bios-image`.

### `actions`

Because `phosphor-ipmi-flash` is a framework for sending data from the host to
the BMC for updating various firmware, it needs to be told what to do to verify
the data and cause an update. It additionally provides a mechanism for preparing
the BMC to receive the data. On a low memory BMC, the preparation step can be
used to shut down unnecessary services, etc.

The actions are split into three fields:

*   `preparation`
*   `verification`
*   `update`

The `preparation`, `verification`, and `update` fields expect a `type` field,
similarly to the `handler` field. This dictates what other parameters may be
required.

In this configuration the `preparation` type is `systemd`. The `systemd` type
expects to receive a `unit`. The `unit` value is expected to be a string that is
the name of the systemd unit to start. In this case, it'll start
`phosphor-ipmi-flash-bios-prepare.target`. This can be a single service name, or
a target.

In this configuration the `verification` type is `systemd`. This will query
systemd for the status of the verification unit to determine running, success,
or failure.

In this configuration the `update` type is `systemd`. This is the same object as
with the `preparation` action.

## config-static-bmc-reboot.json

This file is generated from configuration and therefore some values can be
replaced, here we're using the defaults.

```
[{
    "blob": "/flash/image",
    "handler": {
        "type": "file",
        "path": "/run/initramfs/bmc-image"
    },
    "actions": {
        "preparation": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bmc-prepare.target"
        },
        "verification": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bmc-verify.target"
        },
        "update": {
            "type": "reboot"
        }
    }
}]
```

Given the bios configuration, we can skip the parts of this configuration that
are already explained. The new action type is `reboot`. This is a special action
type that has no parameters. It will simply trigger a reboot via systemd.

## config-static-bmc.json

This file is generated from configuration and therefore some values can be
replaced, here we're using the defaults.

```
[{
    "blob": "/flash/image",
    "handler": {
        "type": "file",
        "path": "/run/initramfs/bmc-image"
    },
    "actions": {
        "preparation": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bmc-prepare.target"
        },
        "verification": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bmc-verify.target"
        },
        "update": {
            "type": "systemd",
            "unit": "phosphor-ipmi-flash-bmc-update.target"
        }
    }
}]
```

This configuration is of no significance in its difference.

## Writing your own json Configuration

If you wish to write your own json configuration, you simply create a legal json
file containing an array of at least one entry. This array will define any
entries you wish. You can provide multiple BMC update json configurations if you
wish. There may be parameters you desire to pass to an update process, for
instance, clearing the whitelisted files. In this case, it's trivial to create
two targets. The first target performs a normal update, the second starts a
service that performs the desired other update. You can leverage the
default-provided configuration files, and also add your own to add additional
support if desired.

An entry, regardless, must contain a `blob`, a `handler`, and `actions`.

### Handler Types

A handler determines how the bytes from the host are stored.

#### `file`

The `file` handler type writes the bytes to a file path specified by the
required parameter `path`.

*   `path` - full file system path to where to write bytes.

### Action Types

Action types are used to define what to do for a specific requested action, such
as "verify the blob contents."

#### `skip`

The `skip` type will effectively make that action a no-op and always return
success.

#### `systemd`

The `systemd` type should be used when you wish to start a systemd service or
target. For verification and update operations this will track the status of the
systemd service to determine success or failure.

*   `unit` - required - string - the systemd unit to start.
*   `mode` - optional - string - default: replace - the mode for starting the
    service.

#### `fileSystemdVerify` & `fileSystemdUpdate`

Because one may care about the result of their actions, the `fileSystemdVerify`
and `fileSystemdUpdate` action type exists. It will start the service and when
asked for a status, it'll read the contents of a file. Therefore, whatever is
performing the action will want to update that file. NOTE: Now that the systemd
type action tracks unit status, that action is now preferred.

*   `path` - required - string - the full file system path to where one finds
    the status.
*   `unit` - required - string - the systemd unit to start
*   `mode` - optional - string - default: replace - the mode for starting the
    service.

#### `reboot`

The `reboot` type causes a reboot via systemd. If this happens quickly enough,
you may not receive a response over IPMI that your command succeeded.

### Installing your own json configuration

To install your own configuration(s) you can add your own bbappend file that
appends to the install task. The json files are installed in
`${D}${datadir}/phosphor-ipmi-flash/`

## Adding your own handler type

Firstly, welcome! The maintainers of this repository appreciate your
contribution. A handler is just an implementation of the
`ImageHandlerInterface`.

Your handler must implement:

*   `bool open(const std::string& path)`
*   `void close()`
*   `bool write(std::uint32_t offset, const std::vector<std::uint8_t>& data)`
*   `int getSize()`

The handler is meant to receive the bytes, and write the bytes.

Once the object is defined you can specify a type and parameters here and in the
`buildjson` module.

## Adding your own action type

Firstly, welcome! The maintainers of this repository appreciate your
contribution. An action is just an implementation of the
`TriggerableActionInterface`.

Your action must implement:

*   `bool trigger()`
*   `void abort()`
*   `ActionStatus status()`

The abort method is not a guarantee.

Once the object is defined you can specify a type and parameters here and in the
`buildjson` module.
