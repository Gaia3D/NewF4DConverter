#--------------------------------------------------------------------
# ASSIMP_INCLUDE_DIRS - include directories for Assimp
# ASSIMP_LIBRARIES - libraries to link against Assimp
# ASSIMP_FOUND - true if Assimp has been found and can be used
#--------------------------------------------------------------------

IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
	SET(ASSIMP_ARCHITECTURE "64")
ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
	SET(ASSIMP_ARCHITECTURE "32")
ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)

IF(WIN32)
	SET(ASSIMP_ROOT_DIR CACHE PATH "ASSIMP root directory")

	# Find include files
	FIND_PATH(ASSIMP_INCLUDE_DIR
		NAMES
			assimp/scene.h
		HINTS
			$ENV{PROGRAMFILES}/include
			${ASSIMP_ROOT_DIR}/include
		DOC "The directory where assimp/scene.h resides"
	)

	# Find library files
	FIND_LIBRARY(ASSIMP_LIBRARY
		NAMES
			assimp
		PATHS
			$ENV{PROGRAMFILES}/lib
			${ASSIMP_ROOT_DIR}/lib
	)

	# IF(MSVC12)
	# 	SET(ASSIMP_MSVC_VERSION "vc120")
	# ELSEIF(MSVC14)	
	# 	SET(ASSIMP_MSVC_VERSION "vc140")
	# ENDIF(MSVC12)
	
	# IF(MSVC12 OR MSVC14)
	# 	FIND_PATH(ASSIMP_LIBRARY_DIR
	# 		NAMES
	# 			assimp-${ASSIMP_MSVC_VERSION}-mt.lib
	# 		HINTS
	# 			${ASSIMP_ROOT_DIR}/lib${ASSIMP_ARCHITECTURE}
	# 	)
		
	# 	FIND_LIBRARY(ASSIMP_LIBRARY_RELEASE
	# 		NAMES
	# 			assimp-${ASSIMP_MSVC_VERSION}-mt.lib
	# 		PATHS
	# 			${ASSIMP_LIBRARY_DIR}
	# 	)
	# 	FIND_LIBRARY(ASSIMP_LIBRARY_DEBUG
	# 		NAMES
	# 			assimp-${ASSIMP_MSVC_VERSION}-mtd.lib
	# 		PATHS
	# 			${ASSIMP_LIBRARY_DIR}
	# 	)
		
	# 	SET(ASSIMP_LIBRARY 
	# 		optimized 	${ASSIMP_LIBRARY_RELEASE}
	# 		debug		${ASSIMP_LIBRARY_DEBUG}
	# 	)
		
	# 	SET(ASSIMP_LIBRARIES "ASSIMP_LIBRARY_RELEASE" "ASSIMP_LIBRARY_DEBUG")
	
	# 	FUNCTION(ASSIMP_COPY_BINARIES TargetDirectory)
	# 		ADD_CUSTOM_TARGET(AssimpCopyBinaries
	# 			COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE}/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll ${TargetDirectory}/Debug/assimp-${ASSIMP_MSVC_VERSION}-mtd.dll
	# 			COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_ROOT_DIR}/bin${ASSIMP_ARCHITECTURE}/assimp-${ASSIMP_MSVC_VERSION}-mt.dll ${TargetDirectory}/Release/assimp-${ASSIMP_MSVC_VERSION}-mt.dll
	# 		COMMENT "Copying Assimp binaries to '${TargetDirectory}'"
	# 		VERBATIM)
	# 	ENDFUNCTION(ASSIMP_COPY_BINARIES)
	# ENDIF()
ELSE()
	# Find include files
	FIND_PATH(ASSIMP_INCLUDE_DIR
		NAMES
			assimp/scene.h
		PATHS
			/usr/include
			/usr/local/include
			/sw/include
			/opt/local/include
		DOC "The directory where assimp/scene.h resides"
	)

	# Find library files
	FIND_LIBRARY(ASSIMP_LIBRARY
		NAMES
			assimp
		PATHS
			/usr/lib64
			/usr/lib
			/usr/local/lib64
			/usr/local/lib
			/sw/lib
			/opt/local/lib
			${ASSIMP_ROOT_DIR}/lib
		DOC "The Assimp library"
	)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs) 
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Assimp
	FOUND_VAR
		ASSIMP_FOUND
	REQUIRED_VARS
		ASSIMP_INCLUDE_DIR
		ASSIMP_LIBRARY
) 

IF(ASSIMP_FOUND) 
	SET(ASSIMP_INCLUDE_DIRS "${ASSIMP_INCLUDE_DIR}") 
	SET(ASSIMP_LIBRARIES "${ASSIMP_LIBRARY}")

	MESSAGE(STATUS "ASSIMP_INCLUDE_DIRS = ${ASSIMP_INCLUDE_DIRS}")
	MESSAGE(STATUS "ASSIMP_LIBRARIES = ${ASSIMP_LIBRARIES}")
ENDIF() 

MARK_AS_ADVANCED(ASSIMP_INCLUDE_DIRS ASSIMP_LIBRARIES)