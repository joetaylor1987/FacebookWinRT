#include "pch.h"

#include "Facebook.h"
#include "WinRTFacebookHelpers.h"

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Security::Authentication::Web;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;
using namespace FBHelpers;

//! ----------------------------

void CWinRTFacebookClient::initialise(const std::wstring& app_id, Windows::UI::Core::CoreDispatcher^ ui_thread_dispatcher)
{
	// We must have a dispatcher for the UI thread
	assert(ui_thread_dispatcher);

	m_AppId = ref new String(app_id.c_str());
	m_Dispatcher = ui_thread_dispatcher;
}

//! ----------------------------

task<bool> CWinRTFacebookClient::login(const std::string& scopes, eLoginConfig config /*= eLoginConfig::ALLOW_UI*/)
{
	return concurrency::create_task([=]()
	{
		bool session_is_valid = false;

		// We've already got an access token
		if (PersistentData::has_access_token())
		{
			// Check it's still valid by refreshing permissions
			session_is_valid = get_permissions().get();
		}

		// If the above failed and we're allowing UI, proceed with full login,
		// followed by another token refresh attempt
		if (!session_is_valid && config == eLoginConfig::ALLOW_UI)
		{
			session_is_valid = full_login(scopes).get() && get_permissions().get();
		}
		
		return session_is_valid;
	})
		.then([=](bool session_is_valid)
	{
		// If permission get failed. Logout
		if (!session_is_valid)
			logout();

		// Signed in if session is valid
		m_bIsSingedIn = session_is_valid;

		// Finally, return success or failure
		return session_is_valid;
	});
}

//! ----------------------------

void CWinRTFacebookClient::logout(eLogoutConfig config /*= CLEAR_STATE*/)
{
	m_bIsSingedIn = false;

	if (config == eLogoutConfig::CLEAR_STATE)
	{
		if (PersistentData::has_access_token())
			PersistentData::del_access_token();

		//! TODO: Clear permissions
	}
}

//! ----------------------------

task<bool> CWinRTFacebookClient::full_login(const std::string& scopes)
{
	assert(m_Dispatcher);

	// Construct facebook oauth URI
	NKUri<std::wstring> login_uri(L"https://www.facebook.com/dialog/oauth");

	login_uri.AppendQuery( L"client_id", m_AppId->Data() );
	login_uri.AppendQuery( L"redirect_uri", L"https://www.facebook.com/connect/login_success.html" );
	login_uri.AppendQuery( L"scope", widen(scopes));
	login_uri.AppendQuery( L"display", L"popup" );
	login_uri.AppendQuery( L"response_type", L"token" );

	String^ wStrUri = ref new String(login_uri.ToString().c_str());

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	WebAuthenticationBroker::AuthenticateAndContinue(
		ref new Uri(wStrUri),
		WebAuthenticationBroker::GetCurrentApplicationCallbackUri());

	//! TODO: Catch App's 'Continue' event, and set m_AuthenticateCompletion
#else

	concurrency::create_task(m_Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
		ref new DispatchedHandler([=]()
	{
		concurrency::create_task(WebAuthenticationBroker::AuthenticateAsync(
		WebAuthenticationOptions::None,
		ref new Uri(wStrUri),
		ref new Uri("https://www.facebook.com/connect/login_success.html")))
			.then([=](WebAuthenticationResult^ result)
		{
			String^ short_term_access_token;

			if (result->ResponseStatus == WebAuthenticationStatus::Success)
			{
				// Parse response for the access token
				std::wstring wstr_response(result->ResponseData->Data());
				std::wstring find_str = L"access_token=";
				auto start = wstr_response.find(find_str);
				if (start != std::wstring::npos)
				{
					std::wstring wstr_access_token = wstr_response.substr(start += find_str.size(), wstr_response.find('&') - start);
					short_term_access_token = ref new String(wstr_access_token.c_str());
				}
			}
			else
			{
				// something went wrong.
				unsigned int responseCode = result->ResponseErrorDetail;
			}

			// Return access token (can be null if auth failed)
			return short_term_access_token;
		})
			.then([](String^ short_term_access_token)
		{
			//! TODO: Replace me with request for long-term access token
			return concurrency::create_task([=]() { return short_term_access_token; });
		})
			.then([=](String^ long_term_access_token)
		{
			// If we have a long-term access token, store it.
			if (long_term_access_token)
				PersistentData::set_access_token(long_term_access_token);

			// Set our completion handler adn we're done.
			m_AuthenticateCompletion.set(long_term_access_token != nullptr);
		});
	})));

#endif

	// Wait for completion handler to be set
	return task<bool>(m_AuthenticateCompletion);
}

//! ----------------------------

task<bool> CWinRTFacebookClient::get_permissions()
{
	//! TODO: Clear permissions first.

	return GraphRequestAsync(NKUri<std::wstring>(L"https://graph.facebook.com/me/permissions"))
		.then([](GraphResponse^ response)
	{
		bool bSuccess = false;

		if (!response->error)
		{
			try
			{
				JsonArray^ values = response->result->GetObject()->GetNamedArray("data");

				for (unsigned int i = 0; i < values->Size; i++)
				{
					JsonObject^ permission = values->GetObjectAt(i);

					String^ name = permission->GetNamedString("permission");
					String^ state = permission->GetNamedString("status");

					if (String::CompareOrdinal(state, "granted") == 0)
					{
						//! TODO: Store permission: name
						std::wstring wstr_name = name->Data();
					}
				}

				bSuccess = true;
			}
			catch (Exception^ e)
			{

			}
		}

		return bSuccess;
	});
}

//! ----------------------------