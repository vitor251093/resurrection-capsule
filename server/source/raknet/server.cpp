
// Include
#include "server.h"
#include "../utils/functions.h"
#include "../game/creature.h"

#include <MessageIdentifiers.h>
#include <RakSleep.h>
#include <RakNetTypes.h>
#include <RakNetworkFactory.h>
#include <BitStream.h>
#include <GetTime.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <algorithm>

/*
	C2S or C2C
		???

	S2C or C2C
		ID_CONNECTION_REQUEST,
		ID_CONNECTION_REQUEST_ACCEPTED,
		ID_CONNECTION_ATTEMPT_FAILED,
		ID_ALREADY_CONNECTED, (should never happen?)
		ID_NEW_INCOMING_CONNECTION,
		ID_DISCONNECTION_NOTIFICATION,
		ID_CONNECTION_LOST,
		ID_MODIFIED_PACKET, (should never happen?)
		ID_REMOTE_DISCONNECTION_NOTIFICATION, (unexpected in client)
		ID_REMOTE_CONNECTION_LOST, (unexpected in client)
		ID_REMOTE_NEW_INCOMING_CONNECTION (unexpected in client)

	Client packet reliability
		UNRELIABLE
		UNRELIABLE_SEQUENCED
		RELIABLE
		RELIABLE_ORDERED

	Client packet priority
		HIGH_PRIORITY
		MEDIUM_PRIORITY (default)
		LOW_PRIORITY
*/

