#--------------------------------------------------------------------
# JSONCPP_INCLUDE_DIRS - include directories for Jsoncpp
# JSONCPP_LIBRARIES - libraries to link against Jsoncpp
# JSONCPP_FOUND - true if Jsoncpp has been found and can be used
#--------------------------------------------------------------------


# Find include files
FIND_PATH(JSONCPP_INCLUDE_DIR
	NAMES
		json/json.h jsoncpp/json/json.h
	HINTS
		$ENV{PROGRAMFILES}/include
	DOC "The directory where json/json.h resides"
)

# Find library files
FIND_LIBRARY(JSONCPP_LIBRARY
	NAMES
		jsoncpp
	PATHS
		$ENV{PROGRAMFILES}/lib
)

FIND_LIBRARY(JSONCPP_LIBRARY_DEBUG
	NAMES
		jsoncppd
	PATHS
		$ENV{PROGRAMFILES}/lib
)

INCLUDE(FindPackageHandleStandardArgs) 
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Jsoncpp
	FOUND_VAR
		JSONCPP_FOUND
	REQUIRED_VARS
		JSONCPP_INCLUDE_DIR
		JSONCPP_LIBRARY
) 

IF(JSONCPP_FOUND) 
	SET(JSONCPP_INCLUDE_DIRS "${JSONCPP_INCLUDE_DIR}") 
	SET(JSONCPP_LIBRARIES "${JSONCPP_LIBRARY}")

	MESSAGE(STATUS "JSONCPP_INCLUDE_DIRS = ${JSONCPP_INCLUDE_DIRS}")
	MESSAGE(STATUS "JSONCPP_LIBRARIES = ${JSONCPP_LIBRARIES}")
ENDIF() 

MARK_AS_ADVANCED(JSONCPP_INCLUDE_DIRS JSONCPP_LIBRARIES)
