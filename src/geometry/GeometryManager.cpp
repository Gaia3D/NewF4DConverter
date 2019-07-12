#include "GeometryManager.h"


namespace indoorgml {

	GeometryManager::GeometryManager()
	{
	}


	unsigned int GeometryManager::getPolygonsCount() const
	{
		return m_polygons.size();
	}

	std::shared_ptr<gaia3d::Polygon> GeometryManager::getPolygon(unsigned int i)
	{
		return m_polygons.at(i);
	}

	std::shared_ptr<gaia3d::Solid> GeometryManager::getSolid(unsigned int i) {
		return m_solids.at(i);
	}

	/*
	std::shared_ptr<const gaia3d::Polygon> GeometryManager::getPolygon(unsigned int i) const
	{
		return m_polygons.at(i);
	}
	*/

	unsigned int GeometryManager::getLineStringCount() const
	{
		return m_lineStrings.size();
	}

	std::shared_ptr<gaia3d::LineString> GeometryManager::getLineString(unsigned int i)
	{
		return m_lineStrings.at(i);
	}

	std::shared_ptr<const gaia3d::LineString> GeometryManager::getLineString(unsigned int i) const
	{
		return m_lineStrings.at(i);
	}

	unsigned int GeometryManager::getGeometriesCount() const
	{
		return m_childGeometries.size();
	}

	/*
	const GeometryManager& GeometryManager::getGeometry(unsigned int i) const
	{
	return *m_childGeometries.at(i);
	}

	GeometryManager& GeometryManager::getGeometry(unsigned int i)
	{
	return *m_childGeometries.at(i);
	}
	*/

	unsigned int GeometryManager::getSolidsCount() const {
		return m_solids.size();
	}


	GeometryManager::~GeometryManager()
	{
	}

	void GeometryManager::addPolygon(std::shared_ptr<gaia3d::Polygon> p)
	{
		m_polygons.push_back(p);
	}

	void GeometryManager::addSolid(std::shared_ptr<gaia3d::Solid> s) {
		m_solids.push_back(s);
	}

	void GeometryManager::addLineString(std::shared_ptr<gaia3d::LineString> l)
	{
		m_lineStrings.push_back(l);
	}





}
