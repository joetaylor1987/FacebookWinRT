#include "pch.h"

#include "WinRTFacebookSession.h"
#include "WinRTFacebookHelpers.h"

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Security::Authentication::Web;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;
using namespace FBHelpers;

//! ----------------------------

void CWinRTFacebookSession::initialise(Windows::UI::Core::CoreDispatcher^ ui_thread_dispatcher)
{
	// We must have a dispatcher for the UI thread
	assert(ui_thread_dispatcher);

	m_Dispatcher = ui_thread_dispatcher;
}

//! ----------------------------

task<bool> CWinRTFacebookSession::open(
	const std::wstring& app_id,
	const std::wstring& scopes,
	eLoginConfig config)
{
	return concurrency::create_task([=]()
	{
		return
			isSessionActive()							// already signed in
			||
			(
				PersistentData::has_access_token() &&	// have access token and
				update_permissions().get()				// successfully updated permissions
			)
			||
			(
				config == eLoginConfig::ALLOW_UI &&		// can show UI
				full_login(app_id, scopes).get() &&		// retrieved new access token
				update_permissions().get()				// successfully updated permissions
			);
	})
		.then([=](bool session_is_valid)
	{
		if (!session_is_valid)							// If permission get failed. Logout
			close(eLogoutConfig::CLEAR_STATE);

		return m_SessionActive = session_is_valid;		// Finally, return success or failure
	});
}

//! ----------------------------

void CWinRTFacebookSession::close(eLogoutConfig config)
{
	m_SessionActive = false;

	if (config == eLogoutConfig::CLEAR_STATE)
	{
		if (PersistentData::has_access_token())
			PersistentData::del_access_token();

		m_Permissions.clear();
	}
}

//! ----------------------------

task<bool> CWinRTFacebookSession::full_login(
	const std::wstring& app_id,
	const std::wstring& scopes)
{
	assert(m_Dispatcher);

	// Construct facebook oauth URI
	wNKUri login_uri(L"https://www.facebook.com/dialog/oauth");

	login_uri.AppendQuery( L"client_id", app_id );
	login_uri.AppendQuery( L"redirect_uri", L"https://www.facebook.com/connect/login_success.html" );
	login_uri.AppendQuery( L"scope", scopes);
	login_uri.AppendQuery( L"display", L"popup" );
	login_uri.AppendQuery( L"response_type", L"token" );

	auto startUrl = ref new Uri(ref new String(login_uri.ToString().c_str()));
	auto endUrl = ref new Uri("https://www.facebook.com/connect/login_success.html");

	// reset completion handler
	m_Authentication_tce = task_completion_event<bool>();

	concurrency::create_task(m_Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
		ref new DispatchedHandler([=]()
	{
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
		WebAuthenticationBroker::AuthenticateAndContinue(
			startUrl,
			endUrl,
			nullptr,
			WebAuthenticationOptions::None);

		m_bExpectingContinuation = true;
#else
		concurrency::create_task(WebAuthenticationBroker::AuthenticateAsync(
			WebAuthenticationOptions::None,
			startUrl,
			endUrl))
			.then([=](WebAuthenticationResult^ result)
		{
			parse_auth_result(result);
		});
#endif
	})));

	// Wait for completion handler to be set
	return task<bool>(m_Authentication_tce);
}

//! ----------------------------

void CWinRTFacebookSession::on_app_activated(IActivatedEventArgs^ args)
{
#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	if (m_bExpectingContinuation && args->Kind == ActivationKind::WebAuthenticationBrokerContinuation)
	{
		m_bExpectingContinuation = false;

		if (auto continuationEventArgs = safe_cast<WebAuthenticationBrokerContinuationEventArgs^>(args))
			parse_auth_result(continuationEventArgs->WebAuthenticationResult);
	}
#endif
}

//! ----------------------------

void CWinRTFacebookSession::parse_auth_result(WebAuthenticationResult^ result)
{
	create_task([=]()
	{
		String^ short_term_access_token = nullptr;

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
			//				unsigned int responseCode = result->ResponseErrorDetail;
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
		m_Authentication_tce.set(long_term_access_token != nullptr);
	});
}

//! ----------------------------

task<bool> CWinRTFacebookSession::update_permissions()
{
	m_Permissions.clear();

	return GraphRequestAsync(wNKUri(L"https://graph.facebook.com/me/permissions"))
		.then([=](GraphResponse^ response)
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
						m_Permissions.emplace(name->Data());
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