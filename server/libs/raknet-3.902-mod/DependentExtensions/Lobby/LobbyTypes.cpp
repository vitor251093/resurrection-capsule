#include "LobbyTypes.h"

using namespace RakNet;

static ErrorCodeDescription errorCodeDescriptions[LC_RC_COUNT] =
{
	{LC_RC_SUCCESS, "Success."},
	{LC_RC_NOT_CONNECTED_TO_SERVER, "Not connected to the lobby server, or not logged in."},
	{LC_RC_BAD_TITLE_ID, "The titleId field was either left blank, or is unknown on the server. It must be created on the server before usage by a client."},
	{LC_RC_NOT_IN_ROOM, "You are not in a room, and this action requires that you be in one."},
	{LC_RC_ALREADY_IN_ROOM, "You are already in a room. Leave your current room first."},
	{LC_RC_MODERATOR_ONLY, "Only the moderator can do this. Create your own room or ask the current moderator to take over."},
	{LC_RC_IN_PROGRESS, "This action is currently in progress. Wait for the server response."},
	{LC_RC_BUSY, "Cancel your current action before taking this action."},
	{LC_RC_ROOM_DOES_NOT_EXIST, "Sorry, this room no longer exists."},
	{LC_RC_ROOM_FULL, "Sorry, this room is full."},
	{LC_RC_ROOM_ONLY_HAS_RESERVED, "Sorry, all remaining slots are reserved by invitation only."},
	{LC_RC_INVALID_INPUT, "Invalid input to function."},
	{LC_RC_DISCONNECTED_USER_ID, "The specified user is not connected."},
	{LC_RC_UNKNOWN_USER_ID, "The specified user does not exist."}, // Login result
	{LC_RC_BANNED_USER_ID, "The specified user is banned from the server permanently."}, // Login result
	{LC_RC_SUSPENDED_USER_ID, "The specified user is suspended from the server temporarily."}, // Login result
	{LC_RC_USER_NOT_IN_ROOM, "This action requires that the specified user be in our current room."},
	{LC_RC_USER_ALREADY_IN_ROOM, "This user is already in the room, this action cannot be taken."},
	{LC_RC_CONNECTION_FAILURE, "The connection to the lobby server has been lost."},
	{LC_RC_DATABASE_FAILURE, "The database query failed. Call lobbyClient->GetDBError() to get the description string."},
	{LC_RC_PERMISSION_DENIED, "You do not have account permissions to do this."},
	{LC_RC_UNKNOWN_PERMISSIONS, "This operation requires permissions, and the server does not know your account permissions. Possible resolution: Call SetTitleLoginId()."},
	{LC_RC_ALREADY_CURRENT_STATE, "The operation we are requesting is already true with the current state of the system."},
	{LC_RC_NAME_IN_USE, "This name is already in use."},
	{LC_RC_IGNORED, "We are being ignored."},
	{LC_RC_NOT_ENOUGH_PARTICIPANTS, "Not enough participants to start the match."},
	{LC_RC_NOT_READY, "Not all users are ready yet."},
	{LC_RC_INVALID_CLAN, "The specified clan does not exist."},	
	{LC_RC_BLACKLISTED_FROM_CLAN, "The user is blacklisted from the clan, and must be unblacklisted by the leader."},	
	{LC_RC_NOT_INVITED, "This operation requires an invitation, and you were not invited."},
	{LC_RC_USER_KEY_NOT_SET, "The title requires that a per-user validation key be set, and it was not."},
	{LC_RC_USER_KEY_WRONG_TITLE, "The per-user validation key title does not match the title being set."},
	{LC_RC_USER_KEY_UNKNOWN, "The per-user validation key is not registered as a valid key."},
	{LC_RC_USER_KEY_INVALID, "The per-user validation key was specifically flagged as invalid."},
};

const char *ErrorCodeDescription::ToEnglish(LobbyClientCBResultCode result)
{
	return errorCodeDescriptions[result].englishDesc;
}

static DefaultTableColumns defaultTableColumns[DefaultTableColumns::TC_TABLE_COLUMNS_COUNT] =
{
	{DefaultTableColumns::TC_TITLE_NAME, "Title name", DataStructures::Table::STRING},
	{DefaultTableColumns::TC_ROOM_NAME, "Room name", DataStructures::Table::STRING},
	{DefaultTableColumns::TC_ROOM_ID, "Room id", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_ALLOW_SPECTATORS, "Allow spectators", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_ROOM_HIDDEN, "Room hidden", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_TOTAL_SLOTS, "Total slots", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_TOTAL_PUBLIC_SLOTS, "Public total", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_TOTAL_PRIVATE_SLOTS, "Private total", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_OPEN_PUBLIC_SLOTS, "Public open", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_OPEN_PRIVATE_SLOTS, "Private open", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_USED_SLOTS, "Used slots", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_USED_PUBLIC_SLOTS, "Public used", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_USED_PRIVATE_SLOTS, "Private used", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_USED_SPECTATE_SLOTS, "Spectate used", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_TITLE_ID, "Title Id", DataStructures::Table::NUMERIC},
	{DefaultTableColumns::TC_LOBBY_ROOM_PTR, "Lobby room ptr [Internal]", DataStructures::Table::POINTER},
};

const char *DefaultTableColumns::GetColumnName(int columnId) {return defaultTableColumns[columnId].columnName;}
DataStructures::Table::ColumnType DefaultTableColumns::GetColumnType(int columnId) {return defaultTableColumns[columnId].columnType;}
