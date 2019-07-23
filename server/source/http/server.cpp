
// Include
#include "server.h"

#include <boost/bind.hpp>
#include <boost/beast/core.hpp>
#include <iostream>

// HTTP
namespace HTTP {
	// Server
	Server::Server(boost::asio::io_context& io_service, uint16_t port) :
		mIoService(io_service),
		mAcceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
	{
		do_accept();
		set_router(nullptr);
	}

	Server::~Server() {

	}

	const std::shared_ptr<Router>& Server::get_router() const {
		return mRouter;
	}

	void Server::set_router(const std::shared_ptr<Router>& router) {
		if (router) {
			mRouter = router;
		} else {
			mRouter = std::make_shared<Router>();
		}
	}

	void Server::do_accept() {
		mAcceptor.async_accept(mIoService,
			boost::beast::bind_front_handler(&Server::handle_accept, this));
	}

	void Server::handle_accept(boost::beast::error_code error, boost::asio::ip::tcp::socket socket) {
		if (!error) {
			std::make_shared<Session>(this, std::move(socket))->start();
			do_accept();
		} else {
			std::cout << error.message() << std::endl;
		}
	}
}
