/*
*  HttpRequestInterface.cpp
*
*  Created by Joe Taylor on 29/01/2015.
*  Copyright 2010 Ninja Kiwi. All rights reserved.
*
*/

#include "pch.h"
#include "HttpRequestInterface.h"

//! ----------------------------

SHttpRequest::SHttpRequest()
	: ID(0)
	, State(eRS_Uninitialised)
	, ErrorCode(0)
	, HttpCode(0)
	, Method(HTTP_GET)
	, DataFormat(HTTP_NO_FORMAT)
	, SaveType(HTTP_MEMORY)
	, TimeStamp(0)
	, TCPKeepAliveInterval(-1L)
	, pFile(NULL)
	, postData(NULL)
	, postDataLen(0)
{
}

//! ----------------------------

SHttpRequest::SHttpRequest(const SHttpRequest& _rhs)
	: ID(_rhs.ID)
	, CallBackKey(_rhs.CallBackKey)
	, State(_rhs.State)
	, ErrorCode(_rhs.ErrorCode)
	, HttpCode(_rhs.HttpCode)
	, URL(_rhs.URL)
	, Method(_rhs.Method)
	, DataFormat(_rhs.DataFormat)
	, SaveType(_rhs.SaveType)
	, TimeStamp(_rhs.TimeStamp)
	, TCPKeepAliveInterval(_rhs.TCPKeepAliveInterval)
	, TimeoutOptions(_rhs.TimeoutOptions)
	, pFile(_rhs.pFile)
	, downloaded_data(_rhs.downloaded_data)
	, response_header(_rhs.response_header)
	, postData(NULL) // memcpy below
	, postDataLen(_rhs.postDataLen)
{
	if (_rhs.postData && postDataLen > 0)
	{
		postData = new char[postDataLen];
		memcpy(postData, _rhs.postData, postDataLen);
	}

	if (_rhs.platformData)
		platformData = _rhs.platformData->clone();
}

//! ----------------------------

SHttpRequest& SHttpRequest::operator = (const SHttpRequest& _rhs)
{
	if (this != &_rhs)
	{
		ID = _rhs.ID;
		CallBackKey = _rhs.CallBackKey;
		State = _rhs.State;
		ErrorCode = _rhs.ErrorCode;
		HttpCode = _rhs.HttpCode;
		URL = _rhs.URL;
		Method = _rhs.Method;
		DataFormat = _rhs.DataFormat;
		SaveType = _rhs.SaveType;
		TimeStamp = _rhs.TimeStamp;
		TCPKeepAliveInterval = _rhs.TCPKeepAliveInterval;
		TimeoutOptions = _rhs.TimeoutOptions;
		pFile = _rhs.pFile;
		downloaded_data = _rhs.downloaded_data;
		response_header = _rhs.response_header;
		
		if (postData)
		{
			delete[] postData;
			postData = NULL;
		}

		postDataLen = _rhs.postDataLen;

		if (_rhs.postData && postDataLen > 0)
		{
			postData = new char[postDataLen];
			memcpy(postData, _rhs.postData, postDataLen);
		}

		if (_rhs.platformData)
			platformData = _rhs.platformData->clone();
	}

	return *this;
}

//! ----------------------------

SHttpRequest::~SHttpRequest()
{
	if (postData)
	{
		delete[] postData;
		postData = NULL;
	}
}

//! ----------------------------

const std::string SHttpRequest::GetDownloadedDataStr() const
{
	return std::string(downloaded_data.begin(), downloaded_data.end());
}

//! ----------------------------

void SHttpRequest::SetPostData(const std::string& _string)
{
	if (postData)
	{
		delete[] postData;
		postData = NULL;
	}

	if (!_string.empty())
	{
		postDataLen = _string.length() + 1;
		postData = new char[postDataLen];
		strcpy_s((char*)postData, postDataLen, _string.c_str());
	}
}

//! ----------------------------