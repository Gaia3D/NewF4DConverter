/**
 * Implementation of the TrianglePolyhedron class
 */
#include "TrianglePolyhedron.h"
#include "predefinition.h"
#include "../converter/LogWriter.h"

#include <iostream>

namespace gaia3d
{

	TrianglePolyhedron::TrianglePolyhedron()
	{
		refInfo.model = NULL;
		refInfo.mat.identity();
		refInfo.modelIndex = MaxUnsignedLong;

		hasNormals = hasTextureCoordinates = false;

		colorMode = NoColor;
		singleColor = 0UL;

		id = MaxUnsignedLong;
	}


	TrianglePolyhedron::~TrianglePolyhedron()
	{
		size_t vertexCount = vertices.size();
		for(size_t i = 0; i < vertexCount; i++)
			delete vertices[i];
		vertices.clear();

		size_t surfaceCount = surfaces.size();
		for(size_t i = 0; i < surfaceCount; i++)
			delete surfaces[i];
		surfaces.clear();

		size_t vboCount = vbos.size();
		for(size_t i = 0; i < vboCount; i++)
			delete vbos[i];
		vbos.clear();
	}

	void TrianglePolyhedron::addStringAttribute(std::string keyString, std::string valueString)
	{
		stringAttributes.insert(std::map<std::string, std::string>::value_type(keyString, valueString));
	}

	bool TrianglePolyhedron::doesStringAttributeExist(std::string keyString)
	{
		if (stringAttributes.find(keyString) == stringAttributes.end())
			return false;
		
		return true;
	}

	std::string TrianglePolyhedron::getStringAttribute(std::string keyString)
	{
		if (!doesStringAttributeExist(keyString))
			return std::string();

		return stringAttributes[keyString];
	}

	bool TrianglePolyhedron::doesHaveAnyExteriorSurface()
	{
		size_t surfaceCount = surfaces.size();
		for(size_t i = 0; i < surfaceCount; i++)
		{
			if(surfaces[i]->isExterior())
				return true;
		}

		return false;
	}

	void TrianglePolyhedron::TexCoord_Flip_Y()
	{
		if (hasTextureCoordinates)
		{
			gaia3d::Surface* surface;
			gaia3d::Triangle* triangle;
			gaia3d::Vertex** vertices;
			gaia3d::Vbo* vbo;

			size_t surfaceCount = surfaces.size();
			for (size_t i = 0; i < surfaceCount; i++)
			{
				surface = surfaces[i];
				size_t triangleCount = surface->getTriangles().size();
				LogWriter::getLogWriter()->addContents("Total Count of triangle : " + std::to_string(triangleCount), true);
				for (size_t j = 0; j < triangleCount; j++)
				{
					triangle = surface->getTriangles()[j];
					vertices = triangle->getVertices();
					for (size_t k = 0; k < 3; k++)
					{
						vertices[k]->textureCoordinate[1] = 1.0 - vertices[k]->textureCoordinate[1];
						//std::cout << k <<":"<< vertices[k]->textureCoordinate[0] << ":" << vertices[k]->textureCoordinate[1] << std::endl;
						LogWriter::getLogWriter()->addContents(std::to_string(k) + ":" + std::to_string(vertices[k]->textureCoordinate[0]) + ":" + std::to_string(vertices[k]->textureCoordinate[1]), true);
					}
				}
			}

			size_t vboCount = vbos.size();
			if (vboCount == 0)	return;
			LogWriter::getLogWriter()->addContents("Total Count of VBO : " + std::to_string(vboCount), true);
			for (size_t i = 0; i < vboCount; i++)
			{
				vbo = vbos[i];
				size_t vertexCount = vbo->vertices.size();
				for (size_t j = 0; j < vertexCount; j++)
				{
					vbo->vertices[j]->textureCoordinate[1] = 1.0 - vbo->vertices[j]->textureCoordinate[1];
					LogWriter::getLogWriter()->addContents(std::to_string(j) + ":" + std::to_string(vbo->vertices[j]->textureCoordinate[0]) + ":" + std::to_string(vbo->vertices[j]->textureCoordinate[1]), true);
				}
			}
		}
	}
}