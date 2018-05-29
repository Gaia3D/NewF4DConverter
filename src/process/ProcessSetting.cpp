/**
* Implementation of the NetSurfaceMeshSetting class
*/
#include "ProcessSetting.h"

ProcessSetting::ProcessSetting()
{
	bExtractExterior = true; // if extract exteriors or not
	bOcclusionCulling = false; // if do visibility indexing or not
	leafSpatialOctreeSize = 24.0f; // deepest spatial octree edge length
	bFlipTextureCoordinateU = false; // if flip texture coordinate u or not
	interiorVisibilityIndexingCameraStep = 2.5f; // camera position step for interior visibility indexing
	exteriorVisibilityIndexingCameraStep = 40.0f; // camera position step for exterior visibility indexing
	interiorVisibilityIndexingOctreeDepth = 3; // visibility octree depth for interior
	exteriorVisibilityIndexingOctreeDepth = 1; // visibility octree depth for exterior
}

ProcessSetting::~ProcessSetting()
{

}
