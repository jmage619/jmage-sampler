set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets)

include_directories(../)

add_executable(jm-sampler-ui main.cpp jm-sampler-ui.cpp
  $<TARGET_OBJECTS:gui_components> $<TARGET_OBJECTS:gui_zonegrid>)
target_link_libraries(jm-sampler-ui Qt5::Widgets)
