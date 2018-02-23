#--------------------------------------------------------------------
# GLFW_INCLUDE_DIRS - include directories for GLFW
# GLFW_LIBRARIES - libraries to link against GLFW
# GLFW_FOUND - true if GLFW has been found and can be used
#--------------------------------------------------------------------

FIND_PATH(GLFW_INCLUDE_DIR GLFW/glfw3.h)
FIND_LIBRARY(GLFW_LIBRARY glfw glfw3)

INCLUDE(FindPackageHandleStandardArgs) 
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLFW
	FOUND_VAR
		GLFW_FOUND
	REQUIRED_VARS
		GLFW_INCLUDE_DIR
		GLFW_LIBRARY
) 

IF(GLFW_FOUND) 
	SET(GLFW_INCLUDE_DIRS "${GLFW_INCLUDE_DIR}") 
	SET(GLFW_LIBRARIES "${GLFW_LIBRARY}")

	MESSAGE(STATUS "GLFW_INCLUDE_DIRS = ${GLFW_INCLUDE_DIRS}")
	MESSAGE(STATUS "GLFW_LIBRARIES = ${GLFW_LIBRARIES}")
ENDIF() 

mark_as_advanced(GLFW_INCLUDE_DIRS GLFW_LIBRARIES)