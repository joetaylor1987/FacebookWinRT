#pragma once

#include "HttpRequestInterface.h"
#include <mutex>

//! ----------------------------

Derive_Platform_Data(WinRTRequestData)
{
public:
	HRESULT _error_code;
	Platform::String^ _error_message;
};

//! ----------------------------

class CWinRTHttpRequestManager
: public IHttpRequestManager
{
public:

	CWinRTHttpRequestManager();

	virtual const uint32 Send(const SHttpRequest &request, struct IHttpCallback * const callback) override;
	virtual void SetUserAgent(const std::string& user_agent_str) override;

private:

	SHttpRequest* GetRequest(uint32 _Id);
	bool RemoveRequest(uint32 _Id);

	Windows::Web::Http::Filters::HttpBaseProtocolFilter^ httpFilter;
	Windows::Web::Http::HttpClient^ httpClient;

	std::mutex m_Mutex;

	uint32 m_NextID;
	Platform::String^ m_sUserAgent;
	std::vector<SHttpRequest> m_ActiveRequests;
};

//! ----------------------------