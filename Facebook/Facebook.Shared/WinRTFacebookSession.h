#pragma once

#include <set>

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

using tPermissionsList = std::set<std::wstring>;

class CWinRTFacebookSession
{
public:

	static CWinRTFacebookSession& shared_instance()
	{
		static CWinRTFacebookSession c;
		return c;
	}

	static void Initialise(
		Windows::UI::Core::CoreDispatcher^ ui_thread_dispatcher)
	{
		shared_instance().initialise(ui_thread_dispatcher);
	}

	static concurrency::task<bool> Open(
		const std::wstring& app_id,
		const std::wstring& scopes,
		eLoginConfig config = eLoginConfig::ALLOW_UI)
	{
		return shared_instance().open(app_id, scopes, config);
	}

	static void Close(
		eLogoutConfig config = eLogoutConfig::CLEAR_STATE)
	{
		shared_instance().close(config);
	}

	static const bool IsSessionActive()
	{
		return shared_instance().isSessionActive(); 
	}

	static const bool HasPermission(const std::wstring& permission)
	{
		return shared_instance().hasPermission(permission);
	}
	
private:

	void initialise(
		Windows::UI::Core::CoreDispatcher^ ui_thread_dispatcher);

	concurrency::task<bool> open(
		const std::wstring& app_id,
		const std::wstring& scopes,
		eLoginConfig config);

	void close(eLogoutConfig config);

	inline const bool isSessionActive() const { return m_SessionActive; }

	concurrency::task<bool>	full_login(
			const std::wstring& app_id,
			const std::wstring& scopes);

	concurrency::task<bool>	update_permissions();

	inline const bool hasPermission(const std::wstring& permission) const
	{
		return m_Permissions.find(permission) != m_Permissions.end();
	}

private:

	Windows::UI::Core::CoreDispatcher^
		m_Dispatcher = nullptr;

	bool m_SessionActive = false;

	tPermissionsList m_Permissions;
};

//! ----------------------------