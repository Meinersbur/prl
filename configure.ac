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

# Options
AC_ARG_WITH([ocl_utilities],
            [AS_HELP_STRING([--with-ocl_utilities], [path to ocl_utilities.c])],
            [],
            [AC_MSG_ERROR(--with-ocl_utilities is mandatory)])
     
AC_CHECK_FILE(["${with_ocl_utilities}"], [], [AC_MSG_ERROR(ocl_utilities.c not found)])
OCL_UTILITIES_C="${with_ocl_utilities}"
AC_SUBST(OCL_UTILITIES_C)
    
   
AC_ARG_ENABLE([host-alloc],
	[AS_HELP_STRING([--enable-host-alloc], [Allocate on host])],
	[case "${enableval}" in
		yes) enabled_host_alloc=${enableval};;
		no)  enabled_host_alloc=${enableval} ;;
		*) AC_MSG_ERROR([bad value ${enableval} for --enable-host-alloc]) ;;
	esac],
	[enabled_host_alloc=no])
AM_CONDITIONAL(HOST_ALLOC, test "x$enabled_host_alloc" = xyes)

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
