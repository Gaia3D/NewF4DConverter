/**
 * Implementation of the F4DWriter class
 */
#include "F4DWriter.h"

#include <iostream>

#ifdef __APPLE__
	#include <sys/uio.h>
#elif defined WIN32
	#include <direct.h>
	#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
	#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
	#define mkdir(dirname,mode)   _mkdir(dirname)
#else
	#include <sys/io.h>
#endif

#include <sys/stat.h>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/filesystem/detail/utf8_codecvt_facet.hpp>

#include "../process/ConversionProcessor.h"
#include "../converter/LogWriter.h"
#include "../geometry/ColorU4.h"
#include "../util/StringUtility.h"


F4DWriter::F4DWriter(ConversionProcessor* conversionResult)
:processor(conversionResult)
{
	version = "v.0.0";
	guid = "abcdefghi";
	guidLength = 9;
}

F4DWriter::~F4DWriter()
{}

bool F4DWriter::write()
{
	// make target root folder
	std::string resultPath = folder + "/F4D_" + processor->getAttribute("id");

	struct stat status;

	bool outputFolderExist = false;
	if (stat(resultPath.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (mkdir(resultPath.c_str(), 0755) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	std::string headerPath = resultPath + "/HeaderAsimetric.hed";
	FILE* file = fopen(headerPath.c_str(), "wb");
	// write header
	writeHeader(file);
	fclose(file);

	// create reference directory
	outputFolderExist = false;
	std::string referencePath = resultPath + "/References";
	if (stat(referencePath.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (mkdir(referencePath.c_str(), 0755) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	// create lego block directory
	outputFolderExist = false;
	std::string legoBlockPath = resultPath + "/Bricks";
	if (stat(legoBlockPath.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (mkdir(legoBlockPath.c_str(), 0755) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	// create model directory
	outputFolderExist = false;
	std::string modelPath = resultPath + "/Models";
	if (stat(modelPath.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}
	if (!outputFolderExist)
	{
		if (mkdir(modelPath.c_str(), 0755) != 0)
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
			return false;
		}
	}

	writeReferencesAndModels(referencePath, modelPath);
	writeLegoBlocks(legoBlockPath);

	// create image directory
	if (!processor->getTextureInfo().empty())
	{
		outputFolderExist = false;
		std::string imagePath = resultPath + "/Images_Resized";
		if (stat(imagePath.c_str(), &status) == 0)
		{
			if (S_ISDIR(status.st_mode))
				outputFolderExist = true;
		}
		if (!outputFolderExist)
		{
			if (mkdir(imagePath.c_str(), 0755) != 0)
			{
				LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
				return false;
			}
		}

		writeTextures(imagePath);
	}
	writeLegoTexture(resultPath);

	return true;
}

bool F4DWriter::writeHeader(FILE* f)
{
	// versoin div : start(20170323)
	// version
	fwrite(version.c_str(), sizeof(char), 5, f);
	// guid length
	fwrite(&guidLength, sizeof(int), 1, f);
	// guid
	fwrite(guid.c_str(), sizeof(char), guidLength, f);
	// representative longitude, latitude, altitude
	double longitude = processor->getLongitude(), latitude = processor->getLatitude();
	float altitude = processor->getAltitude();
	fwrite(&longitude, sizeof(double), 1, f);
	fwrite(&latitude, sizeof(double), 1, f);
	fwrite(&altitude, sizeof(float), 1, f);
	// bounding box
	float minX = (float)processor->getBoundingBox().minX, minY = (float)processor->getBoundingBox().minY, minZ = (float)processor->getBoundingBox().minZ;
	float maxX = (float)processor->getBoundingBox().maxX, maxY = (float)processor->getBoundingBox().maxY, maxZ = (float)processor->getBoundingBox().maxZ;
	fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
	fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);

	// spatial octree info
	writeOctreeInfo(processor->getSpatialOctree(), f);
	// versoin div : end

	return true;
}

bool F4DWriter::writeModels(FILE* f, std::vector<gaia3d::TrianglePolyhedron*>& models)
{
	unsigned int modelCount = (unsigned int)models.size();
	fwrite(&modelCount, sizeof(unsigned int), 1, f);

	gaia3d::TrianglePolyhedron* model;
	unsigned int modelIndex;
	float minX, minY, minZ, maxX, maxY, maxZ;
	unsigned int vboCount, vertexCount, indexCount;
	unsigned char sizeLevels;
	float thresholds[TriangleSizeLevels];
	float x, y, z;
	char nx, ny, nz;
	bool bInterleavedMode = false;
	char padding = 0;
	unsigned short index;
	for (size_t i = 0; i < modelCount; i++)
	{
		model = models[i];
		modelIndex = (unsigned int)model->getReferenceInfo().modelIndex;
		fwrite(&modelIndex, sizeof(unsigned int), 1, f);

		// bounding box
		minX = (float)model->getBoundingBox().minX; minY = (float)model->getBoundingBox().minY; minZ = (float)model->getBoundingBox().minZ;
		maxX = (float)model->getBoundingBox().maxX; maxY = (float)model->getBoundingBox().maxY; maxZ = (float)model->getBoundingBox().maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
		fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);

		// vbo count
		vboCount = (unsigned int)model->getVbos().size();
		fwrite(&vboCount, sizeof(unsigned int), 1, f);

		// vbo
		for (unsigned int j = 0; j < vboCount; j++)
		{
			vertexCount = (unsigned int)model->getVbos()[j]->vertices.size();
			// vertex count
			fwrite(&vertexCount, sizeof(unsigned int), 1, f);

			// vertex positions
			for (unsigned int k = 0; k < vertexCount; k++)
			{
				x = (float)model->getVbos()[j]->vertices[k]->position.x;
				y = (float)model->getVbos()[j]->vertices[k]->position.y;
				z = (float)model->getVbos()[j]->vertices[k]->position.z;
				fwrite(&x, sizeof(float), 1, f); fwrite(&y, sizeof(float), 1, f); fwrite(&z, sizeof(float), 1, f);
			}

			// normal count
			fwrite(&vertexCount, sizeof(unsigned int), 1, f);

			// normals
			for (unsigned int k = 0; k < vertexCount; k++)
			{
				nx = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.x);
				ny = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.y);
				nz = (char)(127.0f * model->getVbos()[j]->vertices[k]->normal.z);
				fwrite(&nx, sizeof(char), 1, f); fwrite(&ny, sizeof(char), 1, f); fwrite(&nz, sizeof(char), 1, f);
				if (bInterleavedMode)
					fwrite(&padding, sizeof(char), 1, f);
			}

			// index count
			indexCount = (unsigned int)model->getVbos()[j]->indices.size();
			fwrite(&indexCount, sizeof(unsigned int), 1, f); // 전체 index개수
			sizeLevels = TriangleSizeLevels; // 삼각형을 크기별로 정렬할 때 적용된 크기 개수
			fwrite(&sizeLevels, sizeof(unsigned char), 1, f);
			for (unsigned char k = 0; k < sizeLevels; k++)
				thresholds[k] = (float)model->getVbos()[j]->triangleSizeThresholds[k]; // 삼각형을 크기별로 정렬할 때 적용된 크기들
			fwrite(thresholds, sizeof(float), sizeLevels, f);
			fwrite(model->getVbos()[j]->indexMarker, sizeof(unsigned int), sizeLevels, f); // 정렬된 vertex들의 index에서 크기 기준이 변경되는 최초 삼각형의 vertex의 index 위치 marker
			for (size_t k = 0; k < indexCount; k++)
			{
				index = models[i]->getVbos()[j]->indices[k];
				fwrite(&index, sizeof(unsigned short), 1, f);
			}
		}
	}

	return true;
}

bool F4DWriter::writeReferencesAndModels(std::string& referencePath, std::string& modelPath)
{
	std::vector<gaia3d::OctreeBox*> leafBoxes;
	gaia3d::OctreeBox* leafBox;
	processor->getSpatialOctree()->getAllLeafBoxes(leafBoxes, true);
	size_t leafCount = leafBoxes.size();
	std::string referenceFilePath; // each reference file full path
	std::string modelFilePath; // each model file full path
	std::vector<gaia3d::TrianglePolyhedron*> models;
	size_t modelCount;
	FILE* file = NULL;
	unsigned int referenceCount; // total reference count
	size_t meshCount = processor->getAllMeshes().size();
	unsigned int referenceId, modelId; // referenc id, block id which reference is refering to
	unsigned char objectIdLength;
	std::string objectId;
	gaia3d::TrianglePolyhedron* reference;
	gaia3d::TrianglePolyhedron* model;
	bool bFound;
	unsigned int vertexCount; // vertex count in a reference
	float m;	// element of transform matrix by which referend block is transformed into current reference position
	bool bColor; // if color exists
	unsigned short valueType; // array value type
	unsigned char colorDimension; // color channel count
	bool bTextureCoordinate; // if texture coordinate exists
	//float u, v;	// texture coordinate
	unsigned int textureCount = 0;
	for (size_t i = 0; i < leafCount; i++)
	{
		leafBox = leafBoxes[i];

		referenceFilePath = referencePath + "/" + std::to_string((long long)((gaia3d::SpatialOctreeBox*)leafBoxes[i])->octreeId) + std::string("_Ref");
		file = fopen(referenceFilePath.c_str(), "wb");

		// reference count in each octrees
		referenceCount = (unsigned int)leafBox->meshes.size();
		fwrite(&referenceCount, sizeof(unsigned int), 1, file);

		for (unsigned int j = 0; j < referenceCount; j++)
		{
			reference = leafBox->meshes[j];

			// extract models
			if (reference->getReferenceInfo().model == NULL)
				model = reference;
			else
				model = reference->getReferenceInfo().model;
			bFound = false;
			modelCount = models.size();
			for (size_t k = 0; k < modelCount; k++)
			{
				if (models[k] == model)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
				models.push_back(model);

			// reference id
			referenceId = (unsigned int)reference->getId();
			fwrite(&referenceId, sizeof(unsigned int), 1, file);

			// reference object id
			if (reference->doesStringAttributeExist(std::string(ObjectGuid)))
			{
				objectId = reference->getStringAttribute(std::string(ObjectGuid));
				//std::string wObjectId = reference->getStringAttribute(std::string(ObjectGuid));
				//objectId = std::string(gaia3d::ws2s(wObjectId.c_str()));
				//objectId = std::string(wObjectId.c_str());

				objectIdLength = (unsigned char)objectId.length();
				fwrite(&objectIdLength, sizeof(unsigned char), 1, file);
				if (objectIdLength > 0)
					fwrite(objectId.c_str(), sizeof(char), objectIdLength, file);
			}
			else
			{
				objectIdLength = (unsigned char)0;
				fwrite(&objectIdLength, sizeof(unsigned char), 1, file);
			}
			

			// model id
			modelId = (unsigned int)reference->getReferenceInfo().modelIndex;
			fwrite(&modelId, sizeof(unsigned int), 1, file);

			// transform matrix
			for (size_t c = 0; c < 4; c++)
			{
				for (size_t r = 0; r < 4; r++)
				{
					m = (float)reference->getReferenceInfo().mat.m[c][r];
					fwrite(&m, sizeof(float), 1, file);
				}
			}

			vertexCount = (unsigned int)reference->getVertices().size();

			// representative color of this reference
			if (reference->getColorMode() == gaia3d::SingleColor)
			{
				bColor = true;
				fwrite(&bColor, sizeof(bool), 1, file);
				valueType = 5121; // (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)
				fwrite(&valueType, sizeof(unsigned short), 1, file);
				colorDimension = 4;
				fwrite(&colorDimension, sizeof(unsigned char), 1, file);
				writeColor(reference->getSingleColor(), valueType, true, file);
			}
			else
			{
				bColor = false;
				fwrite(&bColor, sizeof(bool), 1, file);
			}

			// if vertex color & texture coordinate exist
			bColor = (reference->getColorMode() == gaia3d::ColorsOnVertices) ? true : false;
			fwrite(&bColor, sizeof(bool), 1, file);
			bTextureCoordinate = reference->doesThisHaveTextureCoordinates();
			fwrite(&bTextureCoordinate, sizeof(bool), 1, file);

			// save vertex color & texture coordinate on vbo count loop
			if (bColor || bTextureCoordinate)
			{
				unsigned int vboCount = (unsigned int)reference->getVbos().size();
				fwrite(&vboCount, sizeof(unsigned int), 1, file);

				for (unsigned int k = 0; k < vboCount; k++)
				{
					gaia3d::Vbo* vbo = reference->getVbos()[k];
					unsigned int vboVertexCount = (unsigned int)vbo->vertices.size();

					if (bColor)
					{
						valueType = 5121;
						fwrite(&valueType, sizeof(unsigned short), 1, file);
						colorDimension = 3;
						fwrite(&colorDimension, sizeof(unsigned char), 1, file);

						fwrite(&vboVertexCount, sizeof(unsigned int), 1, file);
						for (unsigned int j = 0; j < vboVertexCount; j++)
							writeColor(vbo->vertices[j]->color, valueType, false, file);
					}

					if (bTextureCoordinate)
					{
						valueType = 5126;
						fwrite(&valueType, sizeof(unsigned short), 1, file);

						fwrite(&vboVertexCount, sizeof(unsigned int), 1, file);
						for (unsigned int j = 0; j < vboVertexCount; j++)
						{
							float tx = (float)vbo->vertices[j]->textureCoordinate[0];
							float ty = (float)vbo->vertices[j]->textureCoordinate[1];
							fwrite(&tx, sizeof(float), 1, file);
							fwrite(&ty, sizeof(float), 1, file);
						}
					}
				}
			}

			// texture file info if exists
			if (bTextureCoordinate)
			{
				textureCount = 1;
				fwrite(&textureCount, sizeof(unsigned int), 1, file);

				std::string textureType("diffuse");
				unsigned int typeLength = (unsigned int)textureType.length();
				fwrite(&typeLength, sizeof(unsigned int), 1, file);
				fwrite(textureType.c_str(), sizeof(char), typeLength, file);

				//std::string textureName(gaia3d::ws2s(reference->getStringAttribute(TextureName).c_str()));
				std::string textureName(reference->getStringAttribute(TextureName).c_str());

				unsigned int nameLength = (unsigned int)textureName.length();
				fwrite(&nameLength, sizeof(unsigned int), 1, file);
				fwrite(textureName.c_str(), sizeof(char), nameLength, file);
			}
			else
			{
				textureCount = 0;
				fwrite(&textureCount, sizeof(unsigned int), 1, file);
			}
		}

		writeVisibilityIndices(file, (static_cast<gaia3d::SpatialOctreeBox*>(leafBox))->exteriorOcclusionInfo);
		writeVisibilityIndices(file, (static_cast<gaia3d::SpatialOctreeBox*>(leafBox))->interiorOcclusionInfo);
		fclose(file);

		modelFilePath = modelPath + "/" + std::to_string((long long)((gaia3d::SpatialOctreeBox*)leafBoxes[i])->octreeId) +  "_Model";
		file = fopen(modelFilePath.c_str(), "wb");
		this->writeModels(file, models);
		fclose(file);
		models.clear();
	}

	return true;
}

bool F4DWriter::writeVisibilityIndices(FILE* f, gaia3d::OctreeBox* octree)
{
	bool bRoot = octree->parent == NULL ? true : false;
	fwrite(&bRoot, sizeof(bool), 1, f);
	if (bRoot)
	{
		float minX, minY, minZ, maxX, maxY, maxZ;

		minX = (float)octree->minX;
		minY = (float)octree->minY;
		minZ = (float)octree->minZ;
		maxX = (float)octree->maxX;
		maxY = (float)octree->maxY;
		maxZ = (float)octree->maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&maxX, sizeof(float), 1, f);
		fwrite(&minY, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f);
		fwrite(&minZ, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}

	unsigned int childCount = (unsigned int)octree->children.size();
	fwrite(&childCount, sizeof(unsigned int), 1, f);

	if (childCount == 0)
	{
		unsigned int referenceCount = (unsigned int)octree->meshes.size();
		fwrite(&referenceCount, sizeof(unsigned int), 1, f);

		unsigned int referenceId;
		for (unsigned int i = 0; i < referenceCount; i++)
		{
			referenceId = (unsigned int)octree->meshes[i]->getId();
			fwrite(&referenceId, sizeof(unsigned int), 1, f);
		}
	}
	else
	{
		for (unsigned int i = 0; i < childCount; i++)
		{
			writeVisibilityIndices(f, octree->children[i]);
		}
	}

	return true;
}

bool F4DWriter::writeLegoBlocks(std::string& legoBlockPath)
{
	std::map<size_t, gaia3d::TrianglePolyhedron*>::iterator itr = processor->getLegos().begin();
	std::string octreeLegoFilePath;
	FILE* file = NULL;
	size_t key;
	gaia3d::TrianglePolyhedron* lego;
	float minX, minY, minZ, maxX, maxY, maxZ; // bounding box
	unsigned int vertexCount;	// vertex count of each lego
	float x, y, z;
	bool bNormal;
	char nx, ny, nz;
	bool bInterleavedMode = false;
	char padding = 0;
	bool bColor;
	bool bTexture;
	for (; itr != processor->getLegos().end(); itr++)
	{
		// octree lego file
		key = itr->first;
		lego = itr->second;

		octreeLegoFilePath = legoBlockPath + "/" + std::to_string((unsigned long long)key) + std::string("_Brick");
		file = fopen(octreeLegoFilePath.c_str(), "wb");

		// bounding box
		minX = (float)lego->getBoundingBox().minX; minY = (float)lego->getBoundingBox().minY; minZ = (float)lego->getBoundingBox().minZ;
		maxX = (float)lego->getBoundingBox().maxX; maxY = (float)lego->getBoundingBox().maxY; maxZ = (float)lego->getBoundingBox().maxZ;
		fwrite(&minX, sizeof(float), 1, file); fwrite(&minY, sizeof(float), 1, file); fwrite(&minZ, sizeof(float), 1, file);
		fwrite(&maxX, sizeof(float), 1, file); fwrite(&maxY, sizeof(float), 1, file); fwrite(&maxZ, sizeof(float), 1, file);

		// vertices count
		vertexCount = (unsigned int)lego->getVertices().size();
		fwrite(&vertexCount, sizeof(unsigned int), 1, file);

		// vertex positions
		for (unsigned int i = 0; i < vertexCount; i++)
		{
			x = (float)lego->getVertices()[i]->position.x; y = (float)lego->getVertices()[i]->position.y; z = (float)lego->getVertices()[i]->position.z;
			fwrite(&x, sizeof(float), 1, file); fwrite(&y, sizeof(float), 1, file); fwrite(&z, sizeof(float), 1, file);
		}

		// normals
		bNormal = lego->doesThisHaveNormals();
		fwrite(&bNormal, sizeof(bool), 1, file);
		if (bNormal)
		{
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			for (unsigned int i = 0; i < vertexCount; i++)
			{
				nx = (char)(127.0f * lego->getVertices()[i]->normal.x);
				ny = (char)(127.0f * lego->getVertices()[i]->normal.y);
				nz = (char)(127.0f * lego->getVertices()[i]->normal.z);
				fwrite(&nx, sizeof(char), 1, file); fwrite(&ny, sizeof(char), 1, file); fwrite(&nz, sizeof(char), 1, file);
				if (bInterleavedMode)
					fwrite(&padding, sizeof(char), 1, file);
			}
		}

		// colors
		if (lego->getColorMode() == gaia3d::ColorsOnVertices)
		{
			bColor = true;
			fwrite(&bColor, sizeof(bool), 1, file);
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			for (unsigned int i = 0; i < vertexCount; i++)
			{
				writeColor(lego->getVertices()[i]->color, 5121, true, file);
			}
		}
		else
		{
			bColor = false;
			fwrite(&bColor, sizeof(bool), 1, file);
		}

		// texture coordinate and resource
		bTexture = lego->doesThisHaveTextureCoordinates();
		fwrite(&bTexture, sizeof(bool), 1, file);
		if (bTexture)
		{
			// save the data type.***
			// (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)
			unsigned short type = 5126;
			fwrite(&type, sizeof(unsigned short), 1, file);

			// vertex count
			fwrite(&vertexCount, sizeof(unsigned int), 1, file);
			// texture coordinates of each vertex
			for (unsigned int i = 0; i < vertexCount; i++)
			{
				float tx = (float)lego->getVertices()[i]->textureCoordinate[0];
				float ty = (float)lego->getVertices()[i]->textureCoordinate[1];
				fwrite(&tx, sizeof(float), 1, file);
				fwrite(&ty, sizeof(float), 1, file);
			}
		}
		fclose(file);
	}

	return true;
}

bool F4DWriter::writeOctreeInfo(gaia3d::OctreeBox* octree, FILE* f)
{
	unsigned int level = octree->level;
	fwrite(&level, sizeof(unsigned int), 1, f);

	float minX, minY, minZ, maxX, maxY, maxZ;
	if (level == 0)
	{
		minX = (float)octree->minX;
		minY = (float)octree->minY;
		minZ = (float)octree->minZ;
		maxX = (float)octree->maxX;
		maxY = (float)octree->maxY;
		maxZ = (float)octree->maxZ;
		fwrite(&minX, sizeof(float), 1, f); fwrite(&maxX, sizeof(float), 1, f);
		fwrite(&minY, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f);
		fwrite(&minZ, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}

	unsigned char childCount = (unsigned char)octree->children.size();
	fwrite(&childCount, sizeof(unsigned char), 1, f);

	unsigned int triangleCount = 0;
	size_t meshCount = octree->meshes.size();
	size_t surfaceCount;
	for (size_t i = 0; i < meshCount; i++)
	{
		surfaceCount = octree->meshes[i]->getSurfaces().size();
		for (size_t j = 0; j < surfaceCount; j++)
			triangleCount += (unsigned int)octree->meshes[i]->getSurfaces()[j]->getTriangles().size();
	}
	fwrite(&triangleCount, sizeof(unsigned int), 1, f);

	for (unsigned char i = 0; i < childCount; i++)
	{
		writeOctreeInfo(octree->children[i], f);
	}

	return true;
}

void F4DWriter::writeColor(unsigned long color, unsigned short type, bool bAlpha, FILE* file)
{
	// color type : (5120 signed byte), (5121 unsigned byte), (5122 signed short), (5123 unsigned short), (5126 float)

	unsigned char red = GetRedValue(color);
	unsigned char green = GetGreenValue(color);
	unsigned char blue = GetBlueValue(color);
	unsigned char alpha = 255;

	switch (type)
	{
	case 5121: // unsigned char mode
	{
		fwrite(&red, sizeof(unsigned char), 1, file);
		fwrite(&green, sizeof(unsigned char), 1, file);
		fwrite(&blue, sizeof(unsigned char), 1, file);
		if (bAlpha)
			fwrite(&alpha, sizeof(unsigned char), 1, file);
	}
	break;
	case 5126: // float mode
	{
		float fRed = red / 255.0f;
		float fGreen = green / 255.0f;
		float fBlue = blue / 255.0f;
		fwrite(&fRed, sizeof(float), 1, file);
		fwrite(&fGreen, sizeof(float), 1, file);
		fwrite(&fBlue, sizeof(float), 1, file);
		if (bAlpha)
		{
			float fAlpha = 1.0f;
			fwrite(&fAlpha, sizeof(float), 1, file);
		}
	}
	break;
	default: // TODO(khj 20170324) : NYI the other color type
		break;
	}
}

bool F4DWriter::writeIndexFile()
{
	std::vector<std::string> convertedDataFolders;

	namespace bfs = boost::filesystem;

	bfs::path folderPath(folder);
	if (bfs::is_directory(folderPath))
	{
		std::cout << "In directory: " << folderPath.string() << std::endl;
		bfs::directory_iterator end;
		for (bfs::directory_iterator it(folderPath); it != end; ++it)
		{
			try
			{
				if (bfs::is_directory(*it))
				{
					convertedDataFolders.push_back(it->path().string());
					std::cout << "[directory]" << it->path() << std::endl;
				}
			}
			catch (const std::exception &ex)
			{
				std::cout << it->path().filename() << " " << ex.what() << std::endl;
			}
		}
	}
	/*
	_wfinddata64_t fd;
	long long handle;
	int result = 1;
	std::string structureJtFilter = folder + std::string( "/*.*");
	handle = _wfindfirsti64(structureJtFilter.c_str(), &fd);

	if(handle == -1)
	{
		return false;
	}

	std::vector<std::string> convertedDataFolders;
	while(result != -1)
	{
		if((fd.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			if(std::string(fd.name) !=  "." && std::string(fd.name) !=  "..")
				convertedDataFolders.push_back(std::string(fd.name));
		}
		result = _wfindnexti64(handle, &fd);
	}

	_findclose(handle);
	*/
	if (convertedDataFolders.size() == 0)
		return false;

	std::string targetFilePath = folder + "/objectIndexFile.ihe";
	FILE* f;
	f = fopen(targetFilePath.c_str(), "wb");

	unsigned int dataFolderCount = (unsigned int)convertedDataFolders.size();
	fwrite(&dataFolderCount, sizeof(unsigned int), 1, f);

	std::string eachDataHeader;
	char version[6];
	int guidLength;
	char guid[256];
	memset(guid, 0x00, 256);
	double longitude, latitude;
	float altitude;
	float minX, minY, minZ, maxX, maxY, maxZ;
	unsigned int dataFolderNameLength;
	for (size_t i = 0; i < dataFolderCount; i++)
	{
		eachDataHeader = convertedDataFolders[i] + "/HeaderAsimetric.hed";
		bfs::path headerPath(eachDataHeader);

		if (!bfs::exists(headerPath))
		{
			std::cout << "[ERROR]" << eachDataHeader << " file does not exist." << std::endl;
			continue;
		}
		FILE* header;
		header = fopen(eachDataHeader.c_str(), "rb");

		// version
		memset(version, 0x00, 6);
		fread(version, sizeof(char), 5, header);
		// guid length
		fread(&guidLength, sizeof(int), 1, header);
		// guid
		memset(guid, 0x00, 256);
		fread(guid, sizeof(char), guidLength, header);
		// representative longitude, latitude, altitude
		fread(&longitude, sizeof(double), 1, header);
		fread(&latitude, sizeof(double), 1, header);
		fread(&altitude, sizeof(float), 1, header);
		// bounding box
		fread(&minX, sizeof(float), 1, header); fread(&minY, sizeof(float), 1, header); fread(&minZ, sizeof(float), 1, header);
		fread(&maxX, sizeof(float), 1, header); fread(&maxY, sizeof(float), 1, header); fread(&maxZ, sizeof(float), 1, header);

		fclose(header);

		bfs::detail::utf8_codecvt_facet utf8;
		bfs::path convertedDataPath(convertedDataFolders[i]);
		std::string singleConvertedDataFolder(convertedDataPath.filename().string(utf8));

		dataFolderNameLength = (unsigned int)singleConvertedDataFolder.length();
		fwrite(&dataFolderNameLength, sizeof(unsigned int), 1, f);

		//std::string singleConvertedDataFolder(gaia3d::ws2s(convertedDataFolders[i].c_str()));
		//std::string singleConvertedDataFolder(convertedDataFolders[i].c_str());

		fwrite(singleConvertedDataFolder.c_str(), sizeof(char), dataFolderNameLength, f);

		fwrite(&longitude, sizeof(double), 1, f);
		fwrite(&latitude, sizeof(double), 1, f);
		fwrite(&altitude, sizeof(float), 1, f);

		fwrite(&minX, sizeof(float), 1, f); fwrite(&minY, sizeof(float), 1, f); fwrite(&minZ, sizeof(float), 1, f);
		fwrite(&maxX, sizeof(float), 1, f); fwrite(&maxY, sizeof(float), 1, f); fwrite(&maxZ, sizeof(float), 1, f);
	}


	fclose(f);

	return true;
}

void F4DWriter::writeLegoTexture(std::string resultPath)
{
	unsigned int* legoTextureDimension = processor->getLegoTextureDimension();
	unsigned char* legoTextureByteArray = processor->getLegoTextureBitmapArray();
	unsigned int stride = legoTextureDimension[0] * 4;

	int width = legoTextureDimension[0];
	int height = legoTextureDimension[1];
	int nrChannels = 4;

	std::string legoTextureFullPath = resultPath + "/SimpleBuildingTexture3x3.png";

	//std::string singleFullPath(gaia3d::ws2s(legoTextureFullPath.c_str()));
	std::string singleFullPath(legoTextureFullPath.c_str());

	stbi_flip_vertically_on_write(false);
	stbi_write_png(singleFullPath.c_str(), width, height, nrChannels, legoTextureByteArray, 0);
}

void F4DWriter::writeTextures(std::string imagePath)
{
	std::map<std::string, std::string>::iterator itr = processor->getTextureInfo().begin();
	for (; itr != processor->getTextureInfo().end(); itr++)
	{
		std::string fileName = itr->first;

		std::string::size_type dotPosition = fileName.rfind(".");
		if (dotPosition == std::string::npos)
			continue;

		std::string fileExt = fileName.substr(dotPosition + 1, fileName.length() - dotPosition - 1);
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

		int nrChannels = 4;
		unsigned int width = processor->getAllTextureWidths()[fileName];
		unsigned int height = processor->getAllTextureHeights()[fileName];
		unsigned char* bmpArray = processor->getResizedTextures()[fileName];

		std::string fullPath = imagePath + "/" + fileName;

		//std::string singleFullPath(gaia3d::ws2s(fullPath.c_str()));
		std::string singleFullPath(fullPath.c_str());

		stbi_flip_vertically_on_write(false);

		if (fileExt.compare("dds") == 0)
		{
			std::string orginalImagePath = itr->second;
			FILE* file = NULL;
			file = fopen(orginalImagePath.c_str(), "rb");
			if (file == NULL)
				continue;

			fseek(file, 0, SEEK_END);
			long fileSize = ftell(file);
			if (fileSize <= 0)
				continue;

			rewind(file);
			unsigned char* fileContents = new unsigned char[fileSize];
			memset(fileContents, 0x00, sizeof(unsigned char)*fileSize);

			fread(fileContents, sizeof(unsigned char), fileSize, file);
			fclose(file);
			file = NULL;

			file = fopen(fullPath.c_str(), "wb");
			if (file == NULL)
			{
				delete[] fileContents;
				continue;
			}

			fwrite(fileContents, sizeof(unsigned char), fileSize, file);
			fclose(file);
			delete[] fileContents;
		}
		else
		{
			if (fileExt.compare("jpg") == 0 || fileExt.compare("jpeg") == 0 || fileExt.compare("jpe") == 0)
			{
				stbi_write_jpg(singleFullPath.c_str(), width, height, nrChannels, bmpArray, 100);
			}
			else if (fileExt.compare("png") == 0)
			{
				stbi_write_png(singleFullPath.c_str(), width, height, nrChannels, bmpArray, 0);
			}
			else if (fileExt.compare("gif") == 0)
			{
				// TODO 
				continue;
			}
			else if (fileExt.compare("tif") == 0 || fileExt.compare("tiff") == 0)
			{
				// TODO 
				continue;
			}
			else if (fileExt.compare("bmp") == 0)
			{
				stbi_write_bmp(singleFullPath.c_str(), width, height, nrChannels, bmpArray);
			}
			else if (fileExt.compare("tga") == 0)
			{
				stbi_write_tga(singleFullPath.c_str(), width, height, nrChannels, bmpArray);
			}
			else
			{
				continue;
			}
		}
	}
}
