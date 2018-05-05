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

ReaderFactory::ReaderFactory()
{
}

ReaderFactory::~ReaderFactory()
{
}

Reader* ReaderFactory::makeReader(std::wstring& filePath)
{
	std::wstring::size_type dotPosition = filePath.rfind(L".");
	if(dotPosition == std::wstring::npos)
	{
		LogWriter::getLogWriter()->addContents(std::wstring(ERROR_FLAG), false);
		LogWriter::getLogWriter()->addContents(std::wstring(INVALID_TRIANGLE_COUNT), false);
		return NULL;
	}

	std::wstring::size_type fileExtLength = filePath.length() - dotPosition - 1;

	std::wstring fileExt = filePath.substr(dotPosition + 1, fileExtLength);

	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), towlower);

#ifdef F4D_FORMAT_SUPPORT_SHIJT
	if(fileExt.compare(std::wstring(L"jt")) == 0)
	{
		return new JtReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_IFC
	if (fileExt.compare(std::wstring(L"ifc")) == 0)
	{
		return new IfcReader;
	}
#endif

#ifdef F4D_FORMAT_SUPPORT_CLASSIC
	if (fileExt.compare(std::wstring(L"obj")) == 0 ||
		fileExt.compare(std::wstring(L"dae")) == 0||
		fileExt.compare(std::wstring(L"3ds")) == 0)
	{
		return new ClassicFormatReader;
	}
#endif

	return NULL;
}