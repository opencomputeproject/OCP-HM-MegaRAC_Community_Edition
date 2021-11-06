SUMMARY = "Rsyslog external Application "
DESCRIPTION = "Rsyslog external application to rotate log files"

LICENSE = "CLOSED"

inherit cmake
#inherit pkgconfig
#inherit python3native
#inherit obmc-phosphor-dbus-service

SRC_URI = "file://CMakeLists.txt \
           file://src/main.cpp \
	   file://include/main.hpp \
          "

S = "${WORKDIR}"


APP_NAME = "rsyslog-rotate"
localdir = "usr/"
bindir = "${localdir}/bin"

DEPENDS += "rsyslog"

do_compile_prepend () {
        cp -r ${TEMP_DIR}/projdef.h ${S}/include/projdef.h

}


FILES_${PN} = "/usr/bin/*"
