#pragma once

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