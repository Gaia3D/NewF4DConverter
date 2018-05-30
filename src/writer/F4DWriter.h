/**
 * F4DWriter Header
 */
#ifndef _F4DWRITER_H_
#define _F4DWRITER_H_
#pragma once

#include <string>
#include <map>
#include <vector>

#include "stb_image_write.h"

class ConversionProcessor;

namespace gaia3d
{
	class OctreeBox;
	class TrianglePolyhedron;
}

class F4DWriter
{
public:
	F4DWriter(ConversionProcessor* conversionResult);

	virtual ~F4DWriter();

public:
	ConversionProcessor* processor;

	std::string folder;

	std::string version;

	std::string guid;

	int guidLength;


public:
	void setWriteFolder(std::string folderPath) {folder = folderPath;}

	bool write();

	bool writeIndexFile();

protected:
	bool writeHeader(FILE* f, std::map<std::string, size_t>& textureIndices);

	bool writeVisibilityIndices(FILE* f, gaia3d::OctreeBox* octree);

	bool writeModels(FILE* f, std::vector<gaia3d::TrianglePolyhedron*>& models);

	bool writeReferencesAndModels(std::string& referencePath, std::string& modelPath, std::string& lod2Path, std::map<std::string, size_t>& textureIndices);

	bool writeLegoBlocks(std::string& legoBlockPath);

	bool writeOctreeInfo(gaia3d::OctreeBox* octree, FILE* f);

	void writeColor(unsigned long color, unsigned short type, bool bAlpha, FILE* file);

	void writeLegoTexture(std::string resultPath);

	void writeTextures(std::string imagePath);

	void writeNetSurfaceMesh(gaia3d::TrianglePolyhedron* mesh, FILE* f);

	void writeNetSurfaceTextures(std::string resultPath);
};

#endif // _F4DWRITER_H_