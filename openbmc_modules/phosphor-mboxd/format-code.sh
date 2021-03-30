#!/bin/sh

set -euo

set -x

[ -f .clang-format ] && rm .clang-format

CLANG_FORMAT="$(which clang-format-5.0)"

# phosphor-mboxd is a fork of mboxbridge, the reference mbox daemon
# implementation. mboxbridge is C written with the style of the Linux kernel.
#
# phosphor-mboxd extended the reference in C++, and used the OpenBMC C++ style.
#
# To remain compliant with the C++ style guide *and* preserve source
# compatibility with the upstream reference implementation, use two separate
# styles.
#
# Further, clang-format supports describing styles for multiple languages in
# the one .clang-format file, but *doesn't* make a distinction between C and
# C++. So we need two files. It gets worse: the -style parameter doesn't take
# the path to a configuration file as an argument, you instead specify the
# literal 'file' and it goes looking for a .clang-format or _clang-format file. 
# So now we need to symlink different files in place before calling
# ${CLANG_FORMAT}. Everything is terrible.
#
# ln -sf .clang-format-c .clang-format
# git ls-files | grep '\.[ch]$' | xargs "${CLANG_FORMAT}" -i -style=file

ln -sf .clang-format-c++ .clang-format
git ls-files | grep '\.[ch]pp$' | xargs "${CLANG_FORMAT}" -i -style=file

rm .clang-format
