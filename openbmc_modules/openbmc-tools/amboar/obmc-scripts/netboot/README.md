Netboot a remote OpenBMC host via telnet (and hopefully at some stage, ssh).

To configure, edit `${HOME}/.config/obmc-scripts/netboot` to configure a
machine:

```
[foo]
platform = "bar"
user = "baz"
password = "quux"

    # telnet serial server
    [foo.console]
    host = "1.2.3.4"
    port = 5678

    [foo.u-boot]
    commands = [
        "setenv bootfile fitImage",
        "tftp",
    ]
```

Then netboot your machine:

```
$ ./netboot foo
```
