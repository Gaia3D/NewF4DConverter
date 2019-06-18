/**
 * Implementation of the Reader class
 */
#include "Reader.h"

Reader::Reader()
{
}

Reader::~Reader()
{
}
std::vector<gaia3d::TrianglePolyhedron*>& Reader::getDataContainer()
{
	return container;
}
std::map<std::string, std::string>& Reader::getTextureInfoContainer()
{
	return textureContainer;
}
void Reader::setUnitScaleFactor(double factor)
{
	unitScaleFactor = factor;
}
void Reader::TexCoord_Flip_Y()
{
	for (int i = 0, count = container.size(); i < count; i++)
	{
		gaia3d::TrianglePolyhedron *polyhedron = container[i];
		polyhedron->TexCoord_Flip_Y();
	}
}

std::string Reader::makeProj4String()
{
	std::string proj4String;

	if (epsg.empty())
	{
		proj4String = std::string("+proj=tmerc +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");
		proj4String += std::string(" +lon_0=") + std::to_string(lonOrigin);
		proj4String += std::string(" +lat_0=") + std::to_string(latOrigin);
	}
	else
	{
		proj4String = std::string("+init=epsg:") + epsg;
	}

	return proj4String;
}