
#ifndef _UDPTEST_HEADER
#define _UDPTEST_HEADER

// Include
#include <array>

#include <boost/asio.hpp>

// UDPTest
class UDPTest {
	public:
		UDPTest(boost::asio::io_context& io_service, uint16_t port);
		~UDPTest();

		void receive();

		void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
		void handle_send(std::shared_ptr<std::string> message, const boost::system::error_code& error, size_t bytes_transferred);

	private:
		boost::asio::io_context& mIoService;
		boost::asio::ip::udp::socket mSocket;
		boost::asio::ip::udp::endpoint mRemoteEndpoint;

		std::array<uint8_t, 10240> mData;
};

#endif
