﻿/**
 * Implementation of the ReaderFactory class
 */
#include "ClassicFormatReader.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <proj_api.h>

#include "../geometry/Matrix4.h"
#include "../util/GeometryUtility.h"
#include "../geometry/TrianglePolyhedron.h"
#include "../converter/LogWriter.h"

#include <iostream>

std::string folder;
bool bMustChangeYZCoordinate = false;

bool proceedMesh(aiMesh* mesh,
				const aiScene* scene,
				gaia3d::Matrix4& transform,
				std::vector<gaia3d::TrianglePolyhedron*>& container,
				std::map<std::string, std::string>& textureContainer)
{
	if (mesh->mNumVertices < 3)
	{
		return false;
	}
	// aiMesh == TrianglePolyhedron
	gaia3d::TrianglePolyhedron* polyhedron = new gaia3d::TrianglePolyhedron;

	// check if texture exists.
	bool textureExistsForMesh = false;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 && mesh->mTextureCoords[0] != NULL)
	{
		textureExistsForMesh = true;
		polyhedron->setHasTextureCoordinates(true);

		// collect texture info
		aiString texturePath;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
		
		std::string fullPath = folder + "/" + std::string(texturePath.C_Str());

		size_t lastSlashIndex = fullPath.find_last_of("\\/");
		std::string fileName;
		if (lastSlashIndex == std::string::npos)
			fileName = fullPath;
		else
		{
			size_t fileNameLength = fullPath.length() - lastSlashIndex - 1;
			fileName = fullPath.substr(lastSlashIndex + 1, fileNameLength);
		}

		if (textureContainer.find(fileName) == textureContainer.end())
		{
			textureContainer.insert(std::map<std::string, std::string>::value_type(fileName, fullPath));
		}

		polyhedron->addStringAttribute(std::string(TextureName), fileName);
	}

	// check if color info exists.
	gaia3d::ColorMode colorMode = gaia3d::NoColor;
	if (!textureExistsForMesh)
	{
		aiReturn result;
		aiColor3D colorDiffuse(0.f, 0.f, 0.f);
		result = material->Get(AI_MATKEY_COLOR_DIFFUSE, colorDiffuse);
		if (result == aiReturn_SUCCESS)
		{
			colorMode = gaia3d::SingleColor;
			polyhedron->setSingleColor(MakeColorU4((unsigned char)(colorDiffuse.r * 255),
				(unsigned char)(colorDiffuse.g * 255),
				(unsigned char)(colorDiffuse.b * 255)));
		}
		else
		{
			if (mesh->mColors[0] != NULL)
				colorMode = gaia3d::ColorsOnVertices;
		}
	}
	polyhedron->setColorMode(colorMode);

	// check if vertex normals
	if (mesh->HasNormals())
		polyhedron->setHasNormals(true);

	// access to vertices
	//double tmpZ;
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		gaia3d::Vertex* vertex = new gaia3d::Vertex;

		vertex->position.x = mesh->mVertices[i].x;
		vertex->position.y = mesh->mVertices[i].y;
		vertex->position.z = mesh->mVertices[i].z;

		//if (bMustChangeYZCoordinate)
		//{
		//	tmpZ = vertex->position.z;
		//	vertex->position.z = vertex->position.y;
		//	vertex->position.y = -tmpZ;
		//}

		vertex->position = transform * vertex->position;

		if (mesh->HasNormals())
		{
			vertex->normal.x = mesh->mNormals[i].x;
			vertex->normal.y = mesh->mNormals[i].y;
			vertex->normal.z = mesh->mNormals[i].z;

			//if (bMustChangeYZCoordinate)
			//{
			//	tmpZ = vertex->normal.z;
			//	vertex->normal.z = vertex->normal.y;
			//	vertex->normal.y = -tmpZ;
			//}

			transform.applyOnlyRotationOnPoint(vertex->normal);

			vertex->normal.normalize();
		}

		if (textureExistsForMesh)
		{
			vertex->textureCoordinate[0] = mesh->mTextureCoords[0][i].x;
			vertex->textureCoordinate[1] = mesh->mTextureCoords[0][i].y;
			//LogWriter::getLogWriter()->addContents(std::to_wstring(vertex->textureCoordinate[0]) + ":" + std::to_wstring(vertex->textureCoordinate[1]), true);
		}
		else
		{
			if (colorMode == gaia3d::ColorsOnVertices)
			{
				vertex->color = MakeColorU4((unsigned char)(mesh->mColors[0][i].r * 255),
											(unsigned char)(mesh->mColors[0][i].g * 255),
											(unsigned char)(mesh->mColors[0][i].b * 255));
			}
		}

		polyhedron->getVertices().push_back(vertex);
	}

	// assimp model has 2 level hierarchy but TrianglePolyhedron has 3 level of it
	// so add 1 Surface to polyhedron
	gaia3d::Surface* surface = new gaia3d::Surface;
	polyhedron->getSurfaces().push_back(surface);

	// make triangles and assign vertices to each triangle
	gaia3d::Triangle* triangle;
	bool wrongIndex;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		if (mesh->mFaces[i].mNumIndices != 3)
			continue;

		triangle = new gaia3d::Triangle;

		wrongIndex = false;
		for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
		{
			if (mesh->mFaces[i].mIndices[j] >= polyhedron->getVertices().size())
			{
				wrongIndex = true;
				break;
			}
		}
		
		if (wrongIndex)
			continue;

		triangle->setVertexIndices(mesh->mFaces[i].mIndices[0],
									mesh->mFaces[i].mIndices[1],
									mesh->mFaces[i].mIndices[2]);

		triangle->setVertices(polyhedron->getVertices()[triangle->getVertexIndices()[0]],
							polyhedron->getVertices()[triangle->getVertexIndices()[1]],
							polyhedron->getVertices()[triangle->getVertexIndices()[2]]);

		surface->getTriangles().push_back(triangle);
	}

	polyhedron->setId(container.size());

	container.push_back(polyhedron);
	
	return true;
}

