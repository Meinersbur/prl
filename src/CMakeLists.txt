
function (add_prl_library name type)
  cmake_parse_arguments(ARG "STATIC;SHARED" "FILENAME;FILENAME_WIN" "" ${ARGN})
  
  add_library(${name} ${type} "prl_opencl.c")
  set_property(TARGET ${name}  PROPERTY C_STANDARD 99)
  if (WIN32)
    if (ARG_FILENAME_WIN) 
      set_target_properties(${name} PROPERTIES OUTPUT_NAME "${ARG_FILENAME_WIN}")
    endif ()
  else ()
    if (ARG_FILENAME) 
      set_target_properties(${name} PROPERTIES OUTPUT_NAME "${ARG_FILENAME}")
    endif ()
  endif ()
  if (CMAKE_COMPILER_IS_CLANGCC OR CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(${name} PRIVATE "-Wall")
  endif ()
  if (MSVC)
    target_compile_options(${name} PRIVATE "/Wall")
  endif ()
  target_include_directories(${name} PUBLIC "${PRL_SOURCE_DIR}/include" ${OpenCL_INCLUDE_DIR})
  target_link_libraries(${name} PUBLIC ${OpenCL_LIBRARIES} -lm)
  set_target_properties(${name} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PRL_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${PRL_BINARY_DIR}/lib"
  )
endfunction ()

add_prl_library(prl_opencl_static STATIC FILENAME "prl_opencl" FILENAME_WIN "prl_opencl_static")
add_prl_library(prl_opencl_shared SHARED FILENAME "prl_opencl" FILENAME_WIN "prl_opencl")
add_library(prl_opencl ALIAS prl_opencl_static)
