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

	bool bOcclusionCulling;
	double unitScaleFactor;
	unsigned char skinLevel;
	bool bYAxisUp;
	bool bAlignPostionToCenter;
	std::string referenceFileName;
	double referenceLon, referenceLat;
	double referencePosX, referencePosY;
	int meshType;

	std::string inputFolderPath, outputFolderPath;

	std::string idPrefix, idSuffix;

public:
	static ConverterManager* getConverterManager() { return &m_ConverterManager; }

public:
	bool isInitialized();
	bool initialize();
	void uninitialize();

	void changeGLDimension(int width, int height);

	//bool processSingleFile(std::string& filePath);

	void setProcessConfiguration(std::map<std::string, std::string>& arguments);

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
	bool writeIndexFile();

	bool processDataFolder();

	//void processDataFolder(std::string inputFolder);

	void collectTargetFiles(std::string inputFolder, std::map<std::string, std::string>& targetFiles);

	void processDataFiles(std::map<std::string, std::string>& targetFiles);

	bool processDataFile(std::string& filePath, Reader* reader);

	void writeRepresentativeLonLatOfEachData(std::map<std::string, double>& posXs, std::map<std::string, double>& posYs);
};

#endif // _CONVERTERMANAGER_H_