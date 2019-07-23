
// Include
#include "router.h"
#include "session.h"
#include "uri.h"

#include "../utils/functions.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <iostream>

// HTTP
namespace HTTP {
	// Response
	void Response::set(boost::beast::http::field field, const std::string& value) {
		mFields[field] = value;
	}

	std::string& Response::body() {
		return mBody;
	}

	boost::beast::http::status& Response::result() {
		return mResult;
	}

	boost::beast::http::verb& Response::method() {
		return mMethod;
	}

	uint32_t& Response::version() {
		return mVersion;
	}

	bool& Response::keep_alive() {
		return mKeepAlive;
	}

	void Response::send(Session& session) {
		uint32_t realVersion = mVersion & 0xFFFFFFF;
		if (mVersion & 0x1000'0000) {
			boost::beast::error_code error;
			boost::beast::http::file_body::value_type body;
			body.open(mBody.c_str(), boost::beast::file_mode::scan, error);
			if (error == boost::beast::errc::no_such_file_or_directory) {
				std::cout << "Could not find file " << mBody << std::endl;
			} else if (error) {
				std::cout << "Something error finding file " << mBody << std::endl;
			}

			const auto size = body.size();
			boost::beast::http::response<boost::beast::http::file_body> response {
				std::piecewise_construct,
				std::make_tuple(std::move(body)),
				std::make_tuple(mResult, realVersion)
			};

			for (const auto& [field, value] : mFields) {
				response.set(field, value);
			}

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, mime_type(mBody));

			response.keep_alive(mKeepAlive);
			response.content_length(size);
			session.send(std::move(response));
		} else {
			boost::beast::http::response<boost::beast::http::string_body> response {
				mResult, realVersion
			};

			for (const auto& [field, value] : mFields) {
				response.set(field, value);
			}

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

			response.keep_alive(mKeepAlive);
			response.body() = mBody;
			response.content_length(mBody.length());
			session.send(std::move(response));
		}
	}

	// RoutePath
	RoutePath::RoutePath(const std::string& path, boost::beast::http::verb method) : mMethod(method), mPath(path) {
		construct();
	}

	RoutePath::RoutePath(std::string&& path, boost::beast::http::verb method) : mMethod(method), mPath(std::move(path)) {
		construct();
	}

	void RoutePath::set_function(RouteFn function) {
		mFunction = function;
	}

	bool RoutePath::equals(const std::string& resource) const {
		if (std::regex_match(resource, mRegExpr)) {
			return true;
		}
		return false;
	}

	void RoutePath::construct() {
		mRegExpr = std::regex(mPath);
	}

	// Router
	bool Router::run(Session& session, Response& response) {
		decltype(auto) request = session.get_request();
		request.uri.parse(request.data.target().to_string());

		for (const auto& route : mRoutes) {
			if (route.mMethod == request.data.method() && route.equals(request.uri.resource())) {
				response.result() = boost::beast::http::status::ok;
				response.version() = request.data.version();
				response.keep_alive() = request.data.keep_alive();

				route.mFunction(session, response);
				return true;
			}
		}
		return false;
	}

	void Router::add(std::string path, std::initializer_list<boost::beast::http::verb> methods, RouteFn function) {
		for (auto method : methods) {
			add(path, method, function);
		}
	}

	void Router::add(std::string path, boost::beast::http::verb method, RouteFn function) {
		const auto set_method = [&](uint32_t index) {
			auto it = mRoutes.begin();
			auto end = mRoutes.end();
			for (; it != end; ++it) {
				if (it->mMethod == method && it->mPath == path) {
					break;
				}
			}

			if (function) {
				if (it == end) {
					mRoutes.emplace_back(path, method).set_function(function);
				} else {
					it->set_function(function);
				}
			} else if (it != end) {
				mRoutes.erase(it);
			}
		};

		if (method == boost::beast::http::verb::get) {
			set_method(0);
		} else if (method == boost::beast::http::verb::post) {
			set_method(1);
		}
	}
}
