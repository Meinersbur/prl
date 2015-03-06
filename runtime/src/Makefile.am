# @configure_input@
 
AM_CPPFLAGS = "-I${abs_srcdir}/../include"

lib_LTLIBRARIES = libprl.la

libprl_la_SOURCES = prl_impl.cpp prl.c ocl_utilities.c

# Private headers; do not install, but distribute
noinst_HEADERS = \
	prl_impl.h \
	ocl_utilities.h
