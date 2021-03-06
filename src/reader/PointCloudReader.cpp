﻿/**
* Implementation of the ReaderFactory class
*/
#include "PointCloudReader.h"

#include <fstream>  // std::ifstream

#include <proj_api.h>
#include <geo_normalize.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>

// libgeotiff-1.4.1 to libgeotiff-1.4.2
// change __geotiff_h_ to LIBGEOTIFF_GEOTIFF_H_
#define __geotiff_h_
#include "liblas/liblas.hpp"

#include "../util/GeometryUtility.h"
#include "../geometry/TrianglePolyhedron.h"
#include "../converter/LogWriter.h"

PointCloudReader::PointCloudReader()
{
}


PointCloudReader::~PointCloudReader()
{
}

bool PointCloudReader::readRawDataFile(std::string& filePath)
{
	std::string::size_type dotPosition = filePath.rfind(".");
	std::string::size_type fileExtLength = filePath.length() - dotPosition - 1;
	std::string fileExt = filePath.substr(dotPosition + 1, fileExtLength);
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

	if (fileExt.compare(std::string("las")) == 0)
		return readLasFile(filePath);
	else if (fileExt.compare(std::string("tpc")) == 0)
		return readTemporaryPointCloudFile(filePath);
	else
		return false;
}

#define ALLOWED_MAX_POINT_COUNT 50000000

bool splitOriginalDataIntoSubDivisionsAndDump(liblas::Reader& reader, unsigned int maxPointCountDumped, std::string& proj4String, std::map<std::string, std::string>& fileContainer, std::string& originalFilePath);

