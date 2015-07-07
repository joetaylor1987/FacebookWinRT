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
	
	void full_login		();
	void refresh_permissions();

private:

	const bool			has_access_token() const;
	const std::string	get_access_token() const;
	void				set_access_token(const std::string& token);
	void				del_access_token() const;

	std::string		m_AccessToken;
	std::string		m_sScopes;
	bool			m_bIsSingedIn;

	std::shared_ptr<
		IHttpRequestManager>
					m_HttpRequestManager;
};