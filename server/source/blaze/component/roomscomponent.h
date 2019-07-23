
#ifndef _BLAZE_COMPONENT_ROOMS_HEADER
#define _BLAZE_COMPONENT_ROOMS_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// RoomsComponent
	class RoomsComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			static void SendSelectCategoryUpdates(Client* client, uint32_t viewId);

			// Notifications
			static void NotifyRoomViewUpdated(Client* client, uint32_t viewId);
			static void NotifyRoomViewAdded(Client* client, uint32_t viewId);
			static void NotifyRoomViewRemoved(Client* client, uint32_t viewId);
			static void NotifyRoomCategoryUpdated(Client* client);
			static void NotifyRoomCategoryAdded(Client* client);
			static void NotifyRoomCategoryRemoved(Client* client, uint32_t categoryId);
			static void NotifyRoomUpdated(Client* client);
			static void NotifyRoomAdded(Client* client);
			static void NotifyRoomRemoved(Client* client, uint32_t roomId);
			static void NotifyRoomPopulationUpdated(Client* client);
			static void NotifyRoomMemberJoined(Client* client);
			static void NotifyRoomMemberLeft(Client* client, uint32_t roomId, uint32_t memberId);
			static void NotifyRoomMemberUpdated(Client* client);
			static void NotifyRoomKick(Client* client, uint32_t roomId, uint32_t memberId);
			static void NotifyRoomHostTransfer(Client* client, uint32_t roomId, uint32_t memberId);
			static void NotifyRoomAttributesSet(Client* client, uint32_t roomId);

		private:
			static void SelectViewUpdates(Client* client, Header header);
			static void SelectCategoryUpdates(Client* client, Header header);
	};
}

#endif
