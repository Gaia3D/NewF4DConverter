#ifndef _SOLID_H_
#define _SOLID_H_
#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include "AbstractFeatures.h"
#include "Polygon.h"
#include "BoundingBox.h"


namespace gaia3d {

	class LineString;
	class Polygon;
	//class Surface;

	class Solid : public indoorgml::AbstractFeatures
	{
		friend class IndoorGMLFactory;
	public:
		BoundingBox bb;
		bool hasExterior();
		bool hasInterior();
		//const Solid& getExterior() const;
		vector<shared_ptr<Polygon>> getExterior();
		std::vector<std::shared_ptr<Solid>> getInterior();
		void addInterior(std::shared_ptr<Solid>);
		//void deleteInterior();
		void setExterior(vector<std::shared_ptr<Polygon>>);
		Solid(const std::string& id);

		~Solid();


	protected:

		bool m_finished;
		unsigned int m_lod;

		std::vector<std::shared_ptr<Polygon> > exterior;
		std::vector<std::shared_ptr<Solid>> interior;
	};

}
#endif