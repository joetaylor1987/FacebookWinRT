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

namespace FBHelpers
{
	Error^ Error::Parse(Windows::Data::Json::JsonValue^ json_value)
	{
		Error^ result = ref new Error;

		try
		{
			JsonObject^ obj = json_value->GetObject();

			if (obj->HasKey("message"))
				result->message = obj->GetNamedString("message");

			if (obj->HasKey("type"))
				result->type = obj->GetNamedString("type");

			if (obj->HasKey("error_user_title"))
				result->error_user_title = obj->GetNamedString("error_user_title");

			if (obj->HasKey("error_user_msg"))
				result->error_user_msg = obj->GetNamedString("error_user_msg");

			if (obj->HasKey("code"))
				result->code = static_cast<decltype(result->code)>(obj->GetNamedNumber("code"));

			if (obj->HasKey("error_subcode"))
				result->sub_code = static_cast<decltype(result->sub_code)>(obj->GetNamedNumber("error_subcode"));
		}
		catch (Exception^ e)
		{

		}

		return result;
	}
}

concurrency::task<FBHelpers::GraphResponse^> CWinRTFacebookClient::GraphRequestAsync(const NKUri& graph_api_uri)
{
	return create_task([=]()
	{
		FBHelpers::GraphResponse^ response = ref new FBHelpers::GraphResponse;

		std::string api_uri = graph_api_uri.ToString();
		bool is_paging = false;

		do
		{
			SHttpRequest req;
			req.URL = api_uri;
			req.DataFormat = HTTP_JSON;
			req.Method = HTTP_GET;

			task_completion_event<SHttpRequest> callbackCompletion;
			m_HttpRequestManager->Send(req, HttpCallbackFunctor::Create([=](const SHttpRequest &request) {
				callbackCompletion.set(request);
			}));

			auto graph_request = task<SHttpRequest>(callbackCompletion)
				.then([&](SHttpRequest request)
			{
				if (request.State == eRS_Succeeded)
				{
					try
					{
						std::string responseStr = request.GetDownloadedDataStr();
						JsonValue^ response_json = JsonValue::Parse(StringConvert(responseStr));

						if (response_json->GetObject()->HasKey("error"))
						{
							response->error = FBHelpers::Error::Parse(
								response_json->GetObject()->GetNamedValue("error"));

							is_paging = false;
						}
						else
						{
							if (response_json->GetObject()->HasKey("paging"))
							{
								// Extract data

								auto paging_ojbect = response_json->GetObject()->GetNamedObject("paging");

								if (paging_ojbect->HasKey("next"))
								{
									is_paging = true;
									api_uri = StringConvert(paging_ojbect->GetNamedString("next"));
								}
								else
								{
									is_paging = false;
								}
							}
							else
							{
								response->result = response_json;
							}
						}
					}
					catch (Exception^ e)
					{
						response->error = ref new FBHelpers::Error;
						response->error->type = "Parse";
						response->error->message = e->Message;

						is_paging = false;
					}
				}
				else
				{
					response->error = ref new FBHelpers::Error;
					response->error->type = "Http";
					response->error->code = request.HttpCode;
					response->error->message = StringConvert(request.GetErrorString());

					is_paging = false;
				}
			});

			graph_request.wait();

		} while (is_paging);

		return response;
	});	
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
	NKUri uri("https://graph.facebook.com/me/likes");
	uri.AppendQuery("access_token", PersistentData::get_access_token());
	
	return GraphRequestAsync(uri).then([](FBHelpers::GraphResponse^ response)
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