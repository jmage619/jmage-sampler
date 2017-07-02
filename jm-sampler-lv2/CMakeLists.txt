find_package(LibLv2)

include_directories(../ ${LIBLV2_INCLUDE_DIRS})

add_library(jm-sampler-lv2 SHARED jm-sampler-lv2.cpp plugin.cpp
  $<TARGET_OBJECTS:wave> $<TARGET_OBJECTS:sfzparser> $<TARGET_OBJECTS:jmsampler>
  $<TARGET_OBJECTS:components>)
set_target_properties(jm-sampler-lv2 PROPERTIES PREFIX "")
