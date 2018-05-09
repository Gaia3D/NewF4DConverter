/**
 * Vbo Header
 */
#ifndef _VBO_H_
#define _VBO_H_
#pragma once

#include <cstring>
#include <vector>

#include "predefinition.h"
#include "Vertex.h"

namespace gaia3d
{
	class Vertex;
	struct Vbo
	{
		std::vector<Vertex*> vertices;
		std::vector<unsigned short> indices;
		double triangleSizeThresholds[TriangleSizeLevels];
		unsigned int indexMarker[TriangleSizeLevels];
		Vbo()
		{
			double value[TriangleSizeLevels] = TriangleSizeThresholds;
			std::memcpy(triangleSizeThresholds, value, sizeof(double)*TriangleSizeLevels);
		}
	};
}

#endif // _VBO_H_
