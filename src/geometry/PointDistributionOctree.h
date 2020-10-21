/**
* PointDistributionOctree Header
*/
#ifndef _POINTDISTRIBUTIONOCTREE_H_
#define _POINTDISTRIBUTIONOCTREE_H_
#pragma once

#include <cstddef>
#include <vector>

#include "Vertex.h"

namespace gaia3d
{
	class PointDistributionOctree
	{
	public:
		PointDistributionOctree(PointDistributionOctree* owner);
		~PointDistributionOctree();

		size_t octreeId;

		PointDistributionOctree* parent;
		std::vector<PointDistributionOctree*> children;

		unsigned char level;

		std::vector<Vertex*> vertices;

		double minX, minY, minZ, maxX, maxY, maxZ;

		void setSize(double xMin, double yMin, double zMin, double xMax, double yMax, double zMax)
		{
			minX = xMin; minY = yMin; minZ = zMin; maxX = xMax; maxY = yMax; maxZ = zMax;
		}

		void makeTreeOfUnfixedDepth(double minSize, bool isObjectInOnlyOneLeaf);

		void distributeVerticesIntoEachChildren(bool isObjectInOnlyOneLeaf);

		void setOctreeId(size_t parentId = 0, size_t orderOfChild = 0);

		void getAllLeafBoxes(std::vector<PointDistributionOctree*>& container);

		///< 어느 leaf octree box와 겹치는가
		PointDistributionOctree* getIntersectedLeafOctree(Vertex* vertex);

		void clear();
	};
}

#endif // _POINTDISTRIBUTIONOCTREE_H_
