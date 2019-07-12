#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include <proj_api.h>

#include "IndoorGMLReader.h"

using namespace std;
using namespace xercesc;
using namespace util;
using namespace gaia3d;


vector<gaia3d::Triangle> tessellate(shared_ptr<gaia3d::Polygon> p, gaia3d::TrianglePolyhedron* newMesh) {
	vector<Triangle> result;
	Face f = p->convertFromPolygonToFace();
	gaia3d::Surface* surface = new gaia3d::Surface;
	newMesh->getSurfaces().push_back(surface);

	result = f.getTessellatedTriangles();



	for (int i = 0; i < result.size(); i++) {
		Triangle temp = result.at(i);
		Triangle* pointerTemp = new Triangle();

		pointerTemp->setVertices(temp.getVertices()[0], temp.getVertices()[1], temp.getVertices()[2]);
		pointerTemp->setVertexIndices(temp.getVertexIndices()[0], temp.getVertexIndices()[1], temp.getVertexIndices()[2]);

		surface->getTriangles().push_back(pointerTemp);
	}

	return result;
}


vector<gaia3d::Triangle> tessellate(shared_ptr<gaia3d::Polygon> p, gaia3d::TrianglePolyhedron* newMesh, int count) {
	vector<Triangle> result;
	Face f = p->convertFromPolygonToFace();
	gaia3d::Surface* surface = new gaia3d::Surface;
	newMesh->getSurfaces().push_back(surface);

	result = f.getTessellatedTriangles();

	

	for (int i = 0; i < result.size(); i++) {
		Triangle temp = result.at(i);
		Triangle* pointerTemp;

		pointerTemp->setVertices(temp.getVertices()[0], temp.getVertices()[1], temp.getVertices()[2]);
		pointerTemp->setVertexIndices(temp.getVertexIndices()[0] + count, temp.getVertexIndices()[1] + count, temp.getVertexIndices()[2] + count);

		surface->getTriangles().push_back(pointerTemp);
	}

	return result;
}


vector<gaia3d::Triangle> tessellate(gaia3d::Polygon p, gaia3d::TrianglePolyhedron* newMesh, int count) {
	vector<gaia3d::Triangle> result;
	Face f = p.convertFromPolygonToFace();
	gaia3d::Surface* surface = new gaia3d::Surface;
	newMesh->getSurfaces().push_back(surface);

	result = f.getTessellatedTriangles();

	int localCount = 0;

	for (int i = 0; i < result.size(); i++) {
		Triangle temp = result.at(i);
		Triangle* pointerTemp = new Triangle();
		
		pointerTemp->setVertices(temp.getVertices()[0], temp.getVertices()[1], temp.getVertices()[2]);
		pointerTemp->setVertexIndices(count + temp.getVertexIndices()[0], count +temp.getVertexIndices()[1], count + temp.getVertexIndices()[2]);
		
		surface->getTriangles().push_back(pointerTemp);
	}

	return result;
}

vector<Triangle> tessellate(gaia3d::Solid s, gaia3d::TrianglePolyhedron* newMesh) {
	vector<gaia3d::Triangle> result;

	vector<shared_ptr<gaia3d::Polygon>> exterior = s.getExterior();

	int count = 0;

	for (int i = 0; i < exterior.size(); i++) {
		vector<Triangle>temp = tessellate(*exterior.at(i),newMesh, count);
		result.insert(result.end(), temp.begin(), temp.end());
		count += exterior.at(i)->getVertices().size();
	}

	return result;
}

vector<gaia3d::Triangle> tessellate(shared_ptr<gaia3d::Solid> s, gaia3d::TrianglePolyhedron* newMesh) {
	vector<gaia3d::Triangle> result;

	vector<shared_ptr<gaia3d::Polygon>> exterior = s->getExterior();

	int count = 0; 
	for (int i = 0; i < exterior.size(); i++) {
		vector<Triangle>temp = tessellate(*exterior.at(i), newMesh, count);
		result.insert(result.end(), temp.begin(), temp.end());
		count += exterior.at(i)->getVertices().size();
	}

	return result;
}

