
#ifndef _HTTP_SERVER_HEADER
#define _HTTP_SERVER_HEADER

// Include
#include "session.h"
#include "router.h"

// HTTP
namespace HTTP {
	// Server
	class Server {
		public:
			Server(boost::asio::io_context& io_service, uint16_t port);
			~Server();

			const std::shared_ptr<Router>& get_router() const;
			void set_router(const std::shared_ptr<Router>& router);

		private:
			void do_accept();
			void handle_accept(boost::beast::error_code error, boost::asio::ip::tcp::socket socket);

		private:
			boost::asio::io_context& mIoService;
			boost::asio::ip::tcp::acceptor mAcceptor;

			std::shared_ptr<Router> mRouter;
	};
}

#endif
