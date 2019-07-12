#ifndef _GEOMETRYMANAGER_H_
#define _GEOMETRYMANAGER_H_

#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include "../geometry/AbstractFeatures.h"
#include "../geometry/Solid.h"
#include "../geometry/Polygon.h"
#include "../geometry/LinearRing.h"
#include "../geometry/LineString.h"
#include "../geometry/BoundingBox.h"

namespace indoorgml {


	class GeometryManager
	{
		friend class IndoorGMLFactory;
	public:
		gaia3d::BoundingBox bb;

		unsigned int getSolidsCount() const;
		std::shared_ptr<gaia3d::Solid> getSolid(unsigned int i);
		//std::shared_ptr<const gaia3d::Solid> getSolid(unsigned int i) const;
		//std::shared_ptr<gaia3d::Solid> getSolid(string id);

		unsigned int getPolygonsCount() const;
		std::shared_ptr<gaia3d::Polygon> getPolygon(unsigned int i);
		//std::shared_ptr<const gaia3d::Polygon> getPolygon(unsigned int i) const;

		unsigned int getLineStringCount() const;
		std::shared_ptr<gaia3d::LineString> getLineString(unsigned int i);
		std::shared_ptr<const gaia3d::LineString> getLineString(unsigned int i) const;

		unsigned int getGeometriesCount() const;
		const AbstractFeatures& getGeometry(unsigned int i) const;
		AbstractFeatures& getGeometry(unsigned int i);
		void addGeometry(string type, AbstractFeatures geom);

		void addPolygon(std::shared_ptr<gaia3d::Polygon>);
		void addLineString(std::shared_ptr<gaia3d::LineString>);
		void addSolid(std::shared_ptr<gaia3d::Solid>);

		/**
		* @brief finishes the geometry by finishing its child polygons after broadcasting its appearances to all child polygons
		* @param tesselate determines wether the polygons are tesselated
		* @param tesselator the tesselator to be used for tesselation
		* @param mergePolygons determines wether all polygons are merged into one
		*/

		~GeometryManager();
		GeometryManager();

	protected:


		bool m_finished;


		std::vector<std::shared_ptr<gaia3d::Solid> > m_childGeometries;
		std::vector<std::shared_ptr<gaia3d::Solid> > m_solids;
		std::vector<std::shared_ptr<gaia3d::Polygon> > m_polygons;
		std::vector<std::shared_ptr<gaia3d::LineString> > m_lineStrings;
	};

}

#endif