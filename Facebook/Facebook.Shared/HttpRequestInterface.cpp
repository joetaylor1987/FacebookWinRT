#include "pch.h"
#include "HttpRequestInterface.h"

#include <sstream>

//! ----------------------------

SHttpRequest::SHttpRequest()
	: ID(0)
	, State(eRS_Uninitialised)
	, ErrorEnum(eHTTP_OK)
	, HttpCode(0)
	, Method(HTTP_GET)
	, DataFormat(HTTP_NO_FORMAT)
	, SaveType(HTTP_MEMORY)
	, TimeStamp(0)
	, TCPKeepAliveInterval(-1L)
	, FailOnError(false)
	, pFile(NULL)
{	
}

//! ----------------------------

const std::string SHttpRequest::GetDownloadedDataStr() const
{
	return std::string(downloaded_data.begin(), downloaded_data.end());
}

//! ----------------------------

NKUri::NKUri(const std::string& uriStr)
{
	*this = NKUri::Parse(uriStr);
}

//! ----------------------------

const std::string NKUri::ToString() const
{
	std::stringstream ss;

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

//! ----------------------------

void NKUri::AppendQuery(
	const std::string& key,
	const std::string& value)
{
	Query.insert(std::make_pair(key, value));
}

//! ----------------------------

NKUri NKUri::Parse(const std::string& uri)
{
	NKUri result;

	if (uri.length() == 0)
		return result;

	//! Protocol
	//! ----------------------------
	std::string::const_iterator protocolEnd = std::find(uri.begin(), uri.end(), ':');
	if (protocolEnd != uri.end())
	{
		std::string prot = &*(protocolEnd);
		if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
		{
			result.Protocol = std::string(uri.begin(), protocolEnd);
			protocolEnd += 3;
		}
	}

	//	"Protocol:://host:port/path?query#fragment

	//! Host
	//! ----------------------------
	std::string::const_iterator hostStart = result.Protocol.empty() ? uri.begin() : protocolEnd;
	std::string::const_iterator fragmentStart = std::find(uri.begin(), uri.end(), '#');
	std::string::const_iterator queryStart = std::find(uri.begin(), fragmentStart, '?');
	std::string::const_iterator pathStart = std::find(hostStart, queryStart, '/');
	std::string::const_iterator hostEnd = std::find(hostStart, (pathStart != uri.end()) ? pathStart : queryStart, ':');

	result.Host = std::string(hostStart, hostEnd);

	//! Port
	//! ----------------------------
	if ((hostEnd != uri.end()) && *hostEnd == ':')
	{
		hostEnd++;
		std::string::const_iterator portEnd = (pathStart != uri.end()) ? pathStart : queryStart;
		result.Port = std::string(hostEnd, portEnd);
	}

	//! Path
	//! ----------------------------
	if (pathStart != uri.end())
		result.Path = std::string(pathStart, queryStart);

	//! Query
	//! ----------------------------
	if (queryStart != fragmentStart)
	{
		queryStart += 1;

		std::string key;
		std::string::const_iterator it = queryStart;
		for (; it != fragmentStart; ++it)
		{
			if (*it == '=')
			{
				key = std::string(queryStart, it);
				queryStart = it + 1;
			}

			if (*it == '&')
			{
				result.Query[key] = std::string(queryStart, it);
				queryStart = it + 1;
			}
		}

		result.Query[key] = std::string(queryStart, it);
	}

	//! Fragment
	//! ----------------------------
	if (fragmentStart != uri.end())
		result.Fragment = std::string(fragmentStart + 1, uri.end());

	return result;
}

//! ----------------------------