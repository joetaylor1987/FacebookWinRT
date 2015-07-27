//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>
#include <locale>
#include <codecvt>

#include "App.xaml.h"

inline const std::string narrow(const std::wstring& _wstr)
{
	return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_wstr);
}

inline const std::wstring widen(const std::string& _str)
{
	return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(_str);
}

inline const std::string StringConvert(Platform::String^ _str)
{
	return narrow(_str->Data());
}

inline Platform::String^ StringConvert(const std::string& _str)
{
	return ref new Platform::String(widen(_str).c_str());
}