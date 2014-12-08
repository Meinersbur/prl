# @configure_input@
 
ACLOCAL_AMFLAGS = -I m4

OCL_UTILITIES_DIR=`dirname "@OCL_UTILITIES_C@"`
AM_CPPFLAGS = "-I${abs_srcdir}/runtime" "-I${OCL_UTILITIES_DIR}"

lib_LTLIBRARIES = libocl_pencil_ref.la libocl_pencil_opt.la

libocl_pencil_ref_la_SOURCES = runtime/opencl/pencil_runtime.c ocl_utilities.c

libocl_pencil_opt_la_SOURCES = runtime/opencl-opt/pencil_runtime.c runtime/opencl-opt/impl/pencil_runtime_impl.cpp ocl_utilities.c

if HOST_ALLOC
	AM_CPPFLAGS+=-DHOST_ALLOC=1
endif

ocl_utilities.c: @OCL_UTILITIES_C@
	ln -sf $< $@
