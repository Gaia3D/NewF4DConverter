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
	#define mkdir(dirname,mode)   _mkdir(dirname)
#else
	#include <sys/io.h>
#endif

#include <sys/stat.h>

#include <boost/filesystem.hpp>
#include <proj_api.h>
#include <json/json.h>
#include <cpl_serv.h>

#include "argdefinition.h"
#include "../reader/ReaderFactory.h"
#include "../process/ConversionProcessor.h"
#include "../process/SceneControlVariables.h"
#include "../util/GeometryUtility.h"
#include "../writer/F4DWriter.h"

#include "LogWriter.h"

 ///< This class is singleton
ConverterManager ConverterManager::m_ConverterManager;

ConverterManager::ConverterManager()
{
	processor = NULL;

	bCreateIndices = bConversion = false;

	bOcclusionCulling = false;

	unitScaleFactor = 1.0;

	skinLevel = 4;

	bYAxisUp = false;

	alignType = -1;

	meshType = 0;

	bUseReferenceLonLat = false;

	bUseEpsg = false;

	offsetX = offsetY = offsetZ = 0.0;
}

ConverterManager::~ConverterManager()
{
	if (processor != NULL)
		delete processor;
}

std::string csvFullPath;

///< Set the resource file named 'csv' for GDAL library.
static const char* CSVFileFullPathOverride(const char* baseFile)
{
	static char szPath[1024];

	sprintf(szPath, "%s/%s", csvFullPath.c_str(), baseFile);

	return szPath;
}

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

#ifdef F4D_FORMAT_SUPPORT_POINTCLOUD
	csvFullPath = programFolder + std::string("csv");

	///< For setting csv folder path, put function pointer as argument.
	SetCSVFilenameHook(CSVFileFullPathOverride);
#endif

	if (processor == NULL)
		processor = new ConversionProcessor();

	return processor->initialize();
}

void ConverterManager::uninitialize()
{
	///< Set proj folder path as default value. 
	pj_set_searchpath(0, NULL);

	if (processor != NULL)
		processor->uninitialize();
}

/*
void ConverterManager::changeGLDimension(int width, int height)
{
	processor->defaultSpaceSetupForVisualization(width, height);
}
*/
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
		LogWriter::getLogWriter()->addContents(std::string(INVALID_OUTPUT_PATH), false);
		LogWriter::getLogWriter()->addContents(outputFolder, true);
		// replaced
		LogWriter::getLogWriter()->setStatus(false, std::string("ConverterManager::processDataFolder : ") + std::string(INVALID_OUTPUT_PATH));
		return false;
	}

	///< test if input folder exist
	bool inputFolderExist = false;
	if (stat(inputFolder.c_str(), &status) == 0)
	{
		if (S_ISDIR(status.st_mode))
			inputFolderExist = true;
	}

	if (!inputFolderExist)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(INVALID_INPUT_PATH), false);
		LogWriter::getLogWriter()->addContents(inputFolder, true);
		// replaced
		LogWriter::getLogWriter()->setStatus(false, std::string("ConverterManager::processDataFolder : ") + std::string(INVALID_INPUT_PATH));
		return false;
	}

	std::map<std::string, std::string> targetFiles;
	collectTargetFiles(inputFolder, targetFiles);
	if (targetFiles.empty())
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_FILES), false);
		LogWriter::getLogWriter()->addContents(inputFolder, true);
		// replaced
		LogWriter::getLogWriter()->setStatus(false, std::string("ConverterManager::processDataFolder : ") + std::string(NO_FILES));
		return false;
	}

	if (!projectName.empty())
	{
		outputFolder = outputFolder + std::string("/") + projectName;

		///< test whether the folder which name is the value of projectName exists under output folder path.
		bool bProjectFolderExist = false;
		if (stat(outputFolder.c_str(), &status) == 0)
		{
			if (S_ISDIR(status.st_mode))
				bProjectFolderExist = true;
		}

		if (!bProjectFolderExist)
		{
			if (mkdir(outputFolder.c_str(), 0755) != 0)
			{
				LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
				LogWriter::getLogWriter()->addContents(outputFolder, true);
				// replaced
				LogWriter::getLogWriter()->setStatus(false, std::string("ConverterManager::processDataFolder : ") + std::string(CANNOT_CREATE_DIRECTORY));
				return false;
			}
		}

		outputFolderPath = outputFolder;
	}

	///< Convert target files at this funtion
	processDataFiles(targetFiles);

	return true;
}

