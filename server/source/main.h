
#ifndef _MAIN_HEADER
#define _MAIN_HEADER

// Include
#include "blaze/server.h"
#include "http/server.h"
#include "game/api.h"
#include "udptest.h"

// Application
class Application {
	private:
		Application();

	public:
		static Application& InitApp(int argc, char* argv[]);
		static Application& GetApp();

		bool OnInit();
		int OnExit();

		void Run();

		boost::asio::io_context& get_io_service();

		Game::API* get_game_api() const;
		Blaze::Server* get_redirector_server() const;
		Blaze::Server* get_blaze_server() const;
		HTTP::Server* get_http_server() const;
		HTTP::Server* get_qos_server() const;

	private:
		static Application* sApplication;

		boost::asio::io_context mIoService;
		boost::asio::signal_set mSignals;

		std::unique_ptr<Game::API> mGameAPI;

		std::unique_ptr<Blaze::Server> mRedirectorServer;
		std::unique_ptr<Blaze::Server> mBlazeServer;
		std::unique_ptr<UDPTest> mGmsServer;
		std::unique_ptr<Blaze::Server> mPssServer;
		std::unique_ptr<Blaze::Server> mTickServer;

		std::unique_ptr<HTTP::Server> mHttpServer;
		std::unique_ptr<HTTP::Server> mQosServer;
};

static Application& GetApp() {
	return Application::GetApp();
}

#endif
