﻿/**
 * Implementation of the OctreeBox class
 */
#include <algorithm>

#include "OctreeBox.h"
#include "BoundingBox.h"
#include "../util/GeometryUtility.h"

namespace gaia3d
{
	OctreeBox::OctreeBox(OctreeBox* owner)
	:parent(owner)
	{
		level = (parent == NULL) ? 0 : parent->level + 1;

		minX = minY = minZ = maxX = maxY = maxZ = 0.0;
	}

	OctreeBox::~OctreeBox()
	{
		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
			delete children[i];
	}

	void OctreeBox::clear()
	{
		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
		{
			children[i]->clear();
			delete children[i];
		}
		children.clear();

		meshes.clear();
	}

	void OctreeBox::getAllLeafBoxes(std::vector<OctreeBox*>& container, bool bExceptEmptyBox)
	{
		size_t childCount = children.size();
		if(childCount >0 )
		{
			for(size_t i=0; i< childCount; i++)
			{
				children[i]->getAllLeafBoxes(container, bExceptEmptyBox);
			}
		}
		else
		{
			if(bExceptEmptyBox)
			{
				if(meshes.size() > 0)
					container.push_back(this);
			}
			else
				container.push_back(this);

		}
	}

	///< Return the octree box list include itself(this) and the children
	void OctreeBox::getAllBoxes(std::vector<OctreeBox*>& container, bool bExceptEmptyBox)
	{
		for (size_t i = 0; i < children.size(); i++)
			children[i]->getAllBoxes(container, bExceptEmptyBox);

		if (bExceptEmptyBox)
		{
			if (!meshes.empty())
				container.push_back(this);
		}
		else
			container.push_back(this);
	}

	///< Copy dimension of other octree box
	void OctreeBox::copyDimensionsFromOtherOctreeBox(OctreeBox& input)
	{
		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
			delete children[i];

		children.clear();

		setSize(input.minX, input.minY, input.minZ, input.maxX, input.maxY, input.maxZ);

		unsigned char originalDepth = input.getDepth();
		if(originalDepth > 0)
			makeTree(originalDepth);
	}

	void OctreeBox::makeTree(unsigned char depth)
	{
		if(level < depth)
		{
			// 1) Create 8 children octrees
			for(size_t i = 0; i < 8; i++)
			{
				OctreeBox* child = makeChild();
				children.push_back(child);
			}

			// 2) set size of each child octree
			//this->Set_SizesSubBoxes();
			double halfX, halfY, halfZ;
			halfX= (maxX+minX)/2.0;
			halfY= (maxY+minY)/2.0;
			halfZ= (maxZ+minZ)/2.0;
			children[0]->setSize(minX, minY, minZ, halfX, halfY, halfZ);
			children[1]->setSize(halfX, minY, minZ, maxX, halfY, halfZ);
			children[2]->setSize(halfX, halfY, minZ, maxX, maxY, halfZ);
			children[3]->setSize(minX, halfY, minZ, halfX, maxY, halfZ);
			children[4]->setSize(minX, minY, halfZ, halfX, halfY, maxZ);
			children[5]->setSize(halfX, minY, halfZ, maxX, halfY, maxZ);
			children[6]->setSize(halfX, halfY, halfZ, maxX, maxY, maxZ);
			children[7]->setSize(minX, halfY, halfZ, halfX, maxY, maxZ);

			// 3) Make tree for subBoxes.***
			size_t childCount = children.size();
			for(size_t i = 0; i < childCount; i++)
				children[i]->makeTree(depth);
		}
	}

	unsigned char OctreeBox::getDepth()
	{
		if(children.size() > 0)
			return children[0]->getDepth();
		else
			return level;
	}

	VisionOctreeBox::VisionOctreeBox(OctreeBox* owner)
	:OctreeBox(owner)
	{
	}

	VisionOctreeBox::~VisionOctreeBox()
	{
	}

