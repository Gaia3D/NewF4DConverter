/**
 * ClassicFormatReader Header
 */
#ifndef _CLASSICFORMATREADER_H_
#define _CLASSICFORMATREADER_H_
#pragma once

#include "Reader.h"

class ClassicFormatReader : public Reader
{
public:
	ClassicFormatReader();
	virtual ~ClassicFormatReader();

public:
	virtual bool readRawDataFile(std::wstring& filePath);

	virtual void clear();

};

#endif // _CLASSICFORMATREADER_H_
