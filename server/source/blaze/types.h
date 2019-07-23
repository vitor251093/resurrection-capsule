
#ifndef _BLAZE_TYPES_HEADER
#define _BLAZE_TYPES_HEADER

// Include
#include <cstdint>
#include <string>

// Blaze
namespace Blaze {
	enum class Component {
		Authentication = 0x01,
		GameManager = 0x04,
		Redirector = 0x05,
		Playgroups = 0x06,
		Stats = 0x07,
		Util = 0x09,
		CensusData = 0x0A,
		Clubs = 0x0B,
		Messaging = 0x0F,
		Rooms = 0x15,
		AssociationLists = 0x19,
		GameReporting = 0x1C,
		RSP = 0x801,
		Teams = 0x816,
		UserSessions = 0x7802
	};

	enum class MessageType {
		Message,
		Reply,
		Notification,
		ErrorReply
	};

	enum class NetworkAddressMember {
		XboxClientAddress,
		XboxServerAddress,
		IpPairAddress,
		IpAddress,
		HostnameAddress,
		Unset = 0x7F
	};

	enum class UpnpStatus {
		Unknown,
		Found,
		Enabled
	};

	enum class TelemetryOpt {
		OptOut,
		OptIn
	};

	enum class Slot {
		Public = 0,
		Private
	};

	enum class GameState {
		NewState,
		Initializing,
		Virtual,
		PreGame = 0x82,
		InGame = 0x83,
		PostGame = 4,
		Migrating,
		Destructing,
		Resetable,
		ReplaySetup,
	};

	enum class SessionState {
		Idle = 0,
		Connecting,
		Connected,
		Authenticated,
		Invalid
	};

	enum class PlayerState {
		Reserved = 0,
		Queued,
		Connecting,
		Migrating,
		Connected,
		KickPending
	};

	enum class PersonaStatus {
		Unknown = 0,
		Pending,
		Active,
		Deactivated,
		Disabled,
		Deleted,
		Banned
	};

	enum class PresenceMode {
		None,
		Standard,
		Private
	};

	enum class VoipTopology {
		Disabled,
		DedicatedServer,
		PeerToPeer
	};

	enum class GameNetworkTopology {
		ClientServerPeerHosted,
		ClientServerDedicated,
		PeerToPeerFullMesh = 0x82,
		PeerToPeerPartialMesh,
		PeerToPeerDirtyCastFailover
	};

	enum class PlayerRemovedReason {
		PlayerJoinTimeout,
		PlayerConnLost,
		BlazeServerConnLost,
		MigrationFailed,
		GameDestroyed,
		GameEnded,
		PlayerLeft,
		GroupLeft,
		PlayerKicked,
		PlayerKickedWithBan,
		PlayerJoinFromQueueFailed,
		PlayerReservationTimeout,
		HostEjected
	};

	enum class ClientType {
		GameplayUser,
		HttpUser,
		DedicatedServer,
		Tools,
		Invalid
	};

	enum class ExternalRefType {
		Unknown,
		Xbox,
		PS3,
		Wii,
		Mobile,
		LegacyProfileID,
		Twitter,
		Facebook
	};

	enum class NatType {
		Open,
		Moderate,
		Sequential,
		Strict,
		Unknown
	};

	enum class DatalessSetup {
		CreateGame = 0,
		JoinGame,
		IndirectJoinGameFromQueue
	};

	enum class AddressInfoType {
		InternalIp = 0,
		ExternalIp,
		XboxServerAddress
	};

	enum class GameType {
		Tutorial = 1,
		Chain = 2,
		Arena = 3,
		KillRace = 4,
		Juggernaut = 5,
		Quickplay = 6,
		DirectEntry = 7
	};

	// Header
	struct Header {
		uint16_t length;
		Component component;
		uint16_t command;
		uint16_t error_code = 0;
		MessageType message_type;
		uint32_t message_id = 0;
	};

