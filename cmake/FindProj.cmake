#--------------------------------------------------------------------
# PROJ4_FOUND       = if the library found
# PROJ4_LIBRARY     = full path to the library
# PROJ4_INCLUDE_DIR = where to find the library headers also defined, but not for general use are
# PROJ4_LIBRARY, where to find the PROJ.4 library.
#--------------------------------------------------------------------

# Try to use OSGeo4W installation
IF(WIN32)
  IF(DEFINED ENV{OSGEO4W_ROOT})
    SET(OSGEO4W_ROOT_DIR $ENV{OSGEO4W_ROOT})
    MESSAGE(STATUS " FindProj: trying OSGeo4W using environment variable OSGEO4W_ROOT=$ENV{OSGEO4W_ROOT}")
  ELSE()
    SET(OSGEO4W_ROOT_DIR c:/OSGeo4W)
    MESSAGE(STATUS " FindProj: trying OSGeo4W using default location OSGEO4W_ROOT=${OSGEO4W_ROOT_DIR}")
  ENDIF()
ENDIF()

FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
    PATHS ${OSGEO4W_ROOT_DIR}/include
    DOC "Path to PROJ.4 library include directory")

SET(PROJ4_NAMES ${PROJ4_NAMES} proj proj_i)
FIND_LIBRARY(PROJ4_LIBRARY
    NAMES ${PROJ4_NAMES}
    PATHS ${OSGEO4W_ROOT_DIR}/lib
    DOC "Path to PROJ.4 library file")

# Handle the QUIETLY and REQUIRED arguments and set SPATIALINDEX_FOUND to TRUE
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PROJ4 DEFAULT_MSG PROJ4_LIBRARY PROJ4_INCLUDE_DIR)

IF(PROJ4_FOUND)
  SET(PROJ4_LIBRARIES ${PROJ4_LIBRARY})
ENDIF()