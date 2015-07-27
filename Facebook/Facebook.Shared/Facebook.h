#pragma once

class CWinRTFacebookClient
{
public:

	static CWinRTFacebookClient& instance();

	inline void SetDispatcher(Windows::UI::Core::CoreDispatcher^ dispatcher)
	{
		m_Dispatcher = dispatcher;
	}

	concurrency::task<bool> login(
		std::string scopes,
		bool allow_ui = true);

	void logout(
		bool clearAccessToken = true);
	
private:

	concurrency::task<bool>
		full_login(const std::string& scopes);

	concurrency::task<bool>	
		get_permissions();

private:

	bool m_bIsSingedIn = false;
	Windows::UI::Core::CoreDispatcher^ m_Dispatcher;
};