	/*
DatalessSetupContext - DCTX {
	CREATE_GAME_SETUP_CONTEXT = 0
	JOIN_GAME_SETUP_CONTEXT = 1
	INDIRECT_JOIN_GAME_FROM_QUEUE_SETUP_CONTEXT = 2
}

MatchmakingSetupContext / IndirectMatchmakingSetupContext - RSLT {
	SUCCESS_CREATED_GAME = 0
	SUCCESS_JOINED_NEW_GAME = 1
	SUCCESS_JOINED_EXISTING_GAME = 2
	SESSION_TIMED_OUT = 3
	SESSION_CANCELED = 4
	SESSION_TERMINATED = 5
	SESSION_ERROR_GAME_SETUP_FAILED = 6
}

ClientData - CDAT {
	CLIENT_TYPE_GAMEPLAY_USER = 0
	CLIENT_TYPE_CONSOLE_USER = 0
	CLIENT_TYPE_HTTP_USER = 1
	CLIENT_TYPE_WEB_ACCESS_LAYER = 1
	CLIENT_TYPE_DEDICATED_SERVER = 2
	CLIENT_TYPE_TOOLS = 3
	CLIENT_TYPE_INVALID = 4
}

PersonaDetails - XTYP {
	BLAZE_EXTERNAL_REF_TYPE_UNKNOWN = 0
	BLAZE_EXTERNAL_REF_TYPE_XBOX = 1
	BLAZE_EXTERNAL_REF_TYPE_PS3 = 2
	BLAZE_EXTERNAL_REF_TYPE_WII = 3
	BLAZE_EXTERNAL_REF_TYPE_MOBILE = 4
	BLAZE_EXTERNAL_REF_TYPE_LEGACYPROFILEID = 5
}

ReplicatedGamePlayer - SLOT {
	SLOT_PUBLIC = 0
	SLOT_PRIVATE = 1
	MAX_SLOT_TYPE = 2
}

ReplicatedGamePlayer - STAT {
	RESERVED = 0
	QUEUED = 1
	ACTIVE_CONNECTING = 2
	ACTIVE_MIGRATING = 3
	ACTIVE_CONNECTED = 4
	ACTIVE_KICK_PENDING = 5
}

ReplicatedGameData - GSTA {
	NEW_STATE = 0
	INITIALIZING = 1
	PRE_GAME = 0x82
	IN_GAME = 0x83
	POST_GAME = 4
	MIGRATING = 5
	DESTRUCTING = 6
	RESETABLE = 7
	REPLAY_SETUP = 8
}

ReplicatedGameData - NTOP {
	CLIENT_SERVER_PEER_HOSTED = 0
	CLIENT_SERVER_DEDICATED = 1
	PEER_TO_PEER_FULL_MESH = 0x82
	PEER_TO_PEER_PARTIAL_MESH = 0x83
	PEER_TO_PEER_DIRTYCAST_FAILOVER = 0x84
}

ReplicatedGameData - PRES {
	PRESENSE_MODE_NONE = 0
	PRESENSE_MODE_STANDARD = 1
	PRESENSE_MODE_PRIVATE = 2
}

ReplicatedGameData - VOIP {
	VOIP_DISABLED = 0
	VOIP_DEDICATED_SERVER = 1
	VOIP_PEER_TO_PEER = 2
}

JoinGameRequest - JMET {
	JOIN_BY_BROWSING = 1
	JOIN_BY_MATCHMAKING = 2
	JOIN_BY_INVITES = 4
	JOIN_BY_PLAYER = 8
	SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME = 0x0F
	SYS_JOIN_BY_RESETDEDICATEDSERVER = 0x10
	SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER = 0x20
	SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME_HOST = 0x40
}

NotifyDestroyPlaygroup - REAS {
	PLAYGROUP_DESTROY_REASON_DEFAULT = 0
	PLAYGROUP_DESTROY_REASON_DISCONNECTED = 1
	PLAYGROUP_DESTROY_REASON_LEADER_CHANGE_DISABLED = 2
}

NotifyMemberRemovedFromPlaygroup - REAS {
	PLAYGROUP_MEMBER_REMOVE_REASON_DEFAULT = 0
	PLAYGROUP_MEMBER_REMOVE_REASON_DISCONNECTED = 1
	PLAYGROUP_MEMBER_REMOVE_REASON_KICKED = 2
	PLAYGROUP_MEMBER_REMOVE_TITLE_BASE_REASON = 3
}

NotifyJoinControlsChange - OPEN {
	PLAYGROUP_OPEN = 0
	PLAYGROUP_CLOSED = 1
}

ServerAddressInfo - TYPE {
	INTERNAL_IPPORT = 0
	EXTERNAL_IPPORT = 1
	XBOX_SERVER_ADDRESS = 2
}

RoomReplicationContext - UPRE {
	DELETED_EXPANDED_ROOM = 0
	DELETED_PSEUDO_ROOM = 1
	DELETED_EMPTY_USER_CREATED = 2
	DELETED_BY_USER = 3
	CONFIG_RELOADED = 4
	HOST_TRANSFER = 5
	POPULATION_CHANGED = 6
	USER_CREATED = 7
	ATTRIBUTES_UPDATE = 8
	BAN_LIST_MODIFIED = 9
}

CVAR - STAT {
	PRESENSESTATE_OFFLINE = 0
	PRESENSESTATE_ONLINE_WEB = 1
	PRESENSESTATE_ONLINE_CLIENT = 2
	PRESENSESTATE_ONLINE_CLIENT_MODE1 = 3
	PRESENSESTATE_ONLINE_CLIENT_MODE2 = 4
	PRESENSESTATE_ONLINE_CLIENT_MODE3 = 5
	PRESENSESTATE_ONLINE_CLIENT_MODE4 = 6
	PRESENSESTATE_ONLINE_CLIENT_MODE5 = 7
}
	*/
}

#endif
