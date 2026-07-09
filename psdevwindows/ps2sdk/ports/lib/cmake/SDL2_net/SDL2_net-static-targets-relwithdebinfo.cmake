#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL2_net::SDL2_net-static" for configuration "RelWithDebInfo"
set_property(TARGET SDL2_net::SDL2_net-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(SDL2_net::SDL2_net-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libSDL2_net.a"
  )

list(APPEND _cmake_import_check_targets SDL2_net::SDL2_net-static )
list(APPEND _cmake_import_check_files_for_SDL2_net::SDL2_net-static "${_IMPORT_PREFIX}/lib/libSDL2_net.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
