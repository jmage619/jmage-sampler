set(LIBLV2_DEFINITIONS "")

find_path(LIBLV2_INCLUDE_DIR lv2/lv2plug.in/ns/lv2core/lv2.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibLv2 DEFAULT_MSG
  LIBLV2_INCLUDE_DIR)

mark_as_advanced(LIBLV2_INCLUDE_DIR)

set(LIBLV2_LIBRARIES "")
set(LIBLV2_INCLUDE_DIRS ${LIBLV2_INCLUDE_DIR})
