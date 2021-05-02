find_package(LibJack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSampleRate REQUIRED)

include_directories(../ ${LIBJACK_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})

add_executable(jmage-sampler jmage-sampler.cpp $<TARGET_OBJECTS:wave>
   $<TARGET_OBJECTS:sfzparser> $<TARGET_OBJECTS:jmsampler> $<TARGET_OBJECTS:components>)

target_link_libraries(jmage-sampler ${LIBJACK_LIBRARIES} ${LIBSNDFILE_LIBRARIES} ${LIBSAMPLERATE_LIBRARIES})

install(TARGETS jmage-sampler DESTINATION bin)
