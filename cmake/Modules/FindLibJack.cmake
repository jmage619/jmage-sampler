set(LIBJACK_DEFINITIONS "")

find_path(LIBJACK_INCLUDE_DIR jack/jack.h)

find_library(LIBJACK_LIBRARY jack)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibJack DEFAULT_MSG
  LIBJACK_INCLUDE_DIR LIBJACK_LIBRARY)

mark_as_advanced(LIBJACK_INCLUDE_DIR LIBJACK_LIBRARY)

set(LIBJACK_LIBRARIES ${LIBJACK_LIBRARY})
set(LIBJACK_INCLUDE_DIRS ${LIBJACK_INCLUDE_DIR})
