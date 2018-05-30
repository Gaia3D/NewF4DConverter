﻿/**
 * Conversion Processor Header
 */
#ifndef _CONVERSIONPROCESSOR_H_
#define _CONVERSIONPROCESSOR_H_
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

#include <vector>
#include <map>

#include "../geometry/TrianglePolyhedron.h"
#include "../geometry/BoundingBox.h"
#include "../geometry/ColorU4.h"
#include "../geometry/OctreeBox.h"
#include "ProcessSetting.h"


class SceneControlVariables;

class ConversionProcessor
{
public:
	ConversionProcessor();

	virtual ~ConversionProcessor();

	void setNsmSettingIndex(unsigned char index) { settings.netSurfaceMeshSettingIndex = index; }
	void setExteriorExtraction(bool bDo) { settings.bExtractExterior = bDo; }
	void setVisibilityIndexing(bool bDo) { settings.bOcclusionCulling = bDo; }
	void setSpatialOctreeSize(float fSize) { settings.leafSpatialOctreeSize = fSize; }
	void setTextureCoordinateUFlip(bool bDo) { settings.bFlipTextureCoordinateU = bDo; }
	void setInteriorVisibilityIndexingCameraStep(float step) { settings.interiorVisibilityIndexingCameraStep = step; }
	void setExteriorVisibilityIndexingCameraStep(float step) { settings.exteriorVisibilityIndexingCameraStep = step; }
	void setInteriorVisibilityIndexingOctreeDepth(unsigned char depth) { settings.interiorVisibilityIndexingOctreeDepth = depth; }
	void setExteriorVisibilityIndexingOctreeDepth(unsigned char depth) { settings.exteriorVisibilityIndexingOctreeDepth = depth; }
	void clearNsmSettings() { settings.clearNsmSettings(); }
	void setSkinLevel(unsigned char level) { settings.netSurfaceMeshSettingIndex = level; }

protected:
	ProcessSetting settings;

	SceneControlVariables* scv;

	std::vector<gaia3d::TrianglePolyhedron*> allMeshes;

	std::map<std::string, std::string> allTextureInfo;
	std::map<std::string, unsigned char*> resizedTextures;
	std::map<std::string, unsigned int> allTextureWidths;
	std::map<std::string, unsigned int> allTextureHeights;

	gaia3d::BoundingBox fullBbox;

	gaia3d::SpatialOctreeBox thisSpatialOctree;

	std::map<size_t, gaia3d::TrianglePolyhedron*> legos;

	std::map<unsigned char, gaia3d::TrianglePolyhedron*> netSurfaceMeshes;
	std::map<unsigned char, unsigned char*> netSurfaceTextures;
	std::map<unsigned char, unsigned int> netSurfaceTextureWidth;
	std::map<unsigned char, unsigned int> netSurfaceTextureHeight;

	unsigned char* legoTextureBitmap;
	unsigned int legoTextureDimension[2];
	bool textureFlip[2];

	gaia3d::Matrix4 bboxCenterToLocalOrigin;

	std::map<std::string, std::string> attributes;

	double longitude, latitude;
	float altitude;
	
public:
	bool initialize();

	void uninitialize();

	void clear();

	void defaultSpaceSetupForVisualization(int width, int height);

	bool proceedConversion(std::vector<gaia3d::TrianglePolyhedron*>& originalMeshes,
							std::map<std::string, std::string>& originalTextureInfo,
							bool bExtractExterior = false,
							bool bOcclusionCulling = false);

	gaia3d::SpatialOctreeBox* getSpatialOctree() {return &thisSpatialOctree;}

	std::map<std::string, std::string>& getTextureInfo() { return allTextureInfo; }

	std::map<size_t, gaia3d::TrianglePolyhedron*>& getLegos() {return legos;}

	void getMatrixForBboxCenterToLocalOrigin(gaia3d::Matrix4* matrix) {matrix->set(&bboxCenterToLocalOrigin);}

	void setMatrixForBboxCenterToLocalOrigin(gaia3d::Matrix4* matrix) {bboxCenterToLocalOrigin.set(matrix);}

	void changeSceneControlVariables();

	SceneControlVariables* getSceneControlVariables() {return scv;}

	void addAttribute(std::string key, std::string value);

	std::map<std::string, std::string>& getAttributes() {return attributes;}

	std::string getAttribute(std::string key) {return attributes[key];}

	bool doesAttributeExist(std::string key) { return attributes.find(key) != attributes.end(); }
	
	double getLongitude() {return longitude;}
	void setLongitude(double lon) {longitude = lon;}
	double getLatitude() {return latitude;}
	void setLatitude(double lat) {latitude = lat;}
	float getAltitude() {return altitude;}
	void setAltitude(float alt) {altitude = alt;}
	gaia3d::BoundingBox& getBoundingBox() {return fullBbox;}
	std::vector<gaia3d::TrianglePolyhedron*>& getAllMeshes() {return allMeshes;}

	unsigned char* getLegoTextureBitmapArray() { return legoTextureBitmap; }
	unsigned int* getLegoTextureDimension() { return legoTextureDimension; }
	std::map<std::string, unsigned char*>& getResizedTextures() { return resizedTextures; }
	std::map<std::string, unsigned int>& getAllTextureWidths() { return allTextureWidths; }
	std::map<std::string, unsigned int>& getAllTextureHeights() { return allTextureHeights; }
	std::map<unsigned char, gaia3d::TrianglePolyhedron*>& getNetSurfaceMeshes() { return netSurfaceMeshes; }
	std::map<unsigned char, unsigned char*>&  getNetSurfaceTextures() { return netSurfaceTextures; }
	std::map<unsigned char, unsigned int>& getNetSurfaceTextureWidth() { return netSurfaceTextureWidth; }
	std::map<unsigned char, unsigned int>& getNetSurfaceTextureHeight() { return netSurfaceTextureHeight; }

