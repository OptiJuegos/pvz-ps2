# - Try to find libopenmpt
# Once done this will define
#  OpenMPT_FOUND - System has libopenmpt
#  OpenMPT_INCLUDE_DIRS - The libopenmpt include directories
#  OpenMPT_LIBRARIES - The libraries needed to use libopenmpt

find_path(OpenMPT_INCLUDE_DIR "libopenmpt.h"
          PATH_SUFFIXES libopenmpt)
find_library(OpenMPT_LIBRARY NAMES openmpt)

if(OpenMPT_INCLUDE_DIR AND OpenMPT_LIBRARY)
    if(APPLE)
        find_library(OpenMPT_DYNAMIC_LIBRARY NAMES "openmpt"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(OpenMPT_DYNAMIC_LIBRARY NAMES "openmpt" PATH_SUFFIXES ".dll")
    else()
        find_library(OpenMPT_DYNAMIC_LIBRARY NAMES "openmpt" PATH_SUFFIXES ".so")
    endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OpenMPT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OpenMPT  DEFAULT_MSG
                                  OpenMPT_LIBRARY OpenMPT_INCLUDE_DIR)

mark_as_advanced(OpenMPT_INCLUDE_DIR OpenMPT_LIBRARY)

set(OpenMPT_LIBRARIES ${OpenMPT_LIBRARY} )
set(OpenMPT_INCLUDE_DIRS ${OpenMPT_INCLUDE_DIR} )

