CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

#--------------------------------------------------------------------
# Project Information
#--------------------------------------------------------------------
PROJECT(F4DConverter)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
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
# Find GLAD
#--------------------------------------------------------------------
SET(GLAD_INCLUDE_DIRS lib/glad/include)
IF(NOT WIN32)
	SET(GLAD_LIBRARIES dl)
ENDIF()

INCLUDE_DIRECTORIES(${GLAD_INCLUDE_DIRS})
LINK_LIBRARIES(${GLAD_LIBRARIES})

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
	
