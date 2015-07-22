// Facebook.h
#pragma once

#include <string>
#include <ppltasks.h>

#include "HttpRequestInterface.h"

namespace FBHelpers
{
	public ref class Error sealed
	{
	public:
		property Platform::String^ message;
		property Platform::String^ type;
		property Platform::String^ error_user_title;
		property Platform::String^ error_user_msg;

		property unsigned int code;
		property unsigned int sub_code;

		static Error^ Parse(Windows::Data::Json::JsonValue^ json_value);
	};

	public ref class GraphResponse sealed
	{
	public:

		property Windows::Data::Json::JsonValue^
			result;
		property Error^
			error;
	};
}

class CWinRTFacebookClient
{
public:

	static CWinRTFacebookClient& instance();

	void login	(std::string scopes, bool allow_ui);
	void logout	(bool clearAccessToken = true);
	
private:

	CWinRTFacebookClient();
	
	concurrency::task<FBHelpers::GraphResponse^>
		GraphRequestAsync(const NKUri& graph_api_uri);

	concurrency::task<bool>	
		full_login(std::string scopes);

	concurrency::task<bool>	
		refresh_permissions();	

private:

	std::shared_ptr<IHttpRequestManager>
			m_HttpRequestManager;
	bool	m_bIsSingedIn;
};