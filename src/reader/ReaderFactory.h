/**
 * ReaderFactory Header
 */
#ifndef _READERFACTORY_H_
#define _READERFACTORY_H_
#pragma once

#include "Reader.h"

class ReaderFactory
{
public:
	ReaderFactory();

	virtual ~ReaderFactory();

public:
	static Reader* makeReader(std::string& filePath);
};

#endif // _READERFACTORY_H_