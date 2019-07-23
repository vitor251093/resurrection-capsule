
// Include
#include "uri.h"

#include "../utils/functions.h"

#include <charconv>

// HTTP
namespace HTTP {
	// URI
	std::string URI::encode(std::string_view str) {
		// Cant bother atm
		return std::string();
	}

	bool URI::decode(std::string_view str, std::string& out) {
		out.clear();
		out.reserve(str.size());

		std::string tmp(2, '\0');
		for (auto i = 0; i < str.size(); ++i) {
			char symbol = str[i];
			if (symbol == '%') {
				if (i + 3 <= str.size()) {
					tmp[0] = str[++i];
					tmp[1] = str[++i];
					out += utils::to_number<char>(tmp, 16);
				}
				else {
					return false;
				}
			} else if (symbol == '+') {
				out += ' ';
			} else {
				out += symbol;
			}
		}

		return true;
	}

	void URI::parse(std::string_view path) {
		std::string decoded_path;
		decode(path, decoded_path);
		path = decoded_path;

		auto scheme_end = parse_scheme(path);
		auto authority_end = parse_authority(path, scheme_end);
		if ((authority_end + 1) < path.length()) {
			parse_path(path, authority_end);
		}
		/*
		auto domain_separator = path.find('/', protocol_index);
		if (domain_separator != std::string::npos) {
			auto port_separator = path.rfind(':', domain_separator);
			if (port_separator != std::string::npos && port_separator > protocol_index) {
				mPort = std::stoi(path.substr(port_separator + 1, domain_separator - port_separator - 1));
			} else {
				mPort = (mProtocol == "https" || mProtocol == "wss") ? 443 : 80;
				port_separator = domain_separator;
			}

			auto query_identifier = path.find('?', domain_separator);
			if (query_identifier != std::string::npos) {
				mResource = path.substr(domain_separator, query_identifier - domain_separator);
				parse_query(path.substr(query_identifier + 1));
			} else {
				mResource = path.substr(domain_separator);
			}

			mDomain = path.substr(protocol_index, port_separator - protocol_index);
		} else {
			mDomain = path.substr(protocol_index);
			mPort = (mProtocol == "https" || mProtocol == "wss") ? 443 : 80;
		}*/
	}

	const std::string& URI::protocol() const {
		return mProtocol;
	}

	const std::string& URI::domain() const {
		return mDomain;
	}

	const std::string& URI::resource() const {
		return mResource;
	}

	uint16_t URI::port() const {
		return mPort;
	}

	std::string URI::parameter(const std::string& name) const {
		auto it = mQuery.find(name);
		return it != mQuery.end() ? it->second : std::string();
	}

	int64_t URI::parameteri(const std::string& name) const {
		int64_t value;
		try {
			value = std::stoll(parameter(name));
		} catch (...) {
			value = 0;
		}
		return value;
	}

	uint64_t URI::parameteru(const std::string& name) const {
		uint64_t value;
		try {
			value = std::stoull(parameter(name));
		} catch (...) {
			value = std::numeric_limits<uint64_t>::max();
		}
		return value;
	}

	bool URI::parameterb(const std::string& name) const {
		return parameteru(name) != 0;
	}

	void URI::set_parameter(const std::string& name, const std::string& value) {
		auto [entry, inserted] = mQuery.try_emplace(name, value);
		if (!inserted) {
			entry->second = value;
		}
	}

	size_t URI::parse_scheme(std::string_view path, size_t offset) {
		static constexpr std::string_view protocol_identifier = "://";

		auto position = path.find(protocol_identifier, offset);
		if (position != std::string_view::npos) {
			mProtocol = path.substr(offset, position);
			position += protocol_identifier.length();
		} else {
			mProtocol = "http";
			position = offset;
		}
		return position;
	}

	size_t URI::parse_authority(std::string_view path, size_t offset) {
		static constexpr auto delimiter = '/';

		const auto default_port = [&] {
			return (mProtocol == "https" || mProtocol == "wss") ? 443 : 80;
		};

		auto position = path.find(delimiter, offset);
		if (position != std::string_view::npos) {
			auto autority_separator = path.rfind(':', position);
			if (autority_separator != std::string_view::npos && autority_separator > offset) {
				auto port_string_view = path.substr(autority_separator + 1, position - autority_separator - 1);
				if (auto [_, error] = std::from_chars(port_string_view.data(), port_string_view.data() + port_string_view.size(), mPort); error != std::errc()) {
					mPort = default_port();
				}
				mDomain = path.substr(offset, autority_separator - offset);
			} else {
				mPort = default_port();
				mDomain = path.substr(offset, position - offset);
			}
		} else {
			mPort = default_port();
			mDomain = path.substr(offset);
			position = offset;
		}
		return position;
	}

	void URI::parse_path(std::string_view path, size_t offset) {
		auto position = path.find('?', offset);
		if (position != std::string_view::npos) {
			auto fragment_position = path.find('#', position);
			if (fragment_position != std::string_view::npos) {
				parse_query(path.substr(position + 1, fragment_position - position - 1));
				mFragment = path.substr(fragment_position + 1);
			} else {
				parse_query(path.substr(position + 1));
			}
			mResource = path.substr(offset, position - offset);
		} else {
			position = path.find('#', offset);
			if (position != std::string_view::npos) {
				mFragment = path.substr(position);
				mResource = path.substr(offset, position - offset);
			} else {
				mResource = path.substr(offset);
			}
		}
	}

	/*
	void URI::parse(const std::string& path) {
		auto protocol_identifier = path.find("://");
		if (protocol_identifier != std::string::npos) {
			mProtocol = path.substr(0, protocol_identifier);
			protocol_identifier += 3;
		} else {
			mProtocol = "http";
			protocol_identifier = 0;
		}

		auto domain_separator = path.find('/', protocol_identifier);
		if (domain_separator != std::string::npos) {
			auto port_separator = path.rfind(':', domain_separator);
			if (port_separator != std::string::npos && port_separator > protocol_identifier) {
				mPort = std::stoi(path.substr(port_separator + 1, domain_separator - port_separator - 1));
			} else {
				mPort = (mProtocol == "https" || mProtocol == "wss") ? 443 : 80;
				port_separator = domain_separator;
			}

			auto query_identifier = path.find('?', domain_separator);
			if (query_identifier != std::string::npos) {
				mResource = path.substr(domain_separator, query_identifier - domain_separator);
				parse_query(path.substr(query_identifier + 1));
			} else {
				mResource = path.substr(domain_separator);
			}

			mDomain = path.substr(protocol_identifier, port_separator - protocol_identifier);
		} else {
			mDomain = path.substr(protocol_identifier);
			mPort = (mProtocol == "https" || mProtocol == "wss") ? 443 : 80;
		}
	}
	*/
	void URI::parse_query(std::string_view query) {
		for (const auto& variable : utils::explode_string(query, '&')) {
			auto separator = variable.find('=');
			if (separator != std::string::npos) {
				auto variableValue = std::string(variable.substr(separator + 1));
				std::string decodedVariableValue;

				if (decode(variableValue, decodedVariableValue)) {
					mQuery.emplace(variable.substr(0, separator), decodedVariableValue);
				} else {
					mQuery.emplace(variable.substr(0, separator), variableValue);
				}
			} else {
				mQuery.emplace(variable, std::string());
			}
		}
	}
}
