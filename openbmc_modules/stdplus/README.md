# stdplus

stdplus is a c++ project containing commonly used classes and functions
for the Linux platform. Note the goals of this project are to implement only
the minimum set of features needed by the OpenBMC project and other users. You
may find the included set of functions currently quite sparse.

This project provides the following primitives:
* A [movable](src/stdplus/handle/managed.hpp) and [copyable](src/stdplus/handle/copyable.hpp) RAII helper wrapper which is used for wrapping c-native types that have custom destruction or copy reference logic.
* [Functions](src/stdplus/signal.hpp) for trivially configuring signals without having to do the normal signal set operations from libc
* [C-Style Error Handler](src/stdplus/util/cexec.hpp) that wrap c-style functions which return errnos and negative error values into functions that throw c++ exceptions.
* [String Utilities](src/stdplus/util/string.hpp) that focus on providing helpful wrappers like efficient string append and concatenation.

## Dependencies

Test cases require google{test,mock}, valgrind, and lcov. It also requires the Meson Build System which can be installed following the instructions at [mesonbuild.com](https://mesonbuild.com/Getting-meson.html).

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