	bool isTextureFlipX() { return textureFlip[0]; }
	bool isTextureFlipY() { return textureFlip[1]; }
protected:
	// main processing steps - start
	void trimVertexNormals(std::vector<gaia3d::TrianglePolyhedron*>& meshes);

	void calculateBoundingBox(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox);

	void makeVboObjects(std::vector<gaia3d::TrianglePolyhedron*>& meshes, bool bBind = false);

	void determineWhichSurfacesAreExterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& bbox);

	void determineModelAndReference(std::vector<gaia3d::TrianglePolyhedron*>& meshes);

	void assignReferencesIntoExteriorAndInterior(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
												std::vector<gaia3d::TrianglePolyhedron*>& interiors,
												std::vector<gaia3d::TrianglePolyhedron*>& exteriors);

	void assignReferencesIntoEachSpatialOctrees(gaia3d::SpatialOctreeBox& spatialOctree,
												std::vector<gaia3d::TrianglePolyhedron*>& meshes,
												gaia3d::BoundingBox& bbox,
												bool bFixedDepth = true,
												double leafBoxSize = 0.0,
												bool bRefOnOnlyOneLeaf = false);

	void makeOcclusionInformation(std::vector<gaia3d::TrianglePolyhedron*>& meshes,
									gaia3d::VisionOctreeBox& interiorOcclusionOctree,
									gaia3d::VisionOctreeBox& exteriorOcclusionOctree,
									gaia3d::BoundingBox& interiorBbox,
									gaia3d::BoundingBox& exteriorBbox);

	void applyOcclusionInformationOnSpatialOctree(gaia3d::SpatialOctreeBox& spatialOctree,
													gaia3d::VisionOctreeBox& interiorOcclusionOctree,
													gaia3d::VisionOctreeBox& exteriorOcclusionOctree);

	void makeLegoStructure(gaia3d::SpatialOctreeBox& octree, double minLegoSize, std::map<size_t, gaia3d::TrianglePolyhedron*>& result, gaia3d::BoundingBox& textureBbox, bool bMakeTexutreCoordinate);

	void makeLegoTexture(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::string, std::string>& textureInfo);

	void normalizeTextures(std::map<std::string, std::string>& textureInfo);
	// main processing steps - end

	void calculateBoundingBox(gaia3d::TrianglePolyhedron* mesh);

	void setupPerspectiveViewSetting(gaia3d::BoundingBox& bbox);

	void checkIfEachSurfaceIsExterior(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors);

	void drawSurfacesWithIndexColor(std::vector<gaia3d::Surface*>& surfaces, std::vector<gaia3d::ColorU4>& colors);

	void makeDisplayListOfMeshes(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::vector<gaia3d::ColorU4>& colors);

	void renderMesh(gaia3d::TrianglePolyhedron* mesh, bool bNormal, bool bTextureCoordinate);

	void makeVisibilityIndices(gaia3d::VisionOctreeBox& octree, std::vector<gaia3d::TrianglePolyhedron*>& meshes,
								float scanStepX, float scanStepY, float scanStepZ,
								gaia3d::VisionOctreeBox* excludedBox);

	void drawAndDetectVisibleColorIndices(std::map<size_t, size_t>& container);

	void extractMatchedReferencesFromOcclusionInfo(gaia3d::VisionOctreeBox* receiver,
													gaia3d::VisionOctreeBox& info,
													std::vector<gaia3d::TrianglePolyhedron*>& meshesToBeCompared);

	gaia3d::TrianglePolyhedron* makeLegoStructure(std::vector<gaia3d::TrianglePolyhedron*>& meshes, gaia3d::BoundingBox& seedBbox, gaia3d::BoundingBox& textureBbox, double minLegoSize, bool bMakeTexutreCoordinate);

	void sortTrianglesBySize(std::vector<gaia3d::Triangle*>& inputTriangles,
							unsigned char sizeLevels,
							double* sizeArray,
							std::vector<gaia3d::Triangle*>& outputTriangles,
							unsigned int* sizeIndexMarkers);

	void loadAndBindTextures(std::map<std::string, std::string>& textureInfo, std::map<std::string, unsigned int>& bindingResult);

	void extractLegoTextures(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::string, unsigned int>& bindingResult, unsigned int shaderProgram);

	void drawMeshesWithTextures(std::vector<gaia3d::TrianglePolyhedron*>& meshes, std::map<std::string, unsigned int>& bindingResult, unsigned int shaderProgram);

	void unbindTextures(std::map<std::string, unsigned int>& bindingResult);

	unsigned int makeShaders();

	unsigned int makeShadersForNSM();

	void deleteShaders(unsigned int programId);
	/*
	void makeNetSurfaceMeshes(gaia3d::SpatialOctreeBox& octrees,
							std::map<std::string, unsigned char*>& textures,
							std::map<std::string, unsigned int>& textureWidths,
							std::map<std::string, unsigned int>& textureHeights);
	*/
	void makeNetSurfaceMeshes(gaia3d::SpatialOctreeBox& octrees, std::map<std::string, std::string>& textureInfo);

	void normalizeMosiacTextures(std::map<unsigned char, unsigned char*>& mosaicTextures,
								std::map<unsigned char, unsigned int>& mosaicTextureWidth,
								std::map<unsigned char, unsigned int>& mosaicTextureHeight);
};

#endif // _CONVERSIONPROCESSOR_H_