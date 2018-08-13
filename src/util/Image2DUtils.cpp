/**
* Implementation of the Image2D class
*/
#include "Image2DUtils.h"

#include <algorithm>

namespace gaia3d
{
	Image2DUtils::Image2DUtils()
	{
	}


	Image2DUtils::~Image2DUtils()
	{
		this->MULTISPLITTIMAGE_Delete_Image2DSplitDatas();
	}

	int* Image2DUtils::Get_AreaOnImage_OfNormalizedRectangle(Image2D *image, Rectangle *normalizedRectangle)
	{
		// return x, y, width and height.***
		int x, y, width, height;
		x = int(double(image->m_imageWidth) * normalizedRectangle->m_minX);
		y = int(double(image->m_imageHeight) * normalizedRectangle->m_minY);
		width = int(double(image->m_imageWidth) * normalizedRectangle->m_maxX) - x;
		height = int(double(image->m_imageHeight) * normalizedRectangle->m_maxY) - y;

		int* area = new int[4];
		area[0] = x;
		area[1] = y;
		area[2] = width;
		area[3] = height;

		return area;
	}

	void Image2DUtils::InsertImage_RGBA(Image2D *image, Rectangle *rectSplitter, Image2D *imageToInsert_RGBA)
	{
		// inserts "imageToInsert" into "image" in the position determined by "rectSplitter".***
		int *imageArea = this->Get_AreaOnImage_OfNormalizedRectangle(image, rectSplitter);

		int x, y, width, height;
		x = imageArea[0];
		y = imageArea[1];
		width = imageToInsert_RGBA->m_imageWidth;
		height = imageToInsert_RGBA->m_imageHeight;
		delete[] imageArea;

		int idx = 0;
		int col_big = 0;
		int row_big = 0;
		for (int row = 0; row < height; row++)
		{
			for (int col = 0; col < width; col++)
			{
				unsigned char *color_RGB = imageToInsert_RGBA->Get_Color_RGB(col, row);
				col_big = x + col;
				row_big = y + row;


				unsigned char color_RGBA[4];
				color_RGBA[0] = color_RGB[0];
				color_RGBA[1] = color_RGB[1];
				color_RGBA[2] = color_RGB[2];
				color_RGBA[3] = 255;

				image->Set_Color_RGBA(col_big, row_big, color_RGBA);
				delete[] color_RGB;
			}
		}
	}

	void Image2DUtils::InsertImage_ARGB(Image2D *image, Rectangle *rectSplitter, Image2D *imageToInsert_RGBA)
	{
		// inserts "imageToInsert" into "image" in the position determined by "rectSplitter".***
		int *imageArea = this->Get_AreaOnImage_OfNormalizedRectangle(image, rectSplitter);

		int x, y, width, height;
		x = imageArea[0];
		y = imageArea[1];
		width = imageToInsert_RGBA->m_imageWidth;
		height = imageToInsert_RGBA->m_imageHeight;
		delete[] imageArea;

		int idx = 0;
		int col_big = 0;
		int row_big = 0;
		for (int row = 0; row < height; row++)
		{
			for (int col = 0; col < width; col++)
			{
				unsigned char *color_RGB = imageToInsert_RGBA->Get_Color_RGB(col, row);
				col_big = x + col;
				row_big = y + row;


				unsigned char color_RGBA[4];
				color_RGBA[0] = color_RGB[0];
				color_RGBA[1] = color_RGB[1];
				color_RGBA[2] = color_RGB[2];
				color_RGBA[3] = 255;
				delete[] color_RGB;

				image->Set_Color_ARGB(col_big, row_big, color_RGBA);
			}
		}
	}

	void Image2DUtils::Get_Region(Image2D *image, Rectangle *rectSplitter, Image2D *resultSplittedImage_RGBA)
	{
		// 1rst, need the area that split.***
		int *imageArea = this->Get_AreaOnImage_OfNormalizedRectangle(image, rectSplitter);

		int x, y, width, height;
		x = imageArea[0];
		y = imageArea[1];
		width = imageArea[2];
		height = imageArea[3];
		delete[] imageArea;

		int splittedImageSize = width * height;
		unsigned char *splittedImage = new unsigned char[splittedImageSize * 4];
		resultSplittedImage_RGBA->Set_Image(splittedImage, width, height);
		int counter = 0;
		for (int row = y; row < y + height; row++)
		{
			for (int col = x; col < x + width; col++)
			{
				unsigned char *color_RGB = image->Get_Color_RGB(col, row);
				// Provisionally returns in RGBA.***
				splittedImage[counter] = color_RGB[0]; counter++; // R.***
				splittedImage[counter] = color_RGB[1]; counter++; // G.***
				splittedImage[counter] = color_RGB[2]; counter++; // B.***
				splittedImage[counter] = 255; counter++;
				delete[] color_RGB;
			}
		}
	}

