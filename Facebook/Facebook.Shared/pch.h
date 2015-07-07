//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>
#include <boost/nowide/convert.hpp>

#include "App.xaml.h"

inline const std::string StringConvert(Platform::String^ _str)
{
	try
	{
		return boost::nowide::narrow(_str->Data());
	}
	catch (...)
	{
		return "";
	}
}

inline Platform::String^ StringConvert(const std::string& _str)
{
	try
	{
		std::wstring _wstr = boost::nowide::widen(_str);
		return ref new Platform::String(_wstr.c_str());
	}
	catch (...)
	{
		return "";
	}
}