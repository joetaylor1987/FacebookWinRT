/*
*  HttpRequestInterface.h
*
*  Created by Joe Taylor on 29/01/2015.
*  Copyright 2010 Ninja Kiwi. All rights reserved.
*
*/

#ifndef _HTTPREQUEST_INTERFACE_H_
#define _HTTPREQUEST_INTERFACE_H_

#include "HttpTypes.h"

#include <time.h>
#include <vector>
#include <map>
#include <string>

//! ----------------------------

struct IHttpRequestData
{
	virtual ~IHttpRequestData() {}
	virtual std::shared_ptr<IHttpRequestData> clone() const = 0;
};

template <typename Derived>
struct IHttpRequestData_CRTP : public IHttpRequestData
{
	virtual std::shared_ptr<IHttpRequestData> clone() const
	{
		return std::make_shared<Derived>(static_cast<Derived const&>(*this));
	}
};

#define Derive_Platform_Data(Type) class Type: public IHttpRequestData_CRTP<Type>

struct SHttpRequest
{
	friend class CCurlHttpRequestManager;

public:

	// Request Parameters
	std::string		CallBackKey;
	std::string		URL;
	HTTP_METHOD		Method;
	HTTP_DATA_FORMAT DataFormat;
	HTTP_SAVE_TYPE	SaveType;
	long			TCPKeepAliveInterval;
	SHttpTimeoutOptions TimeoutOptions;
	class IFile		*pFile;

	// State
	uint32			ID;
	clock_t			TimeStamp;
	eRequestState	State;
	int32			ErrorCode;
	int32			HttpCode;

	// Reponse
	std::vector<char>					downloaded_data;
	std::map<std::string, std::string>	response_header;

public:

	SHttpRequest();
	~SHttpRequest();

	SHttpRequest(const SHttpRequest& _rhs);
	SHttpRequest& operator=	(const SHttpRequest& _rhs);

	const std::string	GetDownloadedDataStr() const;

	void				SetPostData(const std::string& _string);
	const std::string	GetPostData() const { return postData ? std::string(postData) : ""; }
	const char*			GetPostDataRaw() const { return postData; }

	inline std::shared_ptr<IHttpRequestData> GetPlatformData() const { return platformData; }

private:

	// Request
	char*	postData;
	size_t	postDataLen;

	std::shared_ptr<IHttpRequestData> platformData;
};

//! ----------------------------

struct IHttpCallback
{
	virtual ~IHttpCallback() {}

	virtual void HttpComplete(const SHttpRequest &request)	{}
	virtual void HttpFailed(const SHttpRequest &request)	{}
};

//! ----------------------------

struct IHttpRequestManager
{
	virtual ~IHttpRequestManager() {}

	virtual void			Process() {};
	virtual const uint32	Send(const SHttpRequest& request,
	struct IHttpCallback* const callback) = 0;
};

IHttpRequestManager *CreateHttpRequestManager();

//! ----------------------------

#endif // _HTTPREQUEST_INTERFACE_H_
