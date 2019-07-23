/// \file
/// \brief Common source between the lobby client and the lobby server for the PC
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __LOBBY_PC_COMMON_H
#define __LOBBY_PC_COMMON_H

#include "LobbyClientInterface.h"
#include "LobbyDBSpec.h"
#include "TitleValidationDBSpec.h"
#include "DS_List.h"
#include "DS_Table.h"

namespace RakNet
{
	struct LobbyRoomsTableContainer;
}

// Due to a compiler bug this cannot be in a namespace
int RAK_DLL_EXPORT LobbyTitleCompByDatabaseKey( TitleValidationDBSpec::DatabaseKey const &key, RakNet::LobbyRoomsTableContainer * const &data );

namespace RakNet
{
struct LobbyRoom
{
	LobbyRoom();
	~LobbyRoom();
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
	bool RemoveRoomMember(LobbyDBSpec::DatabaseKey memberId, bool *gotNewModerator);
	bool HasRoomMember(LobbyDBSpec::DatabaseKey memberId) const;
	void SetModerator(LobbyDBSpec::DatabaseKey newModerator);
	LobbyDBSpec::DatabaseKey GetModerator(void) const;
	// Add to reserved if possible, otherwise open
	bool AddRoomMemberToFirstAvailableSlot(LobbyDBSpec::DatabaseKey newMember);
	bool AddRoomMemberToPublicSlot(LobbyDBSpec::DatabaseKey newMember);
	bool AddRoomMemberToPrivateSlot(LobbyDBSpec::DatabaseKey newMember);
	void AddRoomMemberAsSpectator(LobbyDBSpec::DatabaseKey newMember);
	bool IsInPublicSlot(LobbyDBSpec::DatabaseKey member) const;
	bool IsInPrivateSlot(LobbyDBSpec::DatabaseKey member) const;
	bool IsInSpectatorSlot(LobbyDBSpec::DatabaseKey member) const;
	void SetRoomName(const char *newRoomName);
	void SetTitleName(const char *newRoomName);
	void SetRoomPassword(const char *newRoomPassword);
	void Clear(void);
	bool RoomIsDead(void) const;
	bool AutoPromoteModerator(void);
	// May be negative if overloaded
	int GetAvailablePublicSlots(void) const;
	int GetAvailablePrivateSlots(void) const;
	void ClearPrivateSlotInvites(void);
	bool RemoveFromInviteList(LobbyDBSpec::DatabaseKey newMember);
	bool HasPrivateSlotInvite(LobbyDBSpec::DatabaseKey invitee) const;
	bool AddPrivateSlotInvite(LobbyDBSpec::DatabaseKey invitee);
		
	// ID of the row in roomsTable. Also used as sort key in roomsList. Used the ID for the room
	unsigned int rowId;

	DataStructures::List<LobbyDBSpec::DatabaseKey> publicSlotMembers;
	DataStructures::List<LobbyDBSpec::DatabaseKey> privateSlotMembers;
	DataStructures::List<LobbyDBSpec::DatabaseKey> spectators;
	unsigned int publicSlots;
	unsigned int privateSlots;
	// What game is this for?
	TitleValidationDBSpec::DatabaseKey titleId;

	// MIRRORED (needed by client)
	char *roomName;
	char *titleName;
	bool allowSpectators;
	bool roomIsHidden;

	// NOT SERIALIZED
	// rowInTable is only valid on the client if you downloaded your own room when you called DownloadRooms();
	DataStructures::Table::Row* rowInTable; // External pointer, do not deallocate.
	void WriteToRow(bool writeRoomName=false);

	// Server only
	char *roomPassword;

	// ID of the room moderator
	LobbyDBSpec::DatabaseKey moderator;

	// Disconnected users will stay in this list unless I cull them. Not sure if it's worthwhile to do or not.
	DataStructures::List<LobbyDBSpec::DatabaseKey> reservedSlotInvites;

};


// All the rooms for a particular game
struct LobbyRoomsTableContainer
{
	LobbyRoomsTableContainer();
	~LobbyRoomsTableContainer();
	unsigned int GetFreeRowId(void);
	void Initialize(TitleValidationDBSpec::DatabaseKey _titleId);

	TitleValidationDBSpec::DatabaseKey titleId;
	unsigned int nextRowId;

	// roomName, allowSpectators, numSpectators, roomIsHidden, numPlayers, totalAllowedPlayers, availableOpenSlots, (LobbyRoom* (as int))
	DataStructures::Table* roomsTable;
};


struct FriendInvitation_PC : public FriendInvitation_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _invitor;
	LobbyClientUserId _invitee;
};


struct RoomMemberDrop_PC : public RoomMemberDrop_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _droppedMember;
	LobbyClientUserId _newModerator;
};

struct RoomMemberJoin_PC : public RoomMemberJoin_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _joinedMember;
};

struct RoomMemberReadyStateChange_PC : public RoomMemberReadyStateChange_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _member;
};

struct RoomSetroomIsHidden_PC : public RoomSetroomIsHidden_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct RoomSetAllowSpectators_PC : public RoomSetAllowSpectators_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct RoomChangeNumSlots_PC : public RoomChangeNumSlots_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct KickedOutOfRoom_PC : public KickedOutOfRoom_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _member;
};

struct NewModerator_PC : public RoomNewModerator_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	LobbyClientUserId _member;
};

struct QuickMatchTimeout_PC : public QuickMatchTimeout_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct StartGameSuccess_PC : public StartGame_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
	LobbyClientTitleId _titleId;
};

struct ClanDissolved_PC : public ClanDissolved_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct LeaveClan_PC : public LeaveClan_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanJoinInvitationWithdrawn_PC : public ClanJoinInvitationWithdrawn_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanJoinInvitationRejected_PC : public ClanJoinInvitationRejected_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanJoinInvitationAccepted_PC : public ClanJoinInvitationAccepted_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanJoinInvitation_PC : public ClanJoinInvitation_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberJoinRequestWithdrawn_PC : public ClanMemberJoinRequestWithdrawn_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberJoinRequestRejected_PC : public ClanMemberJoinRequestRejected_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberJoinRequestAccepted_PC : public ClanMemberJoinRequestAccepted_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberJoinRequest_PC : public ClanMemberJoinRequest_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberRank_PC : public ClanMemberRank_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberKicked_PC : public ClanMemberKicked_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

struct ClanMemberUnblacklisted_PC : public ClanMemberUnblacklisted_Notification
{
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);
};

};

#endif