	bool Image2DUtils::TEST__AllImage2DSplidatas_AreTexCoordsInside_rectangleOriginal()
	{
		bool allImage2DSplidatas_AreTexCoordsInside_rectangleOriginal = true;

		size_t splitDatasCount = this->m_vec_image2DSplitDatas.size();
		for (size_t i = 0; i < splitDatasCount; i++)
		{
			Image2DSplitData *splitData = this->m_vec_image2DSplitDatas[i];
			if (!splitData->TEST__AreTexCoordsInside_rectangleOriginal())
			{
				return false;
			}
		}

		return allImage2DSplidatas_AreTexCoordsInside_rectangleOriginal;
	}

	void Image2DUtils::MULTISPLITTIMAGE_Delete_Image2DSplitDatas()
	{
		size_t image2dSplitDatasCount = this->m_vec_image2DSplitDatas.size();
		for (size_t i = 0; i < image2dSplitDatasCount; i++)
		{
			delete this->m_vec_image2DSplitDatas[i];
		}
		this->m_vec_image2DSplitDatas.clear();
	}

	void Image2DUtils::MULTISPLITTIMAGE_Make_SplittedMosaic()
	{
		// In this function we must calculate the "m_rectangleImage_splitted" of all splitDatas.***
		// Separate the rectangles by widthMajor & heightMajor. "widthMajor" = rectangles that width > height.***

		std::vector<Image2DSplitData*> vec_widthMajor_splitDatas;
		std::vector<Image2DSplitData*> vec_heightMajor_splitDatas;

		size_t splitDatasCount = this->m_vec_image2DSplitDatas.size();
		for (size_t i = 0; i < splitDatasCount; i++)
		{
			Image2DSplitData *splitData = this->m_vec_image2DSplitDatas[i];
			Rectangle *currRect_original = splitData->m_rectangleImage_original;

			if (currRect_original->Get_Width() > currRect_original->Get_Height())
			{
				vec_widthMajor_splitDatas.push_back(splitData);
			}
			else
			{
				vec_heightMajor_splitDatas.push_back(splitData);
			}
		}

		// now sort major to minor the vectors.***
		std::sort(vec_widthMajor_splitDatas.begin(), vec_widthMajor_splitDatas.end(), [](Image2DSplitData *splitData_A, Image2DSplitData *splitData_B)
		{
			double width_A = splitData_A->m_rectangleImage_original->Get_Width();
			double width_B = splitData_B->m_rectangleImage_original->Get_Width();

			if (width_A > width_B)
				return true;
			else
				return false;
		});

		std::sort(vec_heightMajor_splitDatas.begin(), vec_heightMajor_splitDatas.end(), [](Image2DSplitData *splitData_A, Image2DSplitData *splitData_B)
		{
			double height_A = splitData_A->m_rectangleImage_original->Get_Height();
			double height_B = splitData_B->m_rectangleImage_original->Get_Height();

			if (height_A > height_B)
				return true;
			else
				return false;
		});

		// now, put in a vector all splitDatas alternativelly, 1rst widthMajor, 2nd heightMajor, 3rd widthMajor, etc.***
		std::vector<Image2DSplitData*> vec_sorted_splitDatas;
		if (vec_widthMajor_splitDatas.size() > 0)
		{
			if (vec_heightMajor_splitDatas.size() > 0)
			{
				bool finished = false;
				size_t i = 0;
				size_t widthMajorDatasCount = vec_widthMajor_splitDatas.size();
				size_t heightMajorDatasCount = vec_heightMajor_splitDatas.size();
				if (vec_widthMajor_splitDatas[0]->m_rectangleImage_original->Get_Width() > vec_heightMajor_splitDatas[0]->m_rectangleImage_original->Get_Height())
				{
					// start by widthMajors.***
					while (!finished)
					{
						if (i < widthMajorDatasCount)
							vec_sorted_splitDatas.push_back(vec_widthMajor_splitDatas[i]);

						if (i < heightMajorDatasCount)
							vec_sorted_splitDatas.push_back(vec_heightMajor_splitDatas[i]);

						if (i >= widthMajorDatasCount && i >= heightMajorDatasCount)
							finished = true;

						i++;
					}
				}
				else
				{
					// start by heightMajors.***
					while (!finished)
					{
						if (i < heightMajorDatasCount)
							vec_sorted_splitDatas.push_back(vec_heightMajor_splitDatas[i]);

						if (i < widthMajorDatasCount)
							vec_sorted_splitDatas.push_back(vec_widthMajor_splitDatas[i]);

						if (i >= widthMajorDatasCount && i >= heightMajorDatasCount)
							finished = true;

						i++;
					}
				}
			}
			else
			{
				// there are only widthMajor datas.***
				vec_sorted_splitDatas.insert(vec_sorted_splitDatas.end(), vec_widthMajor_splitDatas.begin(), vec_widthMajor_splitDatas.end());
			}
		}
		else
		{
			// there are only heightMajor datas.***
			vec_sorted_splitDatas.insert(vec_sorted_splitDatas.end(), vec_heightMajor_splitDatas.begin(), vec_heightMajor_splitDatas.end());
		}

		// start the mosaic process.************************************************************************
		std::vector<Image2DSplitData*> vec_process_splitDatas;
		splitDatasCount = vec_sorted_splitDatas.size();
		for (size_t i = 0; i < splitDatasCount; i++)
		{
			Image2DSplitData *currSplitData = vec_sorted_splitDatas[i];

			double original_width = currSplitData->m_rectangleImage_original->Get_Width();
			double original_height = currSplitData->m_rectangleImage_original->Get_Height();
			currSplitData->m_rectangleImage_splitted = new Rectangle();

			if (i == 0)
			{
				// put in the left down corner.***
				currSplitData->m_rectangleImage_splitted->Set(0.0, 0.0, original_width, original_height);
			}
			else
			{
				double posX = 0.0;
				double posY = 0.0;
				this->MULTISPLITTIMAGE_Get_BestPositionMosaic(vec_process_splitDatas, currSplitData, &posX, &posY);
				// setUp our rectangle.***

				currSplitData->m_rectangleImage_splitted->Set(posX, posY, posX + original_width, posY + original_height);
				int hola = 0;
			}
			vec_process_splitDatas.push_back(currSplitData);
		}

	}

