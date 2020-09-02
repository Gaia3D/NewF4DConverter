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

	virtual std::map<std::string, std::vector<gaia3d::TrianglePolyhedron*>>& getMultipleDataContainers();

	virtual std::map<std::string, std::string>& getTextureInfoContainer();

	virtual void setUnitScaleFactor(double factor);

	virtual void setOffset(double x, double y, double z) { offsetX = x; offsetY = y; offsetZ = z; }

	virtual void setYAxisUp(bool bUp) { bYAxisUp = bUp; }

	virtual void setBuildHiararchy(bool bBuild) { bBuildHiararchy = bBuild; }

	virtual bool doesHasGeoReferencingInfo() { return bHasGeoReferencingInfo; }

	virtual bool doesHasAdditionalInfo() { return bHasAdditionalInfo; }

	virtual std::map<std::string, std::string>& getAdditionalInfo() { return additionalInfo; }

	virtual void getGeoReferencingInfo(double& lon, double& lat) { lon = refLon; lat = refLat; }

	virtual void injectOringinInfo(double& lon, double& lat) { lonOrigin = lon; latOrigin = lat; bCoordinateInfoInjected = true; }

	virtual void injectSrsInfo(std::string& epsg) { this->epsg = epsg; bCoordinateInfoInjected = true; }

	virtual void alignToBottomCenter(bool bAlign) { bAlignToBottomCenter = bAlign; }

	virtual void alignToCenter(bool bAlign) { bAlignToCenter = bAlign; }

	virtual std::map<std::string, std::string>& getTemporaryFiles() { return temporaryFiles; }

	virtual bool shouldGeometryBeDesroyedOutside() { return (!container.empty() && !containers.empty()); }

	virtual bool shouldRawDataBeConvertedToMuitiFiles() { return !containers.empty(); }

	virtual std::map<std::string, std::vector<std::string>>& getAncestorsOfEachSubGroup() { return ancestorsOfEachSubGroup; }

	virtual std::map<std::string, bool>& getSplitFilter() { return splitFilter; }

	void TexCoord_Flip_Y();

protected:
	std::vector<gaia3d::TrianglePolyhedron*> container;

	std::map<std::string, std::vector<gaia3d::TrianglePolyhedron*>> containers;

	std::map<std::string, std::string> textureContainer;

	bool bYAxisUp;

	double unitScaleFactor;

	double offsetX, offsetY, offsetZ;

	bool bHasGeoReferencingInfo;

	bool bHasAdditionalInfo;

	double refLon, refLat;

	std::string epsg;

	std::map<std::string, std::string> additionalInfo;

	double lonOrigin, latOrigin;

	bool bCoordinateInfoInjected;

	bool bAlignToBottomCenter;

	bool bAlignToCenter;

	bool bBuildHiararchy;

	std::map<std::string, std::vector<std::string>> ancestorsOfEachSubGroup;

	std::map<std::string, bool> splitFilter;

	std::map<std::string, std::string> temporaryFiles;
	
	std::string makeProj4String();
};

#endif // _READER_H_
