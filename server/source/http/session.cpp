
// Include
#include "session.h"
#include "server.h"

#include "../game/config.h"

#include <boost/beast/version.hpp>

#include <iostream>

// HTTP
namespace HTTP {
	// Return a reasonable mime type based on the extension of a file.
	boost::beast::string_view mime_type(boost::beast::string_view path) {
		using boost::beast::iequals;

		const auto ext = [&path] {
			const auto pos = path.rfind(".");
			if (pos == boost::beast::string_view::npos) {
				return boost::beast::string_view {};
			}
			return path.substr(pos);
		}();

		if (iequals(ext, ".htm"))  return "text/html";
		if (iequals(ext, ".html")) return "text/html";
		if (iequals(ext, ".php"))  return "text/html";
		if (iequals(ext, ".css"))  return "text/css";
		if (iequals(ext, ".txt"))  return "text/plain";
		if (iequals(ext, ".js"))   return "application/javascript";
		if (iequals(ext, ".json")) return "application/json";
		if (iequals(ext, ".xml"))  return "application/xml";
		if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
		if (iequals(ext, ".flv"))  return "video/x-flv";
		if (iequals(ext, ".png"))  return "image/png";
		if (iequals(ext, ".jpe"))  return "image/jpeg";
		if (iequals(ext, ".jpeg")) return "image/jpeg";
		if (iequals(ext, ".jpg"))  return "image/jpeg";
		if (iequals(ext, ".gif"))  return "image/gif";
		if (iequals(ext, ".bmp"))  return "image/bmp";
		if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
		if (iequals(ext, ".tiff")) return "image/tiff";
		if (iequals(ext, ".tif"))  return "image/tiff";
		if (iequals(ext, ".svg"))  return "image/svg+xml";
		if (iequals(ext, ".svgz")) return "image/svg+xml";

		return "application/text";
	}

	// Append an HTTP rel-path to a local filesystem path.
	// The returned path is normalized for the platform.
	std::string path_cat(boost::beast::string_view base, boost::beast::string_view path) {
		if (base.empty()) {
			return std::string(path);
		}

		std::string result(base);
#ifdef BOOST_MSVC
		char constexpr path_separator = '\\';
		if (result.back() == path_separator) {
			result.resize(result.size() - 1);
		}

		result.append(path.data(), path.size());
		for (auto& c : result) {
			if (c == '/') {
				c = path_separator;
			}
		}
#else
		char constexpr path_separator = '/';
		if (result.back() == path_separator) {
			result.resize(result.size() - 1);
		}

		result.append(path.data(), path.size());
#endif
		return result;
	}

	void handle_request(
		Session& session,
		Router& router
	) {
		auto& request = session.get_request().data;

		std::cout << request.target() << std::endl;

		// Returns a bad request response
		const auto bad_request = [&request](boost::beast::string_view why) {
			boost::beast::http::response<boost::beast::http::string_body> response {
				boost::beast::http::status::bad_request, request.version()
			};

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, "text/html");
			response.keep_alive(request.keep_alive());
			response.body() = std::string(why);
			response.prepare_payload();
			return response;
		};

		// Returns a not found response
		const auto not_found = [&request](boost::beast::string_view target) {
			boost::beast::http::response<boost::beast::http::string_body> response {
				boost::beast::http::status::not_found, request.version()
			};

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, "text/html");
			response.keep_alive(request.keep_alive());
			response.body() = "The resource '" + std::string(target) + "' was not found.";
			response.prepare_payload();
			return response;
		};

		// Returns a server error response
		const auto server_error = [&request](boost::beast::string_view what) {
			boost::beast::http::response<boost::beast::http::string_body> response {
				boost::beast::http::status::internal_server_error, request.version()
			};

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, "text/html");
			response.keep_alive(request.keep_alive());
			response.body() = "An error occurred: '" + std::string(what) + "'";
			response.prepare_payload();
			return response;
		};

		// Make sure we can handle the method
		if (request.method() != boost::beast::http::verb::get && request.method() != boost::beast::http::verb::head && request.method() != boost::beast::http::verb::post) {
			return session.send(bad_request("Unknown HTTP-method"));
		}

		// Request path must be absolute and not contain "..".
		if (request.target().empty() || request.target()[0] != '/' || request.target().find("..") != boost::beast::string_view::npos) {
			return session.send(bad_request("Illegal request-target"));
		}

		// Router stuff
		{
			Response response;
			if (router.run(session, response)) {
				return response.send(session);
			}
		}

		// Build the path to the requested file
		std::string path = path_cat(Game::Config::Get(Game::CONFIG_STORAGE_PATH), request.target());
		if (path.back() == '/') {
			path.append("index.html");
		}

		// Attempt to open the file
		boost::beast::error_code error;
		boost::beast::http::file_body::value_type body;
		body.open(path.c_str(), boost::beast::file_mode::scan, error);

		// Handle the case where the file doesn't exist
		if (error == boost::beast::errc::no_such_file_or_directory) {
			return session.send(not_found(request.target()));
		}

		// Handle an unknown error
		if (error) {
			return session.send(server_error(error.message()));
		}

		// Cache the size since we need it after the move
		const auto size = body.size();

		if (request.method() == boost::beast::http::verb::head) {
			// Respond to HEAD request
			boost::beast::http::response<boost::beast::http::empty_body> response {
				boost::beast::http::status::ok, request.version()
			};

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, mime_type(path));
			response.content_length(size);
			response.keep_alive(request.keep_alive());

			return session.send(std::move(response));
		} else {
			// Respond to GET request
			boost::beast::http::response<boost::beast::http::file_body> response {
				std::piecewise_construct,
				std::make_tuple(std::move(body)),
				std::make_tuple(boost::beast::http::status::ok, request.version())
			};

			response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			response.set(boost::beast::http::field::content_type, mime_type(path));
			response.content_length(size);
			response.keep_alive(request.keep_alive());

			return session.send(std::move(response));
		}
	}

	// Session
	Session::Session(Server* server, boost::asio::ip::tcp::socket&& socket) :
		mServer(server), mStream(std::move(socket)) {}

	Session::~Session() {
		// Empty
	}

	void Session::start() {
		do_read();
	}

	void Session::do_read() {
		mRequest = {};
		mStream.expires_after(std::chrono::seconds(30));

		boost::beast::http::async_read(mStream, mBuffer, mRequest.data,
			boost::beast::bind_front_handler(&Session::handle_read, shared_from_this()));
	}

	void Session::do_close() {
		boost::beast::error_code error;
		mStream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, error);
	}

	void Session::handle_read(boost::beast::error_code error, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);
		if (error == boost::beast::http::error::end_of_stream) {
			do_close();
		} else if (error) {
			// Error
		} else {
			handle_request(*this, *mServer->get_router());
		}
	}

	void Session::handle_write(bool close, boost::beast::error_code error, std::size_t bytes_transferred) {
		boost::ignore_unused(bytes_transferred);
		if (error) {
			// Error
		} else if (close) {
			do_close();
		} else {
			mResponse = nullptr;
			do_read();
		}
	}
}
