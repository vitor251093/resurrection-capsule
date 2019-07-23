
#ifndef _BLAZE_COMPONENT_ASSOCIATION_HEADER
#define _BLAZE_COMPONENT_ASSOCIATION_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// AssociationComponent
	class AssociationComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			static void SendLists(Client* client);

			// Notifications
			static void NotifyUpdateListMembership(Client* client);

		private:
			static void GetLists(Client* client, Header header);
	};
}

#endif
