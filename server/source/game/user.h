
#ifndef _GAME_USER_HEADER
#define _GAME_USER_HEADER

// Include
#include "game.h"
#include "squad.h"

#include <cstdint>
#include <string>
#include <map>
#include <pugixml.hpp>

// Game
namespace Game {
	// Account
	struct Account {
		bool tutorialCompleted = false;
		bool grantAllAccess = false;
		bool grantOnlineAccess = false;

		uint32_t chainProgression = 0;
		uint32_t creatureRewards = 0;

		uint32_t currentGameId = 1;
		uint32_t currentPlaygroupId = 1;

		uint32_t defaultDeckPveId = 1;
		uint32_t defaultDeckPvpId = 1;

		uint32_t level = 1;
		uint32_t xp = 0;
		uint32_t dna = 0;
		uint32_t avatarId = 0;
		uint32_t id = 1;

		uint32_t newPlayerInventory = 0;
		uint32_t newPlayerProgress = 0;

		uint32_t cashoutBonusTime = 0;
		uint32_t starLevel = 0;

		uint32_t unlockCatalysts = 0;
		uint32_t unlockDiagonalCatalysts = 0;
		uint32_t unlockInventory = 0;
		uint32_t unlockFuelTanks = 0;
		uint32_t unlockPveDecks = 0;
		uint32_t unlockPvpDecks = 0;
		uint32_t unlockStats = 0;
		uint32_t unlockInventoryIdentify = 0;
		uint32_t unlockEditorFlairSlots = 0;

		uint32_t upsell = 0;

		uint32_t capLevel = 0;
		uint32_t capProgression = 0;

		void Read(const pugi::xml_node& node);
		void Write(pugi::xml_node& node) const;
	};

	// FeedItem
	struct FeedItem {
		std::string metadata;
		std::string name;

		uint64_t timestamp;

		uint32_t accountId;
		uint32_t id;
		uint32_t messageId;
	};

	// Feed
	class Feed {
		public:
			decltype(auto) begin() { return mItems.begin(); }
			decltype(auto) begin() const { return mItems.begin(); }
			decltype(auto) end() { return mItems.end(); }
			decltype(auto) end() const { return mItems.end(); }

			auto& data() { return mItems; }
			const auto& data() const { return mItems; }

			void Read(const pugi::xml_node& node);
			void Write(pugi::xml_node& node) const;

			void Add(FeedItem&& item);

		private:
			std::vector<FeedItem> mItems;
	};

	// User
	class User {
		public:
			User(const std::string& name, const std::string& email, const std::string& password);
			User(const std::string& email);
			~User();

			//
			auto& get_account() { return mAccount; }
			const auto& get_account() const { return mAccount; }

			auto& get_creatures() { return mCreatures; }
			const auto& get_creatures() const { return mCreatures; }

			auto& get_squads() { return mSquads; }
			const auto& get_squads() const { return mSquads; }

			auto& get_feed() { return mFeed; }
			const auto& get_feed() const { return mFeed; }

			const std::string& get_auth_token() const { return mAuthToken; }
			void set_auth_token(const std::string& authToken) { mAuthToken = authToken; }

			const GameInfoPtr& get_game_info() const { return mGameInfo; }
			void set_game_info(const GameInfoPtr& gameInfo) { mGameInfo = gameInfo; }

			auto get_id() const { return mAccount.id; }

			const auto& get_email() const { return mEmail; }
			const auto& get_password() const { return mPassword; }
			const auto& get_name() const { return mName; }

			bool UpdateState(uint32_t newState);

			// Creature
			Creature* GetCreatureById(uint32_t id);
			const Creature* GetCreatureById(uint32_t id) const;

			void UnlockCreature(uint32_t templateId);

			// Upgrades
			void UnlockUpgrade(uint32_t unlockId);

			// Auth
			void Logout();

			// Storage
			bool Load();
			bool Save();

		private:
			Account mAccount;

			Creatures mCreatures;
			Squads mSquads;
			Feed mFeed;

			std::string mEmail;
			std::string mPassword;
			std::string mName;
			std::string mAuthToken;

			GameInfoPtr mGameInfo;

			uint32_t mId = 0;
			uint32_t mState = 0;
	};

	using UserPtr = std::shared_ptr<User>;

	// UserManager
	class UserManager {
		public:
			static UserPtr GetUserByEmail(const std::string& email);
			static UserPtr CreateUserWithNameMailAndPassword(const std::string& name, const std::string& email, const std::string& password);
			static UserPtr GetUserByAuthToken(const std::string& authToken);

		private:
			static void RemoveUser(const std::string& email);

		private:
			static std::map<std::string, UserPtr> sUsersByEmail;

			friend class User;
	};
}

#endif
