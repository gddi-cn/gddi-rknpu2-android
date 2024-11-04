#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "tbb" for configuration "Release"
set_property(TARGET tbb APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(tbb PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libtbb.so"
  IMPORTED_SONAME_RELEASE "libtbb.so"
  )

list(APPEND _cmake_import_check_targets tbb )
list(APPEND _cmake_import_check_files_for_tbb "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libtbb.so" )

# Import target "opencv_core" for configuration "Release"
set_property(TARGET opencv_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opencv_core PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "tbb"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_core.so"
  IMPORTED_SONAME_RELEASE "libopencv_core.so"
  )

list(APPEND _cmake_import_check_targets opencv_core )
list(APPEND _cmake_import_check_files_for_opencv_core "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_core.so" )

# Import target "opencv_imgproc" for configuration "Release"
set_property(TARGET opencv_imgproc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opencv_imgproc PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_imgproc.so"
  IMPORTED_SONAME_RELEASE "libopencv_imgproc.so"
  )

list(APPEND _cmake_import_check_targets opencv_imgproc )
list(APPEND _cmake_import_check_files_for_opencv_imgproc "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_imgproc.so" )

# Import target "opencv_imgcodecs" for configuration "Release"
set_property(TARGET opencv_imgcodecs APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opencv_imgcodecs PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_imgcodecs.so"
  IMPORTED_SONAME_RELEASE "libopencv_imgcodecs.so"
  )

list(APPEND _cmake_import_check_targets opencv_imgcodecs )
list(APPEND _cmake_import_check_files_for_opencv_imgcodecs "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_imgcodecs.so" )

# Import target "opencv_videoio" for configuration "Release"
set_property(TARGET opencv_videoio APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opencv_videoio PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_videoio.so"
  IMPORTED_SONAME_RELEASE "libopencv_videoio.so"
  )

list(APPEND _cmake_import_check_targets opencv_videoio )
list(APPEND _cmake_import_check_files_for_opencv_videoio "${_IMPORT_PREFIX}/sdk/native/libs/arm64-v8a/libopencv_videoio.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
