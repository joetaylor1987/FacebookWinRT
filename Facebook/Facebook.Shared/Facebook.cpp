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

const std::wstring fb_app_id = L"710421129063177";

//! ----------------------------

CWinRTFacebookClient& CWinRTFacebookClient::instance()
{
	static CWinRTFacebookClient c;
	return c;
}

//! ----------------------------

task<bool> CWinRTFacebookClient::login(std::string scopes, bool allow_ui /*= true*/)
{
	return create_task([=]()
	{
		return PersistentData::has_access_token() || (allow_ui && full_login(scopes).get());
	})
		.then([=](bool has_access_token)
	{
		return has_access_token && get_permissions().get();
	})
		.then([=](bool session_is_valid)
	{
		if (!session_is_valid)
			logout();

		m_bIsSingedIn = session_is_valid;

		return session_is_valid;
	});
}

void CWinRTFacebookClient::logout(bool clearAccessToken /*= true*/)
{
	m_bIsSingedIn = false;

	if (clearAccessToken)
	{
		if (PersistentData::has_access_token())
			PersistentData::del_access_token();
	}
}

task<bool> CWinRTFacebookClient::full_login(const std::string& scopes)
{
	assert(m_Dispatcher);

	NKUri<std::wstring> login_uri(L"https://www.facebook.com/dialog/oauth");

	login_uri.AppendQuery( L"client_id", fb_app_id );
	login_uri.AppendQuery( L"redirect_uri", L"https://www.facebook.com/connect/login_success.html" );
	login_uri.AppendQuery( L"scope", widen(scopes));
	login_uri.AppendQuery( L"display", L"popup" );
	login_uri.AppendQuery( L"response_type", L"token" );

	String^ wStrUri = ref new String(login_uri.ToString().c_str());	

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	WebAuthenticationBroker::AuthenticateAndContinue(
		ref new Uri(wStrUri),
		WebAuthenticationBroker::GetCurrentApplicationCallbackUri());

	return create_task([](){ return false; });
#else

	task_completion_event<bool> callbackCompletion;
	create_task(m_Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
		ref new DispatchedHandler([=]()
	{
		create_task(WebAuthenticationBroker::AuthenticateAsync(
		WebAuthenticationOptions::None,
		ref new Uri(wStrUri),
		ref new Uri("https://www.facebook.com/connect/login_success.html")))
			.then([=](WebAuthenticationResult^ result)
		{
			String^ short_term_access_token;

			if (result->ResponseStatus == WebAuthenticationStatus::Success)
			{
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
				unsigned int responseCode = result->ResponseErrorDetail;
				// something went wrong.
			}

			return short_term_access_token;
		})
			.then([](String^ short_term_access_token)
		{
			// TODO: Replace me with request for long-term access token
			return create_task([=]() { return short_term_access_token; });
		})
			.then([=](String^ long_term_access_token)
		{
			if (long_term_access_token)
				PersistentData::set_access_token(long_term_access_token);

			callbackCompletion.set(long_term_access_token != nullptr);
		});
	})));

	return task<bool>(callbackCompletion);
	/*
	
	*/
#endif
}

task<bool> CWinRTFacebookClient::get_permissions()
{	
	return GraphRequestAsync(NKUri<std::wstring>(L"https://graph.facebook.com/me/permissions"))
		.then([](GraphResponse^ response)
	{
		if (response->error)
		{

		}
		else
		{
			try
			{
				JsonArray^ values = response->result->GetObject()->GetNamedArray("data");

				for (unsigned int i = 0; i < values->Size; i++)
				{
					JsonObject^ permission = values->GetObjectAt(i);
					String^ name = permission->GetNamedString("permission");

					OutputDebugString(name->Data());
					OutputDebugString(L"\n");
				}

				return true;
			}
			catch (Exception^ e)
			{
				OutputDebugString(e->Message->Data());
			}
		}

		return false;
	});
}