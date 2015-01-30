# @configure_input@
 
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = "-I${abs_srcdir}/runtime"

lib_LTLIBRARIES = libocl_pencil_ref.la libocl_pencil_opt.la

libocl_pencil_ref_la_SOURCES = opencl/pencil_runtime.c

libocl_pencil_opt_la_SOURCES = opencl-opt/pencil_runtime.c opencl-opt/impl/pencil_runtime_impl.cpp ocl_utilities.c

# Note: Not good to install pencil_int.h into opencl subdirectory; Might be confused with official OpenCL headers
nobase_include_HEADERS = pencil_runtime.h opencl/pencil_int.h

# Private headers; do not install
noinst_HEADERS = \
	ocl_utilities.h \
	opencl-opt/pencil_int.h \
	opencl-opt/impl/impl.h