	///< Get the internal frustum point for deferred rendering
	void VisionOctreeBox::getInternalDivisionPoints(std::vector<Point3D>& container, double scanStepX, double scanStepY, double scanStepZ)
	{
		int div_x = (int)ceil((maxX-minX)/scanStepX);
		int div_y = (int)ceil((maxY-minY)/scanStepY);
		int div_z = (int)ceil((maxZ-minZ)/scanStepZ);

		double x_value, y_value, z_value;

		for(int i=0; i<div_x+1; i++)
		{
			for(int j=0; j<div_y+1; j++)
			{
				for(int k=0; k<div_z+1; k++)
				{
					x_value = minX + i*scanStepX;
					y_value = minY + j*scanStepY;
					z_value = minZ + k*scanStepZ;
					if(x_value > maxX)x_value = maxX;
					if(y_value > maxY)y_value = maxY;
					if(z_value > maxZ)z_value = maxZ;

					Point3D point; 
					point.set(x_value, y_value, z_value);

					container.push_back(point);
				}
			}
		}
	}
	
	SpatialOctreeBox::SpatialOctreeBox(OctreeBox* owner)
	:OctreeBox(owner)
	{
		interiorOcclusionInfo = new VisionOctreeBox(NULL);
		exteriorOcclusionInfo = new VisionOctreeBox(NULL);

		netSurfaceMesh = NULL;

		prettySkinMesh = NULL;
	}

	SpatialOctreeBox::~SpatialOctreeBox()
	{
		if(interiorOcclusionInfo != NULL)
			delete interiorOcclusionInfo;

		if(exteriorOcclusionInfo != NULL)
			delete exteriorOcclusionInfo;

		if(netSurfaceMesh != NULL)
			delete netSurfaceMesh;
	}

	void SpatialOctreeBox::makeTreeOfUnfixedDepth(double minSize, bool isObjectInOnlyOneLeaf, bool bSplitMesh)
	{
		double xLength = maxX - minX, yLength = maxY - minY, zLength = maxZ - minZ;
		double maxEdgeLength = (xLength > yLength) ? ( (xLength > zLength) ? xLength : zLength ) : ((yLength > zLength) ? yLength : zLength);
		double tolerance = minSize * 0.4;

		///< 제일 긴 변이 해당 기준 임계값을 넘을때
		if(maxEdgeLength > minSize + tolerance)
		{
			// 1) make 8 children
			for(size_t i = 0; i < 8; i++)
			{
				OctreeBox* child = makeChild();
				children.push_back(child);
			}

			// 2) set size of each child octree
			//this->Set_SizesSubBoxes();
			double halfX, halfY, halfZ;
			halfX= (maxX+minX)/2.0;
			halfY= (maxY+minY)/2.0;
			halfZ= (maxZ+minZ)/2.0;
			children[0]->setSize(minX, minY, minZ, halfX, halfY, halfZ);
			children[1]->setSize(halfX, minY, minZ, maxX, halfY, halfZ);
			children[2]->setSize(halfX, halfY, minZ, maxX, maxY, halfZ);
			children[3]->setSize(minX, halfY, minZ, halfX, maxY, halfZ);
			children[4]->setSize(minX, minY, halfZ, halfX, halfY, maxZ);
			children[5]->setSize(halfX, minY, halfZ, maxX, halfY, maxZ);
			children[6]->setSize(halfX, halfY, halfZ, maxX, maxY, maxZ);
			children[7]->setSize(minX, halfY, halfZ, halfX, maxY, maxZ);

			// 3) distribute meshes into each children
			if (bSplitMesh)
			{
				splitMeshIntoEachChildren();
			}
			else
			{
				distributeMeshesIntoEachChildren(isObjectInOnlyOneLeaf, false);
			}

			// 4) Make tree for subBoxes.***
			for(size_t i = 0; i < 8; i++)
			{
				if(children[i]->meshes.size() > 0)
					((SpatialOctreeBox*)children[i])->makeTreeOfUnfixedDepth(minSize, isObjectInOnlyOneLeaf, bSplitMesh);
			}
		}
	}

	void SpatialOctreeBox::setOctreeId(size_t parentId, size_t orderOfChild)
	{
		if(level == 0)
			octreeId = 0;
		else
			octreeId = parentId*10 + (orderOfChild+1);

		size_t childCount = children.size();
		for(size_t i = 0; i < childCount; i++)
		{
			((SpatialOctreeBox*)children[i])->setOctreeId(octreeId, i);
		}
	}

