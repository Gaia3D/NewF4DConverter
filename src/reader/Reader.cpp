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
