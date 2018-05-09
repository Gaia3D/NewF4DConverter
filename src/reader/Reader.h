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

class Reader
{
public:
	Reader();

	virtual ~Reader();

public:
	virtual bool readRawDataFile(std::string& filePath) = 0;

	virtual void clear() = 0;

	virtual std::vector<gaia3d::TrianglePolyhedron*>& getDataContainer();

	virtual std::map<std::string, std::string>& getTextureInfoContainer();

	virtual void setUnitScaleFactor(double factor);

	void TexCoord_Flip_Y() ;

protected:
	std::vector<gaia3d::TrianglePolyhedron*> container;

	std::map<std::string, std::string> textureContainer;

	double unitScaleFactor;
};

#endif // _READER_H_