	void Image2DUtils::MULTISPLITTIMAGE_Get_BestPositionMosaic(std::vector<Image2DSplitData*> &vec_splitDatasMosaic, Image2DSplitData *splitData_toPutInMosaic, double *posX, double *posY)
	{
		// Private.***
		// Private.***
		// find the best position (leftDown corner of the "splitData_toPutInMosaic").***
		double currPosX, currPosY;
		double candidatePosX, candidatePosY;
		double currMosaicPerimeter, candidateMosaicPerimeter;
		candidatePosX = 0.0;
		candidatePosY = 0.0;
		candidateMosaicPerimeter = -1;

		Rectangle *rect_toPutInMosaic = splitData_toPutInMosaic->m_rectangleImage_original;

		// make existent rectangles vector.***
		std::vector<Rectangle*> vec_rectangles;
		Rectangle beforeMosaicRectangle;
		size_t existentSplitDatasCount = vec_splitDatasMosaic.size();
		for (size_t i = 0; i < existentSplitDatasCount; i++)
		{
			Image2DSplitData *splitData = vec_splitDatasMosaic[i];
			Rectangle *currRect = splitData->m_rectangleImage_splitted;
			if (i == 0)
			{
				beforeMosaicRectangle.CopyFrom(currRect);
			}
			else
			{
				beforeMosaicRectangle.Add_Rectangle(currRect);
			}

			// profit this "for" and make rectangles vector.***
			vec_rectangles.push_back(currRect);
		}

		// Now, try to find the best positions to put our rectangle.***
		for (int i = 0; i < existentSplitDatasCount; i++)
		{
			Image2DSplitData *splitData = vec_splitDatasMosaic[i];
			Rectangle *currRect = splitData->m_rectangleImage_splitted; // Atention!, here uses only the "m_rectangleImage_splitted".***

																			// for each existent rectangles, there are 2 possibles positions: leftUp & rightDown.***
																			// in this 2 possibles positions we put our leftDownCorner of rectangle of "splitData_toPutInMosaic".***

																			// If in any of two positions our rectangle intersects with any other rectangle, then discard.***
																			// If no intersects with others rectangles, then calculate the mosaic-perimeter.
																			// We choose the minor perimeter of the mosaic.***

			if (splitData_toPutInMosaic->m_rectangleImage_splitted == NULL)
				splitData_toPutInMosaic->m_rectangleImage_splitted = new Rectangle();

			double width = splitData_toPutInMosaic->m_rectangleImage_original->Get_Width();
			double height = splitData_toPutInMosaic->m_rectangleImage_original->Get_Height();

			// 1- leftUp cornar.***
			currPosX = currRect->m_minX;
			currPosY = currRect->m_maxY;

			// setUp our rectangle.***
			splitData_toPutInMosaic->m_rectangleImage_splitted->Set(currPosX, currPosY, currPosX + width, currPosY + height);

			// put our rectangle into mosaic & check that no intersects with another rectangles.***
			if (!this->MULTISPLITTIMAGE_IntersectsRectangle(vec_rectangles, splitData_toPutInMosaic->m_rectangleImage_splitted))
			{
				// now, calculate the perimeter of the boundingRect.***
				Rectangle afterMosaicRectangle;
				afterMosaicRectangle.CopyFrom(&beforeMosaicRectangle);
				afterMosaicRectangle.Add_Rectangle(splitData_toPutInMosaic->m_rectangleImage_splitted);
				if (candidateMosaicPerimeter < 0.0)
				{
					candidateMosaicPerimeter = afterMosaicRectangle.Get_Perimeter();
					candidatePosX = currPosX;
					candidatePosY = currPosY;
				}
				else
				{
					currMosaicPerimeter = afterMosaicRectangle.Get_Perimeter();
					if (currMosaicPerimeter < candidateMosaicPerimeter)
					{
						candidateMosaicPerimeter = currMosaicPerimeter;
						candidatePosX = currPosX;
						candidatePosY = currPosY;
					}
				}
			}

			// 2- rightDown cornar.***
			currPosX = currRect->m_maxX;
			currPosY = currRect->m_minY;

			// setUp our rectangle.***
			splitData_toPutInMosaic->m_rectangleImage_splitted->Set(currPosX, currPosY, currPosX + width, currPosY + height);

			// put our rectangle into mosaic & check that no intersects with another rectangles.***
			if (!this->MULTISPLITTIMAGE_IntersectsRectangle(vec_rectangles, splitData_toPutInMosaic->m_rectangleImage_splitted))
			{
				// now, calculate the perimeter of the boundingRect.***
				Rectangle afterMosaicRectangle;
				afterMosaicRectangle.CopyFrom(&beforeMosaicRectangle);
				afterMosaicRectangle.Add_Rectangle(splitData_toPutInMosaic->m_rectangleImage_splitted);
				if (candidateMosaicPerimeter < 0.0)
				{
					candidateMosaicPerimeter = afterMosaicRectangle.Get_Perimeter();
					candidatePosX = currPosX;
					candidatePosY = currPosY;
				}
				else
				{
					currMosaicPerimeter = afterMosaicRectangle.Get_Perimeter();
					if (currMosaicPerimeter < candidateMosaicPerimeter)
					{
						candidateMosaicPerimeter = currMosaicPerimeter;
						candidatePosX = currPosX;
						candidatePosY = currPosY;
					}
				}
			}
		}

		*posX = candidatePosX;
		*posY = candidatePosY;
	}

