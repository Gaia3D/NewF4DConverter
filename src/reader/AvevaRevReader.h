/**
* AvevaRevReader Header
*/
#ifndef _AVEVAREVREADER_H_
#define _AVEVAREVREADER_H_
#pragma once

//#ifdef AVEVAREVIEWFORMAT

#include "Reader.h"

class AvevaRevReader : public Reader
{
public:
	AvevaRevReader();
	virtual ~AvevaRevReader();

public:
	virtual bool readRawDataFile(std::string& filePath);

	virtual void clear();
};

//#endif

#endif // _AVEVAREVREADER_H_
