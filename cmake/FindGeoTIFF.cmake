# FindGeoTIFF.cmake
#
# Finds the GeoTIFF library and its dependencies
#
# This module will define the following variables:
#  GeoTIFF_FOUND        - True if GeoTIFF was found
#  GeoTIFF_INCLUDE_DIRS - GeoTIFF include directories
#  GeoTIFF_LIBRARIES    - GeoTIFF libraries to link against
#  GeoTIFF_VERSION      - Version of GeoTIFF that was found
#
# The following variables can be set as arguments:
#  GeoTIFF_ROOT_DIR     - Root directory to search for GeoTIFF
#  GeoTIFF_USE_STATIC_LIBS - Set to TRUE to look for static libraries

# Save previous settings
set(_GeoTIFF_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Handle static library preference
if(GeoTIFF_USE_STATIC_LIBS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# Determine library architecture suffix
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_lib_suffix 64)
else()
    set(_lib_suffix "")
endif()

# Define search paths
set(_GeoTIFF_SEARCH_PATHS
    ${GeoTIFF_ROOT_DIR}
    $ENV{GeoTIFF_ROOT}
    ${CMAKE_INSTALL_PREFIX}
    /usr
    /usr/local
    /opt
    /opt/local
)

# Find include directory
find_path(GeoTIFF_INCLUDE_DIR
    NAMES geotiff.h
    PATH_SUFFIXES 
        include
        include/geotiff
        geotiff
        libgeotiff
    PATHS ${_GeoTIFF_SEARCH_PATHS}
    DOC "GeoTIFF include directory"
)

# Find library
find_library(GeoTIFF_LIBRARY
    NAMES geotiff libgeotiff
    PATH_SUFFIXES
        lib
        lib${_lib_suffix}
        lib/x86_64-linux-gnu
        lib/aarch64-linux-gnu
        lib/arm-linux-gnueabihf
    PATHS ${_GeoTIFF_SEARCH_PATHS}
    DOC "GeoTIFF library"
)

# Try to find version information
if(GeoTIFF_INCLUDE_DIR AND EXISTS "${GeoTIFF_INCLUDE_DIR}/geotiff.h")
    file(STRINGS "${GeoTIFF_INCLUDE_DIR}/geotiff.h" _GeoTIFF_VERSION_H REGEX "^#define[ \t]+LIBGEOTIFF_VERSION[ \t]+\"[^\"]*\"")
    if(_GeoTIFF_VERSION_H)
        string(REGEX REPLACE "^#define[ \t]+LIBGEOTIFF_VERSION[ \t]+\"([0-9.]+).*\".*" "\\1" GeoTIFF_VERSION "${_GeoTIFF_VERSION_H}")
    endif()
endif()

# Check for dependencies
find_package(TIFF QUIET)
if(TIFF_FOUND)
    list(APPEND GeoTIFF_LIBRARIES ${TIFF_LIBRARIES})
    list(APPEND GeoTIFF_INCLUDE_DIRS ${TIFF_INCLUDE_DIRS})
endif()

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GeoTIFF
    REQUIRED_VARS 
        GeoTIFF_LIBRARY
        GeoTIFF_INCLUDE_DIR
    VERSION_VAR GeoTIFF_VERSION
)

if(GeoTIFF_FOUND)
    # Set output variables
    set(GeoTIFF_LIBRARIES ${GeoTIFF_LIBRARY})
    set(GeoTIFF_INCLUDE_DIRS ${GeoTIFF_INCLUDE_DIR})
    
    # Add any dependencies
    if(TIFF_FOUND)
        list(APPEND GeoTIFF_LIBRARIES ${TIFF_LIBRARIES})
        list(APPEND GeoTIFF_INCLUDE_DIRS ${TIFF_INCLUDE_DIRS})
    endif()
    
    # Remove duplicate entries
    if(GeoTIFF_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES GeoTIFF_INCLUDE_DIRS)
    endif()
    if(GeoTIFF_LIBRARIES)
        list(REMOVE_DUPLICATES GeoTIFF_LIBRARIES)
    endif()
endif()

# Restore original find library settings
set(CMAKE_FIND_LIBRARY_SUFFIXES ${_GeoTIFF_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES})

# Mark advanced variables
mark_as_advanced(
    GeoTIFF_INCLUDE_DIR
    GeoTIFF_LIBRARY
)