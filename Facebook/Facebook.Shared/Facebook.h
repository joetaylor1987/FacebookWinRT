#pragma once

//! ----------------------------

enum class eLoginConfig
{
	ALLOW_UI = 0,
	DO_NOT_ALLOW_UI = 1 << 0
};

//! ----------------------------

enum class eLogoutConfig
{
	KEEP_STATE = 0,
	CLEAR_STATE = 1 << 0
};

//! ----------------------------

class CWinRTFacebookClient
{
public:

	static CWinRTFacebookClient& instance()
	{
		static CWinRTFacebookClient c;
		return c;
	}

public:

	void initialise(
		const std::wstring& app_id,
		Windows::UI::Core::CoreDispatcher^ ui_thread_dispatcher);

	concurrency::task<bool> login(
		const std::string& scopes,
		eLoginConfig config = eLoginConfig::ALLOW_UI);

	void logout(
		eLogoutConfig config = eLogoutConfig::CLEAR_STATE);

	inline const bool IsSignedIn() const { return m_bIsSingedIn; }
	
private:

	concurrency::task<bool>
		full_login(const std::string& scopes);

	concurrency::task<bool>	
		get_permissions();

private:

	Platform::String^ m_AppId;
	Windows::UI::Core::CoreDispatcher^ m_Dispatcher;

	bool m_bIsSingedIn = false;	
	concurrency::task_completion_event<bool> m_AuthenticateCompletion;
};

//! ----------------------------