bool PointCloudReader::readLasFile(std::string& filePath)
{
	// open a .las file with input file stream
	std::ifstream ifs;
	ifs.open(filePath, std::ios::in | std::ios::binary);

	// create a reader for the .las file with the input file stream
	liblas::ReaderFactory f;
	liblas::Reader reader = f.CreateWithStream(ifs);

	// access to the header block
	liblas::Header const& header = reader.GetHeader();

	// find CRS info of this .las for georeferencing
	std::string originalSrsProjString;
	if (bCoordinateInfoInjected)
	{
		originalSrsProjString = makeProj4String();
	}
	else
	{
		liblas::SpatialReference spatialReference = header.GetSRS();
		originalSrsProjString = spatialReference.GetProj4();

		if (originalSrsProjString.empty())
		{
			// in this case, must get srs info through wkt or geotiff api

			std::string wkt = spatialReference.GetWKT();
			if (!wkt.empty())
			{
				// in this case, must change wkt info into proj4
				const char* poWKT = wkt.c_str();
				OGRSpatialReference srs(NULL);
				if (OGRERR_NONE != srs.importFromWkt(const_cast<char **> (&poWKT)))
				{
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : invalid WKT"));
					return false;
				}

				char* pszProj4 = NULL;
				srs.exportToProj4(&pszProj4);
				if (OGRERR_NONE != srs.exportToProj4(&pszProj4))
				{
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to convert WKT to proj string"));
					return false;
				}

				originalSrsProjString = std::string(pszProj4);
				CPLFree(pszProj4);

				if (originalSrsProjString.empty())
				{
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : empty proj string from WKT"));
					return false;
				}
			}
			else
			{
				// in this case, must change geotiff info into proj4
				const GTIF* originalGtif = spatialReference.GetGTIF();
				if (originalGtif == NULL)
				{
					printf("[ERROR] No WKT, No GeoTiff\n");
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : no WKT, no GeoTiff srs"));
					return false;
				}

				GTIF* temp = NULL;
				memcpy(&temp, &originalGtif, sizeof(GTIF*));
				GTIFDefn defn;
				if (GTIFGetDefn(temp, &defn) == 0)
				{
					printf("[ERROR] No WKT, No GeoTiff\n");
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to get GeoTiff srs definition"));
					return false;
				}

				char* pszWKT = GTIFGetOGISDefn(temp, &defn);
				if (pszWKT == NULL)
				{
					printf("[ERROR] No WKT, No GeoTiff\n");
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to convert GeoTiff srs definition to WKT"));
					return false;
				}

				OGRSpatialReference srs(NULL);
				char* pOriginalWkt = pszWKT;
				if (OGRERR_NONE != srs.importFromWkt(&pOriginalWkt))
				{
					CPLFree(pszWKT);
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : WKT from GeoTiff srs definition invalid"));
					return false;
				}

				char* pszProj4 = NULL;
				if (OGRERR_NONE != srs.exportToProj4(&pszProj4))
				{
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to convert WKT to proj string"));
					CPLFree(pszWKT);
					return false;
				}

				originalSrsProjString = std::string(pszProj4);

				CPLFree(pszProj4);
				CPLFree(pszWKT);

				if (originalSrsProjString.empty())
				{
					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : empty proj string from WKT"));
					return false;
				}
			}
		}
	}

	projPJ pjSrc = pj_init_plus(originalSrsProjString.c_str());
	if (!pjSrc)
	{
		ifs.close();
		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to initialize proj"));

		return false;
	}

	unsigned int pointCount = header.GetPointRecordsCount();
	if (pointCount > ALLOWED_MAX_POINT_COUNT)
	{
		unsigned int dumpedFileCount = static_cast<unsigned int>(ceil(static_cast<double>(pointCount) / ALLOWED_MAX_POINT_COUNT));
		unsigned int maxPointCountDumped = static_cast<unsigned int>(ceil(static_cast<double>(pointCount) / dumpedFileCount));
		if (!splitOriginalDataIntoSubDivisionsAndDump(reader, maxPointCountDumped, originalSrsProjString, temporaryFiles, filePath))
		{
			ifs.close();
			printf("[ERROR]Splitting original data failed : %s.\n", filePath.c_str());

			// new log
			LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
			LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to split original points cloud"));

			return false;
		}

		ifs.close();
		printf("[Info]Original file is splitted into %zd sub files and dumped.\n", temporaryFiles.size());

		return true;
	}
	
	std::string wgs84ProjString("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	projPJ pjDst = pj_init_plus(wgs84ProjString.c_str());
	if(!pjDst)
	{
		ifs.close();
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("PointCloudReader::readLasFile : failed to initialize proj"));

		return false;
	}

	double dfToRadians = 1.0;
	OGRSpatialReference srs(NULL);
	srs.importFromProj4(originalSrsProjString.c_str());

	if (srs.IsGeographic()) {
		dfToRadians = srs.GetAngularUnits((char**)NULL);
	}
	
	double cx = (header.GetMaxX() + header.GetMinX())/2.0;
	double cy = (header.GetMaxY() + header.GetMinY())/2.0;
	double cz = (header.GetMaxZ() + header.GetMinZ())/2.0;
	refLon = cx * dfToRadians;
	refLat = cy * dfToRadians;
	double alt = cz;
	int errorCode = pj_transform(pjSrc, pjDst, 1, 1, &refLon, &refLat, &alt);
	char* errorMessage = pj_strerrno(errorCode);
	if (errorMessage != NULL)
	{
		printf("[ERROR]%s\n", errorMessage);
		ifs.close();
		// new log
		LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
		LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string(" PointCloudReader::readLasFile : boungding box center coordinate transform failure"));

		return false;
	}

	refLon *= RAD_TO_DEG;
	refLat *= RAD_TO_DEG;

	bHasGeoReferencingInfo = true;

	double absPosOfCenterXY[3];
	alt = 0.0;
	gaia3d::GeometryUtility::wgs84ToAbsolutePosition(refLon, refLat, alt, absPosOfCenterXY);
	double m[16];
	gaia3d::GeometryUtility::transformMatrixAtAbsolutePosition(absPosOfCenterXY[0], absPosOfCenterXY[1], absPosOfCenterXY[2], m);
	gaia3d::Matrix4 globalTransformMatrix;
	globalTransformMatrix.set(m[0], m[4], m[8], m[12],
							m[1], m[5], m[9], m[13],
							m[2], m[6], m[10], m[14],
							m[3], m[7], m[11], m[15]);
	gaia3d::Matrix4 inverseGlobalTransMatrix = globalTransformMatrix.inverse();

	// retrieve point geometries
	gaia3d::TrianglePolyhedron* polyhedron = new gaia3d::TrianglePolyhedron;
	double px, py, pz;
	unsigned short maxColorChannelValue = 0;
	std::vector<unsigned short> reds, greens, blues;
	while (reader.ReadNextPoint())
	{
		gaia3d::Vertex* vertex = new gaia3d::Vertex;

		// position
		liblas::Point const& p = reader.GetPoint();
		px = p.GetX() * dfToRadians;
		py = p.GetY() * dfToRadians;
		pz = p.GetZ();

		pj_transform(pjSrc, pjDst, 1, 1, &px, &py, &pz);
		px *= RAD_TO_DEG;
		py *= RAD_TO_DEG;

		double absPosOfTargetPointArray[3];
		gaia3d::GeometryUtility::wgs84ToAbsolutePosition(px, py, pz, absPosOfTargetPointArray);
		gaia3d::Point3D absPosOfTargetPoint;
		absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
		vertex->position = inverseGlobalTransMatrix * absPosOfTargetPoint;

		// color
		liblas::Color const &color = p.GetColor();
		unsigned short ocr , ocg, ocb;
		if (color.GetRed() != 0 || color.GetGreen() != 0 || color.GetBlue() != 0)
		{
			ocr = color.GetRed(); ocg = color.GetGreen(); ocb = color.GetBlue();
			if (maxColorChannelValue < ocr )
				maxColorChannelValue = ocr ;
			if (maxColorChannelValue < ocg )
				maxColorChannelValue = ocg ;
			if (maxColorChannelValue < ocb )
				maxColorChannelValue = ocb ;

			reds.push_back(ocr);
			greens.push_back(ocg);
			blues.push_back(ocb);
		}
		else
		{
			ocr = p.GetIntensity();
			if (maxColorChannelValue < ocr )
				maxColorChannelValue = ocr ;

			reds.push_back(ocr );
			greens.push_back(ocr );
			blues.push_back(ocr );
		}

		polyhedron->getVertices().push_back(vertex);
	}

	if (maxColorChannelValue > 255)
	{
		size_t vertexCount = polyhedron->getVertices().size();
		unsigned char r, g, b;
		for (size_t i = 0; i < vertexCount; i++)
		{
			r = (unsigned char)(reds[i] >> 8);
			g = (unsigned char)(greens[i] >> 8);
			b = (unsigned char)(blues[i] >> 8);
			polyhedron->getVertices()[i]->color = MakeColorU4(r, g, b);
		}
	}
	else
	{
		size_t vertexCount = polyhedron->getVertices().size();
		unsigned char r, g, b;
		for (size_t i = 0; i < vertexCount; i++)
		{
			r = (unsigned char)(reds[i]);
			g = (unsigned char)(greens[i]);
			b = (unsigned char)(blues[i]);
			polyhedron->getVertices()[i]->color = MakeColorU4(r, g, b);
		}
	}

	printf("[Info]input point count : %zd\n", polyhedron->getVertices().size());

	polyhedron->setId(container.size());
	container.push_back(polyhedron);

	ifs.close();

	return true;
}

