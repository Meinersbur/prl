# @configure_input@
 
ACLOCAL_AMFLAGS = -I m4



OCL_UTILITIES_DIR=`dirname "@OCL_UTILITIES_H@"`
AM_CPPFLAGS = "-I${abs_srcdir}/runtime" "-I${OCL_UTILITIES_DIR}"

lib_LTLIBRARIES = libocl_pencil_ref.la libocl_pencil_opt.la

libocl_pencil_ref_la_SOURCES = opencl/pencil_runtime.c ocl_utilities.c

libocl_pencil_opt_la_SOURCES = opencl-opt/pencil_runtime.c opencl-opt/impl/pencil_runtime_impl.cpp ocl_utilities.c

if HOST_ALLOC
	AM_CPPFLAGS+=-DHOST_ALLOC=1
endif

ocl_utilities.c: @OCL_UTILITIES_C@
	ln -sf $< $@

ocl_utilities.h: @OCL_UTILITIES_H@
	ln -sf $< $@
	
CLEANFILES = ocl_utilities.c ocl_utilities.h

# Note: Not good to install pencil_int.h into opencl subdirectory; Might be confused with official OpenCL headers
nobase_include_HEADERS = pencil_runtime.h opencl/pencil_int.h
