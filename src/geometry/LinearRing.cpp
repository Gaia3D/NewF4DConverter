#include "LinearRing.h"
#include <float.h>
#include <assert.h>


namespace gaia3d {

	LinearRing::LinearRing(const std::string& id, bool isExterior) : indoorgml::AbstractFeatures(id), m_exterior(isExterior)
	{

	}
	LinearRing::LinearRing(const std::string& id) : indoorgml::AbstractFeatures(id) {

	}
	
	bool LinearRing::isExterior() const
	{
		return m_exterior;
	}

	unsigned int LinearRing::size() const
	{
		return m_vertices.size();
	}

	void LinearRing::addVertex(const Point3D& v)
	{
		m_vertices.push_back(v);
	}

	/*
	Point3D LinearRing::computeNormal() const
	{
	unsigned int len = size();
	if (len < 3) return Point3D();

	// Tampieri, F. 1992. Newell's method for computing the plane equation of a polygon. In Graphics Gems III, pp.231-232.
	Point3D n(0., 0., 0.);
	for (unsigned int i = 0; i < len; i++)
	{
	const Point3D& current = m_vertices[i];
	const Point3D& next = m_vertices[(i + 1) % len];

	n.x += (current.y - next.y) * (current.z + next.z);
	n.y += (current.z - next.z) * (current.x + next.x);
	n.z += (current.x - next.x) * (current.y + next.y);
	}
	return n.normal();
	}
	*/

	std::vector<Point3D>& LinearRing::getVertices()
	{
		return m_vertices;
	}

	void LinearRing::setVertices(std::vector<Point3D> vertices)
	{
		m_vertices = vertices;
	}

	const std::vector<Point3D>& LinearRing::getVertices() const
	{
		return m_vertices;
	}

	void LinearRing::forgetVertices()
	{
		m_vertices.clear();
	}

}
