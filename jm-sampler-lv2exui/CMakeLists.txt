find_package(LibLv2)

include_directories(../ ${LIBLV2_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})

add_library(jm-sampler-lv2exui SHARED jm-sampler-lv2exui.cpp)
set_target_properties(jm-sampler-lv2exui PROPERTIES PREFIX "")

install(TARGETS jm-sampler-lv2exui DESTINATION lib${LIB_SUFFIX}/lv2/jmage-sampler.lv2)
