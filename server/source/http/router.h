
#ifndef _HTTP_ROUTER_HEADER
#define _HTTP_ROUTER_HEADER

// Include
#include "uri.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <vector>
#include <regex>

// HTTP
namespace HTTP {
	class Session;

	// Response
	class Response {
		public:
			void set(boost::beast::http::field field, const std::string& value);

			std::string& body();

			boost::beast::http::status& result();
			boost::beast::http::verb& method();

			uint32_t& version();
			bool& keep_alive();

			void send(Session& session);

		private:
			std::map<boost::beast::http::field, std::string> mFields;
			std::string mBody;

			boost::beast::http::status mResult;
			boost::beast::http::verb mMethod;

			uint32_t mVersion;

			bool mKeepAlive;
	};

	using RouteFn = std::function<void(Session&, Response&)>;

	// RoutePath
	class RoutePath {
		public:
			RoutePath(const std::string& path, boost::beast::http::verb method);
			RoutePath(std::string&& path, boost::beast::http::verb method);

			void set_function(RouteFn function);

			bool equals(const std::string& resource) const;

		private:
			void construct();

		private:
			boost::beast::http::verb mMethod;

			RouteFn mFunction;

			std::string mPath;
			std::regex mRegExpr;

			friend class Router;
	};

	// Router
	class Router {
		public:
			bool run(Session& session, Response& response);

			void add(std::string path, std::initializer_list<boost::beast::http::verb> methods, RouteFn function);
			void add(std::string path, boost::beast::http::verb method, RouteFn function);

		private:
			std::vector<RoutePath> mRoutes;
	};
}

#endif
