#pragma once

#include "HttpRequestInterface.h"

namespace FBHelpers
{
	extern std::shared_ptr<IHttpRequestManager> s_HttpRequestManager;

	public ref class RestError sealed
	{
	public:
		property Platform::String^ message;
		property Platform::String^ type;
		property Platform::String^ error_user_title;
		property Platform::String^ error_user_msg;
		property unsigned int code;
		property unsigned int sub_code;

		static RestError^ Parse(Windows::Data::Json::JsonValue^ json_value);
	};

	public ref class GraphResponse sealed
	{
	public:
		property RestError^	error;
		property Windows::Data::Json::JsonValue^ result;		
	};

	concurrency::task<GraphResponse^> GraphRequestAsync(
		const NKUri<std::wstring>& graph_api_uri, 
		bool use_access_token = true);

	namespace PersistentData
	{
		const bool			has_access_token();
		Platform::String^	get_access_token();
		void				set_access_token(Platform::String^ token);
		void				del_access_token();
	};
}