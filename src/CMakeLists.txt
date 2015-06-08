
add_library(prl_opencl "prl_opencl.c")
target_include_directories(prl_opencl PUBLIC "${PRL_SOURCE_DIR}/include")
target_link_libraries(prl_opencl INTERFACE ${OpenCL_LIBRARIES})
