/**
* Implementation of the StringUtility class
*/
#include "StringUtility.h"

///< 각 나라 언어에 맞는 wide string을 utf8로 변환
std::string gaia3d::StringUtility::convertWideStringToUtf8(const std::wstring& wstr)
{
#ifdef WIN32
	static std::locale loc("");
	auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);

	return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).to_bytes(wstr);
#else
	std::string newString(wstr.begin(), wstr.end());
	return newString;
#endif
}

std::wstring gaia3d::StringUtility::convertUtf8ToWideString(const std::string& str)
{
#ifdef WIN32
	static std::locale loc("");
	auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
	return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).from_bytes(str);
#else
	std::wstring newString(str.begin(), str.end());
	return newString;
#endif
}
