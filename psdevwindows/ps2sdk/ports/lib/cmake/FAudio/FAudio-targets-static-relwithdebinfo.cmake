#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "FAudio::FAudio-static" for configuration "RelWithDebInfo"
set_property(TARGET FAudio::FAudio-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(FAudio::FAudio-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libFAudio.a"
  )

list(APPEND _cmake_import_check_targets FAudio::FAudio-static )
list(APPEND _cmake_import_check_files_for_FAudio::FAudio-static "${_IMPORT_PREFIX}/lib/libFAudio.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