bool proceedNode(aiNode* node,
				const aiScene* scene,
				gaia3d::Matrix4& parentMatrix,
				std::vector<gaia3d::TrianglePolyhedron*>& container,
				std::map<std::string, std::string>& textureContainer)
{
	// TransformationMatrix
	gaia3d::Matrix4 thisMatrix;

	thisMatrix.set(node->mTransformation.a1, node->mTransformation.b1, node->mTransformation.c1, node->mTransformation.d1,
		node->mTransformation.a2, node->mTransformation.b2, node->mTransformation.c2, node->mTransformation.d2,
		node->mTransformation.a3, node->mTransformation.b3, node->mTransformation.c3, node->mTransformation.d3,
		node->mTransformation.a4, node->mTransformation.b4, node->mTransformation.c4, node->mTransformation.d4);

	// TMatrix * SMatrix
	thisMatrix = thisMatrix * parentMatrix;

	std::string nodeName = std::string(node->mName.C_Str());

	aiMesh *mesh = NULL;
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		mesh = scene->mMeshes[node->mMeshes[i]];
		if (proceedMesh(mesh, scene, thisMatrix, container, textureContainer))
		{
			if (!nodeName.empty())
			{
				gaia3d::TrianglePolyhedron* newCreated = container.back();

				newCreated->addStringAttribute(std::string(ObjectGuid), nodeName);
			}
		}
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		proceedNode(node->mChildren[i], scene, thisMatrix, container, textureContainer);
	}
	return true;
}

ClassicFormatReader::ClassicFormatReader()
{
}


ClassicFormatReader::~ClassicFormatReader()
{
}

