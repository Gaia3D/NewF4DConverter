#ifndef _FACE_H_
#define _FACE_H_

#pragma once
#include <iostream>
#include <vector>
#include "Point3D.h"
#include "Point2D.h"
#include "Polygon2D.h"
#include "LinearRing.h"
#include "Vertex.h"
#include "Triangle.h"

using namespace std;

namespace gaia3d {
	class Face {
		vector<Vertex>vertexArray;
		Point3D calculateNormal();
		Point3D _normal;
		bool hasNormalValue;
	public:
		Face();
		void addVertex(Vertex a);
		void setVertexArray(vector<Vertex> arr);
		vector<Vertex> getVertexArray();
		Point3D getNormal();
		void setNormal();
		int getBestFacePlaneTypeToProject();
		Polygon2D getProjectedPolygon();
		void calculateVerticesNormals();
		vector<Triangle> getTessellatedTriangles();
		vector<Triangle> getTrianglesConvex();
		//void convertFromPolygon(shared_ptr<indoorgml::Polygon> p);

	};

}
#endif