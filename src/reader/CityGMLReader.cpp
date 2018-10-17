/**
* Implementation of the ReaderFactory class
*/
#include "CityGMLReader.h"

#include <iostream>

#include <citygml/citygml.h>
#include <citygml/citymodel.h>
#include <citygml/cityobject.h>
#include <citygml/envelope.h>
#include <citygml/geometry.h>
#include <citygml/polygon.h>
#include <citygml/material.h>
#include <citygml/texture.h>
#include <citygml/citygmllogger.h>

#include "../geometry/ColorU4.h"

unsigned int getHighestLodForObject(const citygml::CityObject& object)
{
	unsigned int highestLOD = 0;
	// first find out highest LOD for this object
	for (unsigned int i = 0; i < object.getGeometriesCount(); i++) {
		const citygml::Geometry &geometry = object.getGeometry(i);

		if (geometry.getLOD() > highestLOD) {
			highestLOD = geometry.getLOD();
		}
	}

	//check for the highest LODs of Children
	for (unsigned int i = 0; i < object.getChildCityObjectsCount(); ++i) {
		unsigned int tempHighestLOD = getHighestLodForObject(object.getChildCityObject(i));
		if (tempHighestLOD > highestLOD) {
			tempHighestLOD = highestLOD;
		}
	}

	return highestLOD;
}

void setMaterial(const citygml::Polygon& polygon) {

	const auto citygmlMaterial = polygon.getMaterialFor("usemaxlodonly");

	if (!citygmlMaterial) {
		return;
	}

	TVec4f diffuse(citygmlMaterial->getDiffuse(), 0.f);
	TVec4f emissive(citygmlMaterial->getEmissive(), 0.f);
	TVec4f specular(citygmlMaterial->getSpecular(), 0.f);
	float ambient = citygmlMaterial->getAmbientIntensity();

	///osg::Material* material = new osg::Material;
	///material->setColorMode(osg::Material::OFF);
	///material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a));
	///material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(specular.r, specular.g, specular.b, specular.a));
	///material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(emissive.r, emissive.g, emissive.b, emissive.a));
	///material->setShininess(osg::Material::FRONT_AND_BACK, 128.f * citygmlMaterial->getShininess());
	///material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(ambient, ambient, ambient, 1.0));
	///material->setTransparency(osg::Material::FRONT_AND_BACK, citygmlMaterial->getTransparency());
	///stateset->setAttributeAndModes(material, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
	///stateset->setMode(GL_LIGHTING, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
}

void setTexture(const citygml::Polygon& polygon) {
	///const auto citygmlTex = polygon.getTextureFor(settings._theme);

	///if (!citygmlTex)
	///{
	///	return;
	///}
	const std::vector<TVec2f>& texCoords = polygon.getTexCoordsForTheme("usemaxlodonly", true);

	if (texCoords.empty()) {
		///osg::notify(osg::WARN) << "Texture coordinates not found for poly " << polygon.getId() << std::endl;
	}
	/*
	osg::Texture2D* texture = nullptr;

	if (settings._textureMap.find(citygmlTex->getUrl()) == settings._textureMap.end()) {
	std::string fullPath = osgDB::findDataFile(citygmlTex->getUrl());

	if (fullPath.empty()) {
	osg::notify(osg::NOTICE) << "  Texture file " << citygmlTex->getUrl() << " not found..." << std::endl;
	return;
	}

	// Load a new texture
	osg::notify(osg::NOTICE) << "  Loading texture " << fullPath << "..." << std::endl;

	osg::Image* image = osgDB::readImageFile(citygmlTex->getUrl());

	if (!image) {
	osg::notify(osg::NOTICE) << "  Warning: Failed to read Texture " << fullPath << std::endl;
	return;
	}

	texture = new osg::Texture2D;
	texture->setImage(image);
	texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
	texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
	texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
	texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
	texture->setWrap(osg::Texture::WRAP_R, osg::Texture::REPEAT);

	settings._textureMap[citygmlTex->getUrl()] = texture;
	}
	else {
	texture = settings._textureMap[citygmlTex->getUrl()];
	}

	if (!texture)
	{
	return;
	}
	*/

	///osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array;

	///tex->reserve(texCoords.size());
	for (unsigned int k = 0; k < texCoords.size(); k++)
	{
		///tex->push_back(osg::Vec2(texCoords[k].x, texCoords[k].y));
	}

	///geom->setTexCoordArray(0, tex);

	///stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
}

