/**
 * GeometryUtility Header
 */
#ifndef _GEOMETRYUTILITY_H_
#define _GEOMETRYUTILITY_H_
#pragma once

#include <vector>
#include <cmath>
#include <cstddef>   // size_t

#define M_PI 3.14159265358979323846
#define EarthHRadius 6378137.0
#define EarthVRadius 6356751.9566//10375702081371855231

#include "../geometry/TrianglePolyhedron.h"


namespace gaia3d
{
	class LegoBlock;

	class GeometryUtility
	{
	public:
		///< 점 3개로 이루어진 평면의 normal vector를 계산
		static void calculatePlaneNormal(double& x0, double& y0, double& z0,
										double& x1, double& y1, double& z1,
										double& x2, double& y2, double& z2,
										double& nX, double& nY, double& nZ,
										bool bNormalize);

		static void crossProduct(double x0, double y0, double z0,
								double x1, double y1, double z1,
								double& nX, double& nY, double& nZ);

		enum GeomType {POLYHEDRON, TRIANGLE, POINT};
		static bool areTwoCongruentWithEachOther(void* geom1, void* geom2, void* transform, double tolerance, GeomType geomType);

		static double angleBetweenTwoVectors(double x1, double y1, double z1, double x2, double y2, double z2);

		static bool isInsideBox(double x, double y, double z,
								double minX, double minY, double minZ,
								double maxX, double maxY, double maxZ);

		static bool doesTriangleIntersectWithBox(double& x1, double& y1, double& z1,
												double& x2, double& y2, double& z2,
												double& x3, double& y3, double& z3,
												double& minX, double& minY, double& minZ,
												double& maxX, double& maxY, double& maxZ);

		static void wgs84ToAbsolutePosition(double&lon, double& lat, double& alt, double* result);
		static void normalAtAbsolutePosition(double& x, double& y, double& z, double* result);
		static void transformMatrixAtAbsolutePosition(double& x, double& y, double& z, double* m);

		static void mergeLegoBlocksAlongZAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);

		static void mergeLegoBlocksAlongYAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);

		static void mergeLegoBlocksAlongXAxis(std::vector<LegoBlock*>& legos, bool mustSameColor);

		static bool earCut(double** xs, double** ys, double** zs, std::vector<size_t>& eachRingPointCount, std::vector<std::pair<size_t, size_t>>& result, bool bDebug = false);

		static void tessellate(double* xs, double* ys, double* zs, size_t vertexCount, std::vector<size_t>& polygonIndices, std::vector<size_t>& indices);

		static gaia3d::TrianglePolyhedron* makeSingleMeshWithTriangles(std::vector<gaia3d::Triangle*>& triangles, bool bCopy = true);

		static int divideTriangleWithPlane(
			double a, double b, double c, double d,
			gaia3d::Triangle* triangle,
			std::vector<gaia3d::Triangle*>& frontTriangles,
			std::vector<gaia3d::Triangle*>& rearTriangles,
			std::vector<gaia3d::Triangle*>& coplanarTriangles,
			double tolerance
		);

		static int findRelationBetweenTriangleAndPlane(
			double a, double b, double c, double d,
			gaia3d::Triangle* triangle,
			std::vector<unsigned int>& frontVertices,
			std::vector<unsigned int>& rearVertices,
			std::vector<unsigned int>& coplanarVertices,
			double tolerance
		);

		static int findRelationBetweenPointAndPlane(
			double a, double b, double c, double d,
			gaia3d::Point3D& point,
			double tolerance
		);

		static bool findIntersectionPointBetweenPlaneAndLine(
			double a, double b, double c, double d,
			gaia3d::Point3D& lineStart, gaia3d::Point3D& lineEnd,
			gaia3d::Point3D& intersectionPoint,
			double tolerance
		);
	};
}

#endif // _GEOMETRYUTILITY_H_
