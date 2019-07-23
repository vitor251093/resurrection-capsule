
// Include
#include "tcptest.h"
#include <boost/bind.hpp>

#include <iostream>

// TCPClient
TCPClient::TCPClient(boost::asio::io_context& io_service) : mSocket(io_service) {
	//
}

void TCPClient::start() {
	mSocket.async_read_some(boost::asio::buffer(mReadBuffer.data(), mReadBuffer.size()),
		boost::bind(&TCPClient::handle_handshake, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void TCPClient::handle_handshake(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
		boost::asio::async_write(mSocket,
			boost::asio::buffer(mReadBuffer.data(), bytes_transferred),
			boost::bind(&TCPClient::handle_write, this, boost::asio::placeholders::error));
	} else {
		std::cout << error.message() << std::endl;
		delete this;
	}
}

void TCPClient::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
	if (!error) {
		boost::asio::async_write(mSocket,
			boost::asio::buffer(mWriteBuffer.data(), mWriteBuffer.size()),
			boost::bind(&TCPClient::handle_write, this, boost::asio::placeholders::error));
	} else {
		std::cout << error.message() << std::endl;
		delete this;
	}
}

void TCPClient::handle_write(const boost::system::error_code& error) {
	if (!error) {
		mSocket.async_read_some(boost::asio::buffer(mReadBuffer.data(), mReadBuffer.size()),
			boost::bind(&TCPClient::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	} else {
		std::cout << error.message() << std::endl;
		delete this;
	}
}

// TCPTest
TCPTest::TCPTest(boost::asio::io_context& io_service, uint16_t port) :
	mIoService(io_service),
	mAcceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
	TCPClient* client = new TCPClient(mIoService);
	mAcceptor.async_accept(client->get_socket(),
		boost::bind(&TCPTest::handle_accept, this, client, boost::asio::placeholders::error));
}

TCPTest::~TCPTest() {

}

void TCPTest::handle_accept(TCPClient* client, const boost::system::error_code& error) {
	if (!error) {
		client->start();
		client = new TCPClient(mIoService);
		mAcceptor.async_accept(client->get_socket(),
			boost::bind(&TCPTest::handle_accept, this, client, boost::asio::placeholders::error));
	} else {
		std::cout << error.message() << std::endl;
		delete client;
	}
}