	void SpatialOctreeBox::distributeMeshesIntoEachChildren(bool isObjectInOnlyOneLeaf, bool propagateToDescendents )
	{
		size_t childCount = children.size();
		if(childCount == 0)
			return;

		size_t meshCount = meshes.size();
		size_t vertexCount, surfaceCount, triangleCount;
		double tolerance = 10E-6;
		bool anyTriangleIntersectsWithOctree;
		double v[3][3];
		std::vector<size_t> temp;
		for(size_t i = 0; i < meshCount; i++)
		{
			BoundingBox bbox;
			///< TODO : 중복 제거
			vertexCount = meshes[i]->getVertices().size();
			///< 메쉬의 BB를 계산한다. 
			for(size_t j = 0; j < vertexCount; j++)
				bbox.addPoint(meshes[i]->getVertices()[j]->position.x, meshes[i]->getVertices()[j]->position.y, meshes[i]->getVertices()[j]->position.z);

			for(size_t j = 0; j < childCount; j++)
			{
				// 1차 테스트 : bbox 교차 테스트
				if(children[j]->maxX + tolerance < bbox.minX || children[j]->minX > bbox.maxX + tolerance ||
					children[j]->maxY + tolerance < bbox.minY || children[j]->minY > bbox.maxY + tolerance ||
					children[j]->maxZ + tolerance < bbox.minZ || children[j]->minZ > bbox.maxZ + tolerance )
					continue;

				// 2차 테스트 : mesh를 이루는 각각의 삼각형으로 교차 테스트
				anyTriangleIntersectsWithOctree = false;
				surfaceCount = meshes[i]->getSurfaces().size();
				for(size_t k = 0; k < surfaceCount; k++)
				{
					triangleCount = meshes[i]->getSurfaces()[k]->getTriangles().size();
					for(size_t l = 0; l < triangleCount; l++)
					{
						// 삼각형에서 꼭지점 3개 위치 정보 추출
						for(size_t m = 0; m < 3; m++)
						{
							v[m][0] = meshes[i]->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.x;
							v[m][1] = meshes[i]->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.y;
							v[m][2] = meshes[i]->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.z;
						}

						// 삼각형과 자식 octree가 교차하는지 테스트
						if( GeometryUtility::doesTriangleIntersectWithBox(v[0][0], v[0][1], v[0][2],
																		v[1][0], v[1][1], v[1][2],
																		v[2][0], v[2][1], v[2][2],
																		children[j]->minX, children[j]->minY, children[j]->minZ,
																		children[j]->maxX, children[j]->maxY, children[j]->maxZ) )
						{
							///< 하나라도 겹친다
							anyTriangleIntersectsWithOctree = true;
							break;
						}
					}
					if(anyTriangleIntersectsWithOctree)
						break;
				}

				if(!anyTriangleIntersectsWithOctree)
					continue;

				children[j]->meshes.push_back(meshes[i]);
				///< 한 리프노드에만 메쉬 데이터를 넣을 것인가.
				if(isObjectInOnlyOneLeaf)
				{
					meshes.erase(meshes.begin() + i);
					meshCount--;
					i--;
				}
			}
		}

		if(!isObjectInOnlyOneLeaf)
			meshes.clear();

		///< 아래 children이 leaf가 아니라면 children에 대해서까지 작업을 하려고.
		if(propagateToDescendents)
		{
			for(size_t i = 0; i < childCount; i++)
			{
				if(children[i]->meshes.size() == 0)
					continue;

				((SpatialOctreeBox*)children[i])->distributeMeshesIntoEachChildren(isObjectInOnlyOneLeaf);
			}
		}
	}

