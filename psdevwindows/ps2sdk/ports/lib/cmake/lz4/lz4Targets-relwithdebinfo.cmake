#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LZ4::lz4_static" for configuration "RelWithDebInfo"
set_property(TARGET LZ4::lz4_static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(LZ4::lz4_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/liblz4.a"
  )

list(APPEND _cmake_import_check_targets LZ4::lz4_static )
list(APPEND _cmake_import_check_files_for_LZ4::lz4_static "${_IMPORT_PREFIX}/lib/liblz4.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
