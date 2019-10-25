
#include "IndoorGMLReader.h"
#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <proj_api.h>

#include "../geometry/ColorU4.h"
#include "../converter/ConverterManager.h"
#include "../geometry/Matrix4.h"

#include "../geometry/Point3D.h"

#include "../geometry/Triangle.h"
#include "../geometry/BoundingBox.h"
#include "../util/GeometryUtility.h"
using namespace std;
using namespace xercesc;
using namespace gaia3d;
using namespace std;


IndoorGMLReader::IndoorGMLReader() {}
IndoorGMLReader::~IndoorGMLReader() {}

class ParserUtil {
public:
	ParserUtil();
	~ParserUtil();

	bool isTextNode(DOMNode* node);
	bool isMatchedNodeName(DOMNode* node, string nodeType);
	bool hasNamedChild(DOMNode* node, string s);
	bool hasNamedAttribute(DOMNode* node, string s);
	string changeXMLCh2str(const XMLCh* x);
	string getNamedAttribute(DOMNamedNodeMap* list, string s);

	DOMNode* getNamedNode(DOMNodeList* list, string s);
	DOMNode* getNamedNode(vector<DOMNode*> list, string s);
	bool isNamedNodesExist(DOMNodeList* list, string s);
	vector<DOMNode*> getNamedNodes(DOMNodeList* list, string s);
	vector<DOMNode*> getNamedNodes(vector<DOMNode*> list, string s);
};
ParserUtil::ParserUtil() {};
ParserUtil::~ParserUtil() {};
bool ParserUtil::isTextNode(DOMNode* node) {
	return (node->getNodeType() == DOMNode::TEXT_NODE);
}
bool ParserUtil::isMatchedNodeName(DOMNode* node, string nodeType) {
	if (node == NULL) {
		return false;
	}
	return(!strcmp(XMLString::transcode(node->getNodeName()), nodeType.c_str()));
}
bool ParserUtil::hasNamedChild(DOMNode* node, string s) {
	if (node == NULL) {
		return false;
	}
	if (node->hasChildNodes()) {
		for (int i = 0; i < node->getChildNodes()->getLength(); i++) {
			if (isMatchedNodeName(node->getChildNodes()->item(i), s)) {
				return true;
			}
		}
	}
	return false;
}
bool ParserUtil::hasNamedAttribute(DOMNode* node, string s) {
	if (node == NULL) {
		return false;
	}
	if (node->hasAttributes()) {
		for (int i = 0; i < node->getChildNodes()->getLength(); i++) {
			if (isMatchedNodeName(node->getChildNodes()->item(i), s)) {
				return true;
			}
		}
	}
	return false;
}
string ParserUtil::changeXMLCh2str(const XMLCh* x) {
	return XMLString::transcode(x);
}
string ParserUtil::getNamedAttribute(DOMNamedNodeMap* list, string s) {
	string result;
	for (int j = 0; j < list->getLength(); j++) {
		if (isTextNode(list->item(j))) {
			continue;
		}
		if (isMatchedNodeName(list->item(j), s.c_str())) {
			result = changeXMLCh2str(list->item(j)->getNodeValue());
			//cout << result << endl;
			break;
		}
	}
	return result;
}
DOMNode* ParserUtil::getNamedNode(DOMNodeList* list, string s) {
	DOMNode* result = 0;
	for (int i = 0; i < list->getLength(); i++) {
		if (!isTextNode(list->item(i))) {
			if (isMatchedNodeName(list->item(i), s)) {
				result = list->item(i);
				break;
			}
		}
	}
	return result;
}

DOMNode* ParserUtil::getNamedNode(vector<DOMNode*> list, string s) {
	DOMNode* result = 0;
	for (int i = 0; i < list.size(); i++) {
		if (!isTextNode(list.at(i))) {
			if (isMatchedNodeName(list.at(i), s)) {
				result = list.at(i);
				break;
			}
		}
	}
	return result;
}

vector<DOMNode*> ParserUtil::getNamedNodes(DOMNodeList* list, string s) {
	vector<DOMNode*>result;
	for (int i = 0; i < list->getLength(); i++) {
		if (!isTextNode(list->item(i))) {
			if (isMatchedNodeName(list->item(i), s)) {
				result.push_back(list->item(i));

			}
		}
	}
	return result;
}

