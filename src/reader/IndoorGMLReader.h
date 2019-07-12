#ifndef _INDOORGMLREADER_H_
#define _INDOORGMLREADER_H_

#pragma once

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include "../util/ParserUtil.h"
#include "../geometry/Solid.h"
#include "../geometry/Polygon.h"
#include "../geometry/LinearRing.h"
#include "../geometry/Point3D.h"
#include "../geometry/GeometryManager.h"
#include "../util/GeometryParser.h"
#include "../geometry/Polygon2D.h"
#include "../geometry/Point2D.h"
#include "../geometry/Triangle.h"
#include "../geometry/BoundingBox.h"
#include "aReader.h"

#include "../geometry/TrianglePolyhedron.h"
#include "../util/utility.h"

class IndoorGMLReader : public aReader
{
public:
	IndoorGMLReader();
	virtual ~IndoorGMLReader();
	gaia3d::BoundingBox bb;

public:
	virtual bool readRawDataFile(std::string& filePath);
	virtual void clear();
};

#endif

