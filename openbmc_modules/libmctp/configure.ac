# Initialization
AC_PREREQ([2.69])
AC_INIT([libmctp], 0.10, [https://github.com/openbmc/libmctp/issues])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror -Wno-portability foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# libtool init
LT_INIT

AC_CHECK_HEADERS_ONCE([endian.h])
AC_CHECK_HEADERS_ONCE([unistd.h fcntl.h])

# pkg-config
PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR
AC_ARG_WITH([systemdsystemunitdir],
     [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],
     [],
     [with_systemdsystemunitdir=auto]
)
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"],
    [def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
     AS_IF([test "x$def_systemdsystemunitdir" = "x"],
           [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
                  [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])]
            )
            with_systemdsystemunitdir=no],
           [with_systemdsystemunitdir="$def_systemdsystemunitdir"]
     )]
)
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])]
)
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

AC_ARG_WITH([syslog],
            [AS_HELP_STRING([--with-syslog], [Support logging to syslog])],
            [],
            [with_syslog=check])

AS_IF([test "x$with_syslog" != "xno"],
      [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
#include <stdarg.h>
#include <syslog.h>

void check_vsyslog(int level, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsyslog(0, fmt, ap);
    va_end(ap);
}
        ]],[[
    check_vsyslog(0, "\n");
        ]])],
       [AC_DEFINE([MCTP_HAVE_SYSLOG], [1], [Define to enable syslog])],
       [])],
      [])

AC_ARG_WITH([fileio],
            [AS_HELP_STRING([--with-fileio],
                            [Support interfaces based on file-descriptors])],
            [],
            [with_fileio=check])

AS_IF([test "x$with_fileio" = "xcheck"],
      [AC_DEFINE([MCTP_HAVE_FILEIO], [(HAVE_UNISTD_H && HAVE_FCNTL_H)],
                 [Support interfaces based on file-descriptors])],
      [AS_IF([test "x$with_fileio" = "xyes"],
             [AC_DEFINE([MCTP_HAVE_FILEIO], [1],
                        [Support interfaces based on file-descriptors])],
             [])])

AC_ARG_WITH([stdio],
            [AS_HELP_STRING([--with-stdio], [Support logging to stdio])],
            [],
            [with_stdio=check])

AS_IF([test "x$with_stdio" != "xno"],
      [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
#include <stdarg.h>
#include <stdio.h>
void check_vprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
        ]],[[
    check_vprintf("\n");
        ]])],
       [AC_DEFINE([MCTP_HAVE_STDIO], [1], [Define to enable stdio functions])],
       [])],
      [])

AC_ARG_WITH([default-alloc],
            [AS_HELP_STRING([--with-default-alloc],
                            [Use libc malloc and free for heap memory])],
            [],
            [with_default_alloc=check])

AS_IF([test "x$with_default_alloc" != "xno"],
      [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[
#include <stdlib.h>
        ]], [[
free(malloc(4096));
        ]])],
       [AC_DEFINE([MCTP_DEFAULT_ALLOC],
                  [1],
                  [Define to use libc malloc and free for heap memory])],
       [])],
      [])

# Enable all bindings. AC_ARG_ENABLE in future.
AM_CONDITIONAL([LIBMCTP_BINDING_serial], [true])
AM_CONDITIONAL([LIBMCTP_BINDING_astlpc], [true])

# Check for valgrind
AS_IF([test "x$enable_tests" = "xno"], [enable_valgrind=no])
m4_foreach([vgtool], [valgrind_tool_list],
    [AX_VALGRIND_DFLT(vgtool, [off])])
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_CHECK
AM_EXTRA_RECURSIVE_TARGETS([check-valgrind])
m4_foreach([vgtool], [valgrind_tool_list],
    [AM_EXTRA_RECURSIVE_TARGETS([check-valgrind-]vgtool)])

AX_CODE_COVERAGE
m4_ifdef([_AX_CODE_COVERAGE_RULES],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [true])],
    [AM_CONDITIONAL(AUTOCONF_CODE_COVERAGE_2019_01_06, [false])])
AX_ADD_AM_MACRO_STATIC([])

AC_CONFIG_FILES([Makefile libmctp.pc])
AC_OUTPUT
