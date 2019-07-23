
#ifndef _BLAZE_COMPONENT_PLAYGROUPS_HEADER
#define _BLAZE_COMPONENT_PLAYGROUPS_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// PlaygroupsComponent
	class PlaygroupsComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			// static void SendSelectCategoryUpdates(Client* client, uint32_t viewId);

			// Notifications
			static void NotifyDestroyPlaygroup(Client* client, uint32_t playgroupId, uint32_t reason);
			static void NotifyJoinPlaygroup(Client* client);
			static void NotifyMemberJoinedPlaygroup(Client* client, uint32_t playgroupId);
			static void NotifyMemberRemovedFromPlaygroup(Client* client, uint32_t playgroupId, uint32_t reason);
			static void NotifyPlaygroupAttributesSet(Client* client, uint32_t playgroupId);
			static void NotifyMemberAttributesSet(Client* client, uint32_t playgroupId);
			static void NotifyLeaderChange(Client* client, uint32_t playgroupId);
			static void NotifyMemberPermissionsChange(Client* client, uint32_t playgroupId);
			static void NotifyJoinControlsChange(Client* client, uint32_t playgroupId);
			static void NotifyXboxSessionInfo(Client* client, uint32_t playgroupId);
			static void NotifyXboxSessionChange(Client* client, uint32_t playgroupId);

		private:
			static void CreatePlaygroup(Client* client, Header header);
			static void DestroyPlaygroup(Client* client, Header header);
			static void JoinPlaygroup(Client* client, Header header);
			static void LeavePlaygroup(Client* client, Header header);
			static void SetPlaygroupAttributes(Client* client, Header header);
			static void SetMemberAttributes(Client* client, Header header);
			static void KickPlaygroupMember(Client* client, Header header);
			static void SetPlaygroupJoinControls(Client* client, Header header);
			static void FinalizePlaygroupCreation(Client* client, Header header);
			static void LookupPlaygroupInfo(Client* client, Header header);
			static void ResetPlaygroupSession(Client* client, Header header);
	};
}

#endif