	void SpatialOctreeBox::splitMeshIntoEachChildren()
	{
		size_t childCount = children.size();
		if (childCount == 0)
			return;

		double tolerance = 10E-6;
		bool anyTriangleIntersectsWithOctree;
		double v[3][3];
		size_t surfaceCount, triangleCount;
		size_t meshCount = meshes.size();
		for (size_t i = 0; i < meshCount; i++)
		{
			gaia3d::TrianglePolyhedron* mesh = meshes[i];

			for (size_t j = 0; j < childCount; j++)
			{
				BoundingBox& bbox = mesh->getBoundingBox();

				// 1st test : bbox intersection test
				if (children[j]->maxX + tolerance < bbox.minX || children[j]->minX > bbox.maxX + tolerance ||
					children[j]->maxY + tolerance < bbox.minY || children[j]->minY > bbox.maxY + tolerance ||
					children[j]->maxZ + tolerance < bbox.minZ || children[j]->minZ > bbox.maxZ + tolerance)
				{
					continue;
				}

				// 2nd test : intersection test for all triangles in a mesh
				anyTriangleIntersectsWithOctree = false;
				surfaceCount = mesh->getSurfaces().size();
				for (size_t k = 0; k < surfaceCount; k++)
				{
					triangleCount = mesh->getSurfaces()[k]->getTriangles().size();
					for (size_t l = 0; l < triangleCount; l++)
					{
						// extract positions of 3 vertices of a triangle
						for (size_t m = 0; m < 3; m++)
						{
							v[m][0] = mesh->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.x;
							v[m][1] = mesh->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.y;
							v[m][2] = mesh->getSurfaces()[k]->getTriangles()[l]->getVertices()[m]->position.z;
						}

						// intersection test between this triangle and this child octree box
						if (GeometryUtility::doesTriangleIntersectWithBox(v[0][0], v[0][1], v[0][2],
							v[1][0], v[1][1], v[1][2],
							v[2][0], v[2][1], v[2][2],
							children[j]->minX, children[j]->minY, children[j]->minZ,
							children[j]->maxX, children[j]->maxY, children[j]->maxZ))
						{
							anyTriangleIntersectsWithOctree = true;
							break;
						}
					}
					if (anyTriangleIntersectsWithOctree)
						break;
				}

				if (!anyTriangleIntersectsWithOctree)
					continue;

				// clip intersected part and assign it to this children
				gaia3d::TrianglePolyhedron* intersectedPart = NULL;
				gaia3d::TrianglePolyhedron* nonIntersectedPart = NULL;
				clipIntersectedPartWithBox(mesh,
					children[j]->minX, children[j]->minY, children[j]->minZ,
					children[j]->maxX, children[j]->maxY, children[j]->maxZ,
					&intersectedPart, &nonIntersectedPart);

				if (intersectedPart != NULL)
				{
					calculateBoundingBox(intersectedPart);
					children[j]->meshes.push_back(intersectedPart);
				}

				///< 중복 계산 피하려고 원본을 지운다
				delete mesh;
				///< 이후 계산은 할 필요 없다.
				if (nonIntersectedPart == NULL)
				{
					break;
				}

				if (j != childCount - 1)
				{
					calculateBoundingBox(nonIntersectedPart);

					mesh = nonIntersectedPart;
				}
				else
				{
					delete nonIntersectedPart;
				}
			}
		}

		meshes.clear();
	}

