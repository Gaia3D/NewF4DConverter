﻿/**
* ProcessSetting Header
*/
#ifndef _PROCESSSETTING_H_
#define _PROCESSSETTING_H_
#pragma once

#include <map>

#include "NetSurfaceMeshSetting.h"

//class NetSurfaceMeshSetting;

class ProcessSetting
{
public:
	ProcessSetting();
	~ProcessSetting();

public:
	unsigned char netSurfaceMeshSettingIndex; // index to template NSM setting

	bool bExtractExterior; // if extract exteriors or not
	bool bOcclusionCulling; // if do visibility indexing or not
	float leafSpatialOctreeSize; // deepest spatial octree edge length(meter)
	bool bFlipTextureCoordinateU; // if flip texture coordinate u or not
	float interiorVisibilityIndexingCameraStep; // camera position step for interior visibility indexing(meter)
	float exteriorVisibilityIndexingCameraStep; // camera position step for exterior visibility indexing(meter)
	unsigned char interiorVisibilityIndexingOctreeDepth; // visibility octree depth for interior
	unsigned char exteriorVisibilityIndexingOctreeDepth; // visibility octree depth for exterior

	std::map<unsigned char, NetSurfaceMeshSetting*> nsmSettings; // net surface mesh setting for each lod

	void fillNsmSettings(unsigned char settingIndex);
	void clearNsmSettings();
};

#endif // _PROCESSSETTING_H_