/**
 * Implementation of the ReaderFactory class
 */
#include "ReaderFactory.h"

#include <cctype>
#include <string>
#include <algorithm>
#include "../converter/LogWriter.h"

#ifdef F4D_FORMAT_SUPPORT_SHIJT
#include "JtReader.h"
#endif
#ifdef F4D_FORMAT_SUPPORT_IFC
#include "IfcReader.h"
#endif
#ifdef F4D_FORMAT_SUPPORT_CLASSIC
#include "ClassicFormatReader.h"
#endif
#ifdef F4D_FORMAT_SUPPORT_CITYGML
#include "CitygmlReader.h"
#endif

#ifdef F4D_FORMAT_SUPPORT_INDOORGML
#include "IndoorGMLReader.h"
#endif

ReaderFactory::ReaderFactory()
{
}

ReaderFactory::~ReaderFactory()
{
}

Reader* ReaderFactory::makeReader(std::string& filePath)
{
	std::string::size_type dotPosition = filePath.rfind(".");
	if(dotPosition == std::string::npos)
	{
		LogWriter::getLogWriter()->addContents(std::string(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::string(NO_DATA_OR_INVALID_PATH), false);
		return NULL;
	}

	std::string::size_type fileExtLength = filePath.length() - dotPosition - 1;

	std::string fileExt = filePath.substr(dotPosition + 1, fileExtLength);

	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

#ifdef F4D_FORMAT_SUPPORT_SHIJT
	if(fileExt.compare(std::string("jt")) == 0)
	{
		return new JtReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_IFC
	if (fileExt.compare(std::string("ifc")) == 0)
	{
		return new IfcReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_CLASSIC
	if (fileExt.compare(std::string("obj")) == 0 ||
		fileExt.compare(std::string("dae")) == 0||
		fileExt.compare(std::string("3ds")) == 0)
	{
		return new ClassicFormatReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_CITYGML
	if (fileExt.compare(std::string("citygml")) == 0)
	{
		return new CitygmlReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_INDOORGML
	if (fileExt.compare(std::string("indoorgml")) == 0)
		//checkIndoorGML(filePath))
	{
		return new IndoorGMLReader;

	}
#endif

	return NULL;
}
