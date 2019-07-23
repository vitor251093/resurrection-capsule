
#ifndef _CLIENT_HEADER
#define _CLIENT_HEADER

// Include
#include "../databuffer.h"
#include <boost/asio.hpp>

// Network
namespace Network {
	class Packet;

	// Client
	class Client {
		public:
			Client(boost::asio::io_context& io_service);
			virtual ~Client() = default;

			virtual void start() = 0;

			DataBuffer& get_read_buffer();

		protected:
			boost::asio::io_context& mIoService;

			DataBuffer mReadBuffer;
			DataBuffer mWriteBuffer;
	};
}

#endif
