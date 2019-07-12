#ifndef _POINT2D_H_
#define _POINT2D_H_
#pragma once
#include <iostream>
#include "Point3D.h"
using namespace std;

namespace gaia3d {
	class Point2D {
	public:
		
		double xy[2];
		double rgb[3];
		gaia3d::Point3D originalPoint;
		int originalIndex;
		double x, y;
		double r, g, b;
		
		Point2D(double xp, double yp);
		Point2D();
		Point2D(double vec[]);
		inline double length() const;
		inline double sqrLength() const;

		Point2D  normal() const;
		Point2D& normalEq();
		Point2D& normalEq(const double length);
		Point2D operator+(const Point2D& rhs) const;
		Point2D operator+(const double _v) const;
		Point2D operator-(const Point2D& rhs) const;
		Point2D operator-(const double _v) const;
		Point2D operator-() const;
		Point2D operator*(const Point2D& rhs) const;
		Point2D operator*(const double rhs) const;

		Point2D operator/(const Point2D& rhs) const;
		Point2D operator/(const double rhs) const;

		double cross(const Point2D& vec) const;
		double scalar(const Point2D& vec) const;
		double angleToVector(Point2D vector) const;
		inline bool operator==(const Point2D& rhs) const;
		inline bool operator==(Point2D rhs);
		inline bool operator!=(const Point2D& rhs) const;

		inline operator double*() { return xy; }
		inline operator const double*() const { return xy; }

		void unitary();
		double distTo(Point2D target);
		bool isSame(Point2D target);
	};



}
#endif