bool ParserUtil::isNamedNodesExist(DOMNodeList* list, string s) {
	bool isExist = false;
	for (int i = 0; i < list->getLength(); i++) {
		if (!isTextNode(list->item(i))) {
			if (isMatchedNodeName(list->item(i), s)) {
				isExist = true;
				break;
			}
		}
	}
	return isExist;
}

vector<DOMNode*> ParserUtil::getNamedNodes(vector<DOMNode*> list, string s) {
	vector<DOMNode*>result;
	for (int i = 0; i < list.size(); i++) {
		if (!isTextNode(list.at(i))) {
			if (isMatchedNodeName(list.at(i), s)) {
				result.push_back(list.at(i));

			}
		}
	}
	return result;
}

class IndoorGMLLinearRing {
public:
	IndoorGMLLinearRing(const std::string& _id) { id = _id; }

	bool isExterior() const;

	unsigned int size() const;

	const std::vector<Point3D>& getVertices() const;

	std::vector<Point3D>& getVertices();
	void setVertices(std::vector<Point3D> vertices);

	void addVertex(const Point3D& v);

	Point3D computeNormal() const;

	void forgetVertices();

protected:
	bool m_exterior;
	string id;
	std::vector<Point3D> m_vertices;

};

unsigned int IndoorGMLLinearRing::size() const
{
	return m_vertices.size();
}

void IndoorGMLLinearRing::addVertex(const Point3D& v)
{
	m_vertices.push_back(v);
}

std::vector<Point3D>& IndoorGMLLinearRing::getVertices()
{
	return m_vertices;
}

void IndoorGMLLinearRing::setVertices(std::vector<Point3D> vertices)
{
	m_vertices = vertices;
}

const std::vector<Point3D>& IndoorGMLLinearRing::getVertices() const
{
	return m_vertices;
}

void IndoorGMLLinearRing::forgetVertices()
{
	m_vertices.clear();
}

class IndoorGMLPolygon
{

public:


	// Get the vertices
	const std::vector<Point3D>& getVertices() const;
	std::vector<Point3D>& getVertices();

	// Get the indices
	const std::vector<unsigned int>& getIndices() const;

	bool negNormal() const;
	void setNegNormal(bool negNormal);

	void addRing(IndoorGMLLinearRing*);

	void setExterior(shared_ptr<IndoorGMLLinearRing> l);
	void setInterior(std::vector<shared_ptr<IndoorGMLLinearRing>> l) { m_interiorRings = l; }
	shared_ptr<IndoorGMLLinearRing> getExterior();
	vector<shared_ptr<IndoorGMLLinearRing>> getInterior() { return m_interiorRings; }
	IndoorGMLPolygon(string _id) { id = _id; }

	virtual ~IndoorGMLPolygon();

protected:

	string id;
	std::vector<Point3D> m_vertices;
	std::vector<unsigned int> m_indices;

	std::shared_ptr<IndoorGMLLinearRing> m_exteriorRing;
	std::vector<std::shared_ptr<IndoorGMLLinearRing> > m_interiorRings;

	bool m_negNormal;
	bool m_finished;

	//std::shared_ptr<Logger> m_logger;
};
const std::vector<Point3D>& IndoorGMLPolygon::getVertices() const
{
	return m_vertices;
}

std::vector<Point3D>& IndoorGMLPolygon::getVertices()
{
	return m_vertices;
}

const std::vector<unsigned int>& IndoorGMLPolygon::getIndices() const
{
	return m_indices;
}

bool IndoorGMLPolygon::negNormal() const
{
	return m_negNormal;
}
void IndoorGMLPolygon::setExterior(shared_ptr<IndoorGMLLinearRing> l) {
	m_exteriorRing = l;
	for (int i = 0; i < l->getVertices().size(); i++) {
		m_indices.push_back(i);
	}
}
shared_ptr<IndoorGMLLinearRing> IndoorGMLPolygon::getExterior() {
	return m_exteriorRing;
}
void IndoorGMLPolygon::setNegNormal(bool negNormal)
{
	m_negNormal = negNormal;
}

IndoorGMLPolygon::~IndoorGMLPolygon()
{
}

class IndoorGMLSolid
{
public:
	BoundingBox bb;

	~IndoorGMLSolid() {}