void ConverterManager::processDataFiles(std::map<std::string, std::string>& targetFiles)
{

	processor->setVisibilityIndexing(bOcclusionCulling);
	processor->setSkinLevel(skinLevel);
	//processor->setYAxisUp(bYAxisUp);
	//processor->setAlignPostionToCenter(bAlignPostionToCenter);
	processor->setMeshType(meshType);
	switch (meshType)
	{
	case 1: // single random-shaped 3d mesh
		processor->setSkinLevel(50);
		processor->setLeafSpatialOctreeSize(40.0f);
		break;
	case 2: // splitted random-shaped 3d mesh
		processor->setSkinLevel(51);
		processor->setLeafSpatialOctreeSize(40.0f);
		break;
	case 3: // point cloud
		processor->setLeafSpatialOctreeSize(40.0f);
		break;
	}

	///< The variables needed during converting process.
	std::map<std::string, double> centerXs, centerYs;	//< Get each center positions of each files.
	std::map<std::string, std::string> relativePaths;	//< Get each relative F4D folder paths from root folder.
	std::map<std::string, std::string> additionalInfos;	//< Get the additional information of each files which doesn't have to go through converting process.

	processSingleLoop(targetFiles, centerXs, centerYs, additionalInfos, relativePaths, 0);

	// save representative lon / lat of F4D if a reference file exists
	if (!centerXs.empty() && !centerYs.empty())
	{
		writeRepresentativeLonLatOfEachData(centerXs, centerYs);
	}

	///< save the additional information of each files if it is exists.
	if (!additionalInfos.empty()) {
		writeAdditionalInfosOfEachData(additionalInfos);
	}

	// save relative path of each F4D
	if (!relativePaths.empty())
		writeRelativePathOfEachData(relativePaths);
}

bool ConverterManager::writeIndexFile()
{
	F4DWriter writer(NULL);
	writer.setWriteFolder(outputFolderPath);
	
	return writer.writeIndexFile();
	//return true;
}

#ifdef TEMPORARY_TEST
void writeCheckResult(std::string fileFullPath);
#endif

