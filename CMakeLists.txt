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
#IF(WIN32)
#	SET(Boost_USE_STATIC_LIBS ON)
#ENDIF()
#FIND_PACKAGE(Boost COMPONENTS locale system)
#IF(NOT Boost_FOUND)
#	#SET(Boost_USE_STATIC_LIBS OFF)
#	#SET(Boost_USE_MULTITHREADED ON)
#	#SET(Boost_USE_STATIC_RUNTIME OFF)
#ENDIF()
#MESSAGE(STATUS "Boost_INCLUDE_DIRS : ${Boost_INCLUDE_DIRS}")
#MESSAGE(STATUS "Boost_LIBRARIES : ${Boost_LIBRARIES}")
#
#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
#LINK_LIBRARIES(${Boost_LIBRARIES})

#--------------------------------------------------------------------
# Find GLFW
#--------------------------------------------------------------------
FIND_PACKAGE(GLFW)
IF(NOT GLFW_FOUND)
	MESSAGE(STATUS "GLFW not found! Using local source...")

	OPTION(GLFW_BUILD_DOCS OFF) 
	OPTION(GLFW_BUILD_EXAMPLES OFF) 
	OPTION(GLFW_BUILD_TESTS OFF) 

	ADD_SUBDIRECTORY(lib/glfw)
	SET(GLFW_INCLUDE_DIRS "${glfw_INCLUDE_DIRS}" lib/glfw/include)
    SET(GLFW_LIBRARIES "${glfw_LIBRARIES}" glfw)
ENDIF()

MESSAGE(STATUS "${GLFW_INCLUDE_DIRS}")
MESSAGE(STATUS "${GLFW_LIBRARIES}")

INCLUDE_DIRECTORIES(${GLFW_INCLUDE_DIRS})
LINK_LIBRARIES(${GLFW_LIBRARIES})

#--------------------------------------------------------------------
# Find GLM
#--------------------------------------------------------------------
FIND_PACKAGE(GLM)
IF(NOT GLM_FOUND)
	MESSAGE(STATUS "GLM not found! Using local source...")

	option(GLM_STATIC_LIBRARY_ENABLE ON)

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
LINK_LIBRARIES(${GLAD_LIBRARIES})

#--------------------------------------------------------------------
# Find Assimp
#--------------------------------------------------------------------
FIND_PACKAGE(ASSIMP)
IF(NOT ASSIMP_FOUND)
	MESSAGE(STATUS "ASSIMP not found! Using local source...")

	OPTION(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
	OPTION(ASSIMP_BUILD_SAMPLES OFF)
	OPTION(ASSIMP_BUILD_TESTS OFF)

	ADD_SUBDIRECTORY(lib/assimp)
	SET(ASSIMP_INCLUDE_DIRS "${Assimp_BINARY_DIR}/include" lib/assimp/include)
    SET(ASSIMP_LIBRARIES assimp)
ENDIF()

MESSAGE(STATUS "${ASSIMP_INCLUDE_DIRS}")
MESSAGE(STATUS "${ASSIMP_LIBRARIES}")

INCLUDE_DIRECTORIES(${ASSIMP_INCLUDE_DIRS})
LINK_LIBRARIES(${ASSIMP_LIBRARIES})

#--------------------------------------------------------------------
#
#--------------------------------------------------------------------
#ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_SHIJT)
#ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_IFC)
ADD_DEFINITIONS(-DF4D_FORMAT_SUPPORT_CLASSIC)

INCLUDE_DIRECTORIES(include)

FILE(GLOB			PROJECT_HEADERS include/*.h)
FILE(GLOB_RECURSE	PROJECT_SOURCES src/*.cpp src/*.c lib/glad/src/glad.c)
FILE(GLOB			PROJECT_CONFIGS	CMakeLists.txt)
									
FOREACH(item ${PROJECT_SOURCES})
	MESSAGE ("Now Globbing..... ${item}")
ENDFOREACH()

ADD_EXECUTABLE(${PROJECT_NAME} ${PROJECT_SOURCES})

TARGET_LINK_LIBRARIES(${PROJECT_NAME})

SET_TARGET_PROPERTIES(${PROJECT_NAME}	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}) 
	
