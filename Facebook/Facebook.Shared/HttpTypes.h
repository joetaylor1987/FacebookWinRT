#pragma once

struct SHttpTimeoutOptions
{
	SHttpTimeoutOptions()
	: connectionTimeout(0L)
	, maxTimeout(0L)
	, lowSpeedLimit(0L)
	, lowSpeedTime(0L) {}
	
	long connectionTimeout;
	long maxTimeout;
	long lowSpeedLimit;
	long lowSpeedTime;
};

enum eRequestState
{
	eRS_Uninitialised,
	eRS_Initialised,
	eRS_Waiting,
	eRS_InProgress,
	eRS_Succeeded,
	eRS_Failed
};

enum HTTP_METHOD
{
	HTTP_GET,
	HTTP_POST,
	HTTP_HEAD,
	HTTP_NULL,
};

enum HTTP_DATA_FORMAT
{
	HTTP_NO_FORMAT,
	HTTP_ATOM_XML,
	HTTP_JSON,
};

enum HTTP_SAVE_TYPE
{
    HTTP_MEMORY,
    HTTP_FILE,
};

enum HTTP_ERROR
{
	eHTTP_OK,
	eHTTP_COULDNT_RESOLVE_PROXY,
	eHTTP_COULDNT_RESOLVE_HOST,
	eHTTP_COULDNT_CONNECT,

	eHTTP_UNSPECIFIED,
};