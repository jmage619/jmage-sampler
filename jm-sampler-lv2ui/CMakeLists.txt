find_package(LibLv2)
find_package(LibSndFile REQUIRED)
find_package(LibSampleRate REQUIRED)

include_directories(../ ${LIBLV2_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})

add_library(jm-sampler-lv2ui SHARED jm-sampler-lv2ui.cpp
  $<TARGET_OBJECTS:wave> $<TARGET_OBJECTS:sfzparser> $<TARGET_OBJECTS:components>
  $<TARGET_OBJECTS:jmsampler>)
set_target_properties(jm-sampler-lv2ui PROPERTIES PREFIX "")
target_link_libraries(jm-sampler-lv2ui ${LIBSNDFILE_LIBRARIES} ${LIBSAMPLERATE_LIBRARIES})

install(TARGETS jm-sampler-lv2ui DESTINATION lib${LIB_SUFFIX}/lv2/jmage-sampler.lv2)