	bool IndoorGMLSolid::hasExterior() {
		if (exterior.size() == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	bool IndoorGMLSolid::hasInterior() {
		if (interior.size() == 0) {
			return false;
		}
		else {
			return true;
		}
	}
	//const IndoorGMLSolid& IndoorGMLSolid::getExterior() const {	}
	vector<shared_ptr<IndoorGMLPolygon>> IndoorGMLSolid::getExterior() {
		return exterior;
	}
	std::vector<std::shared_ptr<IndoorGMLSolid>> IndoorGMLSolid::getInterior() {
		return interior;
	}
	void IndoorGMLSolid::addInterior(std::shared_ptr<IndoorGMLSolid> s) {
		interior.push_back(s);
	}
	//void deleteInterior(){}
	void IndoorGMLSolid::setExterior(vector<std::shared_ptr<IndoorGMLPolygon>> s) {
		exterior = s;
	}

	IndoorGMLSolid::IndoorGMLSolid(const std::string& _id) { id = _id; }

protected:

	bool m_finished;
	unsigned int m_lod;
	string id;
	std::vector<std::shared_ptr<IndoorGMLPolygon> > exterior;
	std::vector<std::shared_ptr<IndoorGMLSolid>> interior;
};




class GeometryManager
{
public:
	gaia3d::BoundingBox bb;

	size_t getIndoorGMLSolidsCount() const;
	std::shared_ptr<IndoorGMLSolid> getIndoorGMLSolid(size_t i);
	//std::shared_ptr<const gaia3d::IndoorGMLSolid> getIndoorGMLSolid(size_t i) const;
	//std::shared_ptr<gaia3d::IndoorGMLSolid> getIndoorGMLSolid(string id);

	size_t getIndoorGMLPolygonsCount() const;
	std::shared_ptr<IndoorGMLPolygon> getIndoorGMLPolygon(size_t i);
	//std::shared_ptr<const gaia3d::IndoorGMLPolygon> getIndoorGMLPolygon(size_t i) const;
	;
	void addIndoorGMLPolygon(std::shared_ptr<IndoorGMLPolygon>);
	void addIndoorGMLSolid(std::shared_ptr<IndoorGMLSolid>);

	~GeometryManager();
	GeometryManager();

protected:

	bool m_finished;
	std::vector<std::shared_ptr<IndoorGMLSolid> > m_IndoorGMLSolids;
	std::vector<std::shared_ptr<IndoorGMLPolygon> > m_IndoorGMLPolygons;
};

GeometryManager::GeometryManager()
{
}


size_t GeometryManager::getIndoorGMLPolygonsCount() const
{
	return m_IndoorGMLPolygons.size();
}

std::shared_ptr<IndoorGMLPolygon> GeometryManager::getIndoorGMLPolygon(size_t i)
{
	return m_IndoorGMLPolygons.at(i);
}

std::shared_ptr<IndoorGMLSolid> GeometryManager::getIndoorGMLSolid(size_t i) {
	return m_IndoorGMLSolids.at(i);
}

size_t GeometryManager::getIndoorGMLSolidsCount() const {
	return m_IndoorGMLSolids.size();
}


GeometryManager::~GeometryManager()
{
}

void GeometryManager::addIndoorGMLPolygon(std::shared_ptr<IndoorGMLPolygon> p)
{
	m_IndoorGMLPolygons.push_back(p);
}

void GeometryManager::addIndoorGMLSolid(std::shared_ptr<IndoorGMLSolid> s) {
	m_IndoorGMLSolids.push_back(s);
}


class GeometryParser {
public:
	shared_ptr<IndoorGMLLinearRing> parseIndoorGMLLinearRing(xercesc::DOMNode* l, gaia3d::BoundingBox* b);

	shared_ptr<IndoorGMLPolygon> parseIndoorGMLPolygon(xercesc::DOMNode* p, gaia3d::BoundingBox* b);

	shared_ptr<IndoorGMLSolid> parseIndoorGMLSolid(xercesc::DOMNode* s, gaia3d::BoundingBox* b);

};
shared_ptr<IndoorGMLLinearRing> GeometryParser::parseIndoorGMLLinearRing(DOMNode* l, gaia3d::BoundingBox* b) {
	ParserUtil* parseHelper = new ParserUtil();
	shared_ptr<IndoorGMLLinearRing> result = shared_ptr<IndoorGMLLinearRing>(new IndoorGMLLinearRing(parseHelper->getNamedAttribute(l->getAttributes(), "gml:id")));
	if (parseHelper->hasNamedChild(l, "gml:pos")) {
		double arr[3];
		int count = 0;
		vector<gaia3d::Point3D>pointList;
		for (int i = 0; i < l->getChildNodes()->getLength(); i++) {
			if (!parseHelper->isTextNode(l->getChildNodes()->item(i))) {
				//cout << parseHelper->changeXMLCh2str(l->getChildNodes()->item(i)->getTextContent()) << endl;
				string pointvalue = parseHelper->changeXMLCh2str(l->getChildNodes()->item(i)->getTextContent());
				stringstream ss(pointvalue);
				ss >> arr[0] >> arr[1] >> arr[2];
				b->addPoint(arr[0], arr[1], arr[2]);
				//cout << arr[0] << " " << arr[1] << " " << arr[2] << endl;
				gaia3d::Point3D newPoint;
				newPoint.set(arr[0], arr[1], arr[2]);
				pointList.push_back(newPoint);
			}

		}
		result->setVertices(pointList);

	}
	else if (parseHelper->hasNamedChild(l, "gml:posList")) {
		//TODO: 
		double arr[3];
		int count = 0;
		vector<gaia3d::Point3D>pointList;
		vector<double>singleAxisValues;
		//get single line of the posList 
		for (int i = 0; i < l->getChildNodes()->getLength(); i++) {
			if (!parseHelper->isTextNode(l->getChildNodes()->item(i))) {
				string posListValue = parseHelper->changeXMLCh2str(l->getChildNodes()->item(i)->getTextContent());
				stringstream ss(posListValue);
				for (double s; ss >> s;) {
					singleAxisValues.push_back(s);
				}
				break;
			}
		}
		for (int i = 0; i < singleAxisValues.size(); i += 3) {
			arr[0] = singleAxisValues.at(i);
			arr[1] = singleAxisValues.at(i + 1);
			arr[2] = singleAxisValues.at(i + 2);
			gaia3d::Point3D newPoint;
			newPoint.set(arr[0], arr[1], arr[2]);
			pointList.push_back(newPoint);
		}
		result->setVertices(pointList);
	}
	result->getVertices().pop_back();
	return result;
}

shared_ptr<IndoorGMLPolygon> GeometryParser::parseIndoorGMLPolygon(DOMNode* p, gaia3d::BoundingBox* b) {
	ParserUtil* parseHelper = new ParserUtil();

	shared_ptr<IndoorGMLPolygon> result = shared_ptr<IndoorGMLPolygon>(new IndoorGMLPolygon(parseHelper->getNamedAttribute(p->getAttributes(), "gml:id")));
	DOMNode* exterior = parseHelper->getNamedNode(p->getChildNodes(), "gml:exterior");
	if (parseHelper->hasNamedChild(exterior, "gml:LinearRing")) {
		result->setExterior(parseIndoorGMLLinearRing(parseHelper->getNamedNode(exterior->getChildNodes(), "gml:LinearRing"), b));
	}
	else if (parseHelper->isMatchedNodeName(exterior, "gml:LineString")) {}
	return result;
}


shared_ptr<IndoorGMLSolid> GeometryParser::parseIndoorGMLSolid(DOMNode* s, gaia3d::BoundingBox* b) {
	ParserUtil* parseHelper = new ParserUtil();
	shared_ptr<IndoorGMLSolid> result = shared_ptr<IndoorGMLSolid>(new IndoorGMLSolid(parseHelper->getNamedAttribute(s->getAttributes(), "gml:id")));
	DOMNode* exterior = parseHelper->getNamedNode(s->getChildNodes(), "gml:exterior");
	DOMNode* shell = parseHelper->getNamedNode(exterior->getChildNodes(), "gml:Shell");
	vector<DOMNode*> surfaceMember = parseHelper->getNamedNodes(shell->getChildNodes(), "gml:surfaceMember");
	vector<DOMNode*> polygonlist;
	//get polygon
	for (int i = 0; i < surfaceMember.size(); i++) {
		DOMNode* p = parseHelper->getNamedNode(surfaceMember.at(i)->getChildNodes(), "gml:Polygon");
		polygonlist.push_back(p);
	}
	//parse polygon
	vector<shared_ptr<IndoorGMLPolygon>>parsedPolygon;
	for (int i = 0; i < surfaceMember.size(); i++) {
		parsedPolygon.push_back(parseIndoorGMLPolygon(polygonlist.at(i), b));
	}
	result->setExterior(parsedPolygon);

	return result;
}
GeometryManager parseIndoorGeometry(DOMDocument* dom) {


	ParserUtil* parseHelper = new ParserUtil();
	GeometryParser* gmp = new GeometryParser();
	GeometryManager geomManager;
	try {
		DOMElement* rootNode = dom->getDocumentElement();
		//cout << XMLString::transcode(rootNode->getTagName()) << endl;
		DOMNodeList* rootChild = rootNode->getChildNodes();

		DOMNode* primalSpaceFeatures = 0;
		DOMNode* PrimalSpaceFeatures = 0;
		DOMNode* multiLayeredGraph = 0;
		DOMNode* MultiLayeredGraph = 0;
		vector<DOMNode*> cellSpaceMember;
		vector<DOMNode*> cellSpaceBoundaryMember;
		BoundingBox* b = new BoundingBox();

		//primalSpaceFeatures -> PrimalSpaceFeatures

		string frontTag = "core:";
		string nextTag;
		//primalSpaceFeatures -> PrimalSpaceFeatures
		primalSpaceFeatures = parseHelper->getNamedNode(rootChild, "core:primalSpaceFeatures");
		if (primalSpaceFeatures == 0) {
			primalSpaceFeatures = parseHelper->getNamedNode(rootChild, "primalSpaceFeatures");
			frontTag = "";
			if (primalSpaceFeatures == 0) {
				primalSpaceFeatures = parseHelper->getNamedNode(rootChild, "indoor:primalSpaceFeatures");
				frontTag = "indoor:";
			}
		}
		nextTag = frontTag + "PrimalSpaceFeatures";
		primalSpaceFeatures = parseHelper->getNamedNode(primalSpaceFeatures->getChildNodes(), nextTag);

		//multiLayeredGraph -> MultiLayeredGraph
		nextTag = frontTag + "multiLayeredGraph";

		multiLayeredGraph = parseHelper->getNamedNode(rootChild, nextTag);
		nextTag = frontTag + "MultiLayeredGraph";

		multiLayeredGraph = parseHelper->getNamedNode(multiLayeredGraph->getChildNodes(), nextTag);

		//cellSpaceMember -> cellSpace & cellSpaceBoundaryMember -> cellSpaceBoundary

		nextTag = frontTag + "cellSpaceMember";

		cellSpaceMember = parseHelper->getNamedNodes(primalSpaceFeatures->getChildNodes(), nextTag);
		nextTag = frontTag + "cellSpaceBoundaryMember";

		cellSpaceBoundaryMember = parseHelper->getNamedNodes(primalSpaceFeatures->getChildNodes(), nextTag);

		vector<DOMNode*>cellspacelist;
		vector<DOMNode*>cellspaceboundarylist;
		vector<DOMNode*>IndoorGMLSolidList;
		vector<DOMNode*>surfaceList;

		nextTag = frontTag + "CellSpace";

		for (int i = 0; i < cellSpaceMember.size(); i++) {
			//cellspacelist.push_back();

			DOMNode* cellSpace = parseHelper->getNamedNode(cellSpaceMember.at(i)->getChildNodes(), nextTag);
			if (cellSpace != 0) {
				for (int j = 0; j < cellSpace->getChildNodes()->getLength(); j++) {
					string nextGeometryTag = frontTag + "cellSpaceGeometry";

					if (parseHelper->isMatchedNodeName(cellSpace->getChildNodes()->item(j), nextGeometryTag)) {
						DOMNode* solid = cellSpace->getChildNodes()->item(j)->getChildNodes()->item(1)->getChildNodes()->item(1);
						std::shared_ptr<IndoorGMLSolid> result = gmp->parseIndoorGMLSolid(solid, b);
						geomManager.addIndoorGMLSolid(result);
					}
				}
			}

		}
		nextTag = frontTag + "CellSpaceBoundary";

		for (int i = 0; i < cellSpaceBoundaryMember.size(); i++) {
			DOMNode* cellSpaceboundary = parseHelper->getNamedNode(cellSpaceBoundaryMember.at(i)->getChildNodes(), nextTag);
			if (cellSpaceboundary != 0) {
				for (int j = 0; j < cellSpaceboundary->getChildNodes()->getLength(); j++) {
					string nextGeometryTag = frontTag + "cellSpaceBoundaryGeometry";

					if (parseHelper->isMatchedNodeName(cellSpaceboundary->getChildNodes()->item(j), nextGeometryTag)) {
						DOMNode* surface = cellSpaceboundary->getChildNodes()->item(j)->getChildNodes()->item(1)->getChildNodes()->item(1);
						if (parseHelper->changeXMLCh2str(surface->getNodeName()) == "gml:Polygon") {

							shared_ptr<IndoorGMLPolygon> result = gmp->parseIndoorGMLPolygon(surface, b);
							geomManager.addIndoorGMLPolygon(result);
						}
					}

				}
			}
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
bool IndoorGMLReader::readIndoorSpace(DOMDocument* dom, std::vector<gaia3d::TrianglePolyhedron*>& container, double& lon, double& lat) {



	GeometryManager geomManager = parseIndoorGeometry(dom);
	//cout << "start read IndoorGML data" << endl;

	//gaia3d::Matrix4* mat;
	std::string wgs84ProjString("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	projPJ pjWgs84 = pj_init_plus(wgs84ProjString.c_str());
	gaia3d::Point3D BBcenterPoint;

	geomManager.bb.minX *= unitScaleFactor;
	geomManager.bb.minY *= unitScaleFactor;
	geomManager.bb.minZ *= unitScaleFactor;
	geomManager.bb.maxX *= unitScaleFactor;
	geomManager.bb.maxY *= unitScaleFactor;
	geomManager.bb.maxZ *= unitScaleFactor;

	gaia3d::Point3D lowerBound;
	lowerBound.set(geomManager.bb.minX, geomManager.bb.minY, geomManager.bb.minZ);
	gaia3d::Point3D upperBound;
	upperBound.set(geomManager.bb.maxX, geomManager.bb.maxY, geomManager.bb.maxZ);

	//cout << geomManager.bb.minX << "," << geomManager.bb.minY << "," << geomManager.bb.minZ << endl;
	//cout << geomManager.bb.maxX << "," << geomManager.bb.maxY << "," << geomManager.bb.maxZ << endl;

	projPJ pjSrc = pj_init_plus("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

	if (pjSrc == NULL || pjWgs84 == NULL)
	{
		printf("[ERROR][proj4]CANNOT initialize SRS\n");
		return false;
	}

	BBcenterPoint.set((lowerBound.x + upperBound.x) / 2.0, (lowerBound.y + upperBound.y) / 2.0, (lowerBound.z + upperBound.z) / 2.0);

	lon = BBcenterPoint.x;
	lat = BBcenterPoint.y;

	double alt = BBcenterPoint.z;

	//lon *= RAD_TO_DEG;
	//lat *= RAD_TO_DEG;

	vector<gaia3d::Triangle>tessellatedResult;

	for (size_t i = 0; i < geomManager.getIndoorGMLSolidsCount(); i++) {

		gaia3d::TrianglePolyhedron* newMesh = new gaia3d::TrianglePolyhedron();

		shared_ptr<IndoorGMLSolid> tempIndoorGMLSolid = geomManager.getIndoorGMLSolid(i);
		for (size_t j = 0; j < tempIndoorGMLSolid->getExterior().size(); j++) {
			size_t offset = newMesh->getVertices().size();

			Surface* tempSurface = new Surface();
			shared_ptr<IndoorGMLPolygon>tempIndoorGMLPolygon = tempIndoorGMLSolid->getExterior().at(j);
			size_t verticesCount = tempIndoorGMLPolygon->getExterior()->getVertices().size();
			vector<Point3D>tempPointList = tempIndoorGMLPolygon->getExterior()->getVertices();

			double* nx = new double[verticesCount];
			double* ny = new double[verticesCount];
			double* nz = new double[verticesCount];

			vector<size_t>verticesIndices;
			vector<size_t>polygonIndices;

			if (tempIndoorGMLSolid->getExterior().at(j)->getInterior().size() != 0) {}
			else {
				for (size_t z = 0; z < verticesCount; z++) {
					polygonIndices.push_back(z);
				}
			}

			// push new vertex at the vertex list of the surface
			for (size_t z = 0; z < verticesCount; z++) {

				nx[z] = tempPointList.at(z).x * unitScaleFactor;
				ny[z] = tempPointList.at(z).y * unitScaleFactor;
				nz[z] = tempPointList.at(z).z * unitScaleFactor;
				Vertex* tempVertex = new Vertex();
				tempVertex->position.set(nx[z], ny[z], nz[z]);
				newMesh->getVertices().push_back(tempVertex);
			}

			//GeometryUtility::tessellate(nx, ny, nz, verticesCount, verticesIndices);
			GeometryUtility::tessellate(nx, ny, nz, verticesCount, polygonIndices, verticesIndices);

			// Cause of dealing single IndoorGMLSolid as one single model. add offset and set the vertices indices of the new vertices
			for (size_t z = 0; z < verticesIndices.size(); z++) {
				size_t tempIndex = verticesIndices.at(z);
				tempIndex += offset;
				verticesIndices.at(z) = tempIndex;
			}

			//Create triangle 
			for (size_t z = 0; z < verticesIndices.size(); z += 3) {
				Vertex* v1 = newMesh->getVertices().at(verticesIndices.at(z));
				Vertex* v2 = newMesh->getVertices().at(verticesIndices.at(z + 1));
				Vertex* v3 = newMesh->getVertices().at(verticesIndices.at(z + 2));

				Point3D vec1 = v1->position - v2->position;
				Point3D vec2 = v1->position - v3->position;

				//if the sum of the each element of the cross product vector between 2 edges of the triangle, then those of the edges are parrallel
				Point3D crossProductOfTriangle = vec1 ^ vec2;
				double tolerance = 1E-6;
				double checkParallelValue = crossProductOfTriangle.x + crossProductOfTriangle.y + crossProductOfTriangle.z;
				if (abs(checkParallelValue) < tolerance)
					continue;

				double r1 = 0, r2 = 0, r3 = 0;
				GeometryUtility::calculatePlaneNormal(
					v1->position.x, v1->position.y, v1->position.z,
					v2->position.x, v2->position.y, v2->position.z,
					v3->position.x, v3->position.y, v3->position.z,
					r1, r2, r3, true
				);
				v1->normal.set(r1, r2, r3);
				v2->normal.set(r1, r2, r3);
				v3->normal.set(r1, r2, r3);

				Triangle* resultTriangle = new Triangle();
				resultTriangle->setNormal(r1, r2, r3);
				resultTriangle->setVertexIndices(verticesIndices.at(z), verticesIndices.at(z + 1), verticesIndices.at(z + 2));
				resultTriangle->setVertices(v1, v2, v3);
				tempSurface->getTriangles().push_back(resultTriangle);

			}

			newMesh->getSurfaces().push_back(tempSurface);

			delete[] nx;
			delete[] ny;
			delete[] nz;
		}

		/*Tessellate tessellator;
		tessellator.tessellate(geomManager.getIndoorGMLSolid(i), newMesh);
		tessellator.setTriangleVerticesIndices(newMesh, isNormalIn);

		vector<Vertex>tempVertexList;
		map<Vertex, int>vertexList;
		int count = 0;



		for (size_t j = 0; j < newMesh->getVertices().size(); j++) {
			gaia3d::Vertex* vertex = newMesh->getVertices().at(j);
			double px, py, pz;

			px = vertex->position.x;
			py = vertex->position.y;
			pz = vertex->position.z;

			//pj_transform(pjSrc, pjWgs84, 1, 1, &px, &py, &pz);
			px *= unitScaleFactor;
			py *= unitScaleFactor;
			pz *= unitScaleFactor;

			vertex->position.x = px;
			vertex->position.y = py;
			vertex->position.z = pz;

		}*/


		newMesh->setHasNormals(true);
		newMesh->setId(container.size());
		newMesh->setColorMode(SingleColor);
		newMesh->setSingleColor(MakeColorU4(250, 250, 250));
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
		ParserUtil* parseHelper = new ParserUtil();
		GeometryParser* gmp = new GeometryParser();

		ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
		parser->setErrorHandler(errHandler);
		parser->setIncludeIgnorableWhitespace(false);
		parser->setDoSchema(true);
		//const char * xmlFile = "../samples/seouluniv21centry.gml";
		parser->parse(xmlFile);

		//cout << xmlFile << ": parse OK" << endl;
		DOMDocument* dom = parser->getDocument();
		//cout << "Now processing start" << endl;
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