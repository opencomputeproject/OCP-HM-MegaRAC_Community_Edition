# ipmi-blob-tool

ipmi-blob-tool is a host-side tool that speaks the BLOB protocol over IPMI.

## Dependencies

Test cases require google{test,mock}, valgrind, and lcov.

## Building
For a standard release build, you want something like:
```
./bootstrap.sh
./configure --disable-tests
make
make install
```

For a test / debug build, a typical configuration is
```
./bootstrap.sh
./configure --enable-tests --enable-coverage --enable-valgrind
make
make check
make check-valgrind
make check-code-coverage
```
