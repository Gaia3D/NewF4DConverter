/**
 * Implementation of the ConverterManager class
 */
#include "ConverterManager.h"

#include <iostream>

#ifdef __APPLE__
        #include <sys/uio.h>
#else
        #include <sys/io.h>
#endif
#include <sys/stat.h>

#include <boost/filesystem.hpp>

#include "../reader/ReaderFactory.h"
#include "../process/ConversionProcessor.h"
#include "../process/SceneControlVariables.h"
#include "../writer/F4DWriter.h"

#include "LogWriter.h"


ConverterManager ConverterManager::m_ConverterManager;

ConverterManager::ConverterManager()
{
	processor = new ConversionProcessor();

	bCreateIndices = bCliMode = bConversion = false;

	bOcclusionCulling = false;

	unitScaleFactor = 1.0;
}

ConverterManager::~ConverterManager()
{
	delete processor;
}


// ConverterManager 멤버 함수

bool ConverterManager::initialize(GLFWwindow* window, int width, int height)
{
	if(window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(CANNOT_INITIALIZE_WND), true);
		glfwTerminate();
		return false;
	}

	return processor->initialize(window, width, height);
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
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), true);
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
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), true);
		return false;
	}

	processDataFolder(inputFolder);

	return true;
}

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

	processor->addAttribute(std::string("id"), fullId);

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

void ConverterManager::processDataFolder(std::string inputFolder)
{
	std::vector<std::string> dataFiles;
	std::vector<std::string> subFolders;
	
	namespace bfs = boost::filesystem;

	bfs::path folderPath(inputFolder);
	if (bfs::is_directory(folderPath))
	{
		std::cout << "In directory: " << folderPath.string() << std::endl;
		bfs::directory_iterator end;
		for(bfs::directory_iterator it(folderPath); it != end; ++it)
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
					dataFiles.push_back(it->path().filename().string());
					std::cout << "[file]" << it->path().filename() << std::endl;
				}
			}
			catch (const std::exception &ex)
			{
				std::cout << it->path().filename() << " " << ex.what() << std::endl;
			}
		}
	}
/*
	_wfinddata64_t fd;
	long long handle;
	int result = 1;
	std::string fileFilter = inputFolder + std::string("/*.*");
	handle = _wfindfirsti64(fileFilter.c_str(), &fd);

	if (handle == -1)
		return;

	std::vector<std::string> dataFiles;
	std::vector<std::string> subFolders;
	while (result != -1)
	{
		if ((fd.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			if (std::string(fd.name) != std::string(".") && std::string(fd.name) != std::string(".."))
			{
				std::string subFolderFullPath = inputFolder + "/" + std::string(fd.name);
				subFolders.push_back(subFolderFullPath);
			}
		}
		else
		{
			dataFiles.push_back(std::string(fd.name));
		}

		result = _wfindnexti64(handle, &fd);
	}

	_findclose(handle);
*/
	size_t subFolderCount = subFolders.size();
	for (size_t i = 0; i < subFolderCount; i++)
	{
		processDataFolder(subFolders[i]);
	}

	size_t dataFileCount = dataFiles.size();
	if (dataFileCount == 0)
		return;

	std::string outputFolder = outputFolderPath;

	std::string fullId;
	for (size_t i = 0; i < dataFileCount; i++)
	{
		// 1. raw data file을 하나씩 변환
		std::string dataFileFullPath = inputFolder + std::string("/") + dataFiles[i];
		
		Reader* reader = ReaderFactory::makeReader(dataFileFullPath);
		if (reader == NULL)
			continue;

		LogWriter::getLogWriter()->numberOfFilesToBeConverted += 1;
		reader->setUnitScaleFactor(unitScaleFactor);

		if (!processDataFile(dataFileFullPath, reader))
		{
			LogWriter::getLogWriter()->addContents(dataFiles[i], true);
			delete reader;
			processor->clear();
			continue;
		}

		delete reader;
		
		std::string::size_type dotPosition = dataFiles[i].rfind(".");
		fullId = dataFiles[i].substr(0, dotPosition);
		if (!idPrefix.empty())
			fullId = idPrefix + fullId;

		if (!idSuffix.empty())
			fullId += idSuffix;

		processor->addAttribute(std::string("id"), fullId);


		// 2. 변환 결과에서 bbox centerpoint를 로컬 원점으로 이동시키는 변환행렬 추출

		// 3. 변환 결과를 저장
		F4DWriter writer(processor);
		writer.setWriteFolder(outputFolder);
		if (!writer.write())
		{
			LogWriter::getLogWriter()->addContents(dataFiles[i], true);
			processor->clear();
			continue;
		}

		// 4. processor clear
		processor->clear();
		LogWriter::getLogWriter()->numberOfFilesConverted += 1;
	}
}

bool ConverterManager::writeIndexFile()
{
	F4DWriter writer(processor);
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

	if(!processor->proceedConversion(reader->getDataContainer(), reader->getTextureInfoContainer(), true, bOcclusionCulling))
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
