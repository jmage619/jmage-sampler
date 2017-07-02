find_package(LibSampleRate REQUIRED)
find_package(LibSndFile REQUIRED)

include_directories(${LIBSAMPLERATE_INCLUDE_DIRS} ${LIBSNDFILE_INCLUDE_DIRS})

#set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)

add_library(components OBJECT components.cpp)
set_property(TARGET components PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(sfzparser OBJECT sfzparser.cpp)
set_property(TARGET sfzparser PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(wave OBJECT wave.cpp)
set_property(TARGET wave PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(jmsampler OBJECT jmsampler.cpp)
set_property(TARGET jmsampler PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(gui_zonegrid OBJECT gui_zonegrid.cpp)
target_include_directories(gui_zonegrid PRIVATE
  $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(gui_zonegrid PRIVATE
  $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_COMPILE_DEFINITIONS>)
set_property(TARGET gui_zonegrid PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(gui_zonegrid PROPERTIES AUTOMOC TRUE)

add_library(gui_components OBJECT gui_components.cpp)
target_include_directories(gui_components PRIVATE
  $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(gui_components PRIVATE
  $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_COMPILE_DEFINITIONS>)
set_property(TARGET gui_components PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(gui_components PROPERTIES AUTOMOC TRUE)
