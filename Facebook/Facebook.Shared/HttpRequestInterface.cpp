#include "pch.h"
#include "HttpRequestInterface.h"

#include <sstream>

//! ----------------------------

SHttpRequest::SHttpRequest()
	: ID(0)
	, State(eRS_Uninitialised)
	, ErrorEnum(eHTTP_OK)
	, HttpCode(0)
	, Method(HTTP_GET)
	, DataFormat(HTTP_NO_FORMAT)
	, SaveType(HTTP_MEMORY)
	, TimeStamp(0)
	, TCPKeepAliveInterval(-1L)
	, FailOnError(false)
	, pFile(NULL)
{	
}

//! ----------------------------

const std::string SHttpRequest::GetDownloadedDataStr() const
{
	return std::string(downloaded_data.begin(), downloaded_data.end());
}

//! ----------------------------