
#ifndef _BLAZE_COMPONENT_REDIRECTOR_HEADER
#define _BLAZE_COMPONENT_REDIRECTOR_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// RedirectorComponent
	class RedirectorComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			static void SendServerInstanceInfo(Client* client, const std::string& host, uint16_t port);
			static void SendServerAddressInfo(Client* client, const std::string& host, uint16_t port);

		private:
			static void ServerInstanceInfo(Client* client, Header header);
	};
}

#endif
