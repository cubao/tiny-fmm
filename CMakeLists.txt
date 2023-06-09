cmake_minimum_required(VERSION 3.4...3.18)
project(tiny_fmm)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()

include_directories(3rdparty src src/tiny_fmm)

set(HEADER_ROOT "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}")
# file(GLOB_RECURSE HEADERS src/**/*.hpp) file(GLOB_RECURSE SOURCES
# src/**/*.cpp)
set(HEADERS # files
    src/tiny_fmm/dummy.hpp src/tiny_fmm/network/network.hpp)
set(SOURCES # files
    src/tiny_fmm/dummy.cpp src/tiny_fmm/network/network.cpp)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
foreach(file ${HEADERS})
  get_filename_component(dir ${file} DIRECTORY)
  string(REGEX REPLACE "${HEADER_ROOT}" "" dir ${dir})
  install(FILES ${file} DESTINATION "include/${PROJECT_NAME}/${dir}")
endforeach()

option(BUILD_TESTS "Build tests." ON)
if(BUILD_TESTS)
  file(GLOB SRCS tests/test_*.cpp)
  foreach(src ${SRCS})
    string(REGEX REPLACE "(^.*/|.cpp$)" "" exe ${src})
    add_executable(${exe} ${src})
    target_link_libraries(${exe} ${PROJECT_NAME})
    install(TARGETS ${exe} RUNTIME DESTINATION bin)
  endforeach(src)
endif()

set(PYBIND11_CPP_STANDARD -std=c++17)
add_subdirectory(pybind11)
file(GLOB_RECURSE BINDING_SRCS python/*.cpp)
pybind11_add_module(pybind11_tiny_fmm ${BINDING_SRCS})

target_compile_definitions(pybind11_tiny_fmm
                           PRIVATE VERSION_INFO=${TINY_FMM_VERSION_INFO})