void createGeometryFromCityGMLGeometry(const citygml::Geometry& geometry,
											std::vector<gaia3d::TrianglePolyhedron*>& container,
											std::map<std::string, std::string>& textureContainer)
{
	gaia3d::TrianglePolyhedron* polyhedron = new gaia3d::TrianglePolyhedron;

	//polyhedron->m_objectId = geometry.getId();

	gaia3d::Surface* surface = new gaia3d::Surface;
	polyhedron->getSurfaces().push_back(surface);

	for (unsigned int j = 0; j < geometry.getPolygonsCount(); j++)
	{
		const citygml::Polygon& p = *geometry.getPolygon(j);

		if (p.getIndices().size() == 0) continue;

		gaia3d::Triangle* triangle;
		// Geometry management

		///osg::Geometry* geom = new osg::Geometry;
		///geom->setName(p.getId());
		///geom->setUserValue("cot_type", geometry.getTypeAsString());

		// access to vertices
		const std::vector<TVec3d>& vert = p.getVertices();
		for (unsigned int i = 0; i < vert.size(); i++)
		{
			gaia3d::Vertex* vertex = new gaia3d::Vertex;

			vertex->position.x = vert[i].x;
			vertex->position.y = vert[i].y;
			vertex->position.z = vert[i].z;

			polyhedron->getVertices().push_back(vertex);
		}
		// Indices
		///osg::DrawElementsUInt* indices = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, p.getIndices().begin(), p.getIndices().end());
		///geom->addPrimitiveSet(indices);

		const std::vector<unsigned int> vec_indices = p.getIndices();
		int indicesCount = vec_indices.size();
		int trianglesCount = indicesCount / 3;
		for (int i = 0; i < trianglesCount; i++)
		{
			triangle = new gaia3d::Triangle;
			triangle->setVertexIndices(vec_indices[i * 3], vec_indices[i * 3 + 1], vec_indices[i * 3 + 2]);
			triangle->setVertices(polyhedron->getVertices()[triangle->getVertexIndices()[0]],
				polyhedron->getVertices()[triangle->getVertexIndices()[1]],
				polyhedron->getVertices()[triangle->getVertexIndices()[2]]);

			surface->getTriangles().push_back(triangle);
		}
		// Appearance

		///osg::ref_ptr<osg::StateSet> stateset = geom->getOrCreateStateSet();

		///setMaterial(stateset, p, settings);
		///setTexture(stateset, geom, p, settings);
		setMaterial(p);
		//setTexture(p);

		///geometryContainer->addDrawable(geom);
	}
	polyhedron->setId(container.size());

	container.push_back(polyhedron);

	// check if texture exists.
	bool textureExistsForMesh = false;

	// check if color info exists.
	gaia3d::ColorMode colorMode = gaia3d::NoColor;
	if (!textureExistsForMesh)
	{
		colorMode = gaia3d::SingleColor;
		polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.7 * 255),
												(unsigned char)(0.7 * 255),
												(unsigned char)(0.7 * 255)));

		citygml::Geometry::GeometryType geoType = geometry.getType();
		if (geoType != citygml::Geometry::GeometryType::GT_Unknown)
		{
			if (geoType == citygml::Geometry::GeometryType::GT_Roof)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.95 * 255),
														(unsigned char)(0.1 * 255),
														(unsigned char)(0.1 * 255)));
				}
			else if (geoType == citygml::Geometry::GeometryType::GT_Wall)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.7 * 255),
														(unsigned char)(0.7 * 255),
														(unsigned char)(0.7 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_Ground)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.7 * 255),
														(unsigned char)(0.7 * 255),
														(unsigned char)(0.7 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_Closure)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.2 * 255),
														(unsigned char)(0.6 * 255),
														(unsigned char)(0.6 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_Floor)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.5 * 255),
														(unsigned char)(0.5 * 255),
														(unsigned char)(0.5 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_InteriorWall)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.8 * 255),
														(unsigned char)(0.8 * 255),
														(unsigned char)(0.8 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_Ceiling)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.9 * 255),
														(unsigned char)(0.3 * 255),
														(unsigned char)(0.3 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_OuterCeiling)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.2 * 255),
														(unsigned char)(0.9 * 255),
														(unsigned char)(0.1 * 255)));
			}
			else if (geoType == citygml::Geometry::GeometryType::GT_OuterFloor)
			{
				polyhedron->setSingleColor(MakeColorU4((unsigned char)(0.7 * 255),
														(unsigned char)(0.7 * 255),
														(unsigned char)(0.1 * 255)));
			}
		}
	}
	polyhedron->setColorMode(colorMode);



	// Parse child geoemtries
	for (unsigned int i = 0; i < geometry.getGeometriesCount(); i++) {
		createGeometryFromCityGMLGeometry(geometry.getGeometry(i), container, textureContainer);
	}
}

bool createCityObject(const citygml::CityObject& object,
						std::vector<gaia3d::TrianglePolyhedron*>& container,
						std::map<std::string, std::string>& textureContainer,
						bool bUseMaxLODOnly)
{
	unsigned int highestLOD = getHighestLodForObject(object);

	for (unsigned int i = 0; i < object.getGeometriesCount(); i++)
	{
		const citygml::Geometry& geometry = object.getGeometry(i);

		const unsigned int currentLOD = geometry.getLOD();

		if (bUseMaxLODOnly && (currentLOD < highestLOD))// || currentLOD < minimumLODToConsider)) 
		{
			continue;
		}

		createGeometryFromCityGMLGeometry(geometry, container, textureContainer);
	}

	for (unsigned int i = 0; i < object.getChildCityObjectsCount(); ++i)
		createCityObject(object.getChildCityObject(i), container, textureContainer, bUseMaxLODOnly);

	return true;
}

bool readCity(std::shared_ptr<const citygml::CityModel> city, 
				std::vector<gaia3d::TrianglePolyhedron*>& container,
				std::map<std::string, std::string>& textureContainer,
				bool bUseMaxLODOnly)
{
	const citygml::ConstCityObjects& roots = city->getRootCityObjects();

	if (roots.size() == 0) return false;
	
	for (unsigned int i = 0; i < roots.size(); ++i)
	{
		createCityObject(*roots[i], container, textureContainer, bUseMaxLODOnly);
	}

	return true;
}

CityGMLReader::CityGMLReader()
{
	bUseMaxLODOnly = true;
}

CityGMLReader::~CityGMLReader()
{
}

bool CityGMLReader::readRawDataFile(std::string& filePath)
{
	citygml::ParserParams params;
	std::shared_ptr<const citygml::CityModel> city;

	try {
		city = citygml::load(filePath, params);
	}
	catch (const std::runtime_error& e) {

	}

	if (!city) return false;

	readCity(city, container, textureContainer, bUseMaxLODOnly);

	return true;
}

void CityGMLReader::clear()
{
	container.clear();

	textureContainer.clear();
}
