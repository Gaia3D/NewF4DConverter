/**
* IfcLoader Header
*/
#ifndef _IFCLOADER_H_
#define _IFCLOADER_H_
#pragma once

#include <string>

// Abstract Class
class IfcLoader 
{
public:
	virtual bool loadIfcFile(std::wstring& filePath) = 0;

	virtual void setVertexReductionMode(bool bOn) = 0;

	virtual size_t getPolyhedronCount() = 0;
	virtual float* getRepresentativeColor(size_t polyhedronIndex) = 0;
	virtual std::wstring getGuid(size_t polyhedronIndex) = 0;
	virtual size_t getVertexCount(size_t polyhedronIndex) = 0;
	virtual double* getVertexPositions(size_t polyhedronIndex) = 0;
	virtual size_t getSurfaceCount(size_t polyhedronIndex) = 0;
	virtual size_t getTrialgleCount(size_t polyhedronIndex, size_t surfaceIndex) = 0;
	virtual size_t* getTriangleIndices(size_t polyhedronIndex, size_t surfaceIndex) = 0;

	virtual bool loadOnlyPropertiesFromIfc(std::wstring& filePath) = 0;
	virtual void setAttributesExtraction(bool bOn) = 0;
	virtual std::string getObjectAttributes() = 0;
	virtual std::string getProjectAttributes() = 0; // not available now

#ifdef TMPTEST
	virtual size_t getAttributeTypeCount() = 0;
	virtual std::string getAttributeType(size_t i) = 0;
#endif
};

#endif // _IFCLOADER_H_