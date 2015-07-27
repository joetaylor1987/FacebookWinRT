#include "pch.h"
#include "WinRTFacebookHelpers.h"

using namespace concurrency;
using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Data::Json;

namespace FBHelpers
{
	std::shared_ptr<IHttpRequestManager> s_HttpRequestManager = nullptr;

	task<GraphResponse^> GraphRequestAsync(const NKUri<std::wstring>& graph_api_uri, bool use_access_token /*= true*/)
	{
		assert(s_HttpRequestManager && "s_HttpRequestManager must be initialised");

		NKUri<std::wstring> uri_copy = graph_api_uri;

		if (use_access_token)
			uri_copy.AppendQuery(L"access_token", std::wstring(PersistentData::get_access_token()->Data()));

		SHttpRequest req;
		req.URL = narrow(uri_copy.ToString());
		req.DataFormat = HTTP_JSON;
		req.Method = HTTP_GET;

		task_completion_event<SHttpRequest> callbackCompletion;
		s_HttpRequestManager->Send(req, HttpCallbackFunctor::Create([=](const SHttpRequest &request) {
			callbackCompletion.set(request);
		}));

		return task<SHttpRequest>(callbackCompletion)
			.then([](SHttpRequest request)
		{
			GraphResponse^ response = ref new GraphResponse;

			if (request.State == eRS_Succeeded)
			{
				try
				{
					std::string responseStr = request.GetDownloadedDataStr();
					JsonValue^ response_json = JsonValue::Parse(StringConvert(responseStr));

					if (response_json->GetObject()->HasKey("error"))
					{
						response->error = RestError::Parse(
							response_json->GetObject()->GetNamedValue("error"));
					}
					else
					{
						if (response_json->GetObject()->HasKey("paging"))
						{
							response->result = response_json;

							auto paging_ojbect = response_json->GetObject()->GetNamedObject("paging");
							if (paging_ojbect->HasKey("next"))
							{
								GraphResponse^ paged_response = GraphRequestAsync
									(NKUri<std::wstring>(paging_ojbect->GetNamedString(L"next")->Data()), false)
									.get();

								if (paged_response->error)
								{
									// What do we do if a page fails?
									assert(false);
								}
								else
								{
									// Merge "data" objects
									if (response->result->GetObject()->HasKey("data") &&
										paged_response->result->GetObject()->HasKey("data"))
									{
										JsonArray^ source = paged_response->result->GetObject()->GetNamedArray("data");
										JsonArray^ destination = response->result->GetObject()->GetNamedArray("data");

										for (unsigned int i = 0; i < source->Size; i++)
											destination->Append(source->GetAt(i));
									}
								}
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
					response->error = ref new RestError;
					response->error->type = "Parse";
					response->error->message = e->Message;
				}
			}
			else
			{
				response->error = ref new RestError;
				response->error->type = "Http";
				response->error->code = request.HttpCode;
				response->error->message = StringConvert(request.GetErrorString());
			}

			return response;
		});
	}

	RestError^ RestError::Parse(JsonValue^ json_value)
	{
		RestError^ result = ref new RestError;

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
			assert(false && "error parsing Json 'Error' Object");
		}

		return result;
	}

	namespace PersistentData
	{
		String^ cache_container = "FacebookCache";
		String^ cache_entry_key = "accessToken";

		//! ----------------------------

		const bool has_access_token()
		{
			try
			{
				auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
					cache_container,
					ApplicationDataCreateDisposition::Existing);

				return ls->Values->HasKey(cache_entry_key);
			}
			catch (Exception^ e)
			{

			}

			return false;
		}

		//! ----------------------------

		String^ get_access_token()
		{
			try
			{
				auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
					cache_container,
					ApplicationDataCreateDisposition::Existing);

				if (ls->Values->HasKey(cache_entry_key))
					return dynamic_cast<String^>(ls->Values->Lookup(cache_entry_key));
			}
			catch (Exception^ e)
			{

			}

			return "";
		}

		//! ----------------------------

		void set_access_token(String^ token)
		{
			try
			{
				auto ls = ApplicationData::Current->LocalSettings->CreateContainer(
					cache_container,
					ApplicationDataCreateDisposition::Always);

				ls->Values->Insert(cache_entry_key, token);
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
					cache_container,
					ApplicationDataCreateDisposition::Existing);

				if (ls->Values->HasKey(cache_entry_key))
					ls->Values->Remove(cache_entry_key);
			}
			catch (Exception^ e)
			{

			}
		}

		//! ----------------------------
	}
}