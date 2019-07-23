
#ifndef _BLAZE_COMPONENT_AUTH_HEADER
#define _BLAZE_COMPONENT_AUTH_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// AuthComponent
	class AuthComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			static void SendAuthToken(Client* client, const std::string& token);

			static void SendLogin(Client* client, Header header);
			static void SendLoginPersona(Client* client, Header header);
			static void SendFullLogin(Client* client, Header header);
			static void SendConsoleLogin(Client* client, Header header);

			static void SendTOSInfo(Client* client, Header header);
			static void SendTermsAndConditions(Client* client, Header header);
			static void SendPrivacyPolicy(Client* client, Header header);

		private:
			static void ListUserEntitlements(Client* client, Header header);
			static void GetAuthToken(Client* client, Header header);

			// Login
			static void Login(Client* client, Header header);
			static void SilentLogin(Client* client, Header header);
			static void LoginPersona(Client* client, Header header);
			static void Logout(Client* client, Header header);

			// Terms and such
			static void AcceptTOS(Client* client, Header header);
			static void GetTOSInfo(Client* client, Header header);
			static void GetTermsAndConditions(Client* client, Header header);
			static void GetPrivacyPolicy(Client* client, Header header);
	};
}

#endif