bool PointCloudReader::readTemporaryPointCloudFile(std::string& filePath)
{
	FILE* file = NULL;
	file = fopen(filePath.c_str(), "rb");

	unsigned int proj4StringLength;
	fread(&proj4StringLength, sizeof(unsigned int), 1, file);

	char szProj4String[1024];
	memset(szProj4String, 0x00, 1024);
	fread(szProj4String, sizeof(char), proj4StringLength, file);

	std::string originalSrsProjString = std::string(szProj4String);
	projPJ pjSrc = pj_init_plus(originalSrsProjString.c_str());
	if (!pjSrc)
	{
		fclose(file);
		return false;
	}

	std::string wgs84ProjString("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
	projPJ pjDst = pj_init_plus(wgs84ProjString.c_str());
	if (!pjDst)
	{
		fclose(file);
		return false;
	}

	double x, y, z;
	unsigned short ocr, ocg, ocb, maxColorComponent = 0;
	gaia3d::TrianglePolyhedron* polyhedron = new gaia3d::TrianglePolyhedron;
	gaia3d::Vertex* vertex = NULL;
	double minx, miny, minz, maxx, maxy, maxz;
	bool bboxInitialized = false;
	unsigned short* reds = new unsigned short[ALLOWED_MAX_POINT_COUNT];
	memset(reds, 0x00, sizeof(unsigned short) * ALLOWED_MAX_POINT_COUNT);
	unsigned short* greens = new unsigned short[ALLOWED_MAX_POINT_COUNT];
	memset(greens, 0x00, sizeof(unsigned short) * ALLOWED_MAX_POINT_COUNT);
	unsigned short* blues = new unsigned short[ALLOWED_MAX_POINT_COUNT];
	memset(blues, 0x00, sizeof(unsigned short) * ALLOWED_MAX_POINT_COUNT);
	unsigned int pointCount = 0;
	while (!feof(file))
	{
		fread(&x, sizeof(double), 1, file);
		fread(&y, sizeof(double), 1, file);
		fread(&z, sizeof(double), 1, file);

		fread(&ocr, sizeof(unsigned short), 1, file);
		fread(&ocg, sizeof(unsigned short), 1, file);
		fread(&ocb, sizeof(unsigned short), 1, file);
		
		if (maxColorComponent < ocr)
			maxColorComponent = ocr;
		if (maxColorComponent < ocg )
			maxColorComponent = ocg ;
		if (maxColorComponent < ocb )
			maxColorComponent = ocb ;

		if (!bboxInitialized)
		{
			minx = maxx = x;
			miny = maxy = y;
			minz = maxz = z;

			bboxInitialized = true;
		}
		else
		{
			if (minx > x)
				minx = x;
			if (maxx < x)
				maxx = x;
			if (miny > y)
				miny = y;
			if (maxy < y)
				maxy = y;
			if (minz > z)
				minz = z;
			if (maxz < z)
				maxz = z;
		}

		vertex = new gaia3d::Vertex;
		vertex->position.set(x, y, z);
		polyhedron->getVertices().push_back(vertex);
		reds[pointCount] = ocr;
		greens[pointCount] = ocg;
		blues[pointCount] = ocb;

		pointCount++;
	}

	fclose(file);

	double dfToRadians = 1.0;
	OGRSpatialReference srs(NULL);
	srs.importFromProj4(originalSrsProjString.c_str());

	if (srs.IsGeographic()) {
		dfToRadians = srs.GetAngularUnits((char**)NULL);
	}
	
	double cx = (maxx + minx) / 2.0;
	double cy = (maxy + miny) / 2.0;
	double cz = (maxz + minz) / 2.0;
	refLon = cx * dfToRadians;
	refLat = cy * dfToRadians;
	double alt = cz;
	int errorCode = pj_transform(pjSrc, pjDst, 1, 1, &refLon, &refLat, &alt);
	char* errorMessage = pj_strerrno(errorCode);
	if (errorMessage != NULL)
	{
		printf("[ERROR]%s\n", errorMessage);
		delete reds;
		delete greens;
		delete blues;

		return false;
	}

	refLon *= RAD_TO_DEG;
	refLat *= RAD_TO_DEG;

	bHasGeoReferencingInfo = true;

	double absPosOfCenterXY[3];
	alt = 0.0;
	gaia3d::GeometryUtility::wgs84ToAbsolutePosition(refLon, refLat, alt, absPosOfCenterXY);
	double m[16];
	gaia3d::GeometryUtility::transformMatrixAtAbsolutePosition(absPosOfCenterXY[0], absPosOfCenterXY[1], absPosOfCenterXY[2], m);
	gaia3d::Matrix4 globalTransformMatrix;
	globalTransformMatrix.set(m[0], m[4], m[8], m[12],
		m[1], m[5], m[9], m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]);
	gaia3d::Matrix4 inverseGlobalTransMatrix = globalTransformMatrix.inverse();

	bool bDownsizeColorByte = (maxColorComponent > 255) ? true : false;
	if (bDownsizeColorByte)
	{
		size_t vertexCount = polyhedron->getVertices().size();
		unsigned char r, g, b;
		double absPosOfTargetPointArray[3];
		for (size_t i = 0; i < vertexCount; i++)
		{
			vertex = polyhedron->getVertices()[i];
			vertex->position.x *= dfToRadians;
			vertex->position.y *= dfToRadians;

			pj_transform(pjSrc, pjDst, 1, 1, &(vertex->position.x), &(vertex->position.y), &(vertex->position.z));
			vertex->position.x *= RAD_TO_DEG;
			vertex->position.y *= RAD_TO_DEG;

			gaia3d::GeometryUtility::wgs84ToAbsolutePosition(vertex->position.x, vertex->position.y, vertex->position.z, absPosOfTargetPointArray);
			gaia3d::Point3D absPosOfTargetPoint;
			absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
			vertex->position = inverseGlobalTransMatrix * absPosOfTargetPoint;

			r = (unsigned char)(reds[i] >> 8);
			g = (unsigned char)(greens[i] >> 8);
			b = (unsigned char)(blues[i] >> 8);

			vertex->color = MakeColorU4(r, g, b);
		}
	}
	else
	{
		size_t vertexCount = polyhedron->getVertices().size();
		unsigned char r, g, b;
		double absPosOfTargetPointArray[3];
		for (size_t i = 0; i < vertexCount; i++)
		{
			vertex = polyhedron->getVertices()[i];
			vertex->position.x *= dfToRadians;
			vertex->position.y *= dfToRadians;

			pj_transform(pjSrc, pjDst, 1, 1, &(vertex->position.x), &(vertex->position.y), &(vertex->position.z));
			vertex->position.x *= RAD_TO_DEG;
			vertex->position.y *= RAD_TO_DEG;

			gaia3d::GeometryUtility::wgs84ToAbsolutePosition(vertex->position.x, vertex->position.y, vertex->position.z, absPosOfTargetPointArray);
			gaia3d::Point3D absPosOfTargetPoint;
			absPosOfTargetPoint.set(absPosOfTargetPointArray[0], absPosOfTargetPointArray[1], absPosOfTargetPointArray[2]);
			vertex->position = inverseGlobalTransMatrix * absPosOfTargetPoint;

			r = (unsigned char)reds[i];
			g = (unsigned char)greens[i];
			b = (unsigned char)blues[i];

			vertex->color = MakeColorU4(r, g, b);
		}
	}

	polyhedron->setId(container.size());
	container.push_back(polyhedron);

	delete reds;
	delete greens;
	delete blues;

	return true;
}

