#ifndef _LINESTRING_H_
#define _LINESTRING_H_
#pragma once

#include "../geometry/Point3D.h"
#include "../geometry/AbstractFeatures.h"
#include <memory>

#include <vector>

namespace gaia3d {

	class IndoorGMLFactory;

	/**
	* @brief The LineString class implements the gml:LineString object may also be used as a container of a single gml::Point
	*/
	class LineString : public indoorgml::AbstractFeatures {
		friend class IndoorGMLFactory;
	public:
		int getDimensions() const;

		//const std::vector<TVec2d>& getVertices2D() const;
		const std::vector<Point3D>& getVertices3D() const;

		//std::vector<TVec2d>& getVertices2D();
		std::vector<Point3D>& getVertices3D();

		//void setVertices2D(const std::vector<TVec2d>& vertices);
		void setVertices3D(const std::vector<Point3D>& vertices);

		void setDimensions(int dim);

	protected:
		LineString(const std::string& id);
		//std::vector<TVec2d> m_vertices_2d;
		std::vector<Point3D> m_vertices_3d;
		int m_dimensions;
	};

}
#endif