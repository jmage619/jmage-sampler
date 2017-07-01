find_package(LibSampleRate REQUIRED)
find_package(LibSndFile REQUIRED)
include_directories(${LIBSAMPLERATE_INCLUDE_DIRS} ${LIBSNDFILE_INCLUDE_DIRS})

add_library(components OBJECT components.cpp)
#set_property(TARGET components PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(sfzparser OBJECT sfzparser.cpp)

add_library(wave OBJECT wave.cpp)
#target_link_libraries(wave ${LIBSNDFILE_LIBRARIES})

add_library(jmsampler OBJECT jmsampler.cpp)
#target_link_libraries(jmsampler ${LIBSAMPLERATE_LIBRARIES})
