CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

#--------------------------------------------------------------------
# Project Information
#--------------------------------------------------------------------
PROJECT(F4DConverter)

IF(CMAKE_VERSION VERSION_LESS "3.1")
	IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
	ENDIF()
ELSE()
	SET(CMAKE_CXX_STANDARD 11)
ENDIF()

IF(WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_UNICODE /DUNICODE")
ENDIF()

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#--------------------------------------------------------------------
# Find Boost
#--------------------------------------------------------------------
#SET(Boost_DEBUG ON)
IF(WIN32)
	SET(Boost_USE_STATIC_LIBS ON CACHE INTERNAL "" FORCE)
ENDIF()
FIND_PACKAGE(Boost COMPONENTS locale system filesystem)
IF(NOT Boost_FOUND)
	#SET(Boost_USE_STATIC_LIBS OFF)
	#SET(Boost_USE_MULTITHREADED ON)
	#SET(Boost_USE_STATIC_RUNTIME OFF)
ENDIF()
MESSAGE(STATUS "Boost_INCLUDE_DIRS : ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost_LIBRARIES : ${Boost_LIBRARIES}")

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
#LINK_LIBRARIES(${Boost_LIBRARIES})

#--------------------------------------------------------------------
# Find GLFW
#--------------------------------------------------------------------
FIND_PACKAGE(GLFW)
IF(NOT GLFW_FOUND)
	MESSAGE(STATUS "GLFW not found! Using local source...")

	SET(GLFW_BUILD_DOCS OFF CACHE INTERNAL "" FORCE)
	SET(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
	SET(GLFW_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)

	ADD_SUBDIRECTORY(lib/glfw)
	SET(GLFW_INCLUDE_DIRS "${glfw_INCLUDE_DIRS}" lib/glfw/include)
    SET(GLFW_LIBRARIES "${glfw_LIBRARIES}" glfw)
ENDIF()

MESSAGE(STATUS "${GLFW_INCLUDE_DIRS}")
MESSAGE(STATUS "${GLFW_LIBRARIES}")

INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
#LINK_LIBRARIES(${GLFW_LIBRARIES})

#--------------------------------------------------------------------
# Find GLM
#--------------------------------------------------------------------
FIND_PACKAGE(GLM)
IF(NOT GLM_FOUND)
	MESSAGE(STATUS "GLM not found! Using local source...")

	SET(GLM_STATIC_LIBRARY_ENABLE ON CACHE INTERNAL "" FORCE)

	ADD_SUBDIRECTORY(lib/glm)
	SET(GLM_INCLUDE_DIRS "${GLM_INCLUDE_DIRS}" lib/glm)
ENDIF()

MESSAGE(STATUS "${GLM_INCLUDE_DIRS}")

INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIRS})

#--------------------------------------------------------------------
# Find STB
#--------------------------------------------------------------------
#FIND_PACKAGE(STB)
#IF(NOT STB_FOUND)
#	MESSAGE(STATUS "STB not found! Using local source...")
#
#	ADD_SUBDIRECTORY(lib/stb)
	SET(STB_INCLUDE_DIRS "${STB_INCLUDE_DIRS}" lib/stb)
#ENDIF()

MESSAGE(STATUS "${STB_INCLUDE_DIRS}")

INCLUDE_DIRECTORIES(${STB_INCLUDE_DIRS})

#--------------------------------------------------------------------
# Find GLAD
#--------------------------------------------------------------------
SET(GLAD_INCLUDE_DIRS lib/glad/include)
IF(NOT WIN32)
	SET(GLAD_LIBRARIES dl)
ENDIF()

INCLUDE_DIRECTORIES(${GLAD_INCLUDE_DIRS})
#LINK_LIBRARIES(${GLAD_LIBRARIES})

#--------------------------------------------------------------------
# Find XercesC
#--------------------------------------------------------------------
FIND_PACKAGE( Xerces REQUIRED )
MESSAGE(STATUS "${XERCESC_INCLUDE}")
MESSAGE(STATUS "${XERCESC_LIBRARY}")

INCLUDE_DIRECTORIES(${XERCESC_INCLUDE})
LINK_LIBRARIES(${XERCESC_LIBRARY})

