/*
*  WinRTHttpRequestManager.h
*
*  Created by Joe Taylor on 29/01/2015.
*  Copyright 2010 Ninja Kiwi. All rights reserved.
*
*/

#ifndef _WINRT_HTTPREQUEST_MANAGER_H_
#define _WINRT_HTTPREQUEST_MANAGER_H_

#include "HttpRequestInterface.h"

//! ----------------------------

Derive_Platform_Data(WinRTRequestData) { };

//! ----------------------------

class CWinRTHttpRequestManager
	: public IHttpRequestManager
{
public:

	CWinRTHttpRequestManager();

	virtual const uint32 Send(const SHttpRequest &request, struct IHttpCallback * const callback) override;

private:

	SHttpRequest* GetRequest(uint32 _Id);
	bool RemoveRequest(uint32 _Id);

	Windows::Web::Http::Filters::HttpBaseProtocolFilter^ httpFilter;
	Windows::Web::Http::HttpClient^ httpClient;

	uint32 m_NextID;
	std::vector<SHttpRequest> m_ActiveRequests;
};

//! ----------------------------

#endif // _WINRT_HTTPREQUEST_MANAGER_H_