bool ClassicFormatReader::readRawDataFile(std::string& filePath)
{
	// before processing more, check if the georeferencing information is valid
	projPJ pjSrc = NULL, pjWgs84 = NULL;
	if (bCoordinateInfoInjected)
	{
		std::string originalSrsProjString = makeProj4String();
		std::string wgs84ProjString("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

		pjSrc = pj_init_plus(originalSrsProjString.c_str());
		pjWgs84 = pj_init_plus(wgs84ProjString.c_str());
		if (pjSrc == NULL || pjWgs84 == NULL)
		{
			printf("[ERROR][proj4]CANNOT initialize SRS\n");
			// new log
			LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
			LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : failed to initialize proj"));

			return false;
		}
	}

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(filePath,
		//aiProcess_SplitLargeMeshes |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	if (scene == NULL)
	{
		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : loading failure"));
		return false;
	}

	if (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE)
	{
		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : loading imcomplete"));
		return false;
	}

	if (scene->mRootNode == NULL)
	{
		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : no available data in file"));
		return false;
	}

	size_t slashPosition = filePath.find_last_of("\\/");
	if (slashPosition == std::string::npos)
		folder = filePath;
	else
		folder = filePath.substr(0, slashPosition);

	gaia3d::Matrix4 scaleMatrix, rotationMatrix, offsetMatrix, transformMatrix;

	bMustChangeYZCoordinate = bYAxisUp;

	scaleMatrix.set(unitScaleFactor,			 0.0,			  0.0, 0.0,
								0.0, unitScaleFactor,			  0.0, 0.0,
								0.0,			 0.0, unitScaleFactor, 0.0,
								0.0,			 0.0,			  0.0, 1.0);

	if (bYAxisUp)
	{
		rotationMatrix.set( 1.0,  0.0, 0.0, 0.0,
							0.0,  0.0, 1.0, 0.0,
							0.0, -1.0, 0.0, 0.0,
							0.0,  0.0, 0.0, 1.0 );
		//rotationMatrix.rotationInDegree(90.0, 1.0, 0.0, 0.0);
	}

	offsetMatrix.set(	1.0,	 0.0,	  0.0, 0.0,
						0.0,	 1.0,	  0.0, 0.0,
						0.0,	 0.0,	  1.0, 0.0,
					offsetX, offsetY, offsetZ, 1.0);

	transformMatrix = scaleMatrix * rotationMatrix * offsetMatrix;

	if (!proceedNode(scene->mRootNode, scene, transformMatrix, container, textureContainer))
		return false;

	// bounding box limitation checking
	gaia3d::BoundingBox bbox;
	size_t meshCount = container.size();
	for (size_t i = 0; i < meshCount; i++)
	{
		std::vector<gaia3d::Vertex*>& vertices = container[i]->getVertices();
		size_t vertexCount = vertices.size();
		for (size_t j = 0; j < vertexCount; j++)
			bbox.addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
	}

	if (bbox.getMaxLength() > 600.0)
	{
		for (size_t i = 0; i < meshCount; i++)
		{
			delete container[i];
		}

		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : bbox is too large. check length unit."));

		return false;
	}

	// transform coordinates if information for georeferencing is injected.
	if (bCoordinateInfoInjected)
	{
		/*gaia3d::BoundingBox bbox;
		size_t meshCount = container.size();
		for (size_t i = 0; i < meshCount; i++)
		{
			std::vector<gaia3d::Vertex*>& vertices = container[i]->getVertices();
			size_t vertexCount = vertices.size();
			for (size_t j = 0; j < vertexCount; j++)
				bbox.addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
		}*/

		double cx, cy, cz;
		bbox.getCenterPoint(cx, cy, cz);

		refLon = cx; refLat = cy;
		double alt = cz;
		int errorCode = pj_transform(pjSrc, pjWgs84, 1, 1, &refLon, &refLat, &alt);
		char* errorMessage = pj_strerrno(errorCode);
		if (errorMessage != NULL)
		{
			printf("[ERROR][proj4]%s\n", errorMessage);
			size_t meshCount = container.size();
			for (size_t i = 0; i < meshCount; i++)
				delete container[i];
			container.clear();

			// new log
			LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
			LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" ClassicFormatReader::readRawDataFile : boungding box center coordinate transform failure"));

			return false;
		}

		refLon *= RAD_TO_DEG;
		refLat *= RAD_TO_DEG;

		bHasGeoReferencingInfo = true;

		double absPosOfCenterXY[3];
		alt = 0.0;
		gaia3d::GeometryUtility::wgs84ToAbsolutePosition(refLon, refLat, alt, absPosOfCenterXY);
		double m[16];
		gaia3d::GeometryUtility::transformMatrixAtAbsolutePosition(absPosOfCenterXY[0], absPosOfCenterXY[1], absPosOfCenterXY[2], m);
		gaia3d::Matrix4 globalTransformMatrix;
		globalTransformMatrix.set(m[0], m[4], m[8], m[12],
			m[1], m[5], m[9], m[13],
			m[2], m[6], m[10], m[14],
			m[3], m[7], m[11], m[15]);
		gaia3d::Matrix4 inverseGlobalTransMatrix = globalTransformMatrix.inverse();

		double px, py, pz;
		for (size_t i = 0; i < meshCount; i++)
		{
			std::vector<gaia3d::Vertex*>& vertices = container[i]->getVertices();
			size_t vertexCount = vertices.size();
			for (size_t j = 0; j < vertexCount; j++)
			{
				gaia3d::Vertex* vertex = vertices[j];
				px = vertex->position.x;
				py = vertex->position.y;
				pz = vertex->position.z;

				pj_transform(pjSrc, pjWgs84, 1, 1, &px, &py, &pz);
				px *= RAD_TO_DEG;
				py *= RAD_TO_DEG;

				double absPosOfTargetPointArray[3];
				gaia3d::GeometryUtility::wgs84ToAbsolutePosition(px, py, pz, absPosOfTargetPointArray);
				gaia3d::Point3D absPosOfTargetPoint;
				absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
				vertex->position = inverseGlobalTransMatrix * absPosOfTargetPoint;
			}
		}
	}

	return true;
}

void ClassicFormatReader::clear()
{
	container.clear();

	textureContainer.clear();
}
