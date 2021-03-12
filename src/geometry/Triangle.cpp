/**
 * Implementation of the Triangle class
 */
#include "Triangle.h"

namespace gaia3d
{
	
	Triangle::Triangle()
	{
		vertexIndex[0] = vertexIndex[1] = vertexIndex[2] = 0;

		vertex[0] = vertex[1] = vertex[2] = NULL;

		bExterior = false;
	}

	Triangle::~Triangle()
	{
	}

	void Triangle::alignVertexNormalsToPlaneNormal()
	{
		vertex[0]->normal.set(this->normal.x, this->normal.y, this->normal.z);
		vertex[1]->normal.set(this->normal.x, this->normal.y, this->normal.z);
		vertex[2]->normal.set(this->normal.x, this->normal.y, this->normal.z);
	}
}