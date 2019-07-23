
#ifndef _BLAZE_COMPONENT_USERSESSION_HEADER
#define _BLAZE_COMPONENT_USERSESSION_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// UserSessionComponent
	class UserSessionComponent {
		public:
			static void Parse(Client* client, const Header& header);

			static void NotifyUserSessionExtendedDataUpdate(Client* client, uint64_t userId);
			static void NotifyUserAdded(Client* client, uint64_t userId, const std::string& userName);
			static void NotifyUserUpdated(Client* client, uint64_t userId);

		private:
			static void UpdateNetworkInfo(Client* client, Header header);
			static void UpdateUserSessionClientData(Client* client, Header header);
	};
}

#endif
