
// Include
#include "gamemanagercomponent.h"
#include "usersessioncomponent.h"

#include "../client.h"
#include "../../game/game.h"
#include "../../utils/functions.h"

#include <iostream>

#ifdef GetObject
#	undef GetObject
#endif

/*
	STAT
		0 = Boot
		1 = Login
		2 = Moving towards editor (Spaceship any?)
		3 = Squad/Hero screen
		4 = Navigation/Spaceship
		5 = Predungeon
		6 = Unknown (cStateMachine in exe)
		7 = Editor (cStateMachine in exe)
		8 = Cinematic
		9 = Spectator
		10 = Replay
		11 = Chain voting
		12 = Chain cashout
		13 = Game over
		15 = Arena lobby
		16 = Arena round results
		17 = Juggernaut lobby
		18 = Juggernaut results
		19 = Killrace lobby
		20 = Killrace results


	Gamemodes
		Tutorial = 1
		Chain(Matched/Premade) = 2
		Arena = 3
		KillRace = 4
		Juggernaut = 5
		Quickplay = 6
		DirectEntry = 7

	Packet IDs
		0x01 = CreateGame
		0x02 = DestroyGame
		0x03 = AdvanceGameState
		0x04 = SetGameSettings
		0x05 = SetPlayerCapacity
		0x07 = SetGameAttributes
		0x08 = SetPlayerAttributes
		0x09 = JoinGame
		0x0B = RemovePlayer
		0x0D = StartMatchmaking
		0x0E = CancelMatchmaking
		0x0F = FinalizeGameCreation
		0x11 = ListGames
		0x12 = SetPlayerCustomData
		0x13 = ReplayGame
		0x14 = ReturnDedicatedServerToPool
		0x15 = JoinGameByGroup
		0x16 = LeaveGameByGroup
		0x17 = MigrateGame
		0x18 = UpdateGameHostMigrationStatus
		0x19 = ResetDedicatedServer
		0x1A = UpdateGameSession
		0x1B = BanPlayer
		0x1D = UpdateMeshConnection
		0x1F = RemovePlayerFromBannedList
		0x20 = ClearBannedList
		0x21 = GetBannedList
		0x26 = AddQueuedPlayerToGame
		0x27 = UpdateGameName
		0x64 = GetGameListSnapshot
		0x65 = GetGameListSubscription
		0x66 = DestroyGameList
		0x67 = GetFullGameData
		0x68 = GetMatchmakingConfig
		0x69 = GetGameDataFromId
		0x6A = AddAdminPlayer
		0x6B = RemoveAdminPlayer
		0x6C = SetPlayerTeam
		0x6D = ChangePlayerTeam
		0x6E = MigrateAdminPlayer
		0x6F = GetUserSetGameListSubscription
		0x96 = RegisterDynamicDedicatedServerCreator
		0x97 = UnregisterDynamicDedicatedServerCreator

	Notification Packet IDs
		0x0A = NotifyMatchmakingFailed
		0x0C = NotifyMatchmakingAsyncStatus
		0x0F = NotifyGameCreated
		0x10 = NotifyGameRemoved
		0x14 = NotifyGameSetup
		0x15 = NotifyPlayerJoining
		0x1E = NotifyPlayerJoinCompleted
		0x28 = NotifyPlayerRemoved
		0x3C = NotifyHostMigrationCompleted
		0x46 = NotifyHostMigrationStart
		0x47 = NotifyPlatformHostInitialized
		0x50 = NotifyGameAttribChange
		0x5A = NotifyPlayerAttribChange
		0x5F = NotifyPlayerCustomDataChange
		0x64 = NotifyGameStateChange
		0x6E = NotifyGameSettingsChange
		0x6F = NotifyGameCapacityChange
		0x70 = NotifyGameReset
		0x71 = NotifyGameReportingIdChange
		0x73 = NotifyGameSessionUpdated
		0x74 = NotifyGamePlayerStateChange
		0x75 = NotifyGamePlayerTeamChange
		0x76 = NotifyGameTeamIdChange
		0x77 = NotifyProcessQueue
		0xC9 = NotifyGameListUpdate
		0xCA = NotifyAdminListChange
		0xDC = NotifyCreateDynamicDedicatedServerGame
		0xE6 = NotifyGameNameChange

	Blaze Fields
		PDAT
			BLOB = 0x20
			EXID = 0x30
			GID = 0x38
			LOC = 0x38
			NAME = 0x24
			PATT = 0x54
			PID = 0x34
			PNET = 0x14
			SID = 0x48
			SLOT = 0x1C
			STAT = 0x1C
			TIDX = 0x40
			TIME = 0x34
			UGID = 0x08
			UID = 0x38

		ReplicatedPlayerData
			BLOB = 0x20
			EXID = 0x30
			GID = 0x38
			LOC = 0x38
			NAME = 0x24
			PATT = 0x54
			PID = 0x34
			PNET = 0x14
			SID = 0x48
			SLOT = 0x1C
			STAT = 0x1C
			TIDX = 0x40
			TIME = 0x34
			UGID = 0x08
			UID = 0x38

		ReplicatedGameData
			ADMN = 0x58
			ATTR = 0x54
			CAP = 0x58
			CRIT = 0x54
			GID = 0x38
			GNAM = 0x24
			GPVH = 0x30
			GSET = 0x28
			GSID = 0x30
			GSTA = 0x1C
			GTYP = 0x24
			HNET = 0x58
			HSES = 0x38
			IGNO = 0x50
			MATR = 0x54
			MCAP = 0x40
			NQOS = 0x18
			NRES = 0x50
			NTOP = 0x1C
			PGID = 0x24
			PGSR = 0x20
			PHST = 0x18
			PRES = 0x1C
			PSAS = 0x24
			QCAP = 0x40
			SEED = 0x38
			TCAP = 0x40
			THST = 0x18
			TIDS = 0x58
			UUID = 0x24
			VOIP = 0x1C
			VSTR = 0x24
			XNNC = 0x20
			XSES = 0x20

		IndirectJoinGameSetupContext
			GRID = 0x08
			RPVC = 0x50

		ResetDedicatedServerSetupContext
			ERR = 0x38

		GameBrowserGameData
			ADMN = 0x58
			ATTR = 0x54
			CAP = 0x58
			CRIT = 0x54
			GID = 0x38
			GNAM = 0x24
			GSET = 0x28
			GSTA = 0x1C
			HNET = 0x58
			HOST = 0x34
			PCNT = 0x58
			PRES = 0x1C
			PSID = 0x24
			QCAP = 0x40
			QCNT = 0x40
			ROST = 0x58
			SID = 0x30
			TCAP = 0x40
			TINF = 0x58
			VOIP = 0x1C
			VSTR = 0x24

	Request Packets
		CreateGame
			ADMN = 0x58
			ATTR = 0x54
			BTPL = 0x08
			CRIT = 0x54
			GCTR = 0x24
			GNAM = 0x24
			GSET = 0x28
			GTYP = 0x24
			GURL = 0x24
			HNET = 0x58
			IGNO = 0x50
			MATR = 0x54
			NRES = 0x50
			NTOP = 0x1C
			PATT = 0x54
			PCAP = 0x58
			PGID = 0x24
			PGSC = 0x20
			PMAX = 0x40
			PRES = 0x1C
			QCAP = 0x40
			RGID = 0x38
			SEAT = 0x58
			SIDL = 0x58
			SLOT = 0x1C
			TCAP = 0x40
			TIDS = 0x58
			TIDX = 0x40
			VOIP = 0x1C
			VSTR = 0x24

		JoinGame
			ATTR = 0x54
			BTPL = 0x08
			GID = 0x38
			GVER = 0x24
			JMET = 0x1C
			PNET = 0x14
			SLID = 0x48
			SLOT = 0x1C
			TIDX = 0x40
			USER = 0x18

		ResetDedicatedServer
			same as CreateGame?

	Response Packets
		CreateGame
			

		DestroyGame
			GID = 0x38

		JoinGame
			GID = 0x38

		StartMatchmaking
			MSID = 0x38

	Notify Packets
		NotifyGameCreated
			GID = 0x38

		NotifyGameRemoved


		NotifyGameSetup
			GAME = 0x18
			PROS = 0x58
			REAS = 0x14

		NotifyHostMigrationCompleted
			GID = 0x38

		NotifyCreateDynamicDedicatedServerGame
			GREQ = 0x18
			MID = 0x24

		NotifyPlatformHostInitialized
			GID = 0x38
			PHST = 0x48

		NotifyGamePlayerStateChange
			GID = 0x38
			PID = 0x34
			STAT = 0x1C

		NotifyPlatformHostInitialized
			GID = 0x38
			PHST = 0x48
*/

