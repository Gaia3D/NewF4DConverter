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

	bool bCliMode, bCreateIndices, bConversion;

	std::string programPath;
	bool bOcclusionCulling;
	double unitScaleFactor;
	unsigned char skinLevel;
	bool bYAxisUp;
	bool bAlignPostionToCenter;
	bool bUseReferenceLonLat;
	double referenceLon, referenceLat;
	int meshType;
	bool bUseEpsg;
	std::string epsgCode;

	std::string inputFolderPath, outputFolderPath;

	std::string idPrefix, idSuffix;

public:
	static ConverterManager* getConverterManager() { return &m_ConverterManager; }

public:
	bool isInitialized();
	bool initialize(std::map<std::string, std::string>& arguments);
	void uninitialize();

	void changeGLDimension(int width, int height);

	//bool processSingleFile(std::string& filePath);

	void process();

	void setIsCliMode(bool bMode) {bCliMode = bMode;}
	bool getIsCliMode() {return bCliMode;}

	void setInputFolder(std::string input) {inputFolderPath = input;}
	void setOutputFolder(std::string output) {outputFolderPath = output;}
	void setIdPrefix(std::string prefix) { idPrefix = prefix; }
	void setIdSuffix(std::string suffix) { idSuffix = suffix; }

	void setIndexCreation(bool bIndexing) {bCreateIndices = bIndexing;}
	bool getIndexCreation() {return bCreateIndices;}

	void setConversionOn(bool bOn) {bConversion = bOn;}
	bool getConversionOn() {return bConversion;}

	void setOcclusionCulling(bool bOn) { bOcclusionCulling = bOn; }
	void setUnitScaleFactor(double factor) { unitScaleFactor = factor; }

private:

	bool setProcessConfiguration(std::map<std::string, std::string>& arguments);

	bool writeIndexFile();

	bool processDataFolder();

	//void processDataFolder(std::string inputFolder);

	void collectTargetFiles(std::string inputFolder, std::map<std::string, std::string>& targetFiles);

	void processDataFiles(std::map<std::string, std::string>& targetFiles);

	bool processDataFile(std::string& filePath, Reader* reader);

	std::string makeProj4String();

	void writeRepresentativeLonLatOfEachData(std::map<std::string, double>& posXs, std::map<std::string, double>& posYs, std::string proj4String);
};

#endif // _CONVERTERMANAGER_H_