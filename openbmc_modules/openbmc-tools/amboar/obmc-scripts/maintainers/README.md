A Collection of Python Tools to Manipulate MAINTAINERS Files
============================================================

OpenBMC defines its own style of MAINTAINERS file that is almost but not
entirely alike the Linux kernel's MAINTAINERS file.

Historically the MAINTAINERS file was kept in the openbmc/docs repository and
described the maintainers for all repositories under the OpenBMC Github
organisation. Due to its separation from the repositories it was describing,
openbmc/docs:MAINTAINERS was both incomplete and out-of-date.

These scripts were developed to resolve unmaintained state of MAINTAINERS by
distributing the information into each associated repository.

General Use Stuff
=================

`obmc-gerrit` is a helper script for pushing changes to Gerrit. For a
repository with an OpenBMC-compatible MAINTAINERS file at its top level,
`obmc-gerrit` will parse the MAINTAINERS file and mangle the `git push`
`REFSPEC` such that the maintainers and reviewers listed for the repository are
automatically added to the changes pushed:

```
$ obmc-gerrit push gerrit HEAD:refs/for/master/maintainers
Counting objects: 13, done.
Delta compression using up to 4 threads.
Compressing objects: 100% (10/10), done.
Writing objects: 100% (13/13), 7.22 KiB | 7.22 MiB/s, done.
Total 13 (delta 2), reused 0 (delta 0)
remote: Resolving deltas: 100% (2/2)
remote: Processing changes: updated: 1, refs: 1, done
remote:
remote: Updated Changes:
remote:   https://gerrit.openbmc-project.xyz/10735 obmc-scripts: Add maintainers
remote:
To ssh://gerrit.openbmc-project.xyz:29418/openbmc/openbmc-tools
 * [new branch]                HEAD -> refs/for/master/maintainers%r=...
```

Installation
------------

obmc-gerrit requires Python3. If this is not available on your system (!), see
the virtualenv section below.

To install obmc-gerrit:

```
$ pip3 install --user -r requirements.txt
$ python3 setup.py install --user
```

I don't have Python3
--------------------

Well, hopefully you have `virtualenv`. If you do, then you can run the
following commands:

```
$ virtualenv --python=python3 .venv
$ . .venv/bin/activate
$ pip install -r requirements.txt
$ python setup.py install
```

To exit the virtualenv, run `deactivate`. To run `obmc-gerrit` you will need to
ensure you have activated your virtualenv first.

MAINTAINERS Library
===================

`maintainers.py` is the core library that handles parsing and assembling
MAINTAINERS files. An AST can be obtained with `parse_block()`, and the content
of a MAINTAINERS file can be obtained by passing an AST to `assemble_block()`

Once-off Thingos
================

`split_maintainers.py` is the script used to split the monolithic MAINTAINERS
file in openbmc/docs into per-repository MAINTAINERS files and post the patches
to Gerrit. Do not use this, it's captured for posterity.