#--------------------------------------------------------------------
# Find Libcitygml
#--------------------------------------------------------------------
SET(BUILD_DEBIAN_PACKAGE OFF CACHE INTERNAL "" FORCE)
SET(LIBCITYGML_USE_GDAL OFF CACHE INTERNAL "" FORCE)
SET(LIBCITYGML_DYNAMIC OFF CACHE INTERNAL "" FORCE)
SET(LIBCITYGML_STATIC_CRT OFF CACHE INTERNAL "" FORCE)
#FIND_PACKAGE(CityGML)
IF(CITYGML_FOUND)
	SET(CITYGML_INCLUDE_DIRS "${CITYGML_INCLUDE_DIR}")
    SET(CITYGML_LIBRARIES "${CITYGML_LIBRARIES}")
ELSE()
	MESSAGE(STATUS "Libcitygml not found! Using local source...")

	ADD_SUBDIRECTORY(lib/libcitygml)

	SET(CITYGML_INCLUDE_DIRS	"${libcitygml_SOURCE_DIR}/sources/include"
								"${libcitygml_BINARY_DIR}/sources/include")
    SET(CITYGML_LIBRARIES citygml)
ENDIF()
MESSAGE(STATUS "${CITYGML_INCLUDE_DIRS}")
MESSAGE(STATUS "${CITYGML_LIBRARIES}")

INCLUDE_DIRECTORIES(${CITYGML_INCLUDE_DIRS})
#LINK_LIBRARIES(${CITYGML_LIBRARIES})

#--------------------------------------------------------------------
# Find Assimp
#--------------------------------------------------------------------
FIND_PACKAGE(Assimp)
IF(NOT ASSIMP_FOUND)
	MESSAGE(STATUS "ASSIMP not found! Using local source...")

	SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "" FORCE)
	SET(ASSIMP_BUILD_SAMPLES OFF CACHE INTERNAL "" FORCE)
	SET(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
	#SET(INJECT_DEBUG_POSTFIX OFF CACHE INTERNAL "" FORCE)
	
	ADD_SUBDIRECTORY(lib/assimp)
	SET(ASSIMP_INCLUDE_DIRS		${Assimp_SOURCE_DIR}
								${Assimp_SOURCE_DIR}/include
								${Assimp_SOURCE_DIR}/code
								${Assimp_BINARY_DIR}
								${Assimp_BINARY_DIR}/include
								${Assimp_BINARY_DIR}/code)
    SET(ASSIMP_LIBRARIES assimp)
ENDIF()

MESSAGE(STATUS "${ASSIMP_INCLUDE_DIRS}")
MESSAGE(STATUS "${ASSIMP_LIBRARIES}")

INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})
#LINK_LIBRARIES(${ASSIMP_LIBRARIES})

#--------------------------------------------------------------------
# Find PROJ4
#--------------------------------------------------------------------
#FIND_PACKAGE(PROJ4)
#IF(NOT PROJ4_FOUND)
#	MESSAGE(STATUS "Proj4 not found! Using local source...")
	#ADD_DEFINITIONS(-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H)
	SET(PROJ_TESTS OFF CACHE INTERNAL "" FORCE)
	ADD_SUBDIRECTORY(lib/proj)
	SET(PROJ4_INCLUDE_DIRS "${PROJ4_SOURCE_DIR}/src" "${PROJ4_BINARY_DIR}/src")
	SET(PROJ4_LIBRARIES proj)
#ENDIF()
MESSAGE(STATUS "${PROJ4_INCLUDE_DIRS}")
MESSAGE(STATUS "${PROJ4_LIBRARIES}")

INCLUDE_DIRECTORIES(${PROJ4_INCLUDE_DIRS})
#LINK_LIBRARIES(${PROJ4_LIBRARIES})

#--------------------------------------------------------------------
# Find IfcPlusPlus
#--------------------------------------------------------------------
#FIND_PACKAGE(IfcPlusPlus)
#IF(NOT IfcPlusPlus_FOUND)
#	MESSAGE(STATUS "IfcPlusPlus not found! Using local source...")

	SET(BUILD_VIEWER_APPLICATION OFF CACHE INTERNAL "" FORCE)
	#SET(BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)

	ADD_SUBDIRECTORY(lib/ifcplusplus)
	SET(IFCPP_INCLUDE_DIRS "${IFCPP_SOURCE_DIR}/IfcPlusPlus/src/"
							"${IFCPP_SOURCE_DIR}/external/Carve/src/include"
							"${IFCPP_BINARY_DIR}/include")
    SET(IFCPP_LIBRARIES IfcPlusPlus)
#ENDIF()