void PointCloudReader::clear()
{
	container.clear();

	textureContainer.clear();
}

bool splitOriginalDataIntoSubDivisionsAndDump(liblas::Reader& reader,
											unsigned int maxPointCountDumped,
											std::string& proj4String,
											std::map<std::string, std::string>& fileContainer,
											std::string& originalFilePath)
{
	// make some strings for names and full paths of sub files
	size_t dotPosition = originalFilePath.rfind(".");
	size_t slashPosition = originalFilePath.find_last_of("\\/");
	std::string subFileNamePrefix = originalFilePath.substr(slashPosition + 1, dotPosition - slashPosition - 1);
	std::string savePath = originalFilePath.substr(0, slashPosition + 1);
	
	unsigned int proj4StringLength = (unsigned int)proj4String.length();

	unsigned int subFileIndex = 0;
	unsigned int readPointCount = 0;
	std::string subFileName;
	std::string subFileFullPath;
	FILE* file = NULL;
	double x, y, z;
	unsigned short r, g, b;
	while (reader.ReadNextPoint())
	{
		if (file == NULL)
		{
			subFileName = subFileNamePrefix + std::string("_") + std::to_string(subFileIndex) + std::string(".tpc");
			subFileFullPath = savePath + subFileName;
			file = fopen(subFileFullPath.c_str(), "wb");
			if (file == NULL)
				return false;
			
			fwrite(&proj4StringLength, sizeof(unsigned int), 1, file);
			fwrite(proj4String.c_str(), sizeof(char), proj4StringLength, file);
		}

		liblas::Point const& p = reader.GetPoint();
		x = p.GetX(); y = p.GetY(); z = p.GetZ();

		liblas::Color const &color = p.GetColor();
		r = color.GetRed(); g = color.GetGreen(); b = color.GetBlue();
		if (r == 0 && g == 0 && b == 0)
			r = g = b = p.GetIntensity();

		fwrite(&x, sizeof(double), 1, file);
		fwrite(&y, sizeof(double), 1, file);
		fwrite(&z, sizeof(double), 1, file);

		fwrite(&r, sizeof(unsigned short), 1, file);
		fwrite(&g, sizeof(unsigned short), 1, file);
		fwrite(&b, sizeof(unsigned short), 1, file);

		readPointCount++;

		if (readPointCount == maxPointCountDumped)
		{
			readPointCount = 0;
			fclose(file);
			file = NULL;
			fileContainer[subFileName] = subFileFullPath;

			subFileIndex++;
		}
	}

	if (file != NULL)
	{
		fclose(file);
		file = NULL;
		fileContainer[subFileName] = subFileFullPath;
	}

	return true;
}
