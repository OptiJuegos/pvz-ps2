#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "theora::theora" for configuration "RelWithDebInfo"
set_property(TARGET theora::theora APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(theora::theora PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libtheora.a"
  )

list(APPEND _cmake_import_check_targets theora::theora )
list(APPEND _cmake_import_check_files_for_theora::theora "${_IMPORT_PREFIX}/lib/libtheora.a" )

# Import target "theora::theoraenc" for configuration "RelWithDebInfo"
set_property(TARGET theora::theoraenc APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(theora::theoraenc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libtheoraenc.a"
  )

list(APPEND _cmake_import_check_targets theora::theoraenc )
list(APPEND _cmake_import_check_files_for_theora::theoraenc "${_IMPORT_PREFIX}/lib/libtheoraenc.a" )

# Import target "theora::theoradec" for configuration "RelWithDebInfo"
set_property(TARGET theora::theoradec APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(theora::theoradec PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "C"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libtheoradec.a"
  )

list(APPEND _cmake_import_check_targets theora::theoradec )
list(APPEND _cmake_import_check_files_for_theora::theoradec "${_IMPORT_PREFIX}/lib/libtheoradec.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
