cmake_minimum_required(VERSION 3.4...3.18)
project(_tiny_fmm)

add_subdirectory(pybind11)
pybind11_add_module(pybind11_tiny_fmm src/main.cpp)

target_compile_definitions(pybind11_tiny_fmm
                           PRIVATE VERSION_INFO=${TINY_FMM_VERSION_INFO})
