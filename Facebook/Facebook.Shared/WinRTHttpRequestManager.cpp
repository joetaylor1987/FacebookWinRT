/*
*  WinRTHttpRequestManager.h
*
*  Created by Joe Taylor on 29/01/2015.
*  Copyright 2010 Ninja Kiwi. All rights reserved.
*
*/

#include "pch.h"
#include "WinRTHttpRequestManager.h"

#include <wrl.h>
#include <ppltasks.h>
#include <collection.h>
#include <robuffer.h>

using namespace Platform;
using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Storage::Streams;

//! ----------------------------

IHttpRequestManager* CreateHttpRequestManager()
{
	return new CWinRTHttpRequestManager();
}

//! ----------------------------

CWinRTHttpRequestManager::CWinRTHttpRequestManager()
	: m_NextID(1u)
{
	httpFilter = ref new HttpBaseProtocolFilter();
	httpClient = ref new HttpClient(httpFilter);
}

//! ----------------------------

SHttpRequest* CWinRTHttpRequestManager::GetRequest(uint32 _Id)
{
	auto it = std::find_if(m_ActiveRequests.begin(), m_ActiveRequests.end(), [&](const SHttpRequest& _r)
	{
		return _Id == _r.ID;
	});

	if (it != m_ActiveRequests.end())
	{
		return &(*it);
	}

	return nullptr;
}

bool CWinRTHttpRequestManager::RemoveRequest(uint32 _Id)
{
	auto it = std::find_if(m_ActiveRequests.begin(), m_ActiveRequests.end(), [&](const SHttpRequest& _r)
	{
		return _Id == _r.ID;
	});

	if (it != m_ActiveRequests.end())
	{
		m_ActiveRequests.erase(it);
		return true;
	}

	return false;
}

/**
* @return ID of sent request, 0 if failed to send request
*/
const uint32 CWinRTHttpRequestManager::Send(const SHttpRequest &_request, IHttpCallback *const callback)
{
	uint32 Id = m_NextID++;
	if (m_NextID == 0)
		++m_NextID;

	Windows::Foundation::Uri^ uri = ref new Windows::Foundation::Uri(StringConvert(_request.URL));
	HttpMethod^ method = nullptr;

	{ 
		m_ActiveRequests.push_back(_request);
		SHttpRequest& request = m_ActiveRequests.back();
		request.ID = Id;
		request.State = eRS_InProgress;
		request.TimeStamp = clock();

		switch (request.Method)
		{
		case HTTP_GET:
			method = HttpMethod::Get;
			break;
		case HTTP_POST:
			method = HttpMethod::Post;
			break;
		case HTTP_HEAD:
			method = HttpMethod::Head;
			break;
		default:
			break;
		}
	}
	assert(method != nullptr && "Http Method not supported");

	auto httpMessage = ref new HttpRequestMessage(method, uri);

	create_task(httpClient->SendRequestAsync(httpMessage))
		.then([this, Id](task<HttpResponseMessage^> responseResult)
	{
		// JT: In the event of something going wrong , return an async task 
		// that itself returns an empty buffer. We want the next stage in
		// the task chain to execute, so this is a nescessary evil
		IAsyncOperationWithProgress<IBuffer^, unsigned long long>^ returnAsync =
			create_async([](progress_reporter<unsigned long long> reporter) -> IBuffer^ { return nullptr; });

		if (SHttpRequest* request = GetRequest(Id))
		{
			try
			{
				HttpResponseMessage^ response = responseResult.get();

				request->HttpCode = static_cast<int32>(response->StatusCode);

				for each(IKeyValuePair<String^, String^>^ pair in response->Headers)
					request->response_header[StringConvert(pair->Key)] = StringConvert(pair->Value);

				for each(IKeyValuePair<String^, String^>^ pair in response->Content->Headers)
					request->response_header[StringConvert(pair->Key)] = StringConvert(pair->Value);

				request->State = response->IsSuccessStatusCode ? eRS_Finished : eRS_Failed;

				returnAsync = response->Content->ReadAsBufferAsync();
			}
			catch (Exception^ ex)
			{
				request->State = eRS_Failed;
			}
		}

		return returnAsync;
	})
		.then([this, Id, callback](IBuffer^ responseBodyAsBuffer)
	{
		if (SHttpRequest* request = GetRequest(Id))
		{
			if (request->State == eRS_Failed)
			{
				callback->HttpFailed(*request);
			}
			else
			{
				auto reader = DataReader::FromBuffer(responseBodyAsBuffer);

				// Writing to file or memory?
				switch (request->SaveType)
				{
				case HTTP_FILE:
				{
					/*
					if (request->pFile)
					{
						ComPtr<IInspectable> insp(reinterpret_cast<IInspectable*>(responseBodyAsBuffer));

						ComPtr<IBufferByteAccess> bufferByteAccess;
						insp.As(&bufferByteAccess);

						byte* data = nullptr;
						bufferByteAccess->Buffer(&data);

						request->pFile->WriteBytes(data, reader->UnconsumedBufferLength);
					}
					else
					{
						ERROR_LOG("Requested HTTP_FILE operation but no file supplied. (Callback Key: \"%s\")", request->CallBackKey.c_str());
					}
					*/
				}
				break;

				case HTTP_MEMORY:
				{
					request->downloaded_data.resize(reader->UnconsumedBufferLength);
					reader->ReadBytes(
						ArrayReference<unsigned char>((unsigned char*)
						request->downloaded_data.data(),
						request->downloaded_data.size())
						);
				}
				break;
				}

				callback->HttpComplete(*request);
			}

			RemoveRequest(Id);

		}
	}, task_continuation_context::use_current());

	return Id;
}

//! ----------------------------