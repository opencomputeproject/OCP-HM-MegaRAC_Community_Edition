Copyright 2017 IBM

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

MBOX
====

This repo contains the protocol definition for the host to BMC mailbox
communication specification which can be found in
Documentation/mbox_procotol.md.

There is also a reference implementation of a BMC mailbox daemon, the details
of which can be found in Documentation/mboxd.md.

Finally there is also an implementation of a mailbox daemon control program, the
details of which can be found in Documentation/mboxctl.md.

Building
========

The build system is a standard autotools setup. `bootstrap.sh` runs all the
jobs necessary to initialise autotools.

By default mboxd is configured and built _without_ the 'virtual PNOR' feature
discussed below. The virtual PNOR functionality is written in C++, and due to
some autotools clunkiness even if it is disabled mboxd will still be linked
with `CXX`. Point `CXX` to `cc` at configure time if you do not have a C++
compiler for your target (`./configure CXX=cc`).

If you are hacking on the reference implementation it's recommended to run
`bootstrap.sh` with the `dev` argument:

```
$ ./bootstrap.sh dev
$ ./configure
$ make
$ make check
```

This will turn on several of the compiler's sanitizers to help find bad memory
management and undefined behaviour in the code via the test suites.

Otherwise, build with:

```
$ ./bootstrap.sh
$ ./configure
$ make
$ make check
```

In addition to its role as a flash abstraction `mboxd` can also serve as a
partition/filesystem abstraction. This feature is known as 'virtual PNOR' and
it can be enabled at `configure` time (note that this requires a C++ compiler
for your target):

```
$ ./bootstrap.sh
$ ./configure --enable-virtual-pnor
$ make
$ make check
```

Style Guide
===========

Preamble
--------

This codebase is a mix of C (due to its heritage) and C++. This is an ugly
split: message logging and error handling can be vastly different inside the
same codebase. The aim is to remove the split one way or the other over time
and have consistent approaches to solving problems.

phosphor-mboxd is developed as part of the OpenBMC project, which also leads to
integration of frameworks such as
[phosphor-logging](https://github.com/openbmc/phosphor-logging). Specifically
on phosphor-logging, it's noted that without care we can achieve absurd
duplication or irritating splits in where errors are reported, as the C code is
not capable of making use of the interfaces provided.

Rules
-----

1. Message logging MUST be done to stdout or stderr, and MUST NOT be done
   directly via journal APIs or wrappers of the journal APIs.

   Rationale:

   We have two scenarios where we care about output, with the important
   restriction that the method must be consistent between C and C++:

   1. Running in-context on an OpenBMC-based system
   2. Running the test suite

   In the first case it is desirable that the messages appear in the system
   journal. To this end, systemd will by default capture stdout and stderr of
   the launched binary and redirect it to the journal.

   In the second case it is *desirable* that messages be captured by the test
   runner (`make check`) for test failure analysis, and it is *undesirable* for
   messages to appear in the system journal (as these are tests, not issues
   affecting the health of the system they are being executed on).

   Therefore direct calls to the journal MUST be avoided for the purpose of
   message logging.

   Note: This section specifically targets the use of phosphor-logging's
   `log<T>()`. It does not prevent the use of `elog<T>()`.
