/**
 * LegoBlock Header
 */
#ifndef _LEGOBLOCK_H_
#define _LEGOBLOCK_H_
#pragma once

#include "ColorU4.h"

namespace gaia3d
{
	class LegoBlock
	{
	public:
		LegoBlock();

		virtual ~LegoBlock();

	public:
		double minX, minY, minZ, maxX, maxY, maxZ;

		ColorU4 color;

		void setSize(double xMin, double yMin, double zMin, double xMax, double yMax, double zMax)
		{minX = xMin; minY = yMin; minZ = zMin; maxX = xMax; maxY = yMax; maxZ = zMax;}
	};
}

#endif // _LEGOBLOCK_H_