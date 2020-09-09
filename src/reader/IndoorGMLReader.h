#pragma once

#ifndef _INDOORGMLREADER_H_
#define _INDOORGMLREADER_H_


#include "Reader.h"

#include "../geometry/TrianglePolyhedron.h"

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

using namespace xercesc;

class GeometryManager;

class IndoorGMLReader : public Reader
{
public:
	IndoorGMLReader();

	virtual ~IndoorGMLReader();

	virtual bool readRawDataFile(std::string& filePath);

	virtual void clear();
	
private:
	GeometryManager parseIndoorGeometry(DOMDocument* dom, std::string filePath);
	bool readIndoorGML(DOMDocument* dom, std::string filePath, std::map<std::string, bool> splitFilter, std::vector<gaia3d::TrianglePolyhedron*>& container, double& lon, double& lat);

};

#endif

