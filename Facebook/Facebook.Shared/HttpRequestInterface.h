#pragma once

#include "HttpTypes.h"

#include <time.h>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <type_traits>

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

template<class StringType = std::string>
class NKUri
{
public:

	NKUri() = default;
	NKUri(const StringType& uriStr)
	{
		*this = NKUri::Parse(uriStr);
	}

	void AppendQuery(
		const StringType& key,
		const StringType& value)
	{
		Query.insert(std::make_pair(key, value));
	}

	const StringType ToString() const
	{
		using StreamType = typename std::conditional<std::is_same<StringType, std::string>::value,
			std::stringstream,
			std::wstringstream>::type;

		StreamType ss;

		if (!Protocol.empty())
			ss << Protocol << "://";

		if (!Host.empty())
			ss << Host;

		if (!Port.empty())
			ss << ":" << Port;

		if (!Path.empty())
			ss << Path;

		for (QueryContainer::const_iterator it = Query.begin(); it != Query.end(); ++it)
			ss << (it == Query.begin() ? "?" : "&") << it->first << "=" << it->second;

		if (!Fragment.empty())
			ss << "#" << Fragment;

		return ss.str();
	}

private:

	using QueryContainer = std::map<StringType, StringType>;
	StringType Protocol;
	StringType Host;
	StringType Port;
	StringType Path;
	QueryContainer Query;
	StringType Fragment;

	static NKUri Parse(const StringType& uri)
	{
		NKUri result;

		if (uri.length() == 0)
			return result;

		//! Protocol
		//! ----------------------------
		StringType::const_iterator protocolEnd = std::find(uri.begin(), uri.end(), ':');
		if (protocolEnd != uri.end())
		{
			StringType prot = &*(protocolEnd);
			if ((prot.length() > 3) && (prot[1] == '/'))
			{
				result.Protocol = StringType(uri.begin(), protocolEnd);
				protocolEnd += 3;
			}
		}

		//	"Protocol:://host:port/path?query#fragment

		//! Host
		//! ----------------------------
		StringType::const_iterator hostStart = result.Protocol.empty() ? uri.begin() : protocolEnd;
		StringType::const_iterator fragmentStart = std::find(uri.begin(), uri.end(), '#');
		StringType::const_iterator queryStart = std::find(uri.begin(), fragmentStart, '?');
		StringType::const_iterator pathStart = std::find(hostStart, queryStart, '/');
		StringType::const_iterator hostEnd = std::find(hostStart, (pathStart != uri.end()) ? pathStart : queryStart, ':');

		result.Host = StringType(hostStart, hostEnd);

		//! Port
		//! ----------------------------
		if ((hostEnd != uri.end()) && *hostEnd == ':')
		{
			hostEnd++;
			StringType::const_iterator portEnd = (pathStart != uri.end()) ? pathStart : queryStart;
			result.Port = StringType(hostEnd, portEnd);
		}

		//! Path
		//! ----------------------------
		if (pathStart != uri.end())
			result.Path = StringType(pathStart, queryStart);

		//! Query
		//! ----------------------------
		if (queryStart != fragmentStart)
		{
			queryStart += 1;

			StringType key;
			StringType::const_iterator it = queryStart;
			for (; it != fragmentStart; ++it)
			{
				if (*it == '=')
				{
					key = StringType(queryStart, it);
					queryStart = it + 1;
				}

				if (*it == '&')
				{
					result.Query[key] = StringType(queryStart, it);
					queryStart = it + 1;
				}
			}

			result.Query[key] = StringType(queryStart, it);
		}

		//! Fragment
		//! ----------------------------
		if (fragmentStart != uri.end())
			result.Fragment = StringType(fragmentStart + 1, uri.end());

		return result;
	}
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