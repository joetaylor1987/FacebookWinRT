#include "pch.h"
#include "Facebook.h"
#include "URI.h"
#include <sstream>

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Security::Authentication::Web;
using namespace Windows::Storage;

const std::string fb_app_id = "710421129063177";
const std::string cache_container = "FacebookCache";
const std::string token_entry = "accessToken";

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

void CWinRTFacebookClient::del_access_token() const
{
	auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
		StringConvert(cache_container),
		ApplicationDataCreateDisposition::Existing);

	if (ls)
	{
		Platform::String^ accessToken = StringConvert(token_entry);
		if (ls->Values->HasKey(accessToken))
			ls->Values->Remove(accessToken);
	}
}

//! ----------------------------

void CWinRTFacebookClient::login(std::string scopes, bool allow_ui)
{
	m_sScopes = scopes;

	if (has_access_token())
	{
		// Store Local copy of access token
		m_AccessToken = get_access_token();

		// refresh permissions
		refresh_permissions();
	}
	else if (allow_ui)
	{
		full_login();
	}
	else
	{
		// failure
	}
}

void CWinRTFacebookClient::logout(bool clearAccessToken /*= true*/)
{
	m_bIsSingedIn = false;

	if (clearAccessToken)
	{
		del_access_token();
		m_AccessToken = std::string();

		// Clear permissions
	}
}

void CWinRTFacebookClient::full_login()
{
	NKUri login_uri("https://www.facebook.com/dialog/oauth");

	login_uri.AppendQuery( "client_id", fb_app_id );
	login_uri.AppendQuery( "redirect_uri", "https://www.facebook.com/connect/login_success.html" );
	login_uri.AppendQuery( "scope", m_sScopes);
	login_uri.AppendQuery( "display", "popup" );
	login_uri.AppendQuery( "response_type", "token" );

	String^ wUri = StringConvert(login_uri.ToString());

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
	WebAuthenticationBroker::AuthenticateAndContinue(
		ref new Uri(wUri),
		WebAuthenticationBroker::GetCurrentApplicationCallbackUri());
#else
	concurrency::create_task(WebAuthenticationBroker::AuthenticateAsync(
		WebAuthenticationOptions::None,
		ref new Uri(wUri),
		ref new Uri("https://www.facebook.com/connect/login_success.html")
//		WebAuthenticationBroker::GetCurrentApplicationCallbackUri()
		))
		.then([=](WebAuthenticationResult^ result)
	{
		if (result->ResponseStatus == WebAuthenticationStatus::Success)
		{
			m_bIsSingedIn = true;

			std::string response(StringConvert(result->ResponseData));
			auto start = response.find("access_token=");
			start += 13;
			auto end = response.find('&');

			m_AccessToken = response.substr(start, end - start);
			set_access_token(m_AccessToken);

			refresh_permissions();
		}
		else
		{
			unsigned int responseCode = result->ResponseErrorDetail;
			std::wstringstream ss;
			ss << "Error logging in to facebook: " << responseCode;
			OutputDebugString(ss.str().c_str());
		}
	});
#endif
}

//! ----------------------------

class HttpCallbackFunctor : public IHttpCallback
{
	using tFunctor = std::function <void(const SHttpRequest &)>;

public:

	template<class T>
	static HttpCallbackFunctor* Create(T&& arg)
	{
		auto spFunctor = std::make_shared<HttpCallbackFunctor>(std::forward<T>(arg));
		spFunctor->m_Self = spFunctor;
		return spFunctor.get();
	}

	HttpCallbackFunctor(tFunctor functor)
		: m_Functor(functor) {}

private:

	virtual void HttpComplete(const SHttpRequest &request)	override { m_Functor(request); m_Self.reset(); }
	virtual void HttpFailed(const SHttpRequest &request)	override { m_Functor(request); m_Self.reset(); }

	tFunctor m_Functor;
	std::shared_ptr<HttpCallbackFunctor> m_Self;
};

//! ----------------------------

void CWinRTFacebookClient::refresh_permissions()
{
	NKUri uri("https://graph.facebook.com/me/permissions");
	uri.AppendQuery("access_token", m_AccessToken);

	SHttpRequest req;
	req.URL = uri.ToString();
	req.DataFormat = HTTP_JSON;
	req.Method = HTTP_GET;

	m_HttpRequestManager->Send(req, HttpCallbackFunctor::Create([this](const SHttpRequest &request)
	{
		if (request.State == eRS_Finished)
		{
			std::string response_json = request.GetDownloadedDataStr();
		}
		else
		{
			full_login();
		}
	}));
}

//! ----------------------------

const bool CWinRTFacebookClient::has_access_token() const
{
	auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
		StringConvert(cache_container),
		ApplicationDataCreateDisposition::Existing);

	return ls && ls->Values->HasKey(StringConvert(token_entry));
}

//! ----------------------------

const std::string CWinRTFacebookClient::get_access_token() const
{
	auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
		StringConvert(cache_container),
		ApplicationDataCreateDisposition::Existing);

	if (ls && ls->Values->HasKey(StringConvert(token_entry)))
		return StringConvert(
		dynamic_cast<String^>(ls->Values->Lookup(StringConvert(token_entry))));

	return "";
}

//! ----------------------------

void CWinRTFacebookClient::set_access_token(const std::string& token)
{
	auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
		StringConvert(cache_container),
		ApplicationDataCreateDisposition::Always);

	if (ls)
	{
		ls->Values->Insert(
			StringConvert(token_entry),
			StringConvert(m_AccessToken));
	}
}

//! ----------------------------
