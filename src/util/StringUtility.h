/**
 * StringUtility Header
 */
#ifndef _STRINGUTILITY_H_
#define _STRINGUTILITY_H_
#pragma once

//#include <codecvt>

namespace gaia3d
{
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
}

#endif // _STRINGUTILITY_H_