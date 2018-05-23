/**
 * StringUtility Header
 */
#ifndef _STRINGUTILITY_H_
#define _STRINGUTILITY_H_
#pragma once

#ifdef WIN32
#include <codecvt>
#endif

#include <string>

namespace gaia3d
{
	class StringUtility
	{
	public:
		static std::string convertWideStringToUtf8(const std::wstring& sourceString);
		static std::wstring convertUtf8ToWideString(const std::string& sourceString);
	};
}

#endif // _STRINGUTILITY_H_