// Blaze
namespace Blaze {
	// GameManagerComponent
	void GameManagerComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x01:
				CreateGame(client, header);
				break;

			case 0x09:
				JoinGame(client, header);
				break;
			
			case 0x0B:
				RemovePlayer(client, header);
				break;

			case 0x0D:
				StartMatchmaking(client, header);
				break;

			case 0x0E:
				CancelMatchmaking(client, header);
				break;

			case 0x0F:
				FinalizeGameCreation(client, header);
				break;

			case 0x19:
				ResetDedicatedServer(client, header);
				break;

			case 0x1D:
				UpdateMeshConnection(client, header);
				break;

			default:
				std::cout << "Unknown gamemanager command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void GameManagerComponent::SendCreateGame(Client* client, uint32_t gameId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x01;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
	}

	void GameManagerComponent::SendJoinGame(Client* client, uint32_t gameId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x09;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
	}

	void GameManagerComponent::SendStartMatchmaking(Client* client, uint32_t id) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MSID", id);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x0D;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGameCreated(Client* client, uint32_t gameId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x0F;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGameRemoved(Client* client) {

	}

	void GameManagerComponent::NotifyGameSetup(Client* client) {
		/*
			HNET internals

			0x1C
			0x18
			0x18
			0x10
		*/
		auto& request = client->get_request();

		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		const auto& gameInfo = user->get_game_info();
		if (!gameInfo) {
			return;
		}

		bool gameIgnore = request["IGNO"].GetUint();
		std::string playgroupId = request["PGID"].GetString();
		uint32_t tCap = request["TCAP"].GetUint();
		std::string versionString = request["VSTR"].GetString();

		TDF::Packet packet;
		{
			auto& gameStruct = packet.CreateStruct(nullptr, "GAME");
			{
				auto& admnList = packet.CreateList(&gameStruct, "ADMN", TDF::Type::Integer);
				packet.PutInteger(&admnList, "", 1);
			} {
				auto& attrMap = packet.CreateMap(&gameStruct, "ATTR", TDF::Type::String, TDF::Type::String);
				for (const auto& [key, value] : gameInfo->attributes) {
					packet.PutString(&attrMap, key, value);
				}
			} {
				auto& capList = packet.CreateList(&gameStruct, "CAP", TDF::Type::Integer);
				for (auto value : gameInfo->capacity) {
					packet.PutInteger(&capList, "", value);
				}
			}
			packet.PutInteger(&gameStruct, "GID", gameInfo->id);
			packet.PutString(&gameStruct, "GNAM", gameInfo->name);
			packet.PutInteger(&gameStruct, "GPVH", 1);
			packet.PutInteger(&gameStruct, "GSET", gameInfo->settings);
			packet.PutInteger(&gameStruct, "GSID", 1);
			packet.PutInteger(&gameStruct, "GSTA", gameInfo->state);
			packet.PutString(&gameStruct, "GTYP", gameInfo->type);
			{
				auto& hnetList = packet.CreateList(&gameStruct, "HNET", TDF::Type::Struct, true);
				{
#if 1
					auto& valuStruct = packet.CreateStruct(&hnetList, "");
					{
						auto& exipStruct = packet.CreateStruct(&valuStruct, "EXIP");
						packet.PutInteger(&exipStruct, "IP", gameInfo->externalIP.address);
						packet.PutInteger(&exipStruct, "PORT", gameInfo->externalIP.port);
					} {
						auto& inipStruct = packet.CreateStruct(&valuStruct, "INIP");
						packet.PutInteger(&inipStruct, "IP", gameInfo->internalIP.address);
						packet.PutInteger(&inipStruct, "PORT", gameInfo->internalIP.port);
					}
#else
					auto& hnetMemberUnion = packet.CreateUnion(&hnetList, "", NetworkAddressMember::IpPairAddress);
					auto& valuStruct = packet.CreateStruct(&hnetMemberUnion, "VALU");
					{
						auto& exipStruct = packet.CreateStruct(&valuStruct, "EXIP");
						packet.PutInteger(&exipStruct, "IP", gameInfo->externalIP.address);
						packet.PutInteger(&exipStruct, "PORT", gameInfo->externalIP.port);
					} {
						auto& inipStruct = packet.CreateStruct(&valuStruct, "INIP");
						packet.PutInteger(&inipStruct, "IP", gameInfo->internalIP.address);
						packet.PutInteger(&inipStruct, "PORT", gameInfo->internalIP.port);
					}
#endif
				}
			}
			packet.PutInteger(&gameStruct, "HSES", 13666);
			packet.PutInteger(&gameStruct, "IGNO", gameIgnore);
			packet.PutInteger(&gameStruct, "MCAP", 1);
			{
				auto& nqosStruct = packet.CreateStruct(&gameStruct, "NQOS");
				packet.PutInteger(&nqosStruct, "DBPS", 100);
				packet.PutInteger(&nqosStruct, "NATT", NatType::Open);
				packet.PutInteger(&nqosStruct, "UBPS", 100);
			}
			packet.PutInteger(&gameStruct, "NRES", gameInfo->resetable ? 0 : 1);
			packet.PutInteger(&gameStruct, "NTOP", gameInfo->networkTopology);
			packet.PutString(&gameStruct, "PGID", playgroupId); // b6852db1-ba37-4b40-aea3-0bd16efba4f9
			packet.PutBlob(&gameStruct, "PGSR", nullptr, 0);
			{
				auto& phstStruct = packet.CreateStruct(&gameStruct, "PHST");
				packet.PutInteger(&phstStruct, "HPID", 1);
				packet.PutInteger(&phstStruct, "HSLT", 1);
			}
			packet.PutInteger(&gameStruct, "PRES", gameInfo->presenceMode);
			packet.PutString(&gameStruct, "PSAS", "ams");
			packet.PutInteger(&gameStruct, "QCAP", gameInfo->queueCapacity);
			packet.PutInteger(&gameStruct, "SEED", 0);
			packet.PutInteger(&gameStruct, "TCAP", tCap);
			{
				auto& thstStruct = packet.CreateStruct(&gameStruct, "THST");
				packet.PutInteger(&thstStruct, "HPID", gameInfo->id);
				packet.PutInteger(&thstStruct, "HSLT", 0);
			} {
				auto& tidsList = packet.CreateList(&gameStruct, "TIDS", TDF::Type::Integer);
				packet.PutInteger(&tidsList, "", 0);
			}
			packet.PutString(&gameStruct, "UUID", "71bc4bdb-82ec-494d-8d75-ca5123b827ac"); // 714b05dc-93bc-49ac-961c-cb38b574f30a
			packet.PutInteger(&gameStruct, "VOIP", gameInfo->voipTopology);
			packet.PutString(&gameStruct, "VSTR", versionString);
			packet.PutBlob(&gameStruct, "XNNC", nullptr, 0);
			packet.PutBlob(&gameStruct, "XSES", nullptr, 0);
		} {
			auto& prosList = packet.CreateList(nullptr, "PROS", TDF::Type::Struct);
			{
				auto& playerStruct = packet.CreateStruct(&prosList, "");
				packet.PutBlob(&playerStruct, "BLOB", nullptr, 0);
				packet.PutInteger(&playerStruct, "EXID", 0);
				packet.PutInteger(&playerStruct, "GID", gameInfo->id);
				packet.PutInteger(&playerStruct, "LOC", client->localization());
				packet.PutString(&playerStruct, "NAME", user->get_name());
				{
					// PATT
				}
				packet.PutInteger(&playerStruct, "PID", user->get_id());
				{
					auto& pnetUnion = packet.CreateUnion(&playerStruct, "PNET", NetworkAddressMember::IpPairAddress);
					auto& valuStruct = packet.CreateStruct(&pnetUnion, "VALU");
					{
						auto& exipStruct = packet.CreateStruct(&valuStruct, "INIP");
						packet.PutInteger(&exipStruct, "IP", gameInfo->internalIP.address);
						packet.PutInteger(&exipStruct, "PORT", gameInfo->internalIP.port);
					} {
						auto& inipStruct = packet.CreateStruct(&valuStruct, "EXIP");
						packet.PutInteger(&inipStruct, "IP", gameInfo->externalIP.address);
						packet.PutInteger(&inipStruct, "PORT", gameInfo->externalIP.port);
					}
				}
				packet.PutInteger(&playerStruct, "SID", 0);
				packet.PutInteger(&playerStruct, "SLOT", 0);
				packet.PutInteger(&playerStruct, "STAT", PlayerState::Connected);
				packet.PutInteger(&playerStruct, "TIDX", 0xFFFF);
				packet.PutInteger(&playerStruct, "TIME", utils::get_unix_time());
				packet.PutVector3(&playerStruct, "UGID", 0, 0, 0);
				packet.PutInteger(&playerStruct, "UID", client->get_id());
			}
		} {
			auto& reasUnion = packet.CreateUnion(nullptr, "REAS", NetworkAddressMember::XboxClientAddress);
			auto& valuStruct = packet.CreateStruct(&reasUnion, "VALU");
			packet.PutInteger(&valuStruct, "DCTX", DatalessSetup::CreateGame);
			// packet.PutInteger(&valuStruct, "ERR", 0);
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x14;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyPlayerJoining(Client* client, uint32_t gameId) {
		auto& request = client->get_request();

		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		const auto& gameInfo = Game::Manager::GetGame(gameId);
		if (!gameInfo) {
			return;
		}

		user->set_game_info(gameInfo);

		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameInfo->id);
		{
			auto& pdatStruct = packet.CreateStruct(nullptr, "PDAT");
			packet.PutBlob(&pdatStruct, "BLOB", nullptr, 0);
			packet.PutInteger(&pdatStruct, "EXID", 0);
			packet.PutInteger(&pdatStruct, "LOC", client->localization());
			packet.PutString(&pdatStruct, "NAME", user->get_name());
			{
				auto& pattMap = packet.CreateMap(&pdatStruct, "PATT", TDF::Type::String, TDF::Type::String);
				packet.PutString(&pattMap, "Premium", "True");
			}
			packet.PutInteger(&pdatStruct, "PID", user->get_id());
			{
				auto& pnetUnion = packet.CreateUnion(nullptr, "PNET", NetworkAddressMember::IpPairAddress);
				auto& valuStruct = packet.CreateStruct(&pnetUnion, "VALU");
				{
					auto& inipStruct = packet.CreateStruct(&valuStruct, "INIP");
					packet.PutInteger(&inipStruct, "IP", gameInfo->internalIP.address);
					packet.PutInteger(&inipStruct, "PORT", gameInfo->internalIP.port);
				} {
					auto& exipStruct = packet.CreateStruct(&valuStruct, "EXIP");
					packet.PutInteger(&exipStruct, "IP", gameInfo->externalIP.address);
					packet.PutInteger(&exipStruct, "PORT", gameInfo->externalIP.port);
				}
			}
			packet.PutInteger(&pdatStruct, "SID", 0);
			packet.PutInteger(&pdatStruct, "SLOT", 0);
			packet.PutInteger(&pdatStruct, "STAT", PlayerState::Connected);
			packet.PutInteger(&pdatStruct, "TIDX", 0xFFFF);
			packet.PutInteger(&pdatStruct, "TIME", utils::get_unix_time());
			packet.PutVector3(&pdatStruct, "UGID", 0, 0, 0);
			packet.PutInteger(&pdatStruct, "UID", client->get_id());
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x15;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyPlayerJoinCompleted(Client* client, uint32_t gameId, uint32_t personaId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutInteger(nullptr, "PID", personaId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x1E;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyPlayerRemoved(Client* client, uint32_t gameId, uint32_t personaId, PlayerRemovedReason reason) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "CNTX", 0);
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutInteger(nullptr, "PID", personaId);
		packet.PutInteger(nullptr, "REAS", reason);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x28;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyPlatformHostInitialized(Client* client, uint64_t gameId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutInteger(nullptr, "PHST", 1);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x47;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGameStateChange(Client* client, uint64_t gameId, GameState gameState) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutInteger(nullptr, "GSTA", gameState);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x64;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGameReset(Client* client) {
		auto& request = client->get_request();

		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		const auto& gameInfo = user->get_game_info();
		if (!gameInfo) {
			return;
		}

		bool gameIgnore = request["IGNO"].GetUint();
		uint32_t tCap = request["TCAP"].GetUint();
		std::string versionString = request["VSTR"].GetString();

		TDF::Packet packet;
		{
			auto& dataStruct = packet.CreateStruct(nullptr, "DATA");
			{
				auto& admnList = packet.CreateList(&dataStruct, "ADMN", TDF::Type::Integer);
				packet.PutInteger(&admnList, "", 1);
			} {
				auto& attrMap = packet.CreateMap(&dataStruct, "ATTR", TDF::Type::String, TDF::Type::String);
				for (const auto& [key, value] : gameInfo->attributes) {
					packet.PutString(&attrMap, key, value);
				}
			} {
				auto& capList = packet.CreateList(&dataStruct, "CAP", TDF::Type::Integer);
				for (auto value : gameInfo->capacity) {
					packet.PutInteger(&capList, "", value);
				}
			}
			packet.PutInteger(&dataStruct, "GID", gameInfo->id);
			packet.PutString(&dataStruct, "GNAM", gameInfo->name);
			packet.PutInteger(&dataStruct, "GPVH", 1);
			packet.PutInteger(&dataStruct, "GSET", gameInfo->settings);
			packet.PutInteger(&dataStruct, "GSID", 1);
			packet.PutInteger(&dataStruct, "GSTA", gameInfo->state);
			packet.PutString(&dataStruct, "GTYP", gameInfo->type);
			{
				auto& hnetList = packet.CreateList(&dataStruct, "HNET", TDF::Type::Struct, true);
				{
					auto& inipStruct = packet.CreateStruct(&hnetList, "");
					packet.PutInteger(&inipStruct, "IP", gameInfo->internalIP.address);
					packet.PutInteger(&inipStruct, "PORT", gameInfo->internalIP.port);
				} {
					auto& exipStruct = packet.CreateStruct(&hnetList, "");
					packet.PutInteger(&exipStruct, "IP", gameInfo->externalIP.address);
					packet.PutInteger(&exipStruct, "PORT", gameInfo->externalIP.port);
				}
			}
			packet.PutInteger(&dataStruct, "HSES", 0);
			packet.PutInteger(&dataStruct, "IGNO", gameIgnore);
			packet.PutInteger(&dataStruct, "MCAP", 1);
			{
				auto& nqosStruct = packet.CreateStruct(&dataStruct, "NQOS");
				packet.PutInteger(&nqosStruct, "DBPS", 100);
				packet.PutInteger(&nqosStruct, "NATT", NatType::Open);
				packet.PutInteger(&nqosStruct, "UBPS", 100);
			}
			packet.PutInteger(&dataStruct, "NRES", gameInfo->resetable ? 0 : 1);
			packet.PutInteger(&dataStruct, "NTOP", gameInfo->networkTopology);
			packet.PutString(&dataStruct, "PGID", "d4c9d072-24d7-4142-b648-b43c8fe97b4d"); // b6852db1-ba37-4b40-aea3-0bd16efba4f9
			packet.PutBlob(&dataStruct, "PGSR", nullptr, 0);
			{
				auto& phstStruct = packet.CreateStruct(&dataStruct, "PHST");
				packet.PutInteger(&phstStruct, "HPID", 1);
				packet.PutInteger(&phstStruct, "HSLT", 1);
			}
			packet.PutInteger(&dataStruct, "PRES", gameInfo->presenceMode);
			packet.PutString(&dataStruct, "PSAS", "ams");
			packet.PutInteger(&dataStruct, "QCAP", gameInfo->queueCapacity);
			packet.PutInteger(&dataStruct, "SEED", 1);
			packet.PutInteger(&dataStruct, "TCAP", tCap);
			{
				auto& thstStruct = packet.CreateStruct(&dataStruct, "THST");
				packet.PutInteger(&thstStruct, "HPID", 1);
				packet.PutInteger(&thstStruct, "HSLT", 0);
			} {
				auto& tidsList = packet.CreateList(&dataStruct, "TIDS", TDF::Type::Integer);
				packet.PutInteger(&tidsList, "", 1);
			}
			packet.PutString(&dataStruct, "UUID", "71bc4bdb-82ec-494d-8d75-ca5123b827ac"); // 714b05dc-93bc-49ac-961c-cb38b574f30a
			packet.PutInteger(&dataStruct, "VOIP", gameInfo->voipTopology);
			packet.PutString(&dataStruct, "VSTR", versionString);
			packet.PutBlob(&dataStruct, "XNNC", nullptr, 0);
			packet.PutBlob(&dataStruct, "XSES", nullptr, 0);
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x70;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGameSessionUpdated(Client* client, uint32_t gameId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutBlob(nullptr, "XNNC", nullptr, 0);
		packet.PutBlob(nullptr, "XSES", nullptr, 0);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x73;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyGamePlayerStateChange(Client* client, uint32_t gameId, uint32_t personaId, PlayerState playerState) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "GID", gameId);
		packet.PutInteger(nullptr, "PID", personaId);
		packet.PutInteger(nullptr, "STAT", playerState);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0x74;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::NotifyCreateDynamicDedicatedServerGame(Client* client) {
		// Sending this should make the client send back a CreateGameRequest or ResetDedicatedServer

		auto& request = client->get_request();

		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		const auto& gameInfo = user->get_game_info();
		if (!gameInfo) {
			return;
		}

		std::string gameCtr = request["GCTR"].GetString();
		std::string gameUrl = request["GURL"].GetString();
		bool gameIgnore = request["IGNO"].GetUint();

		std::string playgroupId = request["PGID"].GetString();

		uint32_t rgId = request["RGID"].GetUint();
		uint32_t slot = request["SLOT"].GetUint();
		uint32_t tidx = request["TIDX"].GetUint();
		uint32_t tCap = request["TCAP"].GetUint();
		std::string versionString = request["VSTR"].GetString();

		TDF::Packet packet;
		{
			/*
			PATT = 0x54
			*/
			auto& greqStruct = packet.CreateStruct(nullptr, "GREQ");
			{
				auto& admnList = packet.CreateList(&greqStruct, "ADMN", TDF::Type::Integer);
				packet.PutInteger(&admnList, "", 1);
			} {
				auto& attrMap = packet.CreateMap(&greqStruct, "ATTR", TDF::Type::String, TDF::Type::String);
				for (const auto& [key, value] : gameInfo->attributes) {
					packet.PutString(&attrMap, key, value);
				}
			}
			packet.PutVector3(&greqStruct, "BTPL", 1, 1, 1);
			packet.PutString(&greqStruct, "GCTR", gameCtr);
			packet.PutString(&greqStruct, "GNAM", gameInfo->name);
			packet.PutInteger(&greqStruct, "GSET", gameInfo->settings);
			packet.PutString(&greqStruct, "GTYP", gameInfo->type);
			packet.PutString(&greqStruct, "GURL", gameUrl);
			{
				auto& hnetList = packet.CreateList(&greqStruct, "HNET", TDF::Type::Struct, true);
				{
					auto& exipStruct = packet.CreateStruct(&hnetList, "");
					packet.PutInteger(&exipStruct, "IP", gameInfo->externalIP.address);
					packet.PutInteger(&exipStruct, "PORT", gameInfo->externalIP.port);
				} {
					auto& inipStruct = packet.CreateStruct(&hnetList, "");
					packet.PutInteger(&inipStruct, "IP", gameInfo->internalIP.address);
					packet.PutInteger(&inipStruct, "PORT", gameInfo->internalIP.port);
				}
			}
			packet.PutInteger(&greqStruct, "IGNO", gameIgnore);
			packet.PutInteger(&greqStruct, "NRES", gameInfo->resetable ? 0 : 1);
			packet.PutInteger(&greqStruct, "NTOP", static_cast<uint32_t>(gameInfo->networkTopology));
			packet.PutString(&greqStruct, "PGID", playgroupId);
			packet.PutBlob(&greqStruct, "PGSC", nullptr, 0);
			{
				auto& pcapList = packet.CreateList(&greqStruct, "PCAP", TDF::Type::Integer);
				for (auto value : gameInfo->capacity) {
					packet.PutInteger(&pcapList, "", value);
				}
			}
			packet.PutInteger(&greqStruct, "PMAX", gameInfo->maxPlayers);
			packet.PutInteger(&greqStruct, "PRES", static_cast<uint32_t>(gameInfo->presenceMode));
			packet.PutInteger(&greqStruct, "QCAP", gameInfo->queueCapacity);
			packet.PutInteger(&greqStruct, "RGID", rgId);
			{
				auto& sidlList = packet.CreateList(&greqStruct, "SIDL", TDF::Type::Integer);
				packet.PutInteger(&sidlList, "", 1);
			} {
				auto& seatList = packet.CreateList(&greqStruct, "SEAT", TDF::Type::Integer);
				packet.PutInteger(&seatList, "", 1);
			}
			packet.PutInteger(&greqStruct, "SLOT", slot);
			packet.PutInteger(&greqStruct, "TCAP", tCap);
			{
				auto& tidsList = packet.CreateList(&greqStruct, "TIDS", TDF::Type::Integer);
				packet.PutInteger(&tidsList, "", 1);
			}
			packet.PutInteger(&greqStruct, "TIDX", tidx);
			packet.PutInteger(&greqStruct, "VOIP", static_cast<uint32_t>(gameInfo->voipTopology));
			packet.PutString(&greqStruct, "VSTR", versionString);
		}
		packet.PutString(nullptr, "MID", "1");

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::GameManager;
		header.command = 0xDC;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void GameManagerComponent::CreateGame(Client* client, Header header) {
		SendCreateGame(client, 1);

		NotifyGameStateChange(client, 1, GameState::Initializing);
		NotifyGameSetup(client);
	}

	void GameManagerComponent::JoinGame(Client* client, Header header) {
		auto& request = client->get_request();

		uint32_t gameId = request["GID"].GetUint();
		SendJoinGame(client, gameId);

		UserSessionComponent::NotifyUserAdded(client, 1, "Dalkon");
		UserSessionComponent::NotifyUserUpdated(client, 1);

		NotifyPlayerJoining(client, gameId);
	}

	void GameManagerComponent::RemovePlayer(Client* client, Header header) {
		auto& request = client->get_request();

		uint32_t gameId = request["GID"].GetUint();
		uint32_t personaId = request["PID"].GetUint();
		PlayerRemovedReason reason = static_cast<PlayerRemovedReason>(request["REAS"].GetUint());

		header.error_code = 0;
		client->reply(std::move(header));

		NotifyPlayerRemoved(client, gameId, personaId, reason);
	}

	void GameManagerComponent::StartMatchmaking(Client* client, Header header) {
		SendStartMatchmaking(client, 1);
	}

	void GameManagerComponent::CancelMatchmaking(Client* client, Header header) {
		// SendStartMatchmaking(client, 1);
	}

	void GameManagerComponent::FinalizeGameCreation(Client* client, Header header) {
		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		const auto& gameInfo = user->get_game_info();
		if (!gameInfo) {
			return;
		}

		Game::Manager::StartGame(gameInfo->id);

		header.error_code = 0;
		client->reply(std::move(header));
	}

	void GameManagerComponent::ResetDedicatedServer(Client* client, Header header) {
		/*
# failed to start a level (may be used in other cases) - GMS
0x1e5bc0a4 Unable to start planet. Returning to ship. (THIS IS THE CURRENT PROBLEM)

# Dynamic Ded Serv capcity
#seen when the GMS's are offline or at capacity
0x01320004 Unable to start planet. Returning to ship.

#GAMEMANAGER_ERR_INVALID_GAME_ID
0x00020004 Unable to start planet. Returning to ship.

#GAMEMANAGER_ERR_GAME_FULL
0x00040004 Unable to start planet. Returning to ship.
		*/

		auto& request = client->get_request();

		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		auto gameInfo = Game::Manager::CreateGame();
		user->set_game_info(gameInfo);

		// Attributes
		for (const auto& attribute : request["ATTR"]["_Content"].GetObject()) {
			gameInfo->attributes.emplace(attribute.name.GetString(), attribute.value.GetString());
		}

		// Capacity
		for (const auto& capacity : request["PCAP"]["_Content"].GetArray()) {
			gameInfo->capacity.push_back(capacity.GetUint());
		}

		// IPs
		const auto& hnetData = request["HNET"]["_Content"];
		gameInfo->internalIP = {
			.address = hnetData[0]["IP"].GetUint(),
			.port = static_cast<uint16_t>(hnetData[0]["PORT"].GetUint())
		};

		gameInfo->externalIP = {
			.address = hnetData[1]["IP"].GetUint(),
			.port = static_cast<uint16_t>(hnetData[1]["PORT"].GetUint())
		};

		// Other
		gameInfo->name = request["GNAM"].GetString();
		gameInfo->type = request["GTYP"].GetString();

		gameInfo->settings = request["GSET"].GetUint64();

		gameInfo->presenceMode = static_cast<PresenceMode>(request["PRES"].GetUint());
		gameInfo->state = GameState::Initializing;
		gameInfo->networkTopology = static_cast<GameNetworkTopology>(request["NTOP"].GetUint());
		gameInfo->voipTopology = static_cast<VoipTopology>(request["VOIP"].GetUint());

		gameInfo->maxPlayers = request["PMAX"].GetUint();
		gameInfo->queueCapacity = request["QCAP"].GetUint();

		gameInfo->resetable = request["NRES"].GetUint() != 0;

		/////////////////

		SendCreateGame(client, gameInfo->id);
		// SendStartMatchmaking(client, gameInfo->id);
		
		// NotifyCreateDynamicDedicatedServerGame(client);
		// NotifyGameCreated(client);
		NotifyGameStateChange(client, gameInfo->id, gameInfo->state);
		NotifyGameSetup(client);
		// NotifyGameReset(client);

		// NotifyPlayerJoining(client, gameInfo->id);
		// NotifyPlayerJoinCompleted(client, gameInfo->id);
	}

	void GameManagerComponent::UpdateMeshConnection(Client* client, Header header) {
		auto& request = client->get_request();
		auto& targetData = request["TARG"]["_Content"][0];

		uint32_t gameId = request["GID"].GetUint();
		uint32_t personaId = targetData["PID"].GetUint();

		PlayerState playerState = static_cast<PlayerState>(targetData["STAT"].GetUint());
		ClientType type = client->type();

		if (playerState == PlayerState::Connecting) {
			if (type == ClientType::GameplayUser) {
				const auto& user = client->get_user();
				if (user) {
					NotifyGamePlayerStateChange(client, gameId, user->get_id(), PlayerState::Connected);
					NotifyPlayerJoinCompleted(client, gameId, user->get_id());
				}
			} else if (type == ClientType::DedicatedServer) {
				NotifyGamePlayerStateChange(client, gameId, personaId, PlayerState::Connected);
				NotifyPlayerJoinCompleted(client, gameId, personaId);
			}
		} else if (playerState == PlayerState::Reserved) {
			if (type == ClientType::GameplayUser) {
				// var game = GameManager.Games[gameID.Value];
				// game.Slots.Remove(playerID.Value);

				NotifyPlayerRemoved(client, gameId, personaId, PlayerRemovedReason::PlayerConnLost);
			} else if (type == ClientType::DedicatedServer) {
				// var game = GameManager.Games[gameID.Value];
				// game.Slots.Remove(playerID.Value);

				NotifyPlayerRemoved(client, gameId, personaId, PlayerRemovedReason::PlayerConnLost);
			}
		}

		header.error_code = 0;
		client->reply(std::move(header));
	}
}
