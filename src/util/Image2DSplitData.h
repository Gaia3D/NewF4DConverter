/**
* Image2DSplitData Header
*/
#ifndef _IMAGE2DSPLITDATA_H_
#define _IMAGE2DSPLITDATA_H_
#pragma once

#include <map>
#include <vector>

#include "Rectangle.h"
#include "../geometry/Triangle.h"
#include "../geometry/Vertex.h"

namespace gaia3d
{
	class Image2DSplitData
	{
	public:
		Rectangle *m_rectangleImage_original; // region of the original image.***
		Rectangle *m_rectangleImage_splitted; // the size is equal to "original", only differs in the position in the new image.***
		std::vector<gaia3d::Triangle*> m_vec_triangles;

		Image2DSplitData();
		~Image2DSplitData();

		void Get_Vertices(std::vector<gaia3d::Vertex*> &vec_vertices);

		bool TEST__AreTexCoordsInside_rectangleOriginal();
	};
}

#endif // _IMAGE2DSPLITDATA_H_
