/**
 * Surface Header
 */
#ifndef _SURFACE_H_
#define _SURFACE_H_
#pragma once

#include <vector>

#include "Triangle.h"

namespace gaia3d
{
	class Surface
	{
	public:
		Surface();

		virtual ~Surface();

	protected:
		std::vector<Triangle*> triangles;

		bool bExterior;

	public:
		std::vector<Triangle*>& getTriangles() { return triangles; }

		bool isExterior() { return bExterior; }

		void setIsExterior(bool bExterior) { this->bExterior = bExterior; }
	};
}

#endif // _SURFACE_H_