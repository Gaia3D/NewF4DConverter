#--------------------------------------------------------------------
# GLM_INCLUDE_DIRS - include directories for GLM
# GLM_FOUND - true if GLM has been found and can be used
#--------------------------------------------------------------------

FIND_PATH(GLM_INCLUDE_DIR glm/glm.hpp)

INCLUDE(FindPackageHandleStandardArgs) 
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLM
	FOUND_VAR
		GLM_FOUND
	REQUIRED_VARS
		GLM_INCLUDE_DIR
) 

IF(GLM_FOUND) 
	SET(GLM_INCLUDE_DIRS "${GLM_INCLUDE_DIR}") 

	MESSAGE(STATUS "GLM_INCLUDE_DIRS = ${GLM_INCLUDE_DIRS}")
ENDIF() 

mark_as_advanced(GLM_INCLUDE_DIRS)