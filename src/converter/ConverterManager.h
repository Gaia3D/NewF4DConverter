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

	std::wstring inputFolderPath, outputFolderPath;

	std::wstring idPrefix, idSuffix;

public:
	static ConverterManager* getConverterManager() { return &m_ConverterManager; }



public:
	bool initialize(GLFWwindow* window, int width, int height);

	void changeGLDimension(int width, int height);

	bool processDataFolder();

	bool writeIndexFile();

	bool processSingleFile(std::wstring& filePath);

	void setIsCliMode(bool bMode) {bCliMode = bMode;}
	bool getIsCliMode() {return bCliMode;}
	bool isInitialized();
	void setInputFolder(std::wstring input) {inputFolderPath = input;}
	void setOutputFolder(std::wstring output) {outputFolderPath = output;}
	void setIdPrefix(std::wstring prefix) { idPrefix = prefix; }
	void setIdSuffix(std::wstring suffix) { idSuffix = suffix; }
	void setIndexCreation(bool bIndexing) {bCreateIndices = bIndexing;}
	bool getIndexCreation() {return bCreateIndices;}
	void setConversionOn(bool bOn) {bConversion = bOn;}
	bool getConversionOn() {return bConversion;}

	void setOcclusionCulling(bool bOn) { bOcclusionCulling = bOn; }
	void setUnitScaleFactor(double factor) { unitScaleFactor = factor; }

private:
	void processDataFolder(std::wstring inputFolder);

	bool processDataFile(std::wstring& filePath, Reader* reader);
};

#endif // _CONVERTERMANAGER_H_