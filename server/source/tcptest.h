
#ifndef _TCPTEST_HEADER
#define _TCPTEST_HEADER

// Include
#include <array>

#include <boost/asio.hpp>

// TCPClient
class TCPClient {
	public:
		TCPClient(boost::asio::io_context& io_service);

		auto& get_socket() { return mSocket.lowest_layer(); }

		void start();

		void handle_handshake(const boost::system::error_code& error, size_t bytes_transferred);
		void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
		void handle_write(const boost::system::error_code& error);

	private:
		boost::asio::ip::tcp::socket mSocket;

		std::array<uint8_t, 1024> mReadBuffer;
		std::array<uint8_t, 1024> mWriteBuffer;
};

// TCPTest
class TCPTest {
	public:
		TCPTest(boost::asio::io_context& io_service, uint16_t port);
		~TCPTest();

		void handle_accept(TCPClient* client, const boost::system::error_code& error);

	private:
		boost::asio::io_context& mIoService;
		boost::asio::ip::tcp::acceptor mAcceptor;
};

#endif
