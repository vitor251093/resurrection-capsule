
#ifndef _HTTP_SESSION_HEADER
#define _HTTP_SESSION_HEADER

// Include
#include "uri.h"

#include "../game/user.h"

#include <boost/asio.hpp>

// HTTP
namespace HTTP {
	class Server;

	boost::beast::string_view mime_type(boost::beast::string_view path);

	// Request
	struct Request {
		boost::beast::http::request<boost::beast::http::string_body> data;
		URI uri;
	};

	// Session
	class Session : public std::enable_shared_from_this<Session> {
		public:
			Session(Server* server, boost::asio::ip::tcp::socket&& socket);
			~Session();

			void start();

			auto& get_request() { return mRequest; }
			const auto& get_request() const { return mRequest; }

			const auto& get_user() const { return mUser; }
			void set_user(const Game::UserPtr& user) { mUser = user; }

			template<class Message>
			void send(Message&& message) {
				auto sp = std::make_shared<Message>(std::move(message));
				mResponse = sp;

				// Write the response
				boost::beast::http::async_write(mStream, *sp,
					boost::beast::bind_front_handler(&Session::handle_write, shared_from_this(), sp->need_eof()));
			}

		private:
			void do_read();
			void do_close();

			void handle_read(boost::beast::error_code error, std::size_t bytes_transferred);
			void handle_write(bool close, boost::beast::error_code error, std::size_t bytes_transferred);

		private:
			Server* mServer;

			boost::beast::tcp_stream mStream;
			boost::beast::flat_buffer mBuffer;

			Request mRequest;
			std::shared_ptr<void> mResponse;

			Game::UserPtr mUser;
	};
}

#endif
