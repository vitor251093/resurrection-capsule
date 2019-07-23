
#ifndef _HTTP_URI_HEADER
#define _HTTP_URI_HEADER

// Include
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <array>
#include <map>
#include <regex>

// HTTP
namespace HTTP {
	// URI
	class URI {
		public:
			static std::string encode(std::string_view str);
			static bool decode(std::string_view str, std::string& out);

			void parse(std::string_view path);
			
			decltype(auto) begin() { return mQuery.begin(); }
			decltype(auto) begin() const { return mQuery.begin(); }
			decltype(auto) end() { return mQuery.end(); }
			decltype(auto) end() const { return mQuery.end(); }

			const std::string& protocol() const;
			const std::string& domain() const;
			const std::string& resource() const;
			uint16_t port() const;

			std::string parameter(const std::string& name) const;
			int64_t parameteri(const std::string& name) const;
			uint64_t parameteru(const std::string& name) const;
			bool parameterb(const std::string& name) const;

			void set_parameter(const std::string& name, const std::string& value);

		private:
			size_t parse_scheme(std::string_view path, size_t offset = 0);
			size_t parse_authority(std::string_view path, size_t offset = 0);

			void parse_path(std::string_view path, size_t offset = 0);
			void parse_query(std::string_view query);

		private:
			std::map<std::string, std::string> mQuery;

			std::string mProtocol;
			std::string mDomain;
			std::string mResource;
			std::string mFragment;

			uint16_t mPort = 0;
	};
}

#endif
