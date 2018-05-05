/**
 * Reader Header
 */
#ifndef _READER_H_
#define _READER_H_
#pragma once

#include <string>
#include <vector>
#include <map>

#include "../geometry/TrianglePolyhedron.h"

class Reader abstract
{
public:
	Reader() {}

	virtual ~Reader() {}

public:
	virtual bool readRawDataFile(std::wstring& filePath) = 0;

	virtual void clear() = 0;

	virtual std::vector<gaia3d::TrianglePolyhedron*>& getDataContainer() {return container;}

	virtual std::map<std::wstring, std::wstring>& getTextureInfoContainer() { return textureContainer; }

	virtual void setUnitScaleFactor(double factor) { unitScaleFactor = factor; }

	void TexCoord_Flip_Y() {
		for (int i = 0, count = container.size(); i < count; i++)
		{
			gaia3d::TrianglePolyhedron* polyhedron = container[i];
			polyhedron->TexCoord_Flip_Y();
		}
		/*
		for (int i = 0, count = container.size(); i < count; i++)
		{
		gaia3d::TrianglePolyhedron* polyhedron = container[i];
		if (polyhedron->doesThisHaveTextureCoordinates())
		{
		std::vector<gaia3d::Surface*>& surfaces = polyhedron->getSurfaces();
		gaia3d::Surface* surface;
		gaia3d::Triangle* triangle;
		gaia3d::Vertex** vertices;

		size_t surfaceCount = surfaces.size();
		for (size_t i = 0; i < surfaceCount; i++)
		{
		surface = surfaces[i];
		size_t triangleCount = surface->getTriangles().size();
		for (size_t j = 0; j < triangleCount; j++)
		{
		triangle = surface->getTriangles()[j];
		vertices = triangle->getVertices();
		for (size_t k = 0; k < 3; k++)
		{
		vertices[k]->textureCoordinate[1] = abs(vertices[k]->textureCoordinate[1]);
		}
		}
		}
		}
		}
		*/
	}

protected:
	std::vector<gaia3d::TrianglePolyhedron*> container;

	std::map<std::wstring, std::wstring> textureContainer;

	double unitScaleFactor;
};

#endif // _READER_H_