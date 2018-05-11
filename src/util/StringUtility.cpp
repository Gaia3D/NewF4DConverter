/**
* Implementation of the StringUtility class
*/
#include "StringUtility.h"

std::string gaia3d::StringUtility::convertWideStringToUtf8(const std::wstring& wstr)
{
#ifdef WIN32
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
#else
	std::string newString(wstr.begin(), wstr.end());
	return newString;
#endif
}

std::wstring gaia3d::StringUtility::convertUtf8ToWideString(const std::string& str)
{
#ifdef WIN32
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
#else
	std::wstring newString(str.begin(), str.end());
	return newString;
#endif
}

/*
std::string gaia3d::StringUtility::convertWideStringToUtf8(std::wstring& sourceString)
{
#ifdef WIN32
	int neededLength = WideCharToMultiByte(CP_UTF8, 0, sourceString.c_str(), (int)sourceString.size(), NULL, 0, NULL, NULL);
	char* receiver = new char[neededLength + 1];
	memset(receiver, 0x00, sizeof(char)*(neededLength + 1));
	WideCharToMultiByte(CP_UTF8, 0, sourceString.c_str(), (int)sourceString.size(), receiver, neededLength, NULL, NULL);
	std::string newString(receiver);
	delete[] receiver;
	return newString;
#else
	std::string newString(sourceString.begin(), sourceString.end());
	return newString;
#endif
}

std::wstring gaia3d::StringUtility::convertUtf8ToWideString(std::string& sourceString)
{
#ifdef WIN32
	int neededLength = 0;
	neededLength = MultiByteToWideChar(CP_UTF8, 0, sourceString.c_str(), -1, NULL, 0);
	wchar_t* receiver = new wchar_t[neededLength + 1];
	memset(receiver, 0x00, sizeof(wchar_t)*(neededLength + 1));
	MultiByteToWideChar(CP_UTF8, 0, sourceString.c_str(), -1, receiver, neededLength);
	std::wstring newString(receiver);
	delete[] receiver;
	return newString;
#else
	std::wstring newString(sourceString.begin(), sourceString.end());
	return newString;
#endif
}
*/
/*
static std::wstring s2ws(const std::string& str)
{
using convert_typeX = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_typeX, wchar_t> converterX;

return converterX.from_bytes(str);
}

static std::string ws2s(const std::wstring& wstr)
{
using convert_typeX = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_typeX, wchar_t> converterX;

return converterX.to_bytes(wstr);
}

static std::string w2s(const std::wstring &var)
{
static std::locale loc("");
auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).to_bytes(var);
}

static std::wstring s2w(const std::string &var)
{
static std::locale loc("");
auto &facet = std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(loc);
return std::wstring_convert<std::remove_reference<decltype(facet)>::type, wchar_t>(&facet).from_bytes(var);
}
*/
/*
#include <boost/locale.hpp>
namespace lcv = boost::locale::conv;

inline std::wstring fromUTF8(const std::string& s)
{
return lcv::utf_to_utf<wchar_t>(s);
}

inline std::string toUTF8(const std::wstring& ws)
{
return lcv::utf_to_utf<char>(ws);
}
*/