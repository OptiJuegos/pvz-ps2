#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MT32Emu::mt32emu" for configuration "RelWithDebInfo"
set_property(TARGET MT32Emu::mt32emu APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(MT32Emu::mt32emu PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libmt32emu.a"
  )

list(APPEND _cmake_import_check_targets MT32Emu::mt32emu )
list(APPEND _cmake_import_check_files_for_MT32Emu::mt32emu "${_IMPORT_PREFIX}/lib/libmt32emu.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
