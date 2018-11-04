/**
 * Implementation of the ConverterManager class
 */
#include "ConverterManager.h"

#include <iostream>

#ifdef __APPLE__
	#include <sys/uio.h>
#elif defined WIN32
	#include <direct.h>
	#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
	#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#else
	#include <sys/io.h>
#endif

#include <sys/stat.h>

#include <boost/filesystem.hpp>
#include <proj_api.h>
#include <json/json.h>

#include "argdefinition.h"
#include "../reader/ReaderFactory.h"
#include "../process/ConversionProcessor.h"
#include "../process/SceneControlVariables.h"
#include "../util/GeometryUtility.h"
#include "../writer/F4DWriter.h"

#include "LogWriter.h"

ConverterManager ConverterManager::m_ConverterManager;

ConverterManager::ConverterManager()
{
	processor = NULL;

	bCreateIndices = bCliMode = bConversion = false;

	bOcclusionCulling = false;

	unitScaleFactor = 1.0;

	skinLevel = 3;

	bYAxisUp = false;

	bAlignPostionToCenter = false;

	meshType = 0;

	bUseReferenceLonLat = false;

	bUseEpsg = false;
}

ConverterManager::~ConverterManager()
{
	if(processor != NULL)
		delete processor;
}


// ConverterManager 멤버 함수
bool ConverterManager::initialize(std::map<std::string, std::string>& arguments)
{
	std::string programPath = arguments[ProgramPath];
	std::string programFolder;

	namespace bfs = boost::filesystem;

	bfs::path folderPath(programPath);
	bfs::path projPath = folderPath.parent_path();
	projPath /= "proj";

	if (bfs::is_directory(projPath))
	{
		std::cout << "Proj4 : " << projPath.string() << std::endl;
		std::string projString = projPath.string();
		const char* epsgPath = projString.c_str();
		pj_set_searchpath(1, &epsgPath);
	}
	else
	{
		std::cout << "Can not find Proj4 serchpath location : " << projPath.string() << std::endl;
	}
	//size_t lastSlashPos = programPath.rfind('/');
	//programFolder = programPath.substr(0, lastSlashPos + 1);
	//std::string projLibPath = programFolder + std::string("proj");


	if (!setProcessConfiguration(arguments))
	{
		pj_set_searchpath(0, NULL);
		return false;
	}

	if(processor == NULL)
		processor = new ConversionProcessor();

	return processor->initialize();
}

void ConverterManager::uninitialize()
{
	pj_set_searchpath(0, NULL);

	if (processor != NULL)
		processor->uninitialize();
}

void ConverterManager::changeGLDimension(int width, int height)
{
	processor->defaultSpaceSetupForVisualization(width, height);
}

bool ConverterManager::processDataFolder()
{
	std::string inputFolder = inputFolderPath;
	std::string outputFolder = outputFolderPath;
	// test if output folder exist
	struct stat status;

	bool outputFolderExist = false;
	if (stat(outputFolder.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}

	if (!outputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), false);
		LogWriter::getLogWriter()->addContents(outputFolder, true);
		return false;
	}

	bool inputFolderExist = false;
	if (stat(inputFolder.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			inputFolderExist = true;
	}

	if (!inputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), false);
		LogWriter::getLogWriter()->addContents(inputFolder, true);
		return false;
	}

	std::map<std::string, std::string> targetFiles;
	collectTargetFiles(inputFolder, targetFiles);
	if (targetFiles.empty())
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), false);
		LogWriter::getLogWriter()->addContents(inputFolder, true);
		return false;
	}

	processDataFiles(targetFiles);

	return true;
}

/*
bool ConverterManager::processSingleFile(std::string& filePath)
{
	std::string outputFolder = outputFolderPath;

	struct stat status;

	bool outputFolderExist = false;
	if (stat(outputFolder.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			outputFolderExist = true;
	}

	if (!outputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	bool bRawDataFileExists = false;
	if (stat(filePath.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			bRawDataFileExists = true;
	}

	if (!bRawDataFileExists)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}
	
	Reader* reader = ReaderFactory::makeReader(filePath);
	if (reader == NULL)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(UNSUPPORTED_FORMAT), true);
		return false;
	}

	std::string fileName;
	std::string::size_type slashPosition = filePath.find_last_of("\\/");
	if (slashPosition == std::string::npos)
		fileName = filePath;
	else
		fileName = filePath.substr(slashPosition + 1, filePath.length() - slashPosition - 1);

	reader->setUnitScaleFactor(unitScaleFactor);

	if (!processDataFile(filePath, reader))
	{
		LogWriter::getLogWriter()->addContents(filePath, true);
		delete reader;
		processor->clear();
		return false;
	}

	delete reader;

	std::string::size_type dotPosition = fileName.rfind(".");
	std::string fullId = fileName.substr(0, dotPosition);
	if (!idPrefix.empty())
		fullId = idPrefix + fullId;

	if (!idSuffix.empty())
		fullId += idSuffix;

	processor->addAttribute(std::string(F4dId), fullId);

	F4DWriter writer(processor);
	writer.setWriteFolder(outputFolder);
	if (!writer.write())
	{
		LogWriter::getLogWriter()->addContents(filePath, true);
		processor->clear();
		return false;
	}

	processor->clear();

	return true;
}
*/