///< the argument named 'depth' is used for checking the depth of recursive function calling while processing.
void ConverterManager::processSingleLoop(std::map<std::string, std::string>& targetFiles,
	std::map<std::string, double>& centerXs,
	std::map<std::string, double>& centerYs,
	std::map<std::string, std::string>& additionalInfos,
	std::map<std::string, std::string>& relativePaths,
	unsigned char depth)
{
	std::string outputFolder = outputFolderPath;

	enum ResultType { Success = 0, PartiallySuccess = 1, Failure = 2 };
	std::map<std::string, char> conversionResult;	///< Save conversion result of each file with file name.
	std::map<std::string, std::string> conversionDescription;	///< Save the description on conversion result with file name.
	std::map<std::string, std::string> failedSubGroupList;	///< Save the sub group list and the failure log during conversion.

	std::string fullId;
	std::map<std::string, std::string>::iterator iter = targetFiles.begin();
	for (; iter != targetFiles.end(); iter++)
	{
		std::string dataFile = iter->first;
		std::string dataFileFullPath = iter->second;

		///< check the format of the target file at here.
		Reader* reader = ReaderFactory::makeReader(dataFileFullPath);
		if (reader == NULL)
			continue;

		printf("\n===== Start processing this file : %s\n", dataFile.c_str());

		if (depth == 0)
		{
			LogWriter::getLogWriter()->numberOfFilesToBeConverted += 1;
			// new log
			LogWriter::getLogWriter()->createNewConversionJobLog(dataFile, dataFileFullPath);
		}
		reader->setUnitScaleFactor(unitScaleFactor);
		reader->setOffset(offsetX, offsetY, offsetZ);
		reader->setYAxisUp(bYAxisUp);

		if (!splitFilter.empty())
			reader->getSplitFilter().insert(splitFilter.begin(), splitFilter.end());

		// 0. inject coordinate information into reader before reading
		///< 0. inject coordinate information into reader before reading
		if (bUseEpsg)
		{
			reader->injectSrsInfo(epsgCode);
		}

		if (bUseReferenceLonLat)
		{
			reader->injectOringinInfo(referenceLon, referenceLat);
		}

		switch (alignType)
		{
		case 0:
		{
			reader->alignToCenter(true);
		}
		break;
		case 1:
		{
			reader->alignToBottomCenter(true);
		}
		break;
		}

		// 1. read the original file and build data structure
		if (!reader->readRawDataFile(dataFileFullPath))
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(CANNOT_LOAD_FILE), false);
			LogWriter::getLogWriter()->addContents(dataFileFullPath, true);
			printf("[ERROR]%s\n", std::string(CANNOT_LOAD_FILE).c_str());
			printf("===== End processing this file : %s\n", dataFile.c_str());
			if (depth == 0)
			{
				conversionResult[dataFile] = (char)ResultType::Failure;
				conversionDescription[dataFile] = std::string("unable to read data file");
			}

			delete reader;
			// new log
			if (depth == 0)
				LogWriter::getLogWriter()->closeCurrentConversionJobLog();
			continue;
		}

		// 1-1. in cases when original files are converted into multiple temporary files.
		if (!reader->getTemporaryFiles().empty())
		{
			printf("===== %zd temporary files are made. Proceeding conversion of temporary files\n", reader->getTemporaryFiles().size());

			// run recursively
			processSingleLoop(reader->getTemporaryFiles(), centerXs, centerYs, additionalInfos, relativePaths, depth + 1);

			// delete temporary files
			std::map<std::string, std::string>::iterator tmpFileIter = reader->getTemporaryFiles().begin();
			for (; tmpFileIter != reader->getTemporaryFiles().end(); tmpFileIter++)
			{
				if (remove(tmpFileIter->second.c_str()) != 0)
				{
					LogWriter::getLogWriter()->addContents(std::string(CANNOT_DELETE_FILE), false);
					LogWriter::getLogWriter()->addContents(tmpFileIter->second, true);
					// replaced
					if (depth == 0)
					{
						LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::warning);
						LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("ConverterManager::processSingleLoop") + std::string(CANNOT_DELETE_FILE) + tmpFileIter->second);
					}
				}
			}

			delete reader;

			if (depth == 0)
			{
				// new log
				LogWriter::getLogWriter()->closeCurrentConversionJobLog();
			}

			continue;
		}

		// 1-2. basic data reading validation
		bool bGeometryExists = true;
		///< Check whether 'container' is empty or not
		if (reader->getDataContainer().empty())
		{
			///< Check whether 'containers' is empty or not
			if (reader->getMultipleDataContainers().empty())
				bGeometryExists = false;
			///< 'containers' is not empty.
			else
			{
				std::map<std::string, std::vector<gaia3d::TrianglePolyhedron*>>::iterator subItemIter = reader->getMultipleDataContainers().begin();
				bGeometryExists = false;
				for (; subItemIter != reader->getMultipleDataContainers().end(); subItemIter++)
				{
					if (!iter->second.empty())
					{
						bGeometryExists = true;
						break;
					}
				}
			}
		}

		if (!bGeometryExists)
		{
			LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
			LogWriter::getLogWriter()->addContents(std::string(NO_DATA_IN_RAW_DATA), false);
			LogWriter::getLogWriter()->addContents(dataFileFullPath, true);
			printf("[ERROR]%s\n", std::string(NO_DATA_IN_RAW_DATA).c_str());
			conversionResult[dataFile] = (char)ResultType::Failure;
			conversionDescription[dataFile] = std::string("no data in data file");
			delete reader;
			// replaced
			if (depth == 0)
			{
				LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
				LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("ConverterManager::processSingleLoop : ") + std::string(NO_DATA_IN_RAW_DATA));
				LogWriter::getLogWriter()->closeCurrentConversionJobLog();
			}

			continue;
		}

		// 2. convert to F4D
		std::map<std::string, std::vector<gaia3d::TrianglePolyhedron*>> targetOriginalGeometries;
		///< Check whether single raw file is needed to be converted into multiple files.
		if (reader->shouldRawDataBeConvertedToMuitiFiles())
			targetOriginalGeometries = reader->getMultipleDataContainers();
		///< 1 F4D file from 1 raw file.
		else
		{
			std::string::size_type dotPosition = dataFile.rfind(".");
			std::string id = dataFile.substr(0, dotPosition);

			targetOriginalGeometries[id] = reader->getDataContainer();
		}

		///< Get the ancestor of each sub group folder
		std::map<std::string, std::vector<std::string>>& ancestorsOfEachItem = reader->getAncestorsOfEachSubGroup();

		std::map<std::string, std::vector<gaia3d::TrianglePolyhedron*>>::iterator subItemIter = targetOriginalGeometries.begin();
		for (; subItemIter != targetOriginalGeometries.end(); subItemIter++)
		{
			if (subItemIter->second.empty())
				continue;

			///< If the geometry data is needed to be used several times(Duplicated using) then the geometry shouldn't be destroyed at the reader. 
			if (reader->shouldGeometryBeDesroyedOutside())
			{
				processor->setResponsibilityForDisposing(false);

				///< in this case, marked model/reference info in polyhedron should be cleared before proceeding
				gaia3d::Matrix4 identityMat;
				for (size_t i = 0; i < subItemIter->second.size(); i++)
				{
					(subItemIter->second)[i]->setReferenceMatrix(identityMat);
					(subItemIter->second)[i]->setReferenceModel(NULL);
					(subItemIter->second)[i]->setReferenceModelIndex(MaxUnsignedLong);
				}
			}

			fullId = subItemIter->first;
			if (!idPrefix.empty())
				fullId = idPrefix + fullId;
			if (!idSuffix.empty())
				fullId = fullId + idSuffix;

			if (reader->shouldRawDataBeConvertedToMuitiFiles())
				printf("\n===== Start processing sub-group : %s\n", fullId.c_str());

			// 2.1 create directories suiting for hiararchy
			std::string finalOutputFolder = outputFolder;
			std::string relativeOutputFolder;
			if (ancestorsOfEachItem.find(subItemIter->first) != ancestorsOfEachItem.end())
			{
				bool bCanMakeSubDirectory = true;
				///< The most fartest ancestor is the oldest ancestor of that file. 
				for (size_t i = ancestorsOfEachItem[subItemIter->first].size(); i > 0; i--)
				{
					relativeOutputFolder = relativeOutputFolder + std::string("/F4D_") + idPrefix + ancestorsOfEachItem[subItemIter->first][i - 1] + idSuffix;
					finalOutputFolder = outputFolder + relativeOutputFolder;

					struct stat status;

					bool bFinalOutputFolder = false;
					if (stat(finalOutputFolder.c_str(), &status) == 0)
					{
						if (S_ISDIR(status.st_mode))
							bFinalOutputFolder = true;
					}

					if (!bFinalOutputFolder)
					{
						if (mkdir(finalOutputFolder.c_str(), 0755) != 0)
						{
							LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
							LogWriter::getLogWriter()->addContents(std::string(CANNOT_CREATE_DIRECTORY), false);
							LogWriter::getLogWriter()->addContents(finalOutputFolder, true);
							bCanMakeSubDirectory = false;
							break;
						}
					}
				}

				if (!bCanMakeSubDirectory)
				{
					if (reader->shouldRawDataBeConvertedToMuitiFiles())
					{
						if (failedSubGroupList.find(dataFile) != failedSubGroupList.end())
							failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + std::string("|");

						failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + subItemIter->first + std::string("(directory creation failure)");

						printf("\n===== End processing sub-group : %s\n", fullId.c_str());
					}

					continue;
				}
			}

			// 2.2 conversion
			if (!processor->proceedConversion(subItemIter->second, reader->getTextureInfoContainer()))
			{
				LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
				LogWriter::getLogWriter()->addContents(std::string(CONVERSION_FAILURE), false);
				printf("[ERROR]%s\n", std::string(CONVERSION_FAILURE).c_str());
				if (reader->shouldRawDataBeConvertedToMuitiFiles())
				{
					LogWriter::getLogWriter()->addContents(dataFileFullPath + std::string("--") + fullId, true);

					if (failedSubGroupList.find(dataFile) != failedSubGroupList.end())
						failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + std::string("|");
					///< Write the file name and the log of the failure
					failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + subItemIter->first + std::string("(conversion failure)");
				}
				else
				{
					LogWriter::getLogWriter()->addContents(dataFileFullPath, true);

					conversionResult[dataFile] = (char)ResultType::Failure;
					conversionDescription[dataFile] = std::string("conversion failure");

					// new log
					if (depth == 0)
					{
						LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
						LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("ConverterManager::ProcessSingleLoop : conversion process failure"));
					}
				}

				processor->clear();

				if (reader->shouldRawDataBeConvertedToMuitiFiles())
					printf("\n===== End processing sub-group : %s\n", fullId.c_str());

				continue;
			}

			processor->addAttribute(std::string(F4DID), fullId);

			// 2.4 write
			F4DWriter writer(processor);
			writer.setWriteFolder(finalOutputFolder);
			if (!writer.write())
			{
				if (reader->shouldRawDataBeConvertedToMuitiFiles())
				{
					LogWriter::getLogWriter()->addContents(dataFileFullPath + std::string("--") + fullId, true);

					if (failedSubGroupList.find(dataFile) != failedSubGroupList.end())
						failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + std::string("|");

					failedSubGroupList[dataFile] = failedSubGroupList[dataFile] + subItemIter->first + std::string("(writing failure)");
				}
				else
				{
					LogWriter::getLogWriter()->addContents(dataFileFullPath, true);

					conversionResult[dataFile] = (char)ResultType::Failure;
					conversionDescription[dataFile] = std::string("writing failure");

					// new log
					if (depth == 0)
					{
						LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::failure);
						LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("ConverterManager::ProcessSingleLoop : outptu writing failure"));
					}
				}
			}
			else
			{
				// 2.3 get representative lon/lat of original dataset
				if (reader->doesHasGeoReferencingInfo())
				{
					double lon, lat;
					reader->getGeoReferencingInfo(lon, lat);
					centerXs[fullId] = lon;
					centerYs[fullId] = lat;

					if (depth == 0)
					{
						LogWriter::getLogWriter()->setGeoReferencingInfo(true, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
					}
				}
				else
				{
					if (depth == 0)
					{
						gaia3d::BoundingBox bbox = processor->getBoundingBox();
						LogWriter::getLogWriter()->setGeoReferencingInfo(false, bbox.minX, bbox.minY, bbox.minZ, bbox.maxX, bbox.maxY, bbox.maxZ);
					}
				}

				relativePaths[fullId] = relativeOutputFolder;

				// 2.3.1 if is there any additional infomations, check that and take it.

				if (reader->doesHasAdditionalInfo()) {
					std::map<std::string, std::string>::iterator infoIter = reader->getAdditionalInfo().begin();
					for (; infoIter != reader->getAdditionalInfo().end(); infoIter++)
					{
						if (infoIter->first != std::string("attributes"))
						{
							additionalInfos[infoIter->first] = infoIter->second;
							continue;
						}
						else
						{
							additionalInfos[fullId + std::string("_") + infoIter->first] = infoIter->second;
							continue;
						}
					}
				}

			}

			processor->clear();

			if (reader->shouldRawDataBeConvertedToMuitiFiles())
				printf("===== End processing sub-group : %s\n", fullId.c_str());

		}

		delete reader;

		if (depth == 0)
		{
			///< Check whether conversion is fully successed or partially successed.
			if (conversionResult.find(dataFile) == conversionResult.end())
			{
				if (failedSubGroupList.find(dataFile) == failedSubGroupList.end())
					conversionResult[dataFile] = (char)ResultType::Success;
				else
				{
					conversionResult[dataFile] = (char)ResultType::PartiallySuccess;
					conversionDescription[dataFile] = failedSubGroupList[dataFile];

					// new log
					LogWriter::getLogWriter()->changeCurrentConversionJobStatus(LogWriter::warning);
					LogWriter::getLogWriter()->addDescriptionToCurrentConversionJobLog(std::string("ConverterManager::ProcessSingleLoop : sub-division conversion failure - ") + failedSubGroupList[dataFile]);
				}
			}

			LogWriter::getLogWriter()->numberOfFilesConverted += 1;
			// new log
			LogWriter::getLogWriter()->closeCurrentConversionJobLog();
		}

		printf("===== End processing this file : %s\n", dataFile.c_str());
	}

