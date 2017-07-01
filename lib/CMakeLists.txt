find_package(LibSampleRate REQUIRED)
include_directories(${LIBSAMPLERATE_INCLUDE_DIRS})

#add_library(components OBJECT components.cpp)
add_library(components SHARED components.cpp)
target_link_libraries(components ${LIBSAMPLERATE_LIBRARIES})
