#pragma once

class CWinRTFacebookClient
{
public:

	static CWinRTFacebookClient& instance();

	void login	(std::string scopes, bool allow_ui);
	void logout	(bool clearAccessToken = true);
	
private:

	CWinRTFacebookClient();

	concurrency::task<bool>	
		full_login(std::string scopes);

	concurrency::task<bool>	
		refresh_permissions();	

private:

	bool	m_bIsSingedIn;
};