	bool Image2DUtils::MULTISPLITTIMAGE_IntersectsRectangle(std::vector<Rectangle*> &vec_rectangles, Rectangle *rectangle)
	{
		bool intersects = false;

		size_t rectsCount = vec_rectangles.size();
		size_t i = 0;
		double error = 10E-5; // big tolerance.***
		while (!intersects && i < rectsCount)
		{
			Rectangle *currRect = vec_rectangles[i];
			if (currRect->Intersection_withRectangle(rectangle, error))
			{
				intersects = true;
			}
			i++;
		}

		return intersects;
	}

	void Image2DUtils::MULTISPLITTIMAGE_Add_ImageRectangle(Rectangle* imageRectangle, gaia3d::Triangle *triangle)
	{
		// used when there are a big image and we want to split in multiples images by objects.***
		// "imageRectangle" is a piece of a big image.***
		if (this->m_vec_image2DSplitDatas.size() == 0)
		{
			Image2DSplitData *image2dSplitData = new Image2DSplitData();
			image2dSplitData->m_rectangleImage_original = new Rectangle();
			image2dSplitData->m_rectangleImage_original->CopyFrom(imageRectangle);
			image2dSplitData->m_vec_triangles.push_back(triangle);
			this->m_vec_image2DSplitDatas.push_back(image2dSplitData);
		}
		else
		{
			size_t image2dSplitDatasCount = this->m_vec_image2DSplitDatas.size();
			size_t i = 0;
			double error = -10E-6; // tolerance negative => if there are very near -> then there are intersection.***
			bool finished = false;
			while (!finished && i < image2dSplitDatasCount)
			{
				Image2DSplitData *image2dSplitData = this->m_vec_image2DSplitDatas[i];
				if (image2dSplitData->m_rectangleImage_original->Intersection_withRectangle(imageRectangle, error))
				{
					image2dSplitData->m_rectangleImage_original->Add_Rectangle(imageRectangle);
					image2dSplitData->m_vec_triangles.push_back(triangle);
					finished = true;
				}
				i++;
			}

			if (!finished)
			{
				Image2DSplitData *image2dSplitData = new Image2DSplitData();
				image2dSplitData->m_rectangleImage_original = new Rectangle();
				image2dSplitData->m_rectangleImage_original->CopyFrom(imageRectangle);
				image2dSplitData->m_vec_triangles.push_back(triangle);
				this->m_vec_image2DSplitDatas.push_back(image2dSplitData);
			}
		}
	}