	void SpatialOctreeBox::clipIntersectedPartWithBox(gaia3d::TrianglePolyhedron* mesh,
													double minx, double miny, double minz, double maxx, double maxy, double maxz,
													gaia3d::TrianglePolyhedron** intersected,
													gaia3d::TrianglePolyhedron** nonIntersected)
	{
		std::vector<gaia3d::Triangle*> intersectedTriangles;
		std::vector<gaia3d::Triangle*> nonIntersectedTriangles;

		size_t surfaceCount, triangleCount;
		surfaceCount = mesh->getSurfaces().size();
		double v[3][3];
		for (size_t i = 0; i < surfaceCount; i++)
		{
			gaia3d::Surface* surface = mesh->getSurfaces()[i];
			triangleCount = surface->getTriangles().size();
			for (size_t j = 0; j < triangleCount; j++)
			{
				gaia3d::Triangle* triangle = surface->getTriangles()[j];

				for (size_t m = 0; m < 3; m++)
				{
					v[m][0] = triangle->getVertices()[m]->position.x;
					v[m][1] = triangle->getVertices()[m]->position.y;
					v[m][2] = triangle->getVertices()[m]->position.z;
				}

				// intersection test between this triangle and this child octree box
				if (GeometryUtility::doesTriangleIntersectWithBox(v[0][0], v[0][1], v[0][2],
					v[1][0], v[1][1], v[1][2],
					v[2][0], v[2][1], v[2][2],
					minx, miny, minz,
					maxx, maxy, maxz))
				{
					intersectedTriangles.push_back(triangle);
				}
				else
				{
					nonIntersectedTriangles.push_back(triangle);
				}
			}
		}

		if (!intersectedTriangles.empty())
		{
			*intersected = new gaia3d::TrianglePolyhedron;
			gaia3d::Surface* surface = new gaia3d::Surface;
			(*intersected)->getSurfaces().push_back(surface);
			triangleCount = intersectedTriangles.size();
			for (size_t i = 0; i < triangleCount; i++)
			{
				gaia3d::Triangle* triangle = intersectedTriangles[i];

				gaia3d::Triangle* newTriangle = new gaia3d::Triangle;
				surface->getTriangles().push_back(newTriangle);

				gaia3d::Vertex* vertex0 = triangle->getVertices()[0];
				gaia3d::Vertex* newVertex0 = new gaia3d::Vertex;
				newVertex0->color = vertex0->color;
				newVertex0->normal = vertex0->normal;
				newVertex0->position = vertex0->position;
				newVertex0->textureCoordinate[0] = vertex0->textureCoordinate[0];
				newVertex0->textureCoordinate[1] = vertex0->textureCoordinate[1];

				gaia3d::Vertex* vertex1 = triangle->getVertices()[1];
				gaia3d::Vertex* newVertex1 = new gaia3d::Vertex;
				newVertex1->color = vertex1->color;
				newVertex1->normal = vertex1->normal;
				newVertex1->position = vertex1->position;
				newVertex1->textureCoordinate[0] = vertex1->textureCoordinate[0];
				newVertex1->textureCoordinate[1] = vertex1->textureCoordinate[1];

				gaia3d::Vertex* vertex2 = triangle->getVertices()[2];
				gaia3d::Vertex* newVertex2 = new gaia3d::Vertex;
				newVertex2->color = vertex2->color;
				newVertex2->normal = vertex2->normal;
				newVertex2->position = vertex2->position;
				newVertex2->textureCoordinate[0] = vertex2->textureCoordinate[0];
				newVertex2->textureCoordinate[1] = vertex2->textureCoordinate[1];

				newTriangle->setVertices(newVertex0, newVertex1, newVertex2);
				size_t index = (*intersected)->getVertices().size();
				(*intersected)->getVertices().push_back(newVertex0);
				(*intersected)->getVertices().push_back(newVertex1);
				(*intersected)->getVertices().push_back(newVertex2);

				newTriangle->setVertexIndices(index, index + 1, index + 2);
			}

			(*intersected)->setHasNormals(mesh->doesThisHaveNormals());
			(*intersected)->setHasTextureCoordinates(mesh->doesThisHaveTextureCoordinates());
			(*intersected)->setColorMode(mesh->getColorMode());
			(*intersected)->setSingleColor(mesh->getSingleColor());
			(*intersected)->getStringAttributes().insert(mesh->getStringAttributes().begin(), mesh->getStringAttributes().end());
		}

		if (!nonIntersectedTriangles.empty())
		{
			*nonIntersected = new gaia3d::TrianglePolyhedron;
			gaia3d::Surface* surface = new gaia3d::Surface;
			(*nonIntersected)->getSurfaces().push_back(surface);
			triangleCount = nonIntersectedTriangles.size();
			for (size_t i = 0; i < triangleCount; i++)
			{
				gaia3d::Triangle* triangle = nonIntersectedTriangles[i];

				gaia3d::Triangle* newTriangle = new gaia3d::Triangle;
				surface->getTriangles().push_back(newTriangle);

				gaia3d::Vertex* vertex0 = triangle->getVertices()[0];
				gaia3d::Vertex* newVertex0 = new gaia3d::Vertex;
				newVertex0->color = vertex0->color;
				newVertex0->normal = vertex0->normal;
				newVertex0->position = vertex0->position;
				newVertex0->textureCoordinate[0] = vertex0->textureCoordinate[0];
				newVertex0->textureCoordinate[1] = vertex0->textureCoordinate[1];

				gaia3d::Vertex* vertex1 = triangle->getVertices()[1];
				gaia3d::Vertex* newVertex1 = new gaia3d::Vertex;
				newVertex1->color = vertex1->color;
				newVertex1->normal = vertex1->normal;
				newVertex1->position = vertex1->position;
				newVertex1->textureCoordinate[0] = vertex1->textureCoordinate[0];
				newVertex1->textureCoordinate[1] = vertex1->textureCoordinate[1];

				gaia3d::Vertex* vertex2 = triangle->getVertices()[2];
				gaia3d::Vertex* newVertex2 = new gaia3d::Vertex;
				newVertex2->color = vertex2->color;
				newVertex2->normal = vertex2->normal;
				newVertex2->position = vertex2->position;
				newVertex2->textureCoordinate[0] = vertex2->textureCoordinate[0];
				newVertex2->textureCoordinate[1] = vertex2->textureCoordinate[1];

				newTriangle->setVertices(newVertex0, newVertex1, newVertex2);
				size_t index = (*nonIntersected)->getVertices().size();
				(*nonIntersected)->getVertices().push_back(newVertex0);
				(*nonIntersected)->getVertices().push_back(newVertex1);
				(*nonIntersected)->getVertices().push_back(newVertex2);

				newTriangle->setVertexIndices(index, index + 1, index + 2);
			}

			(*nonIntersected)->setHasNormals(mesh->doesThisHaveNormals());
			(*nonIntersected)->setHasTextureCoordinates(mesh->doesThisHaveTextureCoordinates());
			(*nonIntersected)->setColorMode(mesh->getColorMode());
			(*nonIntersected)->setSingleColor(mesh->getSingleColor());
			(*nonIntersected)->getStringAttributes().insert(mesh->getStringAttributes().begin(), mesh->getStringAttributes().end());
		}
	}

