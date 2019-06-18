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

	virtual void setOffset(double x, double y, double z) { offsetX = x; offsetY = y; offsetZ = z; }

	virtual void setYAxisUp(bool bUp) { bYAxisUp = bUp; }

	virtual bool doesHasGeoReferencingInfo() { return bHasGeoReferencingInfo; }

	virtual void getGeoReferencingInfo(double& lon, double& lat) { lon = refLon; lat = refLat; }

	virtual void injectOringinInfo(double& lon, double& lat) { lonOrigin = lon; latOrigin = lat; bCoordinateInfoInjected = true; }

	virtual void injectSrsInfo(std::string& epsg) { this->epsg = epsg; bCoordinateInfoInjected = true; }

	virtual std::map<std::string, std::string>& getTemporaryFiles() { return temporaryFiles; }

	void TexCoord_Flip_Y() ;

protected:
	std::vector<gaia3d::TrianglePolyhedron*> container;

	std::map<std::string, std::string> textureContainer;

	bool bYAxisUp;

	double unitScaleFactor;

	double offsetX, offsetY, offsetZ;

	bool bHasGeoReferencingInfo;

	double refLon, refLat;

	std::string epsg;

	double lonOrigin, latOrigin;

	bool bCoordinateInfoInjected;

	std::map<std::string, std::string> temporaryFiles;
	
	std::string makeProj4String();
};

#endif // _READER_H_
