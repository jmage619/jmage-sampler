find_package(LibLv2 REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSampleRate REQUIRED)

include_directories(../ ${LIBLV2_INCLUDE_DIRS})

add_library(jm-sampler-lv2 SHARED jm-sampler-lv2.cpp plugin.cpp
  $<TARGET_OBJECTS:wave> $<TARGET_OBJECTS:sfzparser> $<TARGET_OBJECTS:jmsampler>
  $<TARGET_OBJECTS:components>)
set_target_properties(jm-sampler-lv2 PROPERTIES PREFIX "")
target_link_libraries(jm-sampler-lv2 ${LIBSNDFILE_LIBRARIES} ${LIBSAMPLERATE_LIBRARIES})

install(TARGETS jm-sampler-lv2 DESTINATION lib${LIB_SUFFIX}/lv2/jmage-sampler.lv2)
