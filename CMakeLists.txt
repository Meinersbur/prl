### CMake configuration
cmake_minimum_required(VERSION 3.2)

### Project details
project("prl" C)
set(PACKAGE_VERSION "0.0.1")
set(PACKAGE_BUGREPORT "prl@meinersbur.de")

set(PRL_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(PRL_BINARY_DIR ${PROJECT_BINARY_DIR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
  set(CMAKE_COMPILER_IS_CLANGC ON)
endif()

if (CMAKE_COMPILER_IS_CLANGC OR CMAKE_COMPILER_IS_GNUCC)
   add_definitions(-Wall)
endif ()

find_package(OpenCL REQUIRED)

add_subdirectory(src)

add_executable(correlation correlation.ppcg_opencl.c)
target_link_libraries(correlation prl_opencl)
