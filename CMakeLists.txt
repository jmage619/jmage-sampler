cmake_minimum_required (VERSION 2.8)

project(jmage_sampler)
set(ctest_VERSION_MAJOR 0)
set(ctest_VERSION_MINOR 3)
set(ctest_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

add_subdirectory(lib)
add_subdirectory(jm-sampler-ui)
