﻿/**
* Image2D Header
*/
#ifndef _IMAGE2D_H_
#define _IMAGE2D_H_
#pragma once

namespace gaia3d
{
	class Image2D
	{
	public:

		unsigned char* m_image;
		int m_imageWidth, m_imageHeight;
		int m_dimensions; // 3 for RGB, 4 for RGBA

		Image2D();
		~Image2D();

		unsigned char* Get_Color_RGB(int col, int row);
		int Get_Idx(int col, int row);
		void Set_Color_RGB(int col, int row, unsigned char *colorRGB);
		void Set_Color_RGBA(int col, int row, unsigned char *colorRGBA);
		void Set_Color_ARGB(int col, int row, unsigned char *colorRGBA);
		unsigned char* Get_Color_RGBA(int col, int row);
		void Set_Image(unsigned char *image, int width, int height);
	};
}

#endif // _IMAGE2D_H_
