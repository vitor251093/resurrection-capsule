
#ifndef _BLAZE_COMPONENT_GAMEMANAGER_HEADER
#define _BLAZE_COMPONENT_GAMEMANAGER_HEADER

// Include
#include "../tdf.h"

// Blaze
namespace Blaze {
	class Client;

	// GameManagerComponent
	class GameManagerComponent {
		public:
			static void Parse(Client* client, const Header& header);

			// Responses
			static void SendCreateGame(Client* client, uint32_t gameId);
			static void SendJoinGame(Client* client, uint32_t gameId);
			static void SendStartMatchmaking(Client* client, uint32_t id);

			// Notifications
			static void NotifyGameCreated(Client* client, uint32_t gameId);
			static void NotifyGameRemoved(Client* client);
			static void NotifyGameSetup(Client* client);
			static void NotifyPlayerJoining(Client* client, uint32_t gameId);
			static void NotifyPlayerJoinCompleted(Client* client, uint32_t gameId, uint32_t personaId);
			static void NotifyPlayerRemoved(Client* client, uint32_t gameId, uint32_t personaId, PlayerRemovedReason reason);
			static void NotifyPlatformHostInitialized(Client* client, uint64_t gameId);
			static void NotifyGameStateChange(Client* client, uint64_t gameId, GameState gameState);
			static void NotifyGameReset(Client* client);
			static void NotifyGameSessionUpdated(Client* client, uint32_t gameId);
			static void NotifyGamePlayerStateChange(Client* client, uint32_t gameId, uint32_t personaId, PlayerState playerState);
			static void NotifyCreateDynamicDedicatedServerGame(Client* client);

		private:
			static void CreateGame(Client* client, Header header);
			static void JoinGame(Client* client, Header header);
			static void RemovePlayer(Client* client, Header header);
			static void StartMatchmaking(Client* client, Header header);
			static void CancelMatchmaking(Client* client, Header header);
			static void FinalizeGameCreation(Client* client, Header header);
			static void ResetDedicatedServer(Client* client, Header header);
			static void UpdateMeshConnection(Client* client, Header header);
	};
}

#endif
