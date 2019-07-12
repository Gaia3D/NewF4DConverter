#ifndef _POLYGON_H_
#define _POLYGON_H_
#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "../geometry/LinearRing.h"
#include "../geometry/Point3D.h"
#include "../geometry/Face.h"
#include "../geometry/Vertex.h"
#include "../geometry/BoundingBox.h"


namespace gaia3d {

	class Polygon : public indoorgml::AbstractFeatures
	{

	public:


		// Get the vertices
		const std::vector<Point3D>& getVertices() const;
		std::vector<Point3D>& getVertices();

		// Get the indices
		const std::vector<unsigned int>& getIndices() const;

		bool negNormal() const;
		void setNegNormal(bool negNormal);

		void addRing(LinearRing*);

		Face convertFromPolygonToFace();
		void setExterior(shared_ptr<LinearRing> l);
		shared_ptr<LinearRing> getExterior();
		Polygon(string id);

		virtual ~Polygon();

	protected:
		//Polygon(const std::string& id);


		std::vector<Point3D> m_vertices;
		std::vector<unsigned int> m_indices;

		std::shared_ptr<LinearRing> m_exteriorRing;
		std::vector<std::shared_ptr<LinearRing> > m_interiorRings;

		bool m_negNormal;
		bool m_finished;

		//std::shared_ptr<Logger> m_logger;
	};
}
#endif