
#ifndef _GAME_GAME_HEADER
#define _GAME_GAME_HEADER

// Include
#include "../blaze/types.h"

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>

// RakNet
namespace RakNet { class Server; }

// Game
namespace Game {
	// IP
	struct IP {
		uint32_t address = 0;
		uint16_t port = 0;
	};

	// GameInfo
	struct GameInfo {
		std::map<std::string, std::string> attributes;

		std::vector<uint32_t> capacity;
		std::vector<uint32_t> slots { 0 };

		std::string name;
		std::string level;
		std::string type;

		uint32_t id = std::numeric_limits<uint32_t>::max();
		uint64_t settings = 0;

		int64_t clientId = 0;

		IP internalIP;
		IP externalIP;

		Blaze::PresenceMode presenceMode;
		Blaze::GameState state;
		Blaze::GameNetworkTopology networkTopology;
		Blaze::VoipTopology voipTopology;

		uint16_t maxPlayers = 0;
		uint16_t queueCapacity = 0;

		bool resetable = false;
	};

	using GameInfoPtr = std::shared_ptr<GameInfo>;

	// Rule
	struct Rule {
		std::string name;
		std::string thld;
		std::string value;
	};

	// Criteria
	struct Criteria {
		// Custom
		uint32_t ida;
		// NAT
		// PSR
		// RANK
		std::vector<Rule> rules;

	};

	// Matchmaking
	struct Matchmaking {
		GameInfoPtr gameInfo;
		std::string id;
	};

	// Manager
	class Manager {
		public:
			// Game
			static GameInfoPtr CreateGame();
			static void RemoveGame(uint32_t id);

			static GameInfoPtr GetGame(uint32_t id);
			static void StartGame(uint32_t id);

			// Matchmaking
			static Matchmaking& StartMatchmaking();

		private:
			static std::map<uint32_t, std::unique_ptr<RakNet::Server>> sActiveGames;

			static std::map<uint32_t, GameInfoPtr> sGames;
			static std::map<std::string, Matchmaking> sMatchmaking;
	};
}

#endif
