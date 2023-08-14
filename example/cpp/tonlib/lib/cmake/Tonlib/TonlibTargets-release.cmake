#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Tonlib::tonlibjson" for configuration "Release"
set_property(TARGET Tonlib::tonlibjson APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Tonlib::tonlibjson PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libtonlibjson.so.0.5"
  IMPORTED_SONAME_RELEASE "libtonlibjson.so.0.5"
  )

list(APPEND _IMPORT_CHECK_TARGETS Tonlib::tonlibjson )
list(APPEND _IMPORT_CHECK_FILES_FOR_Tonlib::tonlibjson "${_IMPORT_PREFIX}/lib/libtonlibjson.so.0.5" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
