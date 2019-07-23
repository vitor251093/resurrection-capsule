
// Include
#include "client.h"

// Network
namespace Network {
	// Client
	Client::Client(boost::asio::io_context& io_service) :
		mIoService(io_service) {}

	DataBuffer& Client::get_read_buffer() {
		return mReadBuffer;
	}
}
