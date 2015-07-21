// Facebook.h
#pragma once

#include <string>
#include <ppltasks.h>

#include "HttpRequestInterface.h"

class CWinRTFacebookClient
{
public:

	static CWinRTFacebookClient& instance();

	void login	(std::string scopes, bool allow_ui);
	void logout	(bool clearAccessToken = true);
	
private:

	CWinRTFacebookClient();
	
	concurrency::task<SHttpRequest> GraphRequestAsync(const NKUri& graph_api_uri);

	concurrency::task<bool>			full_login(std::string scopes);
	concurrency::task<bool>			refresh_permissions();	

private:

	bool			m_bIsSingedIn;

	std::shared_ptr<
		IHttpRequestManager>
					m_HttpRequestManager;
};