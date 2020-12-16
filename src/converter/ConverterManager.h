/**
 * ConverterManager Header
 */
#ifndef _CONVERTERMANAGER_H_
#define _CONVERTERMANAGER_H_
#pragma once

#include "../reader/Reader.h"
#include "../process/ConversionProcessor.h"

#include <string>

 //class Reader;
 //class ConversionProcessor;

class ConverterManager
{
private:
	ConverterManager();

public:
	virtual ~ConverterManager();

private:
	static ConverterManager m_ConverterManager;

	ConversionProcessor* processor;

	bool bCreateIndices, bConversion;

	std::string programPath;
	bool bOcclusionCulling;
	double unitScaleFactor;
	unsigned char skinLevel;
	bool bYAxisUp;
	int alignType;
	///< ReferenceLonLat is used or not? Default value is false.
	bool bUseReferenceLonLat;
	double referenceLon, referenceLat;
	int meshType;
	///< EPSG Code is used or not? 
	bool bUseEpsg;
	std::string epsgCode;
	double offsetX, offsetY, offsetZ;
	std::string projectName;
	std::map<std::string, bool> splitFilter;

	std::string inputFolderPath, outputFolderPath;

	std::string idPrefix, idSuffix;

public:
	static ConverterManager* getConverterManager() { return &m_ConverterManager; }

public:
	bool initialize(std::map<std::string, std::string>& arguments);

	//bool processSingleFile(std::string& filePath);

	void process();

	void uninitialize();

private:
	//Set the passed arguments at here for the program configuration
	///< Set the passed arguments at here for the program configuration
	bool setProcessConfiguration(std::map<std::string, std::string>& arguments);

	///< Traverse data folders to find target data.

	bool processDataFolder();

	///< Collect target files which will be converted..
	void collectTargetFiles(std::string& inputFolder, std::map<std::string, std::string>& targetFiles);

	///< It writes the index file.
	bool writeIndexFile();

	///< Convert and create F4D files at this function.
	void processDataFiles(std::map<std::string, std::string>& targetFiles);

	///< Write each reference lat and lon at here.
	void writeRepresentativeLonLatOfEachData(std::map<std::string, double>& posXs, std::map<std::string, double>& posYs);

	///< Write additional information which doesn't go through converting process.
	void writeAdditionalInfosOfEachData(std::map<std::string, std::string>& additionalInfos);

	///< Write relative path of each data for hierarchy structure of F4D files.
	void writeRelativePathOfEachData(std::map<std::string, std::string>& relativePaths);

	///< relationship when 1 raw data to multiple F4D or multiple raw data to single F4D cases happen
	void writeSplitInfo(std::map<std::string, std::vector<std::string>>& splitInfo );

	///< This function treats the single process of the converting
	void processSingleLoop(
		std::map<std::string, std::string>& targetFiles,
		std::map<std::string, double>& centerXs,
		std::map<std::string, double>& centerYs,
		std::map<std::string, std::string>& additionalInfos,
		std::map<std::string, std::string>& relativePaths,
		std::map<std::string, std::vector<std::string>>& splitInfo,
		unsigned char depth
	);
};

#endif // _CONVERTERMANAGER_H_