void ConverterManager::processDataFiles(std::map<std::string, std::string>& targetFiles)
{

	// TODO(khj 20180417) : NYI setup conversion configuration here
	// now, only set wheter do occlusion culling or not
	processor->setVisibilityIndexing(bOcclusionCulling);
	processor->setSkinLevel(skinLevel);
	processor->setYAxisUp(bYAxisUp);
	processor->setAlignPostionToCenter(bAlignPostionToCenter);
	processor->setMeshType(meshType);
	switch (meshType)
	{
	case 1:
		processor->setSkinLevel(50);
		processor->setLeafSpatialOctreeSize(40.0f);
		break;
	case 2:
		processor->setSkinLevel(51);
		processor->setLeafSpatialOctreeSize(40.0f);
		break;
	}
	// TODO(khj 20180417) end

	//// hard-cord for japan(AIST) realistic mesh and romania data
	//processor->setVisibilityIndexing(false);
	/*processor->setYAxisUp(false);
	processor->setAlignPostionToCenter(bAlignPostionToCenter);
	processor->setMeshType(meshType);
	switch(meshType)
	{
	case 1:
		processor->setSkinLevel(50);
		break;
	case 2:
		processor->setSkinLevel(51);
		break;
	}
	processor->setLeafSpatialOctreeSize(40.0f);*/

	//// hard-cord for new york citygml
	//processor->setVisibilityIndexing(false);
	//processor->setUseNsm(false);
	//processor->setYAxisUp(true);
	//processor->setAlignPostionToCenter(true);
	//processor->setLeafSpatialOctreeSize(422.0f);

	std::string outputFolder = outputFolderPath;

	std::string fullId;
	std::map<std::string, double> centerXs, centerYs;
	std::map<std::string, std::string>::iterator iter = targetFiles.begin();
	for (; iter != targetFiles.end(); iter++)
	{
		// 1. convert raw data files repectively
		std::string dataFile = iter->first;
		std::string dataFileFullPath = iter->second;

		Reader* reader = ReaderFactory::makeReader(dataFileFullPath);
		if (reader == NULL)
			continue;

		printf("===== Start processing this file : %s\n", dataFile.c_str());

		LogWriter::getLogWriter()->numberOfFilesToBeConverted += 1;
		reader->setUnitScaleFactor(unitScaleFactor);

		if (!processDataFile(dataFileFullPath, reader))
		{
			LogWriter::getLogWriter()->addContents(dataFileFullPath, true);
			delete reader;
			processor->clear();
			continue;
		}

		// 1.1 get embedded representative lon/lat of original dataset
		if (reader->doesHasGeoReferencingInfo())
		{
			double lon, lat;
			reader->getGeoReferencingInfo(lon, lat);
			centerXs[dataFile] = lon;
			centerYs[dataFile] = lat;
		}

		delete reader;

		std::string::size_type dotPosition = dataFile.rfind(".");
		fullId = dataFile.substr(0, dotPosition);
		if (!idPrefix.empty())
			fullId = idPrefix + fullId;

		if (!idSuffix.empty())
			fullId += idSuffix;

		processor->addAttribute(std::string(F4dId), fullId);

		// 2. save the result
		F4DWriter writer(processor);
		writer.setWriteFolder(outputFolder);
		if (!writer.write())
		{
			LogWriter::getLogWriter()->addContents(dataFileFullPath, true);
			processor->clear();
			continue;
		}

		// 2.1 extract representative lon/lat of F4D
		if (bUseEpsg || bUseReferenceLonLat)
		{
			double centerX, centerY, centerZ;
			processor->getOriginalBoundingBox().getCenterPoint(centerX, centerY, centerZ);
			centerXs[dataFile] = centerX;
			centerYs[dataFile] = centerY;
		}

		// 3. processor clear
		processor->clear();
		LogWriter::getLogWriter()->numberOfFilesConverted += 1;
	}

	// save representative lon / lat of F4D if a reference file exists
	if (!centerXs.empty())
	{
		std::string proj4String;
		if (bUseEpsg || bUseReferenceLonLat)
			proj4String = makeProj4String();
		writeRepresentativeLonLatOfEachData(centerXs, centerYs, proj4String);
	}
}