	void SpatialOctreeBox::calculateBoundingBox(gaia3d::TrianglePolyhedron* mesh)
	{
		if (mesh->getBoundingBox().isInitialized)
			return;

		std::vector<gaia3d::Vertex*>& vertices = mesh->getVertices();
		size_t vertexCount = vertices.size();
		for (size_t j = 0; j < vertexCount; j++)
		{

			mesh->getBoundingBox().addPoint(vertices[j]->position.x, vertices[j]->position.y, vertices[j]->position.z);
		}
	}

	void SpatialOctreeBox::makeFullCubePyramid(double minCubeSize, bool bObjectInOnlyOneCube, bool bBasedOnMesh)
	{
		if (bBasedOnMesh) // full cube pyramid for meshes
		{
			// TODO(khj 20190218) : NYI must implement making full cube pyramid for 3d meshes
		}
		else // full cube pyramid for point cloud
		{
			size_t maxVertexCountInOneCube = 100000;

			double xLength = maxX - minX, yLength = maxY - minY, zLength = maxZ - minZ;
			double maxEdgeLength = (xLength > yLength) ? ((xLength > zLength) ? xLength : zLength) : ((yLength > zLength) ? yLength : zLength);
			double tolerance = minCubeSize * 0.4;

			size_t vertexCount = meshes[0]->getVertices().size();

			if (maxEdgeLength > minCubeSize + tolerance) // have to make children
			{
				// 1) make 8 children
				for (size_t i = 0; i < 8; i++)
				{
					OctreeBox* child = makeChild();
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

				// 3) distribute points into each children
				std::vector<gaia3d::Vertex*> verticesInThisCube;
				std::vector<gaia3d::Vertex*> verticesInChildren;
				size_t countThisCubeMustHave;
				if (vertexCount > 2 * maxVertexCountInOneCube)
					countThisCubeMustHave = maxVertexCountInOneCube;
				else
					countThisCubeMustHave = vertexCount / 2;

				verticesInThisCube.assign(meshes[0]->getVertices().begin(), meshes[0]->getVertices().begin() + countThisCubeMustHave);
				verticesInChildren.assign(meshes[0]->getVertices().begin() + countThisCubeMustHave, meshes[0]->getVertices().end());

				std::random_shuffle(verticesInThisCube.begin(), verticesInThisCube.end());
				meshes[0]->getVertices().clear();
				meshes[0]->getVertices().assign(verticesInThisCube.begin(), verticesInThisCube.end());

				size_t countChildrenMustHave = verticesInChildren.size();
				OctreeBox* targetChild = NULL;
				for (size_t i = 0; i < countChildrenMustHave; i++)
				{
					gaia3d::Point3D pos = verticesInChildren[i]->position;
					if (pos.x < halfX)
					{
						if (pos.y < halfY)
						{
							if (pos.z < halfZ)
								targetChild = children[0];
							else
								targetChild = children[4];
						}
						else
						{
							if (pos.z < halfZ)
								targetChild = children[3];
							else
								targetChild = children[7];
						}
					}
					else
					{
						if (pos.y < halfY)
						{
							if (pos.z < halfZ)
								targetChild = children[1];
							else
								targetChild = children[5];
						}
						else
						{
							if (pos.z < halfZ)
								targetChild = children[2];
							else
								targetChild = children[6];
						}
					}

					if (targetChild->meshes.empty())
					{
						gaia3d::TrianglePolyhedron* childPolyhedron = new gaia3d::TrianglePolyhedron;
						targetChild->meshes.push_back(childPolyhedron);
					}

					targetChild->meshes[0]->getVertices().push_back(verticesInChildren[i]);
				}

				// 4) Make tree for subBoxes.***
				for (size_t i = 0; i < 8; i++)
				{
					if (children[i]->meshes.size() > 0)
						((SpatialOctreeBox*)children[i])->makeFullCubePyramid(minCubeSize, bObjectInOnlyOneCube, bBasedOnMesh);
				}
			}

			// 5) divide vertices in this cube into partitions if the vertex count is over the max count
			vertexCount = meshes[0]->getVertices().size();
			if (vertexCount > maxVertexCountInOneCube)
			{
				size_t partitionCount;
				if (vertexCount % maxVertexCountInOneCube == 0)
					partitionCount = vertexCount / maxVertexCountInOneCube;
				else
					partitionCount = vertexCount / maxVertexCountInOneCube + 1;

				std::vector<gaia3d::Vertex*> allVertices;
				allVertices.assign(meshes[0]->getVertices().begin(), meshes[0]->getVertices().end());
				meshes[0]->getVertices().clear();
				delete meshes[0];
				meshes.clear();

				for (size_t i = 0; i < partitionCount; i++)
				{
					gaia3d::TrianglePolyhedron* partition = new gaia3d::TrianglePolyhedron;
					if (i == partitionCount - 1)
					{
						partition->getVertices().assign(allVertices.begin() + i * maxVertexCountInOneCube, allVertices.end());
						//allVertices.clear();
					}
					else
					{
						partition->getVertices().assign(allVertices.begin() + i * maxVertexCountInOneCube, allVertices.begin() + (i + 1)*maxVertexCountInOneCube);
						//allVertices.erase(allVertices.begin(), allVertices.begin() + maxVertexCountInOneCube);
					}
					meshes.push_back(partition);
				}

				allVertices.clear();
			}
		}
	}

	void SpatialOctreeBox::makeSkinMesh(std::vector<gaia3d::Triangle*>& triangles)
	{
		if (children.empty())
		{
			this->prettySkinMesh = GeometryUtility::makeSingleMeshWithTriangles(triangles, false);
			calculateBoundingBox(prettySkinMesh);
		}
		else
		{
			double tolerance = 1E-7;
			gaia3d::Point3D center;
			center.set((maxX - minX) / 2.0, (maxY - minY) / 2.0, (maxZ - minZ) / 2.0);

			// divide and group triangles along xy plane
			double a = 0.0, b = 0.0, c = 1.0;
			double d = -a * center.x - b * center.y - c * center.z;
			std::vector<gaia3d::Triangle*> upperTriangles, lowerTriangles, coplanarTriangles;
			for (size_t i = 0; i < triangles.size(); i++)
			{
				gaia3d::Triangle* triangle = triangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, upperTriangles, lowerTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			triangles.clear();

			//// insert coplanar triangles into lower triangle container
			lowerTriangles.insert(lowerTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			// divide and group upper/lower triangles along zx plane
			a = 0.0, b = 1.0, c = 0.0;
			d = -a * center.x - b * center.y - c * center.z;
			std::vector<gaia3d::Triangle*> upperFrontTriangles, upperRearTriangles, lowerFrontTriangles, lowerRearTriangles;

			//// upper triangle container
			for (size_t i = 0; i < upperTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = upperTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, upperRearTriangles, upperFrontTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			upperTriangles.clear();

			////// insert coplanar triangles into upper front triangle container
			upperFrontTriangles.insert(upperFrontTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			//// lower triangle container
			for (size_t i = 0; i < lowerTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = lowerTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, lowerRearTriangles, lowerFrontTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			lowerTriangles.clear();

			////// insert coplanar triangles into lower front triangle container
			lowerFrontTriangles.insert(lowerFrontTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			// divide and group triangles along yz plane
			a = 1.0, b = 0.0, c = 0.0;
			d = -a * center.x - b * center.y - c * center.z;

			std::vector<Triangle*> upperFrontLeftTriangles, upperFrontRightTriangles, upperRearLeftTriangles, upperRearRightTriangles;
			std::vector<Triangle*> lowerFrontLeftTriangles, lowerFrontRightTriangles, lowerRearLeftTriangles, lowerRearRightTriangles;

			//// upper front triangle contaienr
			for (size_t i = 0; i < upperFrontTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = upperFrontTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, upperFrontRightTriangles, upperFrontLeftTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			upperFrontTriangles.clear();

			////// insert coplanar triangles into upper front left triangle container
			upperFrontLeftTriangles.insert(upperFrontLeftTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			//// upper rear triangle contaienr
			for (size_t i = 0; i < upperRearTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = upperRearTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, upperRearRightTriangles, upperRearLeftTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			upperRearTriangles.clear();

			////// insert coplanar triangles into upper rear left triangle container
			upperRearLeftTriangles.insert(upperRearLeftTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			//// lower front triangle contaienr
			for (size_t i = 0; i < lowerFrontTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = lowerFrontTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, lowerFrontRightTriangles, lowerFrontLeftTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			lowerFrontTriangles.clear();

			////// insert coplanar triangles into lower front left triangle container
			lowerFrontLeftTriangles.insert(lowerFrontLeftTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			//// lower rear triangle contaienr
			for (size_t i = 0; i < lowerRearTriangles.size(); i++)
			{
				gaia3d::Triangle* triangle = lowerRearTriangles[i];
				gaia3d::GeometryUtility::divideTriangleWithPlane(a, b, c, d, triangle, lowerRearRightTriangles, lowerRearLeftTriangles, coplanarTriangles, tolerance);
				delete triangle;
			}
			lowerRearTriangles.clear();

			////// insert coplanar triangles into lower front left triangle container
			lowerRearLeftTriangles.insert(lowerRearLeftTriangles.end(), coplanarTriangles.begin(), coplanarTriangles.end());
			coplanarTriangles.clear();

			// distribute re-grouped triangles into 8 children
			// Bottom                      Top
			// +---------+---------+     +---------+---------+       rear
			// |         |         |     |         |         |       Y
			// |   3     |   2     |	 |   7     |   6     |       ^
			// |         |         |     |         |         |       |
			// |---------+---------|     |---------+---------|       |
			// |         |         |     |         |         |       |
			// |    0    |    1    |     |   4     |   5     |       |
			// |         |         |     |         |         |       +----------------> X
			// +---------+---------+     +---------+---------+       front

			((gaia3d::SpatialOctreeBox*)children[0])->makeSkinMesh(lowerFrontLeftTriangles);
			((gaia3d::SpatialOctreeBox*)children[1])->makeSkinMesh(lowerFrontRightTriangles);
			((gaia3d::SpatialOctreeBox*)children[2])->makeSkinMesh(lowerRearRightTriangles);
			((gaia3d::SpatialOctreeBox*)children[3])->makeSkinMesh(lowerRearLeftTriangles);
			((gaia3d::SpatialOctreeBox*)children[4])->makeSkinMesh(upperFrontLeftTriangles);
			((gaia3d::SpatialOctreeBox*)children[5])->makeSkinMesh(upperFrontLeftTriangles);
			((gaia3d::SpatialOctreeBox*)children[6])->makeSkinMesh(upperRearRightTriangles);
			((gaia3d::SpatialOctreeBox*)children[7])->makeSkinMesh(upperRearLeftTriangles);
		}
	}
}