#ifdef TEMPORARY_TEST
	std::string duplicationCheckResult = outputFolder + std::string("/") + projectName + std::string("_duplicationCheckResult.txt");
	writeCheckResult(duplicationCheckResult);
#endif

	/*std::string logFileFullPath = outputFolder + std::string("/resultLog.csv");
	FILE* logFile = NULL;
	logFile = fopen(logFileFullPath.c_str(), "wt");

	std::map<std::string, char>::iterator resultIter = conversionResult.begin();
	for (; resultIter != conversionResult.end(); resultIter++)
	{
		fprintf(logFile, "%s,%d", resultIter->first.c_str(), resultIter->second);
		if (resultIter->second != (char)ResultType::Success)
			fprintf(logFile, ",%s", conversionDescription[resultIter->first].c_str());

		fprintf(logFile, "\n");
	}
	fclose(logFile);*/
}

bool ConverterManager::setProcessConfiguration(std::map<std::string, std::string>& arguments)
{
	if (arguments.find(InputFolder) != arguments.end())
	{
		bConversion = true;
		inputFolderPath = arguments[InputFolder];

		if (inputFolderPath.back() == '\\' || inputFolderPath.back() == '/')
			inputFolderPath = inputFolderPath.substr(0, inputFolderPath.length() - 1);

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
			memset(original, 0x00, sizeof(char) * (lonLatLength + 1));
			memcpy(original, arguments[ReferenceLonLat].c_str(), lonLatLength);
			char* lon = std::strtok(original, ",");
			char* lat = std::strtok(NULL, ",");
			referenceLon = std::stod(lon);
			referenceLat = std::stod(lat);
			delete[] original;
			bUseReferenceLonLat = true;
		}

		if (arguments.find(AlignTo) != arguments.end())
		{
			alignType = std::stoi(arguments[AlignTo]);
		}

		if (arguments.find(Epsg) != arguments.end())
		{
			epsgCode = arguments[Epsg];

			std::string proj4String = std::string("+init=epsg:") + epsgCode;

			projPJ pjEpsg;
			pjEpsg = pj_init_plus(proj4String.c_str());
			if (pjEpsg == NULL)
			{
				// char* errorMsg = pj_strerrno(pj_errno);
				LogWriter::getLogWriter()->addContents(std::string(UNSUPPERTED_EPSG_CODE), false);
				LogWriter::getLogWriter()->addContents(epsgCode, true);
				//replaced
				LogWriter::getLogWriter()->setStatus(false, std::string("ConverterManager::setProcessConfiguration : ") + std::string(UNSUPPERTED_EPSG_CODE) + epsgCode);

				return false;
			}

			bUseEpsg = true;
		}

		if (arguments.find(OffsetX) != arguments.end())
			offsetX = std::stod(arguments[OffsetX]);

		if (arguments.find(OffsetY) != arguments.end())
			offsetY = std::stod(arguments[OffsetY]);

		if (arguments.find(OffsetZ) != arguments.end())
			offsetY = std::stod(arguments[OffsetZ]);

		if (arguments.find(ProjectName) != arguments.end())
			projectName = arguments[ProjectName];

		if (arguments.find(SplitFilter) != arguments.end())
		{
			char filters[4096];
			memset(filters, 0x00, sizeof(char) * 4096);
			strcpy(filters, arguments[SplitFilter].c_str());

			char* token = strtok(filters, ",");
			while (token != NULL)
			{
				splitFilter[std::string(token)] = false;
				token = strtok(NULL, ",");
			}
		}
	}
	else
		bConversion = false;

	if (arguments.find(OutputFolder) != arguments.end())
	{
		outputFolderPath = arguments[OutputFolder];

		if (outputFolderPath.back() == '\\' || outputFolderPath.back() == '/')
			outputFolderPath = outputFolderPath.substr(0, outputFolderPath.length() - 1);
	}

	if (arguments.find(CreateIndex) != arguments.end())
	{
		if (arguments[CreateIndex] == std::string("Y") ||
			arguments[CreateIndex] == std::string("y"))
			bCreateIndices = true;
		else
			bCreateIndices = false;
	}
	else
		bCreateIndices = false;

	return true;
}

