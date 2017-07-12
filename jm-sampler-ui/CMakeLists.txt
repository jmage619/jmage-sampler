set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)

include_directories(../)

add_executable(jm-sampler-ui main.cpp jm-sampler-ui.cpp
  components.cpp zonegrid.cpp)
target_link_libraries(jm-sampler-ui Qt5::Widgets)

install(TARGETS jm-sampler-ui DESTINATION libexec)
