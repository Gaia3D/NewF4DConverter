/**
* IfcReader Header
*/
#ifndef _IFCREADER_H_
#define _IFCREADER_H_
#pragma once

#include "Reader.h"

class IfcReader : public Reader
{
public:
	IfcReader();
	virtual ~IfcReader();

public:
	virtual bool readRawDataFile(std::string& filePath);

	virtual void clear();
};

#endif // _IFCREADER_H_

