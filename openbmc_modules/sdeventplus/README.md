# sdeventplus

sdeventplus is a c++ wrapper around the systemd sd_event apis meant
to provide c++ ergonomics to their usage.

## Dependencies

The sdeventplus library requires a libsystemd development package on the
system for sd-event.

## Building
For a standard release build, you want something like:
```
meson setup -Dexamples=false -Dtests=disabled builddir
ninja -C builddir
ninja -C builddir install
```

For a test / debug build, a typical configuration is
```
meson setup -Dtests=enabled builddir
meson test -C builddir
```
