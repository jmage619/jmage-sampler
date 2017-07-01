find_package(LibSampleRate REQUIRED)
find_package(LibSndFile REQUIRED)
include_directories(${LIBSAMPLERATE_INCLUDE_DIRS} ${LIBSNDFILE_INCLUDE_DIRS})

#add_library(components OBJECT components.cpp)
add_library(components SHARED components.cpp)
target_link_libraries(components ${LIBSAMPLERATE_LIBRARIES})

add_library(sfzparser SHARED sfzparser.cpp)

add_library(wave SHARED wave.cpp)
target_link_libraries(wave ${LIBSNDFILE_LIBRARIES})
