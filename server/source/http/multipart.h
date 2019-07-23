
#ifndef _HTTP_MULTIPART_HEADER
#define _HTTP_MULTIPART_HEADER

// Include
#include <string>
#include <string_view>
#include <map>
#include <regex>

// HTTP
namespace HTTP {
	// Multipart
	class Multipart {
		public:
			Multipart(const std::string& body, const std::string& boundary);

			decltype(auto) begin() { return mFields.begin(); }
			decltype(auto) begin() const { return mFields.begin(); }
			decltype(auto) end() { return mFields.end(); }
			decltype(auto) end() const { return mFields.end(); }

			std::string field(const std::string& name);

		private:
			void parse(const std::string& body);

		private:
			std::map<std::string, std::string> mFields;
			std::string mBoundary;
			std::regex mRegex;
	};
}

#endif
