#----------------------------------------------------------------
# Generated CMake target import file for configuration "release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "hv_static" for configuration "release"
set_property(TARGET hv_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(hv_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libhv_static.a"
  )

list(APPEND _cmake_import_check_targets hv_static )
list(APPEND _cmake_import_check_files_for_hv_static "${_IMPORT_PREFIX}/lib/libhv_static.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
