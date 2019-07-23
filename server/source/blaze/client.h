
#ifndef _BLAZE_CLIENT_HEADER
#define _BLAZE_CLIENT_HEADER

// Include
#include "../network/client.h"
#include "../game/user.h"
#include "types.h"
#include "tdf.h"
#include <boost/asio/ssl.hpp>

// Blaze
namespace Blaze {
	// Client
	class Client : public Network::Client {
		public:
			Client(boost::asio::io_context& io_service, boost::asio::ssl::context& context);

			auto& get_socket() { return mSocket.lowest_layer(); }

			auto get_id() const { return mId; }

			const auto& get_user() const { return mUser; }
			void set_user(const Game::UserPtr& user) { mUser = user; }

			const auto& get_request() const { return mRequest; }

			auto& type() { return mType; }
			const auto& type() const { return mType; }

			auto& localization() { return mLocalization; }
			const auto& localization() const { return mLocalization; }

			void start();

			void send(const Header& header, const DataBuffer* buffer);
			void notify(Header header, const DataBuffer& buffer);
			void reply(Header header);
			void reply(Header header, const DataBuffer& buffer);

			void handle_handshake(const boost::system::error_code& error);
			void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
			void handle_write(const boost::system::error_code& error);

		private:
			void parse_packets();

		protected:
			boost::asio::ssl::stream<boost::asio::ip::tcp::socket> mSocket;

			rapidjson::Document mRequest;

			std::vector<DataBuffer> mWriteBuffers;

			Game::UserPtr mUser;

			ClientType mType;
			uint32_t mLocalization;
			
			uint32_t mCurrentMessageId;
			uint32_t mId;
	};
}

#endif
