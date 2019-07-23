
// Include
#include "udptest.h"
#include <boost/bind.hpp>

#include <iostream>

// UDPTest
UDPTest::UDPTest(boost::asio::io_context& io_service, uint16_t port) :
	mIoService(io_service),
	mSocket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)),
	mData()
{
	receive();
}

UDPTest::~UDPTest() {
	// Empty
}

void UDPTest::receive() {
	mSocket.async_receive_from(
		boost::asio::buffer(mData.data(), mData.size()), mRemoteEndpoint,
		boost::bind(&UDPTest::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UDPTest::handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
	std::cout << "UDP connection" << std::endl;
	if (!error || error == boost::asio::error::message_size) {
		auto file = fopen("udp.txt", "wb");
		fwrite(mData.data(), 1, std::min<size_t>(bytes_transferred, mData.size()), file);
		fclose(file);

		auto message = std::make_shared<std::string>("testing 123");
		mSocket.async_send_to(boost::asio::buffer(*message), mRemoteEndpoint,
			boost::bind(&UDPTest::handle_send, this, message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

		receive();
	} else {
		std::cout << error.message() << std::endl;
	}
}

void UDPTest::handle_send(std::shared_ptr<std::string> message, const boost::system::error_code& error, size_t bytes_transferred) {
	// What
}
