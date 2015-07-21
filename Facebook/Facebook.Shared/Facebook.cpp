#include "pch.h"
#include "Facebook.h"

#include <sstream>

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Security::Authentication::Web;
using namespace Windows::Storage;
using namespace Windows::Data::Json;

const std::string fb_app_id = "710421129063177";

namespace PersistentData
{
	const std::string	cache_container = "FacebookCache";
	const std::string	token_entry = "accessToken";

	const bool			has_access_token();
	const std::string	get_access_token();
	void				set_access_token(const std::string& token);
	void				del_access_token();
}

concurrency::task<SHttpRequest> CWinRTFacebookClient::GraphRequestAsync(const NKUri& graph_api_uri)
{
	SHttpRequest req;
	req.URL = graph_api_uri.ToString();
	req.DataFormat = HTTP_JSON;
	req.Method = HTTP_GET;

	task_completion_event<SHttpRequest> callbackCompletion;
	m_HttpRequestManager->Send(req, HttpCallbackFunctor::Create([=](const SHttpRequest &request) {
		callbackCompletion.set(request);
	}));

	return task<SHttpRequest>(callbackCompletion);
}

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
	m_HttpRequestManager = std::shared_ptr<IHttpRequestManager>(CreateHttpRequestManager());
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

	String^ wUri = StringConvert(login_uri.ToString());

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	WebAuthenticationBroker::AuthenticateAndContinue(
		ref new Uri(wUri),
		WebAuthenticationBroker::GetCurrentApplicationCallbackUri());

	return create_task([](){ return false; });
#else
	return create_task(WebAuthenticationBroker::AuthenticateAsync(
		WebAuthenticationOptions::None,
		ref new Uri(wUri),
		ref new Uri("https://www.facebook.com/connect/login_success.html")))
		.then([=](WebAuthenticationResult^ result)
	{
		if (result->ResponseStatus == WebAuthenticationStatus::Success)
		{
			std::string response(StringConvert(result->ResponseData));
			auto start = response.find("access_token=");
			start += 13;
			auto end = response.find('&');

			PersistentData::set_access_token(
				response.substr(start, end - start));

			return true;
		}
		else
		{
			unsigned int responseCode = result->ResponseErrorDetail;
			std::wstringstream ss;
			ss << "Error logging in to facebook: " << responseCode;
			OutputDebugString(ss.str().c_str());
		}

		return false;
	});
#endif
}

task<bool> CWinRTFacebookClient::refresh_permissions()
{
	NKUri uri("https://graph.facebook.com/me/permissions");
	uri.AppendQuery("access_token", PersistentData::get_access_token());
	
	return GraphRequestAsync(uri).then([](const SHttpRequest& result)
	{
		if (result.State == eRS_Succeeded)
		{
			try
			{
				std::string responseStr = result.GetDownloadedDataStr();
				JsonValue^ response_json = JsonValue::Parse(StringConvert(responseStr));
				JsonArray^ values = response_json->GetObject()->GetNamedArray("data");

				for (unsigned int i = 0; i < values->Size; i++)
				{
					JsonObject^ permission = values->GetObjectAt(i);
					String^ permission_name = permission->GetNamedString("permission");
					String^ permission_status = permission->GetNamedString("status");
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

namespace PersistentData
{
	//! ----------------------------

	const bool has_access_token()
	{
		try
		{
			auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
				StringConvert(cache_container),
				ApplicationDataCreateDisposition::Existing);

			return ls->Values->HasKey(StringConvert(token_entry));
		}
		catch (Exception^ e)
		{

		}

		return false;
	}

	//! ----------------------------

	const std::string get_access_token()
	{
		try
		{
			auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
				StringConvert(cache_container),
				ApplicationDataCreateDisposition::Existing);

			if (ls->Values->HasKey(StringConvert(token_entry)))
				return StringConvert(
				dynamic_cast<String^>(ls->Values->Lookup(StringConvert(token_entry))));
		}
		catch (Exception^ e)
		{

		}

		return "";
	}

	//! ----------------------------

	void set_access_token(const std::string& token)
	{
		try
		{
			auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
				StringConvert(cache_container),
				ApplicationDataCreateDisposition::Always);

			ls->Values->Insert(
				StringConvert(token_entry),
				StringConvert(token));
		}
		catch (Exception^ e)
		{

		}
	}

	//! ----------------------------

	void del_access_token()
	{
		try
		{
			auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
				StringConvert(cache_container),
				ApplicationDataCreateDisposition::Existing);

			Platform::String^ accessToken = StringConvert(token_entry);
			if (ls->Values->HasKey(accessToken))
				ls->Values->Remove(accessToken);
		}
		catch (Exception^ e)
		{

		}
	}

	//! ----------------------------
}