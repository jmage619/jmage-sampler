find_package(LibJack REQUIRED)

include_directories(../ ${LIBJACK_INCLUDE_DIRS} ${PROJECT_BINARY_DIR})

add_executable(jmage-sampler jmage-sampler.cpp)

target_link_libraries(jmage-sampler ${LIBJACK_LIBRARIES})

install(TARGETS jmage-sampler DESTINATION bin)
