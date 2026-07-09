#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpusFile::opusfile" for configuration "RelWithDebInfo"
set_property(TARGET OpusFile::opusfile APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(OpusFile::opusfile PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libopusfile.a"
  )

list(APPEND _cmake_import_check_targets OpusFile::opusfile )
list(APPEND _cmake_import_check_files_for_OpusFile::opusfile "${_IMPORT_PREFIX}/lib/libopusfile.a" )

# Import target "OpusFile::opusurl" for configuration "RelWithDebInfo"
set_property(TARGET OpusFile::opusurl APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(OpusFile::opusurl PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libopusurl.a"
  )

list(APPEND _cmake_import_check_targets OpusFile::opusurl )
list(APPEND _cmake_import_check_files_for_OpusFile::opusurl "${_IMPORT_PREFIX}/lib/libopusurl.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
