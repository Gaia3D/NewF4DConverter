/**
 * PreDefinition Header
 */
#ifndef _PREDEFINITION_H_
#define _PREDEFINITION_H_
#pragma once

#define WindowTitle		"F4DConverter"
#define WindowWidth		1024
#define WindowHeight	1024

#define MemoryDeviceContextEdgeLength 512

#define ExteriorDetectionThreshold 6

#define InteriorOcclusionCullingDepth ((unsigned char)2)
#define InteriorOcclusionCullingStep 1.8f
#define ExteriorOcclusionCullingDepth ((unsigned char)1)
#define ExteriorOcclusionCullingStep 20.0f

#define DisplayListIdForOcclusionCulling 1
#define OcclusionCullingScreenSize 512

#define BigObjectThreshold 5.0
#define MediumObjectThreshold 2.0
#define BoneObjectThreshold 5.0

#define SpatialIndexingDepth ((unsigned char)3)
#define LeafSpatialOctreeSize 12.0

#define MinLegoSize 3.0
#define MaxLodSize 5

#define VboVertexMaxCount 65532

#define MaxUnsignedLong 65532

#define TriangleSizeLevels 4
#define TriangleSizeThresholds {1.0, 0.5, 0.1, 0.05}

// attribute key names
#define ObjectGuid "objectGuid"
#define TextureName "textureName"
#define F4dId "id"

#endif // _PREDEFINITION_H_