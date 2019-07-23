
// Include
#include "multipart.h"

#include "../utils/functions.h"

// HTTP
namespace HTTP {
	// Multipart
	Multipart::Multipart(const std::string& body, const std::string& boundary) {
		mBoundary = "--" + boundary + "\r\n";
		mRegex = std::regex(R"_(name\s*=\s*"([a-zA-Z0-9_]+)"[\r\n]+([^\r\n]+))_");
		parse(body);
	}

	std::string Multipart::field(const std::string& name) {
		auto it = mFields.find(name);
		return it != mFields.end() ? it->second : std::string();
	}

	void Multipart::parse(const std::string& body) {
		auto length = body.length();
		auto position = body.find(mBoundary);
		while (position != std::string::npos) {
			position += mBoundary.length();

			std::string_view bodyView(&body[position], length - position);
			if (bodyView.starts_with("Content-Disposition")) {
				std::cmatch match;
				if (std::regex_search(bodyView.data(), match, mRegex)) {
					mFields.emplace(match[1], match[2]);
				}
			} else if (bodyView.starts_with("Content-Type")) {
				// Don't care atm.
			}

			position = body.find(mBoundary, position);
		}
	}
}
