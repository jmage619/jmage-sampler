cmake_minimum_required (VERSION 2.8)

project(jmage_sampler)
set(ctest_VERSION_MAJOR 0)
set(ctest_VERSION_MINOR 3)
set(ctest_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

configure_file("${PROJECT_SOURCE_DIR}/lib/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h")

add_subdirectory(lib)
add_subdirectory(jm-sampler-ui)
add_subdirectory(jmage-sampler)
add_subdirectory(jm-sampler-lv2ui)
add_subdirectory(jm-sampler-lv2)

install(FILES manifest.ttl jm-sampler.ttl DESTINATION lib${LIB_SUFFIX}/lv2/jmage-sampler.lv2)
