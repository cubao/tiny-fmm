cmake_minimum_required(VERSION 3.4...3.18)
project(tiny_fmm)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(pybind11)
pybind11_add_module(pybind11_tiny_fmm src/main.cpp)

target_compile_definitions(pybind11_tiny_fmm
                           PRIVATE VERSION_INFO=${TINY_FMM_VERSION_INFO})