MESSAGE(STATUS "${IFCPP_INCLUDE_DIRS}")
MESSAGE(STATUS "${IFCPP_LIBRARIES}")

INCLUDE_DIRECTORIES(${IFCPP_INCLUDE_DIRS})
#LINK_LIBRARIES(${IFCPP_LIBRARIES})

#--------------------------------------------------------------------
# Find Jsoncpp
#--------------------------------------------------------------------
#FIND_PACKAGE(Jsoncpp)
#IF(NOT Jsoncpp_FOUND)
#	MESSAGE(STATUS "Jsoncpp not found! Using local source...")
#
#	SET(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF)
#	SET(JSONCPP_WITH_CMAKE_PACKAGE OFF)
#
#	ADD_SUBDIRECTORY(lib/jsoncpp)
#	SET(JSONCPP_INCLUDE_DIRS "${JSONCPP_INCLUDE_DIRS}")
#    SET(JSONCPP_LIBRARIES jsoncpp${STATIC_SUFFIX})
#ENDIF()
#
#MESSAGE(STATUS "${JSONCPP_INCLUDE_DIRS}")
#MESSAGE(STATUS "${JSONCPP_LIBRARIES}")
#
#INCLUDE_DIRECTORIES(${JSONCPP_INCLUDE_DIRS})
#LINK_LIBRARIES(${JSONCPP_LIBRARIES})

#--------------------------------------------------------------------
# Build Jsoncpp as an external project.
#--------------------------------------------------------------------
SET(JSONCPP_INSTALL_DIR ${CMAKE_BINARY_DIR}/lib/jsoncpp)
SET(JSONCPP_INCLUDE_DIR ${JSONCPP_INSTALL_DIR}/include)
SET(JSONCPP_LIBRARY_DIR ${JSONCPP_INSTALL_DIR}/lib)
include(ExternalProject)
ExternalProject_Add(jsoncpp_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/lib/jsoncpp
                    PREFIX      ${JSONCPP_INSTALL_DIR}
                    INSTALL_DIR ${JSONCPP_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
								-DCMAKE_INSTALL_PREFIX:PATH=${JSONCPP_INSTALL_DIR}
								-DCMAKE_INSTALL_LIBDIR=${JSONCPP_LIBRARY_DIR}
                   )

SET(JSONCPP_INCLUDE_DIRS "${JSONCPP_INCLUDE_DIR}")
SET(JSONCPP_LIBRARIES jsoncpp${STATIC_SUFFIX})

MESSAGE(STATUS "${JSONCPP_INCLUDE_DIRS}")
MESSAGE(STATUS "${JSONCPP_LIBRARIES}")

LINK_DIRECTORIES(${JSONCPP_LIBRARY_DIR})
INCLUDE_DIRECTORIES(BEFORE SYSTEM ${JSONCPP_INCLUDE_DIRS})
LINK_LIBRARIES(${JSONCPP_LIBRARIES})

# A target to combine all of the external projects.
ADD_CUSTOM_TARGET(build_external_projects
                  DEPENDS jsoncpp_external_project)

#--------------------------------------------------------------------
#
#--------------------------------------------------------------------
#ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_SHIJT)
ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_IFC)
ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_CLASSIC)
ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_CITYGML)
ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_INDOORGML)
INCLUDE_DIRECTORIES(include)

FILE(GLOB			PROJECT_HEADERS include/*.h)
FILE(GLOB_RECURSE	PROJECT_SOURCES src/*.cpp src/*.c lib/glad/src/glad.c)
FILE(GLOB			PROJECT_CONFIGS	CMakeLists.txt)
									
FOREACH(item ${PROJECT_SOURCES})
	MESSAGE ("Now Globbing..... ${item}")
ENDFOREACH()

ADD_EXECUTABLE(${PROJECT_NAME} ${PROJECT_SOURCES})
ADD_DEPENDENCIES(${PROJECT_NAME} build_external_projects)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Boost_LIBRARIES}
										${GLFW_LIBRARIES}
										${GLAD_LIBRARIES}
										${CITYGML_LIBRARIES}
										${ASSIMP_LIBRARIES}
										${IFCPP_LIBRARIES}
										${PROJ4_LIBRARIES}
										${XercesC_LIBRARY})

SET_TARGET_PROPERTIES(${PROJECT_NAME}	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}) 

SET ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build )
	
INSTALL(
		TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION bin
		ARCHIVE DESTINATION lib
)