/**
* ProcessSetting Header
*/
#ifndef _PROCESSSETTING_H_
#define _PROCESSSETTING_H_
#pragma once

#include <map>

class ProcessSetting
{
public:
	ProcessSetting();
	~ProcessSetting();

public:
	bool bExtractExterior; // if extract exteriors or not
	bool bOcclusionCulling; // if do visibility indexing or not
	float leafSpatialOctreeSize; // deepest spatial octree edge length(meter)
	bool bFlipTextureCoordinateU; // if flip texture coordinate u or not
	float interiorVisibilityIndexingCameraStep; // camera position step for interior visibility indexing(meter)
	float exteriorVisibilityIndexingCameraStep; // camera position step for exterior visibility indexing(meter)
	unsigned char interiorVisibilityIndexingOctreeDepth; // visibility octree depth for interior
	unsigned char exteriorVisibilityIndexingOctreeDepth; // visibility octree depth for exterior
};

#endif // _PROCESSSETTING_H_