IndoorGMLReader::IndoorGMLReader() {}
IndoorGMLReader::~IndoorGMLReader() {}
indoorgml::GeometryManager parseIndoorGeometry(DOMDocument* dom) {
	
	ParserUtil* parseHelper = new util::ParserUtil();
	GeometryParser* gmp = new util::GeometryParser();
	indoorgml::GeometryManager geomManager;
	try {
		DOMElement* rootNode = dom->getDocumentElement();
		//cout << XMLString::transcode(rootNode->getTagName()) << endl;
		DOMNodeList* rootChild = rootNode->getChildNodes();

		DOMNode* primalSpaceFeatures = 0;
		DOMNode* multiLayeredGraph = 0;
		vector<DOMNode*> cellSpaceMember;
		vector<DOMNode*> cellSpaceBoundaryMember;

		//primalSpaceFeatures -> PrimalSpaceFeatures
		primalSpaceFeatures = parseHelper->getNamedNode(rootChild, "core:primalSpaceFeatures");
		primalSpaceFeatures = parseHelper->getNamedNode(primalSpaceFeatures->getChildNodes(), "core:PrimalSpaceFeatures");

		//multiLayeredGraph -> MultiLayeredGraph
		multiLayeredGraph = parseHelper->getNamedNode(rootChild, "core:multiLayeredGraph");
		multiLayeredGraph = parseHelper->getNamedNode(multiLayeredGraph->getChildNodes(), "core:MultiLayeredGraph");

		//cellSpaceMember -> cellSpace & cellSpaceBoundaryMember -> cellSpaceBoundary
		cellSpaceMember = parseHelper->getNamedNodes(primalSpaceFeatures->getChildNodes(), "core:cellSpaceMember");
		cellSpaceBoundaryMember = parseHelper->getNamedNodes(primalSpaceFeatures->getChildNodes(), "core:cellSpaceBoundaryMember");

		vector<DOMNode*>cellspacelist;
		vector<DOMNode*>cellspaceboundarylist;
		for (int i = 0; i < cellSpaceMember.size(); i++) {
			cellspacelist.push_back(parseHelper->getNamedNode(cellSpaceMember.at(i)->getChildNodes(), "core:CellSpace"));
		}

		for (int i = 0; i < cellSpaceBoundaryMember.size(); i++) {
			cellspaceboundarylist.push_back(parseHelper->getNamedNode(cellSpaceBoundaryMember.at(i)->getChildNodes(), "core:CellSpaceBoundary"));
		}
		cout << "Geometry parsing is starting" << endl;
		vector<DOMNode*>solidList;
		vector<DOMNode*>surfaceList;

		for (int i = 0; i < cellspacelist.size(); i++) {
			DOMNode* cellSpace = cellspacelist.at(i);

			for (int j = 0; j < cellSpace->getChildNodes()->getLength(); j++) {
				if (parseHelper->isMatchedNodeName(cellSpace->getChildNodes()->item(j), "core:cellSpaceGeometry")) {
					solidList.push_back(cellSpace->getChildNodes()->item(j)->getChildNodes()->item(1)->getChildNodes()->item(1));
				}

			}
		}

		for (int i = 0; i < cellspaceboundarylist.size(); i++) {
			DOMNode* cellSpaceboundary = cellspaceboundarylist.at(i);

			for (int j = 0; j < cellSpaceboundary->getChildNodes()->getLength(); j++) {
				if (parseHelper->isMatchedNodeName(cellSpaceboundary->getChildNodes()->item(j), "core:cellSpaceBoundaryGeometry")) {
					surfaceList.push_back(cellSpaceboundary->getChildNodes()->item(j)->getChildNodes()->item(1)->getChildNodes()->item(1));
				}

			}
		}
		cout << "Geometry parsing is ended" << endl;
		BoundingBox* b = new BoundingBox();
		for (int i = 0; i < solidList.size(); i++) {
			DOMNamedNodeMap* list = solidList.at(i)->getAttributes();
			//parseHelper->getNamedAttribute(list,"gml:id");
			shared_ptr<Solid> result = gmp->parseSolid(solidList.at(i), b);
			geomManager.addSolid(result);
		}

		for (int i = 0; i < surfaceList.size(); i++) {
			DOMNamedNodeMap* list = surfaceList.at(i)->getAttributes();
			//parseHelper->getNamedAttribute(list, "gml:id");
			shared_ptr<Polygon> result = gmp->parsePolygon(surfaceList.at(i), b);
			geomManager.addPolygon(result);
		}
		geomManager.bb = *b;
	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
	}
	catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
	}
	catch (const SAXParseException& ex) {
		cout << XMLString::transcode(ex.getMessage()) << endl;

	}
	catch (...) {
		cout << "Unexpected Exception \n";
	}
	

	
	return geomManager;
}
bool readIndoorSpace(DOMDocument* dom, std::vector<gaia3d::TrianglePolyhedron*>& container, double& lon, double& lat) {
	

	indoorgml::GeometryManager geomManager = parseIndoorGeometry(dom);
	cout << "start read IndoorGML data" << endl;

	gaia3d::Matrix4* mat;
	std::string wgs84ProjString("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	projPJ pjWgs84 = pj_init_plus(wgs84ProjString.c_str());
	gaia3d::Point3D BBcenterPoint;

	gaia3d::Point3D lowerBound(geomManager.bb.minX, geomManager.bb.minY, geomManager.bb.minZ);
	gaia3d::Point3D upperBound(geomManager.bb.maxX, geomManager.bb.maxY, geomManager.bb.maxZ);
	projPJ pjSrc = pj_init_plus("+init=epsg:4326");
	BBcenterPoint.set((lowerBound.x+upperBound.x)/2.0, (lowerBound.y + upperBound.y) / 2.0, (lowerBound.z + upperBound.z) / 2.0);
	
	lon = BBcenterPoint.x;
	lat = BBcenterPoint.y;

	double alt = BBcenterPoint.z;

	lon *= RAD_TO_DEG;
	lat *= RAD_TO_DEG;

	vector<gaia3d::Triangle>tessellatedResult;
	
	for (int i = 0; i < geomManager.getSolidsCount(); i++) {
		
		gaia3d::TrianglePolyhedron* newMesh = new gaia3d::TrianglePolyhedron();
		vector<gaia3d::Triangle>temp = tessellate(geomManager.getSolid(i),newMesh);
		
		for (int j = 0; j < newMesh->getSurfaces().size(); j++) {
			gaia3d::Surface* surface = newMesh->getSurfaces().at(j);
			for (int k = 0; k < surface->getTriangles().size(); k++) {
				gaia3d::Triangle* original;
				original = surface->getTriangles().at(k);
				for (int n = 0; n < 3; n++) {
					Vertex* tempVertex = original->getVertices()[n];
					double px, py, pz;
					px = tempVertex->getPosition().x;
					py = tempVertex->getPosition().y;
					pz = tempVertex->getPosition().z;					
					
					pj_transform(pjSrc, pjWgs84, 1, 1, &px, &py, &pz);
					px *= RAD_TO_DEG;
					py *= RAD_TO_DEG;

					double absPosOfTargetPointArray[3];
					gaia3d::GeometryUtility::wgs84ToAbsolutePosition(px, py, pz, absPosOfTargetPointArray);
					gaia3d::Point3D absPosOfTargetPoint;
					absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
					tempVertex->position = absPosOfTargetPoint;
				}
			}
		}
		
		container.push_back(newMesh);
		//tessellatedResult.insert(tessellatedResult.end(), temp.begin(), temp.end());
	}

	for (int i = 0; i < geomManager.getPolygonsCount(); i++) {
		gaia3d::TrianglePolyhedron* newMesh = new gaia3d::TrianglePolyhedron();
		vector<Triangle> temp = tessellate(geomManager.getPolygon(i),newMesh);
		for (int j = 0; j < newMesh->getSurfaces().size(); j++) {
			gaia3d::Surface* surface = newMesh->getSurfaces().at(j);
			for (int k = 0; k < surface->getTriangles().size(); k++) {
				gaia3d::Triangle* original;
				original = surface->getTriangles().at(k);
				for (int n = 0; n < 3; n++) {
					Vertex* tempVertex = original->getVertices()[n];
					double px, py, pz;
					px = tempVertex->getPosition().x;
					py = tempVertex->getPosition().y;
					pz = tempVertex->getPosition().z;

					pj_transform(pjSrc, pjWgs84, 1, 1, &px, &py, &pz);
					px *= RAD_TO_DEG;
					py *= RAD_TO_DEG;

					double absPosOfTargetPointArray[3];
					gaia3d::GeometryUtility::wgs84ToAbsolutePosition(px, py, pz, absPosOfTargetPointArray);
					gaia3d::Point3D absPosOfTargetPoint;
					absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
					tempVertex->position = absPosOfTargetPoint;
				}
			}
		}
		//tessellatedResult.insert(tessellatedResult.end(), temp.begin(), temp.end());


		container.push_back(newMesh);
	}
	return true;

}
void IndoorGMLReader::clear()
{
	container.clear();

	//textureContainer.clear();
}
bool IndoorGMLReader::readRawDataFile(std::string& filePath) {
	try {
		const char * xmlFile = filePath.c_str();
		XMLPlatformUtils::Initialize();
		XercesDOMParser* parser = new XercesDOMParser();
		ParserUtil* parseHelper = new util::ParserUtil();
		GeometryParser* gmp = new util::GeometryParser();
		
		ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
		parser->setErrorHandler(errHandler);
		parser->setIncludeIgnorableWhitespace(false);
		parser->setDoSchema(true);
		//const char * xmlFile = "../samples/seouluniv21centry.gml";
		parser->parse(xmlFile);

		cout << xmlFile << ": parse OK" << endl;
		DOMDocument* dom = parser->getDocument();	
		cout << "Now processing start" << endl;
		readIndoorSpace(dom, container, refLon, refLat);

		delete parser;
		delete errHandler;
		delete parseHelper;
		XMLPlatformUtils::Terminate();
	

	}
	catch (const XMLException& toCatch) {
		char* message = XMLString::transcode(toCatch.getMessage());
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return false;
	}
	catch (const DOMException& toCatch) {
		char* message = XMLString::transcode(toCatch.msg);
		cout << "Exception message is: \n" << message << "\n";
		XMLString::release(&message);
		return false;
	}
	catch (const SAXParseException& ex) {
		cout << XMLString::transcode(ex.getMessage()) << endl;

	}
	catch (...) {
		cout << "Unexpected Exception \n";
		return false;
	}


	return true;

}