bool ConverterManager::writeIndexFile()
{
	F4DWriter writer(NULL);
	writer.setWriteFolder(outputFolderPath);
	writer.writeIndexFile();

	return true;
}

bool ConverterManager::processDataFile(std::string& filePath, Reader* reader)
{
	if (!reader->readRawDataFile(filePath))
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(CANNOT_LOAD_FILE), false);
		return false;
	}

	if (reader->getDataContainer().size() == 0)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_IN_RAW_DATA), false);
		return false;
	}

	//reader->TexCoord_Flip_Y();

	if(!processor->proceedConversion(reader->getDataContainer(), reader->getTextureInfoContainer()))
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(CONVERSION_FAILURE), false);
		return false;
	}
	
	return true;
}

bool ConverterManager::isInitialized()
{
	if(processor->getSceneControlVariables()->m_width == 0 || 
		processor->getSceneControlVariables()->m_height == 0 ||
		processor->getSceneControlVariables()->m_window == 0)
		return false;

	return true;
}

bool ConverterManager::setProcessConfiguration(std::map<std::string, std::string>& arguments)
{
	if (arguments.find(InputFolder) != arguments.end())
	{
		bConversion = true;
		inputFolderPath = arguments[InputFolder];

		if (arguments.find(MeshType) != arguments.end())
		{
			meshType = std::stoi(arguments[MeshType]);
		}

		if (arguments.find(PerformOC) != arguments.end())
		{
			if (arguments[PerformOC] == std::string("Y") ||
				arguments[PerformOC] == std::string("y"))
				bOcclusionCulling = true;
			else
				bOcclusionCulling = false;
		}
		else
		{
			bOcclusionCulling = false;
		}

		if (arguments.find(UnitScaleFactor) != arguments.end())
		{
			unitScaleFactor = std::stod(arguments[UnitScaleFactor]);
		}

		if (arguments.find(SkinLevelNsm) != arguments.end())
		{
			skinLevel = (unsigned char)(unsigned int)std::stoi(arguments[SkinLevelNsm]);
		}
	}
	else
	{
		bConversion = false;
	}

	if (arguments.find(OutputFolder) != arguments.end())
	{
		outputFolderPath = arguments[OutputFolder];
	}

	if (arguments.find(CreateIndex) != arguments.end())
	{
		if (arguments[CreateIndex] == std::string("Y") ||
			arguments[CreateIndex] == std::string("y"))
		{
			bCreateIndices = true;
		}
		else
		{
			bCreateIndices = false;
		}
	}
	else
	{
		bCreateIndices = false;
	}

	if (arguments.find(IdPrefix) != arguments.end())
	{
		idPrefix = arguments[IdPrefix];
	}

	if (arguments.find(IdSuffix) != arguments.end())
	{
		idSuffix = arguments[IdSuffix];
	}

	if (arguments.find(IsYAxisUp) != arguments.end())
	{
		if (arguments[IsYAxisUp] == std::string("Y") ||
			arguments[IsYAxisUp] == std::string("y"))
			bYAxisUp = true;
		else
			bYAxisUp = false;
	}

	if (arguments.find(ReferenceLonLat) != arguments.end())
	{
		size_t lonLatLength = arguments[ReferenceLonLat].length();
		char* original = new char[lonLatLength + 1];
		memset(original, 0x00, sizeof(char)*(lonLatLength + 1));
		memcpy(original, arguments[ReferenceLonLat].c_str(), lonLatLength);
		char* lon = std::strtok(original, ",");
		char* lat = std::strtok(NULL, ",");
		referenceLon = std::stod(lon);
		referenceLat = std::stod(lat);
		delete[] original;
		bUseReferenceLonLat = true;
		bAlignPostionToCenter = true;
	}

	if (arguments.find(AlignToCenter) != arguments.end())
	{
		if (arguments[AlignToCenter] == std::string("Y") ||
			arguments[AlignToCenter] == std::string("y"))
			bAlignPostionToCenter = true;
		else
			bAlignPostionToCenter = false;
	}

	if (arguments.find(Epsg) != arguments.end())
	{
		epsgCode = arguments[Epsg];

		std::string proj4String = std::string("+init=epsg:") + epsgCode;

		projPJ pjEpsg;
		pjEpsg = pj_init_plus(proj4String.c_str());
		if (pjEpsg == NULL)
		{
			char* errorMsg = pj_strerrno(pj_errno);
			LogWriter::getLogWriter()->addContents(std::string(UNSUPPERTED_EPSG_CODE), false);
			LogWriter::getLogWriter()->addContents(epsgCode, true);

			return false;
		}

		bUseEpsg = true;
		bAlignPostionToCenter = true;
	}

	return true;
}

