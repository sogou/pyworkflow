cmake_minimum_required(VERSION 3.4)
project(cpp_pyworkflow CXX)
set(CMAKE_BUILD_TYPE "RelWithDebInfo") # Attention Release will strip debug info
set(CMAKE_VERBOSE_MAKEFILE ON)
add_subdirectory(workflow)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_compile_options(-Wall -Wextra -Wno-unused-parameter)
set(CMAKE_CXX_FLAGS, "-std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG, "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE, "-O2 -g")
add_subdirectory(pybind11)
pybind11_add_module(cpp_pyworkflow
    src/common_types.cc
    src/network_types.cc
    src/http_types.cc
    src/redis_types.cc
    src/other_types.cc
    src/pyworkflow.cc)

#find_package(workflow REQUIRED CONFIG HINTS workflow)
include_directories(./workflow/_include)
target_link_libraries(cpp_pyworkflow PRIVATE workflow pthread ssl crypto)
target_compile_definitions(cpp_pyworkflow PRIVATE VERSION_INFO=${PYWORKFLOW_VERSION_INFO})
