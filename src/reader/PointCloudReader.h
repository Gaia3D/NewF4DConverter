/**
* PointCloudReader Header
*/
#ifndef _POINTCLOUDREADER_H_
#define _POINTCLOUDREADER_H_
#pragma once

#include "Reader.h"

class PointCloudReader : public Reader
{
public:
	PointCloudReader();
	~PointCloudReader();

public:
	virtual bool readRawDataFile(std::string& filePath);

	virtual void clear();

private:
	bool readLasFile(std::string& filePath);
	bool readTemporaryPointCloudFile(std::string& filePath);
};

#endif // _POINTCLOUDREADER_H_
