# gpioplus

gpioplus is a c++ wrapper around the linux gpio ioctl interface.
It aims to provide c++ ergonomics to the usage.

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
