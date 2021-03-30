#!/bin/sh

set -eu

AUTOCONF_FILES="Makefile.in aclocal.m4 ar-lib autom4te.cache compile \
        config.guess config.h.in config.sub configure depcomp install-sh \
        ltmain.sh missing *libtool test-driver"

BOOTSTRAP_MODE=""

if [ $# -gt 0 ];
then
    BOOTSTRAP_MODE="${1}"
    shift 1
fi

case "${BOOTSTRAP_MODE}" in
    dev)
        AX_CODE_COVERAGE_PATH="$(aclocal --print-ac-dir)"/ax_code_coverage.m4
        if [ ! -e ${AX_CODE_COVERAGE_PATH} ];
        then
            echo "Failed to find AX_CODE_COVERAGE macro file at ${AX_CODE_COVERAGE_PATH}" 1>&2
            exit 1
        fi
        LCOV_VERSION=$(lcov --version | tr ' ' '\n' | tail -1)

        # Ubuntu Zesty ships with lcov v1.13, but Zesty's autoconf-archive
        # package (the provider of the AX_CODE_COVERAGE macro) doesn't support
        # it.
        #
        # sed-patch ax_code_coverage.m4 as it's GPLv3, and this is an Apache v2
        # licensed repository. The licenses are not compatible in our desired
        # direction[1].
        #
        # [1] https://www.apache.org/licenses/GPL-compatibility.html

        cp ${AX_CODE_COVERAGE_PATH} m4/
        sed -ri 's|(lcov_version_list=)"([ 0-9.]+)"$|\1"'${LCOV_VERSION}'"|' \
            m4/ax_code_coverage.m4
        ;;
    clean)
        test -f Makefile && make maintainer-clean
        test -d linux && find linux -type d -empty | xargs -r rm -rf
        for file in ${AUTOCONF_FILES}; do
            find -name "$file" | xargs -r rm -rf
        done
        exit 0
        ;;
    *)  ;;
esac

autoreconf -i

case "${BOOTSTRAP_MODE}" in
    dev)
        FLAGS="-fsanitize=address -fsanitize=leak -fsanitize=undefined -Wall -Werror"
        ./configure \
            CFLAGS="${FLAGS}" \
            CXXFLAGS="${FLAGS}" \
            --enable-code-coverage \
            "$@"
        ;;
    *)
        echo 'Run "./configure ${CONFIGURE_FLAGS} && make"'
        ;;
esac
