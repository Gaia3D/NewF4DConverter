/**
* Rectangle Header
*/
#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_
#pragma once

namespace gaia3d
{
	class Rectangle
	{
	public:

		double m_minX, m_maxX, m_minY, m_maxY;

		Rectangle();
		~Rectangle();

		void Add_Point(double x, double y);
		void Add_Rectangle(Rectangle *rect);
		void CopyFrom(Rectangle *rect);
		double Get_Height();
		double Get_Width();
		double Get_Perimeter();
		bool Intersection_withPoint(double x, double y, double error);
		bool Intersection_withRectangle(Rectangle *rect);
		bool Intersection_withRectangle(Rectangle *rect, double error);
		void Set(double minX, double minY, double maxX, double maxY);
		void Set_Init(double x, double y);
	};

}

#endif // _RECTANGLE_H_
