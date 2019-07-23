
// Include
#include "game.h"
#include "../raknet/server.h"

// Game
namespace Game {
	// Manager
	std::map<uint32_t, std::unique_ptr<RakNet::Server>> Manager::sActiveGames;

	std::map<uint32_t, GameInfoPtr> Manager::sGames;
	std::map<std::string, Matchmaking> Manager::sMatchmaking;

	GameInfoPtr Manager::CreateGame() {
		uint32_t id;
		if (sGames.empty()) {
			id = 1;
		} else {
			id = sGames.rbegin()->first + 1;
		}

		auto game = std::make_shared<GameInfo>();
		game->id = id;

		sGames[game->id] = game;
		return game;
	}

	void Manager::RemoveGame(uint32_t id) {
		auto it = sGames.find(id);
		if (it != sGames.end()) {
			sGames.erase(it);
		}
	}

	GameInfoPtr Manager::GetGame(uint32_t id) {
		auto it = sGames.find(id);
		return (it != sGames.end()) ? it->second : nullptr;
	}

	void Manager::StartGame(uint32_t id) {
		auto it = sActiveGames.find(id);
		if (it == sActiveGames.end()) {
			auto game = GetGame(id);
			if (game) {
				sActiveGames[id] = std::make_unique<RakNet::Server>(game->externalIP.port, id);
			}
		}
	}

	Matchmaking& Manager::StartMatchmaking() {
		return sMatchmaking["test"];
	}
}