void ConverterManager::process()
{
	if (bConversion)
	{
		if (!isInitialized())	return;

		processDataFolder();
	}

	if (bCreateIndices)
	{
		writeIndexFile();
	}
}

void ConverterManager::collectTargetFiles(std::string inputFolder, std::map<std::string, std::string>& targetFiles)
{
	std::vector<std::string> dataFiles;
	std::vector<std::string> subFolders;

	namespace bfs = boost::filesystem;

	bfs::path folderPath(inputFolder);
	if (bfs::is_directory(folderPath))
	{
		std::cout << "In directory: " << folderPath.string() << std::endl;
		bfs::directory_iterator end;
		for (bfs::directory_iterator it(folderPath); it != end; ++it)
		{
			try
			{
				if (bfs::is_directory(*it))
				{
					subFolders.push_back(it->path().string());
					std::cout << "[directory]" << it->path() << std::endl;
				}
				else
				{
					std::string dataFile = it->path().filename().string();
					std::string dataFileFullPath = inputFolder + std::string("/") + dataFile;
					targetFiles[dataFile] = dataFileFullPath;

					std::cout << "[file]" << dataFile << std::endl;
				}
			}
			catch (const std::exception &ex)
			{
				std::cout << it->path().filename() << " " << ex.what() << std::endl;
			}
		}
	}

	size_t subFolderCount = subFolders.size();
	for (size_t i = 0; i < subFolderCount; i++)
	{
		collectTargetFiles(subFolders[i], targetFiles);
	}
}

std::string ConverterManager::makeProj4String()
{
	std::string proj4String;

	if (bUseEpsg)
	{
		proj4String = std::string("+init=epsg:") + epsgCode;

		return proj4String;
	}

	if (bUseReferenceLonLat)
	{
		proj4String = std::string("+proj=tmerc +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs");
		proj4String += std::string(" +lon_0=") + std::to_string(referenceLon);
		proj4String += std::string(" +lat_0=") + std::to_string(referenceLat);

		return proj4String;
	}

	return proj4String;
}

void ConverterManager::writeRepresentativeLonLatOfEachData(std::map<std::string, double>& posXs, std::map<std::string, double>& posYs, std::string proj4String)
{
	Json::Value arrayNode(Json::arrayValue);

	if (proj4String.empty())
	{
		std::map<std::string, double>::iterator iter = posXs.begin();
		for (; iter != posXs.end(); iter++)
		{
			Json::Value f4d(Json::objectValue);

			// data_key
			std::string fileName = iter->first;
			std::string::size_type dotPosition = fileName.rfind(".");
			std::string dataKey = fileName.substr(0, dotPosition);
			f4d["data_key"] = dataKey;

			// longitude and latitude
			f4d["longitude"] = iter->second;
			f4d["latitude"] = posYs[iter->first];

			arrayNode.append(f4d);
		}
	}
	else
	{
		std::string localCRS = proj4String;
		const char* wgs84 = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";

		projPJ pjLocal, pjWgs84;
		pjLocal = pj_init_plus(localCRS.c_str());
		if (pjLocal == NULL)
		{
			return;
		}

		pjWgs84 = pj_init_plus(wgs84);
		if (pjWgs84 == NULL)
		{
			return;
		}

		std::map<std::string, double>::iterator iter = posXs.begin();
		int transformResult;

		for (; iter != posXs.end(); iter++)
		{
			Json::Value f4d(Json::objectValue);

			// data_key
			std::string fileName = iter->first;
			std::string::size_type dotPosition = fileName.rfind(".");
			std::string dataKey = fileName.substr(0, dotPosition);
			f4d["data_key"] = dataKey;

			// longitude and latitude
			double posX = iter->second;
			double posY = posYs[iter->first];

			transformResult = pj_transform(pjLocal, pjWgs84, 1, 1, &posX, &posY, NULL);
			char* errorMsg = pj_strerrno(transformResult);
			if (errorMsg != NULL)
			{
				continue;
			}

			posX *= RAD_TO_DEG;
			posY *= RAD_TO_DEG;

			f4d["longitude"] = posX;
			f4d["latitude"] = posY;

			arrayNode.append(f4d);
		}
	}

	Json::StyledWriter writer;
	std::string documentContent = writer.write(arrayNode);
	std::string lonLatFileFullPath = outputFolderPath + std::string("/lonsLats.json");
	FILE* file = NULL;
	file = fopen(lonLatFileFullPath.c_str(), "wt");
	fprintf(file, "%s", documentContent.c_str());
	fclose(file);
}