constexpr int const_tolower(int c) {
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr uint32_t hash_id(const char* pStr) {
	uint32_t rez = 0x811C9DC5u;
	while (*pStr) {
		// To avoid compiler warnings
		rez = static_cast<uint32_t>(rez * static_cast<unsigned long long>(0x1000193));
		rez ^= static_cast<uint32_t>(const_tolower(*pStr));
		++pStr;
	}
	return rez;
}

#define DEFINE_HASH(x) constexpr auto x = hash_id(#x)

namespace Hash {
	DEFINE_HASH(cSpaceshipCameraTuning);
	DEFINE_HASH(SpaceshipTuning);
	DEFINE_HASH(cToolPos);
	DEFINE_HASH(cAssetQueryString);
	DEFINE_HASH(cLayerPrefs);
	DEFINE_HASH(EditorPrefs);
	DEFINE_HASH(cCinematicView);
	DEFINE_HASH(Cinematic);
	DEFINE_HASH(PopupTip);
	DEFINE_HASH(UnlockDef);
	DEFINE_HASH(UnlocksTuning);
	DEFINE_HASH(WeaponDef);
	DEFINE_HASH(WeaponTuning);
	DEFINE_HASH(labsCharacter);
	DEFINE_HASH(labsCrystal);
	DEFINE_HASH(labsPlayer);
	DEFINE_HASH(cControllerState);
	DEFINE_HASH(CatalogEntry);
	DEFINE_HASH(Catalog);
	DEFINE_HASH(TestAsset);
	DEFINE_HASH(TestProcessedAsset);
	DEFINE_HASH(cLootData);
	DEFINE_HASH(LootData);
	DEFINE_HASH(LootSuffix);
	DEFINE_HASH(LootPrefix);
	DEFINE_HASH(LootRigblock);
	DEFINE_HASH(LootPreferences);
	DEFINE_HASH(cKeyAsset);
	DEFINE_HASH(PlayerClass);
	DEFINE_HASH(cLongDescription);
	DEFINE_HASH(cEliteAffix);
	DEFINE_HASH(NonPlayerClass);
	DEFINE_HASH(ClassAttributes);
	DEFINE_HASH(CharacterAnimation);
	DEFINE_HASH(CharacterType);
	DEFINE_HASH(ServerEvent);
	DEFINE_HASH(cAudioEventData);
	DEFINE_HASH(cHardpointInfo);
	DEFINE_HASH(cEffectEventData);
	DEFINE_HASH(ServerEventDef);
	DEFINE_HASH(CombatEvent);
	DEFINE_HASH(InteractableDef);
	DEFINE_HASH(cInteractableData);
	DEFINE_HASH(CombatantDef);
	DEFINE_HASH(cCombatantData);
	DEFINE_HASH(cGfxComponentDef);
	DEFINE_HASH(Gfx);
	DEFINE_HASH(cSPBoundingBox);
	DEFINE_HASH(ExtentsCategory);
	DEFINE_HASH(ObjectExtents);
	DEFINE_HASH(cThumbnailCaptureParameters);
	DEFINE_HASH(Noun);
	DEFINE_HASH(DifficultyTuning);
	DEFINE_HASH(cAttributeData);
	DEFINE_HASH(cAgentBlackboard);
	DEFINE_HASH(NavMeshLayer);
	DEFINE_HASH(NavPowerTuning);
	DEFINE_HASH(Markerset);
	DEFINE_HASH(LevelMarkerset);
	DEFINE_HASH(LevelCameraSettings);
	DEFINE_HASH(LevelKey);
	DEFINE_HASH(LevelObjectives);
	DEFINE_HASH(Level);
	DEFINE_HASH(DirectorClass);
	DEFINE_HASH(LevelConfig);
	DEFINE_HASH(DirectorBucket);
	DEFINE_HASH(SectionConfig);
	DEFINE_HASH(SpawnPointDef);
	DEFINE_HASH(SpawnTriggerDef);
	DEFINE_HASH(DirectorTuning);
	DEFINE_HASH(cAIDirector);
	DEFINE_HASH(cMapCameraData);
	DEFINE_HASH(LocomotionTuning);
	DEFINE_HASH(cLobParams);
	DEFINE_HASH(cProjectileParams);
	DEFINE_HASH(cLocomotionData);
	DEFINE_HASH(TriggerVolumeEvents);
	DEFINE_HASH(TriggerVolumeDef);
	DEFINE_HASH(NPCAffix);
	DEFINE_HASH(cAffixDifficultyTuning);
	DEFINE_HASH(EliteNPCGlobals);
	DEFINE_HASH(CrystalDef);
	DEFINE_HASH(CrystalDropDef);
	DEFINE_HASH(CrystalLevel);
	DEFINE_HASH(CrystalTuning);
	DEFINE_HASH(cGameObjectGfxStateData);
	DEFINE_HASH(cGameObjectGfxStates);
	DEFINE_HASH(GameObjectGfxStateTuning);
	DEFINE_HASH(cVolumeDef);
	DEFINE_HASH(cPressureSwitchDef);
	DEFINE_HASH(cSwitchDef);
	DEFINE_HASH(cDoorDef);
	DEFINE_HASH(cNewGfxState);
	DEFINE_HASH(cWaterSimData);
	DEFINE_HASH(cGraphicsData);
	DEFINE_HASH(cGrassData);
	DEFINE_HASH(cSplineCameraData);
	DEFINE_HASH(cAnimatorData);
	DEFINE_HASH(cAnimatedData);
	DEFINE_HASH(cLabsMarker);
	DEFINE_HASH(sporelabsObject);
	DEFINE_HASH(cGameObjectCreateData);
	DEFINE_HASH(cAssetProperty);
	DEFINE_HASH(cAssetPropertyList);
	DEFINE_HASH(ability);
	DEFINE_HASH(AIDefinition);
	DEFINE_HASH(cAINode);
	DEFINE_HASH(Phase);
	DEFINE_HASH(Condition);
	DEFINE_HASH(cGambitDefinition);
	DEFINE_HASH(cAICondition);
	DEFINE_HASH(MagicNumbers);
	DEFINE_HASH(cWaterData);
	DEFINE_HASH(TeleporterDef);
	DEFINE_HASH(SpaceshipSpawnPointDef);
	DEFINE_HASH(SharedComponentData);
	DEFINE_HASH(cPointLightData);
	DEFINE_HASH(cSpotLightData);
	DEFINE_HASH(cLineLightData);
	DEFINE_HASH(cParallelLightData);
	DEFINE_HASH(cSplineCameraNodeBaseData);
	DEFINE_HASH(cSplineCameraNodeData);
	DEFINE_HASH(cOccluderData);
	DEFINE_HASH(EventListenerData);
	DEFINE_HASH(EventListenerDef);
	DEFINE_HASH(cDecalData);
	DEFINE_HASH(cCameraComponentData);
	DEFINE_HASH(cCameraComponent);
	DEFINE_HASH(AudioTriggerDef);
	DEFINE_HASH(objective);
	DEFINE_HASH(affix);
	DEFINE_HASH(AffixTuning);
	DEFINE_HASH(GravityForce);
	DEFINE_HASH(CollisionVolumeDef);
	DEFINE_HASH(ProjectileDef);
	DEFINE_HASH(OrbitDef);
	DEFINE_HASH(TriggerVolumeComponentDef);
}
#undef DEFINE_HASH

template<typename T>
std::enable_if_t<std::is_integral_v<T>, T> bswap(T t) {
	constexpr auto size = sizeof(T);
	if constexpr (size == 2) {
		t = (t >> 8) |
			(t << 8);
	} else if constexpr (size == 4) {
		t = (t << 24) |
			((t & 0x0000FF00U) << 8) |
			((t & 0x00FF0000U) >> 8) |
			(t >> 24);
	} else if constexpr (size == 8) {
		t = (t << 56) |
			((t & 0x000000000000FF00ULL) << 40) |
			((t & 0x0000000000FF0000ULL) << 24) |
			((t & 0x00000000FF000000ULL) << 8) |
			((t & 0x000000FF00000000ULL) >> 8) |
			((t & 0x0000FF0000000000ULL) >> 24) |
			((t & 0x00FF000000000000ULL) >> 40) |
			(t >> 56);
	}
	return t;
}

template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> bswap(T t) {
	T value;

	auto pValue = reinterpret_cast<uint8_t*>(&t);
	auto pNewValue = reinterpret_cast<uint8_t*>(&value);

	constexpr auto size = sizeof(T);
	for (size_t i = 0; i < size; i++) {
		pNewValue[size - 1 - i] = pValue[i];
	}

	return value;
}

template<typename T>
std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, void> Write(RakNet::BitStream& stream, T value) {
	stream.Write<T>(bswap<T>(value));
}

// Test classes
struct cSPVector3 {
	float x;
	float y;
	float z;

	void Write(RakNet::BitStream& stream) {
		::Write(stream, x);
		::Write(stream, y);
		::Write(stream, z);
	}
};

struct cGameObjectCreateData { // size 70h?
	/* 00h */    uint32_t noun;
	/* 04h */    cSPVector3 position;
	/* 10h */    float rotXDegrees;
	/* 14h */    float rotYDegrees;
	/* 1Ch */    float rotZDegrees;
	/* 20h */    uint64_t assetId;
	/* 28h */    float scale;
	/* 2Ch */    uint8_t team;
	/* 2Dh */    bool hasCollision;
	/* 2Eh */    bool playerControlled;

	void Write(RakNet::BitStream& stream) {
		::Write<uint32_t>(stream, noun);
		position.Write(stream);
		::Write<float>(stream, rotXDegrees);
		::Write<float>(stream, rotYDegrees);
		::Write<float>(stream, rotZDegrees);
		::Write<uint64_t>(stream, assetId);
		::Write<float>(stream, scale);
		::Write<uint8_t>(stream, team);
		::Write<uint8_t>(stream, hasCollision);
		::Write<uint8_t>(stream, playerControlled);
	}
};

// RakNet
namespace RakNet {
	enum GState : uint32_t {
		Boot = 0,
		Login,
		Spaceship,
		Editor,
		PreDungeon,
		Dungeon,
		Observer,
		Cinematic,
		Spectator,
		ChainVoting,
		ChainCashOut,
		GameOver,
		Quit,
		ArenaLobby,
		ArenaRoundResults,
		JuggernautLobby,
		JuggernautResults,
		KillRaceLobby,
		KillRaceResults
	};

	// Server
	Server::Server(uint16_t port, uint32_t gameId) : mGameId(gameId) {
		mThread = std::thread([this, port] {
			mSelf = RakNetworkFactory::GetRakPeerInterface();
			mSelf->SetTimeoutTime(30000, UNASSIGNED_SYSTEM_ADDRESS);
#ifdef PACKET_LOGGING
			mSelf->AttachPlugin(&mLogger);
#endif
			if (mSelf->Startup(4, 30, &SocketDescriptor(port, nullptr), 1)) {
				mSelf->SetMaximumIncomingConnections(4);
				mSelf->SetOccasionalPing(true);
				mSelf->SetUnreliableTimeout(1000);
				while (is_running()) {
					run_one();
					RakSleep(30);
				}
			}

			mSelf->Shutdown(300);
			RakNetworkFactory::DestroyRakPeerInterface(mSelf);
		});
	}

	Server::~Server() {
		stop();
	}

	void Server::stop() {
		mMutex.lock();
		mRunning = false;
		mMutex.unlock();
		if (mThread.joinable()) {
			mThread.join();
		}
	}

	bool Server::is_running() {
		std::lock_guard<std::mutex> lock(mMutex);
		return mRunning;
	}

	void Server::run_one() {
		const auto GetPacketIdentifier = [this]() -> MessageID {
			uint8_t message;
			if (mInStream.GetData()) {
				mInStream.Read<uint8_t>(message);
				if (message == ID_TIMESTAMP) {
					constexpr auto timestampSize = sizeof(MessageID) + sizeof(RakNetTime);
					RakAssert((mInStream.GetNumberOfUnreadBits() >> 3) > timestampSize);

					mInStream.IgnoreBytes(timestampSize - 1);
					mInStream.Read<uint8_t>(message);
				}
			} else {
				message = 0xFF;
			}
			return message;
		};

		for (Packet* packet = mSelf->Receive(); packet; mSelf->DeallocatePacket(packet), packet = mSelf->Receive()) {
			mInStream = BitStream(packet->data, packet->bitSize * 8, false);

			uint8_t packetType = GetPacketIdentifier();
			std::cout << std::endl << "--- "  << (int)packetType << " gotten from raknet ---" << std::endl << std::endl;
			switch (packetType) {
				case ID_DISCONNECTION_NOTIFICATION: {
					std::cout << "ID_DISCONNECTION_NOTIFICATION from " << packet->systemAddress.ToString(true) << std::endl;
					break;
				}

				case ID_NEW_INCOMING_CONNECTION: {
					OnNewIncomingConnection(packet);
					break;
				}

				case ID_CONNECTION_REQUEST: {
					std::cout << "Trying to connect to RakNet" << std::endl;
					break;
				}

				case ID_INCOMPATIBLE_PROTOCOL_VERSION: {
					std::cout << "ID_INCOMPATIBLE_PROTOCOL_VERSION" << std::endl;
					break;
				}

				case ID_CONNECTION_LOST: {
					std::cout << "ID_CONNECTION_LOST from " << packet->systemAddress.ToString(true) << std::endl;
					break;
				}

				case ID_USER_PACKET_ENUM: {
					OnHelloPlayer(packet);
					break;
				}

				default: {
					ParsePacket(packet, packetType);
					break;
				}
			}
		}

		mInStream.Reset();
	}

	void Server::ParsePacket(Packet* packet, MessageID packetType) {
		switch (packetType) {
			case PacketID::HelloPlayer: {
				OnHelloPlayer(packet);
				break;
			}

			case PacketID::PlayerStatusUpdate: {
				OnPlayerStatusUpdate(packet);
				break;
			}

			case PacketID::ActionCommandMsgs: {
				OnActionCommandMsgs(packet);
				break;
			}

			case PacketID::DebugPing: {
				OnDebugPing(packet);
				break;
			}

			default: {
				std::cout << "Unknown packet id: 0x" << std::hex << static_cast<int32_t>(packetType) << std::dec << std::endl;
				break;
			}
		}
	}

	void Server::OnNewIncomingConnection(Packet* packet) {
		SendConnected(packet);
	}

	void Server::OnHelloPlayer(Packet* packet) {
		SendHelloPlayer(packet);
		// if ok
		// SendPlayerJoined(packet);
		// else
		// SendPlayerDeparted(packet);

		// SendTestPacket(packet, PacketID::ModifierDeleted, { 0x12, 0x34, 0x56, 0x78 });

		// SendTestPacket(packet, PacketID::ChainGameMsgs, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 });

		// SendTestPacket(packet, PacketID::PlayerCharacterDeploy, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 });

		SendPartyMergeComplete(packet);
		SendGamePrepareForStart(packet);
		// SendTestPacket(packet, PacketID::DirectorState, { 0x01, 0x02 });

		/*
		SendTestPacket(packet, PacketID::GamePrepareForStart + 1, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA });
		SendGameState(packet, Blaze::GameState::PreGame);
		SendTestPacket(packet, PacketID::GameStart + 1, { 0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA });
		*/
	}

	void Server::OnPlayerStatusUpdate(Packet* packet) {
		uint8_t value;
		mInStream.Read<uint8_t>(value);

		std::cout << "Player Status Update: " << (int)value << std::endl;

		switch (value) {
			case 2: {
				break;
			}

			case 4: {
				SendGameState(packet, PreDungeon);
				SendArenaGameMessages(packet);
				SendPlayerDeparted(packet);
				break;
			}

			case 8: {
				SendGameState(packet, Dungeon);
				SendGameStart(packet);

				SendObjectCreate(packet, static_cast<uint32_t>(Game::CreatureID::BlitzAlpha));
				SendPlayerCharacterDeploy(packet);

				SendObjectivesInitForLevel(packet);
				
				// SendTestPacket(packet, PacketID::TutorialGameMsgs, { 0x00, 0x02, 0x03, 0x04 });
				
				// SendTestPacket(packet, PacketID::ObjectJump, { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 });
				break;
			}
		}
	}

	void Server::OnActionCommandMsgs(Packet* packet) {
		std::cout << "OnActionCommandMsgs" << std::endl;

		// We have 64 bytes of stuff
	}
	
	void Server::OnDebugPing(Packet* packet) {
		// Packet size: 0x08
		uint64_t time;
		mInStream.Read<uint64_t>(time);

		std::chrono::milliseconds ms(time);
		std::chrono::time_point<std::chrono::system_clock> debugTime(ms);

		auto debugTime_c = std::chrono::system_clock::to_time_t(debugTime);
		auto debugTime_c2 = std::ctime(&debugTime_c);
		if (debugTime_c2) {
			std::cout << "Debug ping: " << debugTime_c2 << std::endl;
		} else {
			std::cout << "Debug ping: " << time << std::endl;
		}
	}

	void Server::SendHelloPlayer(Packet* packet) {
		// Packet size: 0x08
		const auto& guid = mSelf->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);

		auto addr = mSelf->GetSystemAddressFromGuid(guid);
		if (1) { // add check for local
			addr = SystemAddress("127.0.0.1", addr.port);
		}

		/*
			u8: type
			u8: gameIndex
			u32: ip
			u16: port
		*/

		BitStream outStream(8);
		outStream.Write(PacketID::HelloPlayer);
		outStream.Write<uint8_t>(0x00); // Player id?
		outStream.Write<uint8_t>(mGameId);
		outStream.WriteBits(reinterpret_cast<const uint8_t*>(&addr.binaryAddress), sizeof(addr.binaryAddress) * 8, true);
		outStream.Write(addr.port);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendConnected(Packet* packet) {
		// TODO: verify incoming connection

		BitStream outStream(8);
		outStream.Write(PacketID::Connected);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPlayerJoined(Packet* packet) {
		// Packet size: 0x01
		BitStream outStream(8);
		outStream.Write(PacketID::PlayerJoined);
		outStream.Write<uint8_t>(mGameId);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPlayerDeparted(Packet* packet) {
		// Packet size: 0x01
		BitStream outStream(8);
		outStream.Write(PacketID::PlayerDeparted);
		outStream.Write<uint8_t>(mGameId);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPlayerStatusUpdate(Packet* packet, Blaze::PlayerState playerState) {
		BitStream outStream(8);
		outStream.Write(PacketID::PlayerStatusUpdate);
		outStream.Write<uint8_t>(static_cast<uint8_t>(playerState));

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendGameState(Packet* packet, uint32_t gameState) {
		// Packet size: 0x19
		BitStream outStream(8);
		outStream.Write(PacketID::GameState);

		Write<uint32_t>(outStream, gameState);
		Write<uint32_t>(outStream, 0x99);
		Write<uint32_t>(outStream, 0x82);
		Write<uint32_t>(outStream, 0x66);
		Write<uint32_t>(outStream, 0x31);
		Write<uint8_t>(outStream, 0x00);
		Write<uint32_t>(outStream, 0x52);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendLabsPlayerUpdate(Packet* packet) {
		// Packet size: 0x25
		BitStream outStream(8);
		outStream.Write(PacketID::LabsPlayerUpdate);

		// 0-7?
		outStream.Write<uint32_t>(bswap<uint32_t>(0x03));
		outStream.Write<uint32_t>(0x04);
		outStream.Write<uint32_t>(0x03);
		outStream.Write<uint32_t>(0x02);
		outStream.Write<uint32_t>(0x01);
		outStream.Write<uint32_t>(0x00);
		outStream.Write<uint32_t>(0x07);
		outStream.Write<uint32_t>(0x08);
		outStream.Write<uint32_t>(0x09);
		outStream.Write<uint8_t>(0x0A);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendDirectorState(Packet* packet) {
		// Packet size: reflection(variable)
		BitStream outStream(8);
		outStream.Write(PacketID::DirectorState);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectCreate(Packet* packet, uint32_t objectId) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectCreate);

		Write<uint32_t>(outStream, Hash::cGameObjectCreateData);

		// reflection
		uint16_t value0 = 0x01;
		Write<uint16_t>(outStream, value0);
		if (value0 > 0) {
			cGameObjectCreateData data;
			data.noun = objectId;
			data.position = { 0.f, 0.f, 0.f };
			data.rotXDegrees = 0.f;
			data.rotYDegrees = 0.f;
			data.rotZDegrees = 0.f;
			data.assetId = 0;
			data.scale = 1.f;
			data.team = 0;
			data.hasCollision = true;
			data.playerControlled = true;

			data.Write(outStream);
		}

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectDelete(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectDelete);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendActionCommandResponse(Packet* packet) {
		// Packet size: 0x2C
		BitStream outStream(8);
		outStream.Write(PacketID::ActionCommandResponse);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectPlayerMove(Packet* packet) {
		// Packet size: 0x20
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectPlayerMove);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectTeleport(Packet* packet) {
		// Packet size: 0x50
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectTeleport);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendForcePhysicsUpdate(Packet* packet) {
		// Packet size: 0x28
		BitStream outStream(8);
		outStream.Write(PacketID::ForcePhysicsUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPhysicsChanged(Packet* packet) {
		// Packet size: 0x05
		BitStream outStream(8);
		outStream.Write(PacketID::PhysicsChanged);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendLocomotionDataUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::LocomotionDataUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendLocomotionDataUnreliableUpdate(Packet* packet) {
		// Packet size: 0x10
		BitStream outStream(8);
		outStream.Write(PacketID::LocomotionDataUnreliableUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendAttributeDataUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::AttributeDataUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendCombatantDataUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::CombatantDataUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendInteractableDataUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::InteractableDataUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendAgentBlackboardUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::AgentBlackboardUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendLootDataUpdate(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::LootDataUpdate);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendServerEvent(Packet* packet) {
		// Packet size: variable
		BitStream outStream(8);
		outStream.Write(PacketID::ServerEvent);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendModifierCreated(Packet* packet) {
		// Packet size: 0x15
		BitStream outStream(8);
		outStream.Write(PacketID::ModifierCreated);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendModifierUpdated(Packet* packet) {
		// Packet size: 0x08
		BitStream outStream(8);
		outStream.Write(PacketID::ModifierUpdated);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendModifierDeleted(Packet* packet) {
		// Packet size: 0x19
		BitStream outStream(8);
		outStream.Write(PacketID::ModifierDeleted);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPlayerCharacterDeploy(Packet* packet) {
		/*
			0 : u8 (unk)
			1 : u8 (1-16)
			2 : u8 (unk)
			3 : u8 (unk)
			
			if [1] == 1 {
				0 : u8 (arg0 -> sub_4E21D0)
				4 : u32 (hash, default 'none', arg0 -> sub_9DFB40)
				8 : u32 (compared to 0?, var -> sub_9DFB40)

				16 : u32 (arg0 -> sub_9D7A00)
				20 : u32 (arg1 -> sub_9D7A00)

				24 : u32 (arg0 -> sub_9D7A00)
				28 : u32 (arg1 -> sub_9D7A00)

				32 : u32 (arg0 -> sub_9D7A00)
				36 : u32 (arg1 -> sub_9D7A00)

				40 : u32 (arg0 -> sub_9D7A00)
				44 : u32 (arg1 -> sub_9D7A00)

				52 : u32 (arg1 -> sub_4E21D0)
			}
		*/

		constexpr uint32_t hash_none = hash_id("none");

		// Packet size: 0x09
		BitStream outStream(8);
		outStream.Write(PacketID::PlayerCharacterDeploy);

		Write<uint32_t>(outStream, static_cast<uint32_t>(Game::CreatureID::BlitzAlpha));
		Write<uint8_t>(outStream, 0x01);
		Write<uint32_t>(outStream, static_cast<uint32_t>(Game::CreatureID::BlitzAlpha));

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendGamePrepareForStart(Packet* packet) {
		// C8AC4657 = zelems_1
		// 0xEEB4D1D9 = nocturna_1?
		// 0xA3ADE6F2 = tutorial v1
		// 0xEEB4D1D9 = tutorial v2
		// 0x3ABA8857 = unk

		// Packet size: 0x10
		BitStream outStream(8);
		outStream.Write(PacketID::GamePrepareForStart);

		Write<uint32_t>(outStream, hash_id("Darkspore_Tutorial_cryos_1_v2.level"));
		Write<uint32_t>(outStream, 0x00000000);
		Write<uint32_t>(outStream, 0x00000000);

		// level index? must be (>= 0 %% <= 72)
		Write<uint32_t>(outStream, 0x00000000);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendGameStart(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::GameStart);

		Write<uint32_t>(outStream, 0x00000000);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendArenaGameMessages(Packet* packet) {
		// Packet size: 0x04
		BitStream outStream(8);
		outStream.Write(PacketID::ArenaGameMsgs);

		Write<uint8_t>(outStream, 0x09);
		Write<float>(outStream, 1.25f);
		Write<uint16_t>(outStream, 0x0001);
		Write<uint16_t>(outStream, 0);
		Write<uint16_t>(outStream, 0x0001);
		Write<uint16_t>(outStream, 0);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendObjectivesInitForLevel(Packet* packet) {
		// Packet size: variable
		BitStream outStream(8);
		outStream.Write(PacketID::ObjectivesInitForLevel);

		uint8_t count = 0x01;
		Write<uint8_t>(outStream, count);

		for (uint8_t i = 0; i < count; ++i) {
			Write<uint32_t>(outStream, hash_id("DontUseHealthObelisks"));

			Write<uint8_t>(outStream, 0x01);
			Write<uint8_t>(outStream, 0x01);
			Write<uint8_t>(outStream, 0x01);
			Write<uint8_t>(outStream, 0x01);

			std::string description = "Do some stuff bruh";

			size_t length = std::min<size_t>(0x30, description.length());
			size_t padding = 0x30 - length;

			for (size_t i = 0; i < length; ++i) {
				Write<char>(outStream, description[i]);
			}

			for (size_t i = 0; i < padding; ++i) {
				Write<char>(outStream, 0x00);
			}
		}

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendPartyMergeComplete(Packet* packet) {
		// Packet size: 0x08
		BitStream outStream(8);
		outStream.Write(PacketID::PartyMergeComplete);
		outStream.Write<uint64_t>(utils::get_unix_time());

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendDebugPing(Packet* packet) {
		// Packet size: 0x08
		BitStream outStream(8);
		outStream.Write(PacketID::DebugPing);
		outStream.Write<uint64_t>(utils::get_unix_time());

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendTutorial(Packet* packet) {
		BitStream outStream(8);
		outStream.Write(PacketID::TutorialGameMsgs);

		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}

	void Server::SendTestPacket(Packet* packet, MessageID id, const std::vector<uint8_t>& data) {
		BitStream outStream(8);
		outStream.Write(id);
		for (auto byte : data) {
			outStream.Write(byte);
		}
		mSelf->Send(&outStream, HIGH_PRIORITY, UNRELIABLE_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
	}
}
