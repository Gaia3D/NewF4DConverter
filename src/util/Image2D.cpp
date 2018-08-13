/**
* Implementation of the Image2D class
*/
#include "Image2D.h"

namespace gaia3d
{
	Image2D::Image2D()
	{
		this->m_image = 0;
		this->m_dimensions = 4; // default value.***
	}
	
	Image2D::~Image2D()
	{
		if (m_image)
			delete[] m_image;
	}

	void Image2D::Set_Image(unsigned char *image, int width, int height)
	{
		this->m_image = image;
		this->m_imageWidth = width;
		this->m_imageHeight = height;
	}

	int Image2D::Get_Idx(int col, int row)
	{
		return row * this->m_imageWidth + col;
	}

	unsigned char* Image2D::Get_Color_RGB(int col, int row)
	{
		unsigned char* colorRGB = new unsigned char[3];

		int idx = this->Get_Idx(col, row);
		colorRGB[0] = this->m_image[idx*this->m_dimensions];
		colorRGB[1] = this->m_image[idx*this->m_dimensions + 1];
		colorRGB[2] = this->m_image[idx*this->m_dimensions + 2];

		return colorRGB;
	}

	unsigned char* Image2D::Get_Color_RGBA(int col, int row)
	{
		unsigned char* colorRGB = new unsigned char[4];

		int idx = this->Get_Idx(col, row);
		colorRGB[0] = this->m_image[idx*this->m_dimensions];
		colorRGB[1] = this->m_image[idx*this->m_dimensions + 1];
		colorRGB[2] = this->m_image[idx*this->m_dimensions + 2];
		colorRGB[3] = this->m_image[idx*this->m_dimensions + 2];

		return colorRGB;
	}

	void Image2D::Set_Color_RGB(int col, int row, unsigned char *colorRGB)
	{
		int idx = this->Get_Idx(col, row);
		this->m_image[idx*this->m_dimensions] = colorRGB[0];
		this->m_image[idx*this->m_dimensions + 1] = colorRGB[1];
		this->m_image[idx*this->m_dimensions + 2] = colorRGB[2];
	}

	void Image2D::Set_Color_RGBA(int col, int row, unsigned char *colorRGBA)
	{
		if (col >= this->m_imageWidth)
			return;

		if (row >= this->m_imageHeight)
			return;

		int idx = this->Get_Idx(col, row);
		this->m_image[idx*this->m_dimensions] = colorRGBA[0];
		this->m_image[idx*this->m_dimensions + 1] = colorRGBA[1];
		this->m_image[idx*this->m_dimensions + 2] = colorRGBA[2];
		this->m_image[idx*this->m_dimensions + 3] = colorRGBA[3];
	}

	void Image2D::Set_Color_ARGB(int col, int row, unsigned char *colorRGBA)
	{
		if (col >= this->m_imageWidth)
			return;

		if (row >= this->m_imageHeight)
			return;

		int idx = this->Get_Idx(col, row);
		this->m_image[idx*this->m_dimensions] = colorRGBA[3];
		this->m_image[idx*this->m_dimensions + 1] = colorRGBA[0];
		this->m_image[idx*this->m_dimensions + 2] = colorRGBA[1];
		this->m_image[idx*this->m_dimensions + 3] = colorRGBA[2];
	}
}
