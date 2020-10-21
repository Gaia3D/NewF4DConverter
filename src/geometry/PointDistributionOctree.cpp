/**
* Implementation of the PointDistributionOctree class
*/
#include "PointDistributionOctree.h"

namespace gaia3d
{
	PointDistributionOctree::PointDistributionOctree(PointDistributionOctree* owner)
		:parent(owner)
	{
		level = (parent == NULL) ? 0 : parent->level + 1;

		minX = minY = minZ = maxX = maxY = maxZ = 0.0;
	}

	PointDistributionOctree::~PointDistributionOctree()
	{
		clear();
	}

	void PointDistributionOctree::makeTreeOfUnfixedDepth(double minSize, bool isObjectInOnlyOneLeaf)
	{
		double xLength = maxX - minX, yLength = maxY - minY, zLength = maxZ - minZ;
		double maxEdgeLength = (xLength > yLength) ? ((xLength > zLength) ? xLength : zLength) : ((yLength > zLength) ? yLength : zLength);
		double tolerance = minSize * 0.4;
		///< 제일 긴 변이 해당 기준 임계값을 넘을때
		if (maxEdgeLength > minSize + tolerance)
		{
			// 1) make 8 children
			for (size_t i = 0; i < 8; i++)
			{
				PointDistributionOctree* child = new PointDistributionOctree(this);
				children.push_back(child);
			}

			// 2) set size of each child octree
			//this->Set_SizesSubBoxes();
			double halfX, halfY, halfZ;
			halfX = (maxX + minX) / 2.0;
			halfY = (maxY + minY) / 2.0;
			halfZ = (maxZ + minZ) / 2.0;
			children[0]->setSize(minX, minY, minZ, halfX, halfY, halfZ);
			children[1]->setSize(halfX, minY, minZ, maxX, halfY, halfZ);
			children[2]->setSize(halfX, halfY, minZ, maxX, maxY, halfZ);
			children[3]->setSize(minX, halfY, minZ, halfX, maxY, halfZ);
			children[4]->setSize(minX, minY, halfZ, halfX, halfY, maxZ);
			children[5]->setSize(halfX, minY, halfZ, maxX, halfY, maxZ);
			children[6]->setSize(halfX, halfY, halfZ, maxX, maxY, maxZ);
			children[7]->setSize(minX, halfY, halfZ, halfX, maxY, maxZ);

			// 3) distribute meshes into each children
			distributeVerticesIntoEachChildren(isObjectInOnlyOneLeaf);

			// 4) Make tree for subBoxes.***
			for (size_t i = 0; i < 8; i++)
			{
				if (children[i]->vertices.size() > 0)
					children[i]->makeTreeOfUnfixedDepth(minSize, isObjectInOnlyOneLeaf);
			}
		}
	}

	void PointDistributionOctree::distributeVerticesIntoEachChildren(bool isObjectInOnlyOneLeaf)
	{
		size_t childCount = children.size();
		if (childCount == 0)
			return;

		size_t vertexCount = vertices.size();
		for (size_t i = 0; i < vertexCount; i++)
		{
			gaia3d::Vertex* vertex = vertices[i];

			for (size_t j = 0; j < childCount; j++)
			{
				if (children[j]->maxX < vertex->position.x || children[j]->minX > vertex->position.x ||
					children[j]->maxY < vertex->position.y || children[j]->minY > vertex->position.y ||
					children[j]->maxZ < vertex->position.z || children[j]->minZ > vertex->position.z)
					continue;

				children[j]->vertices.push_back(vertex);

				break;
			}
		}

		if (isObjectInOnlyOneLeaf)
			vertices.clear();
	}

	void PointDistributionOctree::setOctreeId(size_t parentId, size_t orderOfChild)
	{
		if (level == 0)
			octreeId = 0;
		else
			octreeId = parentId * 10 + (orderOfChild + 1);

		size_t childCount = children.size();
		for (size_t i = 0; i < childCount; i++)
		{
			children[i]->setOctreeId(octreeId, i);
		}
	}

	void PointDistributionOctree::getAllLeafBoxes(std::vector<PointDistributionOctree*>& container)
	{
		size_t childCount = children.size();
		if (childCount >0)
		{
			for (size_t i = 0; i< childCount; i++)
			{
				children[i]->getAllLeafBoxes(container);
			}
		}
		else
		{
			if (vertices.size() > 0)
				container.push_back(this);
		}
	}

	PointDistributionOctree* PointDistributionOctree::getIntersectedLeafOctree(Vertex* vertex)
	{
		if (maxX < vertex->position.x || minX > vertex->position.x ||
			maxY < vertex->position.y || minY > vertex->position.y ||
			maxZ < vertex->position.z || minZ > vertex->position.z)
			return NULL;

		if (children.empty())
			return this;

		size_t childCount = children.size();
		for (size_t i = 0; i < childCount; i++)
		{
			PointDistributionOctree* child = children[i]->getIntersectedLeafOctree(vertex);
			if (child != NULL)
				return child;
		}

		return NULL;
	}

	void PointDistributionOctree::clear()
	{
		size_t childCount = children.size();
		for (size_t i = 0; i < childCount; i++)
		{
			children[i]->clear();
			delete children[i];
		}
		children.clear();

		vertices.clear();
	}
}