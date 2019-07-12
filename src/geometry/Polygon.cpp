#include <algorithm>
#include <stdexcept>
#include <assert.h>
#include <stdexcept>
#include <iostream>
#include "Polygon.h"

namespace gaia3d {

	Polygon::Polygon(string id) : AbstractFeatures(id)
	{
	}

	const std::vector<Point3D>& Polygon::getVertices() const
	{
		return m_vertices;
	}

	std::vector<Point3D>& Polygon::getVertices()
	{
		return m_vertices;
	}

	const std::vector<unsigned int>& Polygon::getIndices() const
	{
		return m_indices;
	}

	Face Polygon::convertFromPolygonToFace() {
		shared_ptr<LinearRing> ext = getExterior();
		Face result;
		for (int i = 0; i < ext->getVertices().size(); i++) {
			Point3D p3 = ext->getVertices().at(i);
			Vertex v;
			v.setPosition(p3);
			v.setVertexIndex(i);
			result.addVertex(v);
		}

		return result;
	}

	/*
	Point3D Polygon::computeNormal()
	{
	if (m_exteriorRing == nullptr) return Point3D();

	Point3D normal = m_exteriorRing->computeNormal();

	return m_negNormal ? -normal : normal;
	}
	*/

	bool Polygon::negNormal() const
	{
		return m_negNormal;
	}
	void Polygon::setExterior(shared_ptr<LinearRing> l) {
		m_exteriorRing = l;
		for (int i = 0; i < l->getVertices().size(); i++) {
			m_indices.push_back(i);
		}
	}
	shared_ptr<LinearRing> Polygon::getExterior() {
		return m_exteriorRing;
	}
	void Polygon::setNegNormal(bool negNormal)
	{
		m_negNormal = negNormal;
	}


	/*
	void Polygon::finish(bool optimize, std::shared_ptr<Logger> logger)
	{
	if (m_finished) {
	// This may happen as Polygons can be shared between geometries
	return;
	}

	m_finished = true;

	}
	*/

	void Polygon::addRing(LinearRing* ring)
	{
		if (m_finished) {
			throw std::runtime_error("Can't add LinearRing to finished Polygon.");
		}

		if (ring->isExterior() && m_exteriorRing != nullptr) {
			//CITYGML_LOG_WARN(m_logger, "Duplicate definition of exterior LinearRing for Polygon with id '" << this->getId() << "'."
			//	<< " Keeping exterior LinearRing with id '" << m_exteriorRing->getId() << "' and ignore LinearRing with id '" << ring->getId() << "'");
			delete ring;
			return;
		}

		if (ring->isExterior()) {
			m_exteriorRing = std::shared_ptr<LinearRing>(ring);
		}
		else {
			m_interiorRings.push_back(std::shared_ptr<LinearRing>(ring));
		}
	}

	Polygon::~Polygon()
	{
	}

}
