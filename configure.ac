#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pencil-driver], [0.0], [carp-pencil@imperial.ac.uk])
AC_CONFIG_MACRO_DIR([m4])

# Automake
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AM_MAINTAINER_MODE([enable])

# Programs
AM_PROG_AR
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11(noext,mandatory)
AC_PROG_LIBTOOL
   
AC_ARG_ENABLE([host-alloc],
	[AS_HELP_STRING([--enable-host-alloc], [Allocate on host])],
	[case "${enableval}" in
		yes) CPPFLAGS="${CPPFLAGS} -DHOST_ALLOC=1" ;;
		no)  ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-host-alloc]) ;;
	esac],
	[])
AM_CONDITIONAL(HOST_ALLOC, test "x$enabled_host_alloc" = xyes)

AC_ARG_ENABLE([thread-safety],
    [AS_HELP_STRING([--enable-thread-safety], [Make thread-safe library])],
    [case "${enableval}" in
        yes) CPPFLAGS="${CPPFLAGS} -DTHREAD_SAFE=1" ;;
        no)  ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-thread-safety]) ;;
    esac],
    [])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([include/Makefile])
AC_CONFIG_FILES([runtime/Makefile])

AC_OUTPUT
