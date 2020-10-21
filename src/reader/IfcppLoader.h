/**
* IfcppLoader Header
*/
#ifndef _IFCPPLOADER_H_
#define _IFCPPLOADER_H_
#pragma once

#include <string>

#include "IfcLoader.h"

#include <ifcpp/model/StatusCallback.h>
#include <ifcpp/model/BuildingModel.h>
#include <ifcpp/geometry/Carve/GeometryConverter.h>
#include <ifcpp/reader/ReaderSTEP.h>
#include <ifcpp/IFC4/include/IfcSite.h>
#include <ifcpp/IFC4/include/IfcSpace.h>
#include <ifcpp/IFC4/include/IfcGloballyUniqueId.h>

// for geometry structure
#include <ifcpp/IFC4/include/IfcBuilding.h>
#include <ifcpp/IFC4/include/IfcBuildingStorey.h>
#include <ifcpp/IFC4/include/IfcFooting.h>
#include <ifcpp/IFC4/include/IfcColumn.h>
#include <ifcpp/IFC4/include/IfcSlab.h>
#include <ifcpp/IFC4/include/IfcBeam.h>
#include <ifcpp/IFC4/include/IfcWallStandardCase.h>

// for property value
#include <ifcpp/IFC4/include/IfcAreaMeasure.h>
#include <ifcpp/IFC4/include/IfcBoolean.h>
#include <ifcpp/IFC4/include/IfcIdentifier.h>
#include <ifcpp/IFC4/include/IfcInteger.h>
#include <ifcpp/IFC4/include/IfcLabel.h>
#include <ifcpp/IFC4/include/IfcText.h>
#include <ifcpp/IFC4/include/IfcLengthMeasure.h>
#include <ifcpp/IFC4/include/IfcPlaneAngleMeasure.h>
#include <ifcpp/IFC4/include/IfcPositiveLengthMeasure.h>
#include <ifcpp/IFC4/include/IfcReal.h>
#include <ifcpp/IFC4/include/IfcVolumeMeasure.h>

// for unit
#include <ifcpp/IFC4/include/IfcDerivedUnit.h>
#include <ifcpp/IFC4/include/IfcNamedUnit.h>
#include <ifcpp/IFC4/include/IfcMonetaryUnit.h>
#include <ifcpp/IFC4/include/IfcUnitEnum.h>

#include <json/json.h>

#include "../util/StringUtility.h"

class IfcppLoader : public IfcLoader
{
public:
	IfcppLoader();
	virtual ~IfcppLoader();

public:
	virtual bool loadIfcFile(std::wstring& filePath);

	virtual void setVertexReductionMode(bool bOn);

	virtual size_t getPolyhedronCount();
	virtual float* getRepresentativeColor(size_t polyhedronIndex);
	virtual void getGuid(size_t polyhedronIndex, wchar_t buffer[]);
	virtual size_t getVertexCount(size_t polyhedronIndex);
	virtual double* getVertexPositions(size_t polyhedronIndex);
	virtual size_t getSurfaceCount(size_t polyhedronIndex);
	virtual size_t getTrialgleCount(size_t polyhedronIndex, size_t surfaceIndex);
	virtual size_t* getTriangleIndices(size_t polyhedronIndex, size_t surfaceIndex);

	virtual size_t getStoryCount();
	virtual size_t getStoryDivisionCount(size_t storyIndex);
	virtual size_t getPolyhedronCount(size_t storyIndex, size_t divisionIndex);
	virtual float* getRepresentativeColor(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex);
	virtual void getGuid(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex, wchar_t buffer[]);
	virtual size_t getVertexCount(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex);
	virtual double* getVertexPositions(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex);
	virtual size_t getSurfaceCount(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex);
	virtual size_t getTrialgleCount(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex, size_t surfaceIndex);
	virtual size_t* getTriangleIndices(size_t storyIndex, size_t divisionIndex, size_t polyhedronIndex, size_t surfaceIndex);

	virtual bool loadOnlyPropertiesFromIfc(std::wstring& filePath);
	virtual void setAttributesExtraction(bool bOn);
	virtual std::string getObjectAttributes();
	virtual std::string getProjectAttributes();

#ifdef TMPTEST
	virtual size_t getAttributeTypeCount();
	virtual std::string getAttributeType(size_t i);
	std::vector<std::string> attributeTypes;
	std::map<std::string, std::string> attributeMap;
#endif

private:

	void loadProjectAttributes();
	void loadObjectAttributes(shared_ptr<IfcProduct> ifcProduct, Json::Value& root);
	bool checkIfPropertiesCanBeExtracted(std::string className);
	//bool checkIfPropertiesCanBeExtracted(IfcPPEntityEnum ppEnum);
	void parsePropertySingleValue(Json::Value& valueObject, shared_ptr<IfcValue> value);

	bool bAttributesExtraction;
	bool bVertexReduction;

	struct Surface
	{
		size_t triangleCount;
		size_t* triangleIndices;
	};

	struct Polyhedron
	{
		size_t vertexCount;
		double* vertices;
		float color[4];
		std::vector<Surface*> surfaces;
		std::wstring guid;
	};

	std::vector<Polyhedron*> polyhedrons;

	std::vector<std::vector<std::vector<Polyhedron*>>> stories;

	std::vector<Polyhedron*> objectsOutsideStory;

	Json::Value objectPropertyRoot;
	Json::Value projectPropertyRoot;
};

IfcLoader* createIfcLoader();
void destroyIfcLoader(IfcLoader* aLoader);

#endif // _IFCPPLOADER_H_