#!/bin/sh

AUTOCONF_FILES="Makefile.in aclocal.m4 ar-lib autom4te.cache compile \
        config.guess config.h.in config.sub configure depcomp install-sh \
        ltmain.sh missing *libtool test-driver"

case $1 in
    clean)
        test -f Makefile && make maintainer-clean
        test -f linux/ipmi_bmc.h && rm -rf linux/ipmi_bmc.h
        test -d linux && find linux -type d -empty | xargs -r rm -rf
        for file in ${AUTOCONF_FILES}; do
            find -name "$file" | xargs -r rm -rf
        done
        exit 0
        ;;
esac

autoreconf -i
echo 'Run "./configure ${CONFIGURE_FLAGS} && make"'
