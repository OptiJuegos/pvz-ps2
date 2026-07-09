#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MPG123::libmpg123" for configuration "RelWithDebInfo"
set_property(TARGET MPG123::libmpg123 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(MPG123::libmpg123 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libmpg123.a"
  )

list(APPEND _cmake_import_check_targets MPG123::libmpg123 )
list(APPEND _cmake_import_check_files_for_MPG123::libmpg123 "${_IMPORT_PREFIX}/lib/libmpg123.a" )

# Import target "MPG123::libout123" for configuration "RelWithDebInfo"
set_property(TARGET MPG123::libout123 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(MPG123::libout123 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libout123.a"
  )

list(APPEND _cmake_import_check_targets MPG123::libout123 )
list(APPEND _cmake_import_check_files_for_MPG123::libout123 "${_IMPORT_PREFIX}/lib/libout123.a" )

# Import target "MPG123::libsyn123" for configuration "RelWithDebInfo"
set_property(TARGET MPG123::libsyn123 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(MPG123::libsyn123 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libsyn123.a"
  )

list(APPEND _cmake_import_check_targets MPG123::libsyn123 )
list(APPEND _cmake_import_check_files_for_MPG123::libsyn123 "${_IMPORT_PREFIX}/lib/libsyn123.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
