#ifndef __LOBBY_TYPES_H
#define __LOBBY_TYPES_H

#include "DS_Table.h"

namespace RakNet
{

enum LobbyUserStatus
{
	LOBBY_USER_STATUS_OFFLINE,
	LOBBY_USER_STATUS_LOGGING_ON,
	LOBBY_USER_STATUS_LOGGING_OFF,
	LOBBY_USER_STATUS_IN_LOBBY_IDLE,
	LOBBY_USER_STATUS_CREATING_ROOM,
	LOBBY_USER_STATUS_IN_ROOM,
	LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH,
};

enum LobbyClientCBResultCode
{
	// Success
	LC_RC_SUCCESS,

	// Everything else is a reason why we failed.
	LC_RC_NOT_CONNECTED_TO_SERVER,
	LC_RC_BAD_TITLE_ID,
	LC_RC_NOT_IN_ROOM,
	LC_RC_ALREADY_IN_ROOM,
	LC_RC_MODERATOR_ONLY,
	LC_RC_IN_PROGRESS,
	LC_RC_BUSY,
	LC_RC_ROOM_DOES_NOT_EXIST,
	LC_RC_ROOM_FULL,
	LC_RC_ROOM_ONLY_HAS_RESERVED,
	LC_RC_INVALID_INPUT, // May have additional info
	LC_RC_DISCONNECTED_USER_ID,
	LC_RC_UNKNOWN_USER_ID,
	LC_RC_BANNED_USER_ID, // Has additionalInfo
	LC_RC_SUSPENDED_USER_ID, // Has additionalInfo, epochTime
	LC_RC_USER_NOT_IN_ROOM,
	LC_RC_USER_ALREADY_IN_ROOM,
	LC_RC_CONNECTION_FAILURE,
	LC_RC_DATABASE_FAILURE, // Has additionalInfo
	LC_RC_PERMISSION_DENIED,
	LC_RC_UNKNOWN_PERMISSIONS,
	LC_RC_ALREADY_CURRENT_STATE,
	LC_RC_NAME_IN_USE,
	LC_RC_IGNORED,
	LC_RC_NOT_ENOUGH_PARTICIPANTS,
	LC_RC_NOT_READY,
	LC_RC_INVALID_CLAN,
	LC_RC_BLACKLISTED_FROM_CLAN,
	LC_RC_NOT_INVITED,
	LC_RC_USER_KEY_NOT_SET,
	LC_RC_USER_KEY_WRONG_TITLE,
	LC_RC_USER_KEY_UNKNOWN,
	LC_RC_USER_KEY_INVALID,
	LC_RC_COUNT
};
struct ErrorCodeDescription
{
	LobbyClientCBResultCode errorCode;
	const char *englishDesc;

	static const char *ToEnglish(LobbyClientCBResultCode result);
};

#define RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS 20

struct DefaultTableColumns
{
	enum
	{
		TC_TITLE_NAME,
		TC_ROOM_NAME,
		TC_ROOM_ID,
		TC_ALLOW_SPECTATORS,
		TC_ROOM_HIDDEN,
		TC_TOTAL_SLOTS,
		TC_TOTAL_PUBLIC_SLOTS,
		TC_TOTAL_PRIVATE_SLOTS,
		TC_OPEN_PUBLIC_SLOTS,
		TC_OPEN_PRIVATE_SLOTS,
		TC_USED_SLOTS,
		TC_USED_PUBLIC_SLOTS,
		TC_USED_PRIVATE_SLOTS,
		TC_USED_SPECTATE_SLOTS,
		TC_TITLE_ID,
		TC_LOBBY_ROOM_PTR, // IDs that I do not send to the client should be last
		TC_TABLE_COLUMNS_COUNT
	} columnId;

	const char *columnName;
	DataStructures::Table::ColumnType columnType;

	static const char *GetColumnName(int columnId);
	static DataStructures::Table::ColumnType GetColumnType(int columnId);
};



}

#endif
