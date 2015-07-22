#pragma once

#include "HttpTypes.h"

#include <time.h>
#include <vector>
#include <map>
#include <string>

//! ----------------------------

struct IHttpRequestData
{
	virtual ~IHttpRequestData() {}
	virtual std::shared_ptr<IHttpRequestData> clone() const = 0;
};

template <typename Derived>
struct IHttpRequestData_CRTP : public IHttpRequestData
{
	virtual std::shared_ptr<IHttpRequestData> clone() const
	{
		return std::make_shared<Derived>(static_cast<Derived const&>(*this));
	}
};

#define Derive_Platform_Data(Type) class Type: public IHttpRequestData_CRTP<Type>

struct SHttpRequest
{
	friend class CCurlHttpRequestManager;
	friend class CWinRTHttpRequestManager;

public:
	
	// Request Parameters
	std::string		 _UserAgent;  ///< User agent set by HttpRequestManager
	std::string		CallBackKey;
	std::string		URL;
	HTTP_METHOD		Method;
	HTTP_DATA_FORMAT DataFormat;
	HTTP_SAVE_TYPE	SaveType;
	long			TCPKeepAliveInterval;
	SHttpTimeoutOptions TimeoutOptions;
	bool			FailOnError;
	class IFile		*pFile;

	// State
	uint32			ID;
	clock_t			TimeStamp;
	eRequestState	State;
	int32			HttpCode;
	HTTP_ERROR		ErrorEnum;

	// Reponse
	std::vector<char>					downloaded_data;
	std::map<std::string, std::string>	response_header;

public:

	SHttpRequest();

	const std::string	GetDownloadedDataStr() const;
	const std::string	GetErrorString() const;

	inline void			SetData(const std::string& _string) { postData = _string; }
	inline const std::string GetData() const { return postData; }

	inline std::shared_ptr<IHttpRequestData> GetPlatformData() const { return platformData; }

private:

	std::string	postData;

	std::shared_ptr<IHttpRequestData> platformData;
};

//! ----------------------------

struct IHttpCallback
{
	virtual ~IHttpCallback() {}

	virtual void HttpComplete(const SHttpRequest &request)	{}
	virtual void HttpFailed(const SHttpRequest &request)	{}
};

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

class NKUri
{
public:

	NKUri() = default;
	NKUri(const std::string& uriStr);

	void AppendQuery(
		const std::string& key,
		const std::string& value);

	const std::string ToString() const;

private:

	using QueryContainer = std::map<std::string, std::string>;
	std::string Protocol;
	std::string Host;
	std::string Port;
	std::string Path;
	QueryContainer Query;
	std::string Fragment;

	static NKUri Parse(const std::string& uri);
};

//! ----------------------------

struct IHttpRequestManager
{
	virtual ~IHttpRequestManager	() {}

	virtual void SetUserAgent		(const std::string& user_agent_str) = 0;
	virtual void			Process	() {};
	virtual const uint32	Send	(const SHttpRequest& request,
									struct IHttpCallback* const callback) = 0;
};

IHttpRequestManager *CreateHttpRequestManager();

//! ----------------------------