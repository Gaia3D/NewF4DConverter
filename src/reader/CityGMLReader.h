/**
* CityGMLReader Header
*/
#ifndef _CITYGMLREADER_H_
#define _CITYGMLREADER_H_
#pragma once

#include "Reader.h"

class CityGMLReader : public Reader
{
public:
	CityGMLReader();
	virtual ~CityGMLReader();

public:
	virtual bool readRawDataFile(std::string& filePath);

	virtual void clear();

private:
	bool bUseMaxLODOnly;
};

#endif // _CITYGMLREADER_H_
