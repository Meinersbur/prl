
#TODO Allow shared and static library

add_library(prl_opencl "prl_opencl.c")
target_include_directories(prl_opencl PUBLIC ${OpenCL_INCLUDE_DIR} "${PRL_SOURCE_DIR}/include")
target_link_libraries(prl_opencl INTERFACE ${OpenCL_LIBRARIES} -lm)
set_target_properties(prl_opencl PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${PRL_BINARY_DIR}/lib"
  ARCHIVE_OUTPUT_DIRECTORY "${PRL_BINARY_DIR}/lib"
)