void ConverterManager::process()
{
	if (bConversion)
	{
		if (processor->getSceneControlVariables()->m_width == 0 ||
			processor->getSceneControlVariables()->m_height == 0 ||
			processor->getSceneControlVariables()->m_window == 0)
			return;

		processDataFolder();
	}

	if (bCreateIndices)
	{
		writeIndexFile();
	}
}

void ConverterManager::collectTargetFiles(std::string& inputFolder, std::map<std::string, std::string>& targetFiles)
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
			catch (const std::exception& ex)
			{
				std::cout << it->path().filename() << " " << ex.what() << std::endl;
			}
		}
	}

	size_t subFolderCount = subFolders.size();
	for (size_t i = 0; i < subFolderCount; i++)	//< Recursively traverse sub folders for searching target files.
	{
		collectTargetFiles(subFolders[i], targetFiles);
	}
}

void ConverterManager::writeAdditionalInfosOfEachData(std::map<std::string, std::string>& additionalInfos) {

	std::map<std::string, std::string>::iterator iter = additionalInfos.begin();
	FILE* file = NULL;

	for (; iter != additionalInfos.end(); iter++) {
		std::string dataKey = iter->first;

		if (dataKey.find(std::string("attributes")) != std::string::npos)
		{
			std::string realDataKey = iter->first.substr(0, iter->first.rfind(std::string("_")));
			std::string attributesFileFullPath = outputFolderPath + std::string("/F4D_") + realDataKey + std::string("/attributes.json");

			std::string documentContent = iter->second;

			file = fopen(attributesFileFullPath.c_str(), "wt");
			fprintf(file, "%s", documentContent.c_str());
			fclose(file);
		}
		else
		{
			std::string documentContent = iter->second;
			std::string relativePathName = "/" + dataKey + ".json";
			std::string networkInfoFileFullPath = outputFolderPath + std::string(relativePathName);

			file = fopen(networkInfoFileFullPath.c_str(), "wt");
			fprintf(file, "%s", documentContent.c_str());
			fclose(file);
		}
	}
}

