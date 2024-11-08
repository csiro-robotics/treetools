# FindPROJ.cmake
#
# Finds the PROJ library installation
#
# This module will define the following variables:
#  PROJ_FOUND          - True if PROJ was found
#  PROJ_INCLUDE_DIRS   - PROJ include directories
#  PROJ_LIBRARIES      - PROJ libraries to link against
#  PROJ_VERSION        - Version of PROJ that was found
#
# The following variables can be set as arguments:
#  PROJ_ROOT_DIR       - Root directory to search for PROJ
#  PROJ_USE_STATIC_LIBS - Set to TRUE to look for static libraries

if(PROJ_FOUND)
    # PROJ is already found, no need to search again
    return()
endif()

# Save previous settings
set(_PROJ_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Handle static library preference
if(PROJ_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# Determine library architecture suffix
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_lib_suffix 64)
else()
    set(_lib_suffix "")
endif()

# Define search paths
set(_PROJ_SEARCH_PATHS
    ${PROJ_ROOT_DIR}
    $ENV{PROJ_ROOT}
    ${CMAKE_INSTALL_PREFIX}
    /usr
    /usr/local
    /opt
    /opt/local
)

# Find include directory
find_path(PROJ_INCLUDE_DIR
    NAMES proj.h
    PATH_SUFFIXES 
        include
        include/proj
        proj/include
    PATHS ${_PROJ_SEARCH_PATHS}
    DOC "PROJ include directory"
)

# Find library
find_library(PROJ_LIBRARY
    NAMES proj libproj
    PATH_SUFFIXES
        lib
        lib${_lib_suffix}
        lib/x86_64-linux-gnu
        lib/aarch64-linux-gnu
        lib/arm-linux-gnueabihf
    PATHS ${_PROJ_SEARCH_PATHS}
    DOC "PROJ library"
)

# Try to find version information
unset(PROJ_VERSION)
if(PROJ_INCLUDE_DIR AND EXISTS "${PROJ_INCLUDE_DIR}/proj.h")
    file(STRINGS "${PROJ_INCLUDE_DIR}/proj.h" _PROJ_VERSION_MAJOR_H REGEX "^#define[ \t]+PROJ_VERSION_MAJOR[ \t]+[0-9]+$")
    file(STRINGS "${PROJ_INCLUDE_DIR}/proj.h" _PROJ_VERSION_MINOR_H REGEX "^#define[ \t]+PROJ_VERSION_MINOR[ \t]+[0-9]+$")
    file(STRINGS "${PROJ_INCLUDE_DIR}/proj.h" _PROJ_VERSION_PATCH_H REGEX "^#define[ \t]+PROJ_VERSION_PATCH[ \t]+[0-9]+$")

    string(REGEX REPLACE "^#define[ \t]+PROJ_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" PROJ_VERSION_MAJOR "${_PROJ_VERSION_MAJOR_H}")
    string(REGEX REPLACE "^#define[ \t]+PROJ_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" PROJ_VERSION_MINOR "${_PROJ_VERSION_MINOR_H}")
    string(REGEX REPLACE "^#define[ \t]+PROJ_VERSION_PATCH[ \t]+([0-9]+)$" "\\1" PROJ_VERSION_PATCH "${_PROJ_VERSION_PATCH_H}")

    if(PROJ_VERSION_MAJOR AND PROJ_VERSION_MINOR AND PROJ_VERSION_PATCH)
        set(PROJ_VERSION "${PROJ_VERSION_MAJOR}.${PROJ_VERSION_MINOR}.${PROJ_VERSION_PATCH}")
    endif()
endif()

# Check for dependencies
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    list(APPEND PROJ_LIBRARY ${SQLite3_LIBRARIES})
endif()

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ
    REQUIRED_VARS 
        PROJ_LIBRARY
        PROJ_INCLUDE_DIR
    VERSION_VAR PROJ_VERSION
)

if(PROJ_FOUND)
    # Set output variables
    set(PROJ_LIBRARIES ${PROJ_LIBRARY})
    set(PROJ_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
    
    # Add any dependencies
    if(SQLite3_FOUND)
        list(APPEND PROJ_LIBRARIES ${SQLite3_LIBRARIES})
        list(APPEND PROJ_INCLUDE_DIRS ${SQLite3_INCLUDE_DIRS})
    endif()
    
    # Remove duplicate entries
    if(PROJ_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES PROJ_INCLUDE_DIRS)
    endif()
    if(PROJ_LIBRARIES)
        list(REMOVE_DUPLICATES PROJ_LIBRARIES)
    endif()

    # Set version string
    if(PROJ_VERSION)
        set(PROJ_VERSION_STRING "${PROJ_VERSION}")
    else()
        set(PROJ_VERSION_STRING "unknown")
    endif()
endif()

# Restore original find library settings
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_PROJ_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

# Mark advanced variables
mark_as_advanced(
    PROJ_INCLUDE_DIR
    PROJ_LIBRARY
)