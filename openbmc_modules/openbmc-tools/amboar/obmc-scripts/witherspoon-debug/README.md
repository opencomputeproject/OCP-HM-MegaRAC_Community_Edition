Description
===========

The OpenBMC debugtools tarball clashes with the filesystem layout of
witherspoon systems, and contains a number of other outstanding issues with
respect to the tools it deploys. The debug script here documents and executes
the workarounds necessary to make it functional.

Usage
-----
```
./debug [USER@]HOST TARBALL
```

For example:

```
$ ./debug root@my-witherspoon obmc-phosphor-debug-tarball-witherspoon.tar.xz
```

Notes
-----

The script will:

1. Ensure `/usr/local` is a tmpfs mountpoint (may not be if system is
   configured for Field Mode)
2. Deploy the debugtools tarball to `/usr/local`
3. Make `perf(1)` work by
   1. Installing a fake `expand(1)` if necessary
   2. Fixing the `objdump(1)` symlink
   3. Disabling `perf(1)`'s buildid tracking (fills the RW filesystem)
4. Make `latencytop(1)` work by deploying and symlinking libncurses{,w}.so
