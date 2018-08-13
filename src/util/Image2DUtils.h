/**
* Image2DUtils Header
*/
#ifndef _IMAGE2DUTILS_H_
#define _IMAGE2DUTILS_H_
#pragma once

#include "Image2D.h"
#include "Image2DSplitData.h"

namespace gaia3d
{
	class Image2DUtils
	{
	public:

		std::vector<Image2DSplitData*> m_vec_image2DSplitDatas;

		Image2DUtils();
		~Image2DUtils();

		int* Get_AreaOnImage_OfNormalizedRectangle(Image2D *image, Rectangle *normalizedRectangle);

		void MULTISPLITTIMAGE_Add_ImageRectangle(Rectangle* imageRectangle, gaia3d::Triangle *triangle);
		void MULTISPLITTIMAGE_Delete_Image2DSplitDatas();
		void MULTISPLITTIMAGE_Make_SplittedMosaic();
		bool MULTISPLITTIMAGE_Recombine_ImageRectangles();
		void InsertImage_ARGB(Image2D *image, Rectangle *rectSplitter, Image2D *imageToInsert_RGBA);
		void InsertImage_RGBA(Image2D *image, Rectangle *rectSplitter, Image2D *imageToInsert_RGBA);
		void Get_Region(Image2D *image, Rectangle *rectSplitter, Image2D *resultSplittedImage_RGBA);

		bool TEST__AllImage2DSplidatas_AreTexCoordsInside_rectangleOriginal();

	private:
		void MULTISPLITTIMAGE_Get_BestPositionMosaic(std::vector<Image2DSplitData*> &vec_splitDatasMosaic, Image2DSplitData *splitData_toPutInMosaic, double *posX, double *posY);
		bool MULTISPLITTIMAGE_IntersectsRectangle(std::vector<Rectangle*> &vec_rectangles, Rectangle *rectangle);
		bool MULTISPLITTIMAGE_TryToRecombine_ImageRectangle(Image2DSplitData *image2dSplitData);
	};
}

#endif // _IMAGE2DUTILS_H_