void ConverterManager::writeRepresentativeLonLatOfEachData(std::map<std::string, double>& posXs, std::map<std::string, double>& posYs)
{
	std::map<std::string, double>::iterator iter = posXs.begin();
	for (; iter != posXs.end(); iter++)
	{
		Json::Value f4d(Json::objectValue);

		// data_key
		std::string dataKey = iter->first;
		f4d["data_key"] = dataKey;

		// longitude and latitude
		f4d["longitude"] = iter->second;
		f4d["latitude"] = posYs[iter->first];

		Json::StyledWriter writer;
		std::string documentContent = writer.write(f4d);
		std::string lonLatFileFullPath = outputFolderPath + std::string("/F4D_") + dataKey + std::string("/lonsLats.json");
		FILE* file = NULL;
		file = fopen(lonLatFileFullPath.c_str(), "wt");
		fprintf(file, "%s", documentContent.c_str());
		fclose(file);
	}
}

void ConverterManager::writeRelativePathOfEachData(std::map<std::string, std::string>& relativePaths)
{
	Json::Value arrayNode(Json::arrayValue);

	std::map<std::string, std::string>::iterator iter = relativePaths.begin();
	for (; iter != relativePaths.end(); iter++)
	{
		Json::Value f4d(Json::objectValue);

		// data_key
		std::string dataKey = iter->first;
		f4d["data_key"] = dataKey;

		// relative path
		f4d["relativePath"] = iter->second;

		arrayNode.append(f4d);
	}

	Json::StyledWriter writer;
	std::string documentContent = writer.write(arrayNode);
	std::string lonLatFileFullPath = outputFolderPath + std::string("/relativePaths.json");
	FILE* file = NULL;
	file = fopen(lonLatFileFullPath.c_str(), "wt");
	fprintf(file, "%s", documentContent.c_str());
	fclose(file);
}
