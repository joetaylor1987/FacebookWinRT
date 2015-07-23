#include "pch.h"

#include "Facebook.h"
#include "WinRTFacebookHelpers.h"

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Security::Authentication::Web;
using namespace Windows::Data::Json;
using namespace FBHelpers;

const std::string fb_app_id = "710421129063177";

//! ----------------------------

CWinRTFacebookClient& CWinRTFacebookClient::instance()
{
	static CWinRTFacebookClient c;
	return c;
}

//! ----------------------------

CWinRTFacebookClient::CWinRTFacebookClient()
	: m_bIsSingedIn(false)
{
	s_HttpRequestManager = std::shared_ptr<IHttpRequestManager>(CreateHttpRequestManager());
}

//! ----------------------------

void CWinRTFacebookClient::login(std::string scopes, bool allow_ui)
{
	auto full_login_flow = [=]()
	{
		if (allow_ui)
		{
			full_login(scopes)
				.then([=](bool bSuccess)
			{
				if (bSuccess)
				{
					return refresh_permissions();
				}
				else
				{
					return create_task([](){ return false; });
				}
			})
				.then([=](bool bSuccess)
			{
				if (bSuccess)
				{
					m_bIsSingedIn = true;
					// success
				}
				else
				{
					// failure
					logout();
				}
			});
		}
		else
		{
			// failure
			logout();
		}
	};

	if (PersistentData::has_access_token())
	{
		refresh_permissions()
			.then([=](bool bSuccess)
		{
			if (bSuccess)
			{
				m_bIsSingedIn = true;
				// success
			}
			else
			{
				full_login_flow();
			}
		});
	}
	else
	{
		full_login_flow();
	}
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

task<bool> CWinRTFacebookClient::full_login(std::string scopes)
{
	NKUri login_uri("https://www.facebook.com/dialog/oauth");

	login_uri.AppendQuery( "client_id", fb_app_id );
	login_uri.AppendQuery( "redirect_uri", "https://www.facebook.com/connect/login_success.html" );
	login_uri.AppendQuery( "scope", scopes);
	login_uri.AppendQuery( "display", "popup" );
	login_uri.AppendQuery( "response_type", "token" );

	String^ wStrUri = StringConvert(login_uri.ToString());

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	WebAuthenticationBroker::AuthenticateAndContinue(
		ref new Uri(wStrUri),
		WebAuthenticationBroker::GetCurrentApplicationCallbackUri());

	return create_task([](){ return false; });
#else
	return create_task(WebAuthenticationBroker::AuthenticateAsync(
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
		.then([](String^ long_term_access_token)
	{
		if (long_term_access_token)
		{
			PersistentData::set_access_token(long_term_access_token);
			return true;
		}

		return false;
	});
#endif
}

task<bool> CWinRTFacebookClient::refresh_permissions()
{	
	return GraphRequestAsync(NKUri("https://graph.facebook.com/me/permissions"))
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