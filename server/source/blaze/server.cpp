
// Include
#include "server.h"

#include <boost/bind.hpp>
#include <iostream>

// Blaze
namespace Blaze {
	// Certificate & Private key
	std::string_view certificate = R"(
-----BEGIN CERTIFICATE-----
MIICUjCCAbugAwIBAgIJAJy42RKx5s84MA0GCSqGSIb3DQEBBQUAMEIxCzAJBgNV
BAYTAkFVMQswCQYDVQQIDAJBVTEMMAoGA1UEBwwDQVVBMQswCQYDVQQKDAJBdTEL
MAkGA1UECwwCYXUwHhcNMTQwMTIwMDUxNjA4WhcNMTYxMDE2MDUxNjA4WjBCMQsw
CQYDVQQGEwJBVTELMAkGA1UECAwCQVUxDDAKBgNVBAcMA0FVQTELMAkGA1UECgwC
QXUxCzAJBgNVBAsMAmF1MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDGbvOt
W6OJfVxFEWB1Vv4nQSMmgpuE7U4qN4Syy5n1vVcVTXwr2jwFXp5rpliPpkA0eD8F
J++UP9LK2yA/EbF2Xa6iBSSRYwkTvFwqTI0edTq46NP//++Ttale1nWaBiPQZWwW
DA5YvfPhpXdCm+u5qxw8eTL+d4DeyB546HZwOQIDAQABo1AwTjAdBgNVHQ4EFgQU
HfdlE40DLCgzkJcVDYD3NOsl07kwHwYDVR0jBBgwFoAUHfdlE40DLCgzkJcVDYD3
NOsl07kwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQUFAAOBgQBJbj74tBY1BXd6
u7s+Jc8S4r7jxx3BG3ubwddqlRJrFIWeVu2raweLbgoVwmMpmq29S6Ey/VcYM0wF
q4ih0tIXQvcItFwcquQly4MBUetk4b532vKoxeNZMnvlozwxvTn/inQYrNq5igXX
spDCzuIGNFb7Z1ph04HS70LqOHtVww==
-----END CERTIFICATE-----
)";

	std::string_view privateKey = R"(
-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQDGbvOtW6OJfVxFEWB1Vv4nQSMmgpuE7U4qN4Syy5n1vVcVTXwr
2jwFXp5rpliPpkA0eD8FJ++UP9LK2yA/EbF2Xa6iBSSRYwkTvFwqTI0edTq46NP/
/++Ttale1nWaBiPQZWwWDA5YvfPhpXdCm+u5qxw8eTL+d4DeyB546HZwOQIDAQAB
AoGBAJpUksriuedmC4xgPnAkj5jCLW93JzOUSTXGZjuU5JJeh0s3L3r/yay3cWjy
QHDA8bCdUQ5WiBv6I5zIHmVPAoBXn/qEregW0whtrneSFZQIdakG02/dZs+nW3Fr
CHKBLMBLkySeUyWBJp4HTiigMILtFnTYvLXicVe+sZfljiwNAkEA9+YAZ42eqE0M
bU9J7j7225fxA9X093Bryvj1X66NrJSqjs0ME7z+Ma7L/EdtLPO0LINWMm17STic
SP+lZKPTSwJBAMzrHBw0LXQT7jFgy6cpEC4hrlUui+zszRHQSSCAmG7rHea3TngS
AuEb1EYZBFtKRhXd7wEoSobRLmQ2n+2vlAsCQH1IVxH+h51k+w/PT3zBg36tRlf6
7IeU4FU/Brspe14p8BylUiIzlH/FaEACVRGvxHHumkR4AiOaIZne4VaAWQsCQCcU
Bw5R3QEv89Ky1OOR7yX9AlP4RnLuTcVB5VAvdeJhMBiZiHtZY/ct6XNcvfny1h7B
bhzYZC4FokU2LZWUUDUCQCOdlEVsk42T36t837wE4HCpfw4Zdk1+ZgumkKXJmt+c
0X5ap5irVnD91Ol7/EgeQC0nBST5IQnQmcMRXnyj4Zo=
-----END RSA PRIVATE KEY-----
)";

	// Server
	Server::Server(boost::asio::io_context& io_service, uint16_t port) :
		mIoService(io_service),
		mAcceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		mContext(boost::asio::ssl::context::sslv3)
	{
		auto nativeHandle = mContext.native_handle();

		SSL_CTX_set_security_level(nativeHandle, 0);
		SSL_CTX_set_cipher_list(nativeHandle, "RC4-MD5:RC4-SHA");

		if (SSL_CTX_set_default_verify_paths(nativeHandle) != 1) {
			std::cout << "SSL_CTX: Could not set default verify paths" << std::endl;
		}

		mContext.set_verify_callback(&Server::verify_callback);
		mContext.use_certificate(boost::asio::const_buffer(certificate.data(), certificate.length()), boost::asio::ssl::context::pem);
		mContext.use_private_key(boost::asio::const_buffer(privateKey.data(), privateKey.length()), boost::asio::ssl::context::pem);

		if (!SSL_CTX_check_private_key(nativeHandle)) {
			std::cout << "SSL_CTX: Private key does not match the public certificate" << std::endl;
			abort();
		}

		Client* client = new Client(mIoService, mContext);
		mAcceptor.async_accept(client->get_socket(),
			boost::bind(&Server::handle_accept, this, client, boost::asio::placeholders::error));
	}

	Server::~Server() {

	}

	void Server::handle_accept(Client* client, const boost::system::error_code& error) {
		if (!error) {
			client->start();
			client = new Client(mIoService, mContext);
			mAcceptor.async_accept(client->get_socket(),
				boost::bind(&Server::handle_accept, this, client, boost::asio::placeholders::error));
		} else {
			std::cout << error.message() << std::endl;
			delete client;
		}
	}

	bool Server::verify_callback(bool preverified, boost::asio::ssl::verify_context& context) {
		char subject_name[256];

		X509* cert = X509_STORE_CTX_get_current_cert(context.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

		bool verified = boost::asio::ssl::rfc2818_verification("gosredirector.ea.com")(preverified, context);
		std::cout << "Verifying: " << subject_name << "\nVerified: " << verified << std::endl;

		return verified;
	}
}