	bool Image2DUtils::MULTISPLITTIMAGE_Recombine_ImageRectangles()
	{
		// take rectangles from "this->m_map_rectangle_triangles" and try to merge.***
		bool recombined = false;

		size_t image2dSplitDatasCount = this->m_vec_image2DSplitDatas.size();
		for (size_t i = 0; i < image2dSplitDatasCount; i++)
		{
			Image2DSplitData *image2dSplitData = this->m_vec_image2DSplitDatas[i];
			if (this->MULTISPLITTIMAGE_TryToRecombine_ImageRectangle(image2dSplitData))
			{
				recombined = true; // if any rectangle was recombined, then return true.***
				image2dSplitData->m_vec_triangles.clear();
				delete image2dSplitData;
				this->m_vec_image2DSplitDatas.erase(this->m_vec_image2DSplitDatas.begin() + i);
				image2dSplitDatasCount = this->m_vec_image2DSplitDatas.size();
				i = 0;
			}
		}


		// once finished the loop, must repeat the process until there are no recombinations.***
		return recombined;
	}

	bool Image2DUtils::MULTISPLITTIMAGE_TryToRecombine_ImageRectangle(Image2DSplitData *image2dSplitData)
	{
		// PRIVATE.***
		bool recombined = false;

		size_t image2dSplitDatasCount = this->m_vec_image2DSplitDatas.size();
		size_t i = 0;
		double error = -10E-8;  // tolerance negative => if there are very near -> then there are intersection.***
		while (!recombined && i < image2dSplitDatasCount)
		{
			Image2DSplitData *currImage2dSplitData = this->m_vec_image2DSplitDatas[i];
			if (currImage2dSplitData != image2dSplitData)
			{
				if (currImage2dSplitData->m_rectangleImage_original->Intersection_withRectangle(image2dSplitData->m_rectangleImage_original, error))
				{
					currImage2dSplitData->m_rectangleImage_original->Add_Rectangle(image2dSplitData->m_rectangleImage_original);

					// now, translate all triangles of "image2dSplitData" to "currImage2dSplitData".***
					currImage2dSplitData->m_vec_triangles.insert(currImage2dSplitData->m_vec_triangles.end(), image2dSplitData->m_vec_triangles.begin(), image2dSplitData->m_vec_triangles.end());

					// finally erase triangles of "image2dSplitData".***
					image2dSplitData->m_vec_triangles.clear();

					recombined = true;
				}
			}
			i++;
		}

		return recombined;
	}
}
