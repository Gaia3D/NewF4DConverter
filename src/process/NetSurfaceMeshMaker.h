/**
* NetSurfaceMeshMaker Header
*/
#ifndef _NETSURFACEMESHMAKER_H_
#define _NETSURFACEMESHMAKER_H_
#pragma once

#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
#error windows.h was included!
#endif

#include <GLFW/glfw3.h>

#if USE_NATIVE_OSMESA
#define GLFW_EXPOSE_NATIVE_OSMESA
#include <GLFW/glfw3native.h>
#endif

#include <map>
#include <vector>

#include "NetSurfaceMeshSetting.h"
#include "SceneControlVariables.h"
#include "../util/GeometryUtility.h"
#include "../geometry/OctreeBox.h"
#include "../geometry/TrianglePolyhedron.h"

/*
namespace gaia3d
{
	class TrianglePolyhedron;
	class OctreeBox;
}

class NetSurfaceMeshSetting;
class SceneControlVariables;
*/
class NetSurfaceMeshMaker
{
public:
	NetSurfaceMeshMaker();
	~NetSurfaceMeshMaker();

public:
	void makeNetSurfaceMesh(std::vector<gaia3d::OctreeBox*>& octrees,
							NetSurfaceMeshSetting* setting,
							SceneControlVariables* scv,
							unsigned int shaderProgramDepthDetection,
							unsigned int shaderProgramTexture,
							std::map<std::string, unsigned int>& bindingResult,
							std::map<unsigned char, gaia3d::TrianglePolyhedron*>& netSurfaceMeshes,
							std::map<unsigned char, unsigned char*>& netSurfaceTextures,
							std::map<unsigned char, unsigned int>& netSurfaceTextureWidth,
							std::map<unsigned char, unsigned int>& netSurfaceTextureHeight);

};

#endif // _NETSURFACEMESHMAKER_H_