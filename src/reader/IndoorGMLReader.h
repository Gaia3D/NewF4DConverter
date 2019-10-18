#pragma once

#ifndef _INDOORGMLREADER_H_
#define _INDOORGMLREADER_H_


#include "Reader.h"

#include "../geometry/TrianglePolyhedron.h"
#include <../xerces/src/xercesc/parsers/XercesDOMParser.hpp>
#include <../xerces/src/xercesc/dom/DOM.hpp>
#include <../xerces/src/xercesc/sax/HandlerBase.hpp>
#include <../xerces/src/xercesc/util/XMLString.hpp>
#include <../xerces/src/xercesc/util/PlatformUtils.hpp>

using namespace xercesc;

class IndoorGMLReader : public Reader
{
public:
	IndoorGMLReader();
	virtual ~IndoorGMLReader();
	gaia3d::BoundingBox bb;

public:
	bool readIndoorSpace(DOMDocument* dom, std::vector<gaia3d::TrianglePolyhedron*>& container, double& lon, double& lat);
	virtual void clear();
	virtual bool readRawDataFile(std::string& filePath);
};

#endif

