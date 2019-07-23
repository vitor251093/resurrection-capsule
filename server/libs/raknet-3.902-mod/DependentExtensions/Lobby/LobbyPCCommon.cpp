#include "LobbyPCCommon.h"
#include "StringCompressor.h"
#include "DS_Table.h"
#include "BitStream.h"
#include "RakAssert.h"

using namespace RakNet;

LobbyRoom::LobbyRoom()
{
	titleName=0;
	roomName=0;
	rowInTable=0;
	roomPassword=0;
}
LobbyRoom::~LobbyRoom()
{
	if (titleName)
		delete [] titleName;
	if (roomName)
		delete [] roomName;
	if (roomPassword)
		delete [] roomPassword;
}
void LobbyRoom::Serialize(RakNet::BitStream *bs)
{
	unsigned i;
	bs->Write(rowId);
	bs->Write(moderator);

	bs->WriteCompressed(publicSlotMembers.Size());
	for (i=0; i < publicSlotMembers.Size(); i++)
		bs->Write(publicSlotMembers[i]);

	bs->WriteCompressed(privateSlotMembers.Size());
	for (i=0; i < privateSlotMembers.Size(); i++)
		bs->Write(privateSlotMembers[i]);

	bs->WriteCompressed(spectators.Size());
	for (i=0; i < spectators.Size(); i++)
		bs->Write(spectators[i]);

	bs->WriteCompressed(publicSlots);
	bs->WriteCompressed(privateSlots);

	stringCompressor->EncodeString(titleName, 512, bs, 0);
	stringCompressor->EncodeString(roomName, 512, bs, 0);

	bs->Write(allowSpectators);
	bs->Write(roomIsHidden);
	bs->Write(titleId);
}
bool LobbyRoom::Deserialize(RakNet::BitStream *bs)
{
	LobbyDBSpec::DatabaseKey key;
	unsigned int listSize, i;
	bs->Read(rowId);
	bs->Read(moderator);

	bs->ReadCompressed(listSize);
	for (i=0; i < listSize; i++)
	{
		bs->Read(key);
		publicSlotMembers.Insert(key);
	}

	bs->ReadCompressed(listSize);
	for (i=0; i < listSize; i++)
	{
		bs->Read(key);
		privateSlotMembers.Insert(key);
	}

	bs->ReadCompressed(listSize);
	for (i=0; i < listSize; i++)
	{
		bs->Read(key);
		spectators.Insert(key);
	}

	bs->ReadCompressed(publicSlots);
	bs->ReadCompressed(privateSlots);

	char titleNameIn[512];
	stringCompressor->DecodeString(titleNameIn, 512, bs, 0);
	if (titleName)
		delete [] titleName;
	titleName = new char [strlen(titleNameIn)+1];
	strcpy(titleName, titleNameIn);

	char roomNameIn[512];
	stringCompressor->DecodeString(roomNameIn, 512, bs, 0);
	if (roomName)
		delete [] roomName;
	roomName = new char [strlen(roomNameIn)+1];
	strcpy(roomName, roomNameIn);

	bs->Read(allowSpectators);
	bs->Read(roomIsHidden);
	return bs->Read(titleId);
}
bool LobbyRoom::RemoveRoomMember(LobbyDBSpec::DatabaseKey memberId, bool *gotNewModerator)
{
	unsigned i;
	*gotNewModerator=false;
	for (i=0; i < publicSlotMembers.Size(); i++)
	{
		if (publicSlotMembers[i]==memberId)
		{
			publicSlotMembers.RemoveAtIndex(i);
			if (memberId==moderator)
				*gotNewModerator=AutoPromoteModerator();
			return true;
		}
	}
	for (i=0; i < privateSlotMembers.Size(); i++)
	{
		if (privateSlotMembers[i]==memberId)
		{
			privateSlotMembers.RemoveAtIndex(i);
			if (memberId==moderator)
				*gotNewModerator=AutoPromoteModerator();
			return true;
		}
	}
	for (i=0; i < spectators.Size(); i++)
	{
		if (spectators[i]==memberId)
		{
			spectators.RemoveAtIndex(i);
			return true;
		}
	}
	return false;
}
bool LobbyRoom::AutoPromoteModerator(void)
{
	if (RoomIsDead()==false)
	{
		if (privateSlotMembers.Size())
			SetModerator(privateSlotMembers[0]);
		else
			SetModerator(publicSlotMembers[0]);
		return true;
	}
	return false;
}
int LobbyRoom::GetAvailablePublicSlots(void) const
{
	return (int)publicSlots-(int)publicSlotMembers.Size();
}
int LobbyRoom::GetAvailablePrivateSlots(void) const
{
	return (int)privateSlots-(int)privateSlotMembers.Size();
}
bool LobbyRoom::RoomIsDead(void) const
{
	return publicSlotMembers.Size()==0 && privateSlotMembers.Size()==0;
}
bool LobbyRoom::HasRoomMember(LobbyDBSpec::DatabaseKey memberId) const
{
	unsigned i;
	for (i=0; i < publicSlotMembers.Size(); i++)
	{
		if (publicSlotMembers[i]==memberId)
			return true;
	}
	for (i=0; i < privateSlotMembers.Size(); i++)
	{
		if (privateSlotMembers[i]==memberId)
			return true;
	}
	for (i=0; i < spectators.Size(); i++)
	{
		if (spectators[i]==memberId)
			return true;
	}
	return false;
}
void LobbyRoom::SetModerator(LobbyDBSpec::DatabaseKey newModerator)
{
#ifdef _DEBUG
	RakAssert(HasRoomMember(newModerator));
#endif

	moderator=newModerator;

	// Reserved slot invites only apply to that moderator
	ClearPrivateSlotInvites();
}
LobbyDBSpec::DatabaseKey LobbyRoom::GetModerator(void) const
{
	RakAssert(RoomIsDead()==false);
	return moderator;
}
bool LobbyRoom::AddRoomMemberToFirstAvailableSlot(LobbyDBSpec::DatabaseKey newMember)
{
	RemoveFromInviteList(newMember);
	if (privateSlotMembers.Size() < privateSlots)
		privateSlotMembers.Insert(newMember);
	else if (publicSlotMembers.Size()<publicSlots)
		publicSlotMembers.Insert(newMember);
	else
		return false;
	return true;
}
bool LobbyRoom::AddRoomMemberToPublicSlot(LobbyDBSpec::DatabaseKey newMember)
{
	RemoveFromInviteList(newMember);

#ifdef _DEBUG
	unsigned i;
	for (i=0; i < publicSlotMembers.Size(); i++)
	{
		if (publicSlotMembers[i]==newMember)
		{
			RakAssert(0);
		}
	}
#endif

	if (publicSlotMembers.Size()<publicSlots)
	{
		publicSlotMembers.Insert(newMember);
		return true;
	}
	return false;
}
bool LobbyRoom::AddRoomMemberToPrivateSlot(LobbyDBSpec::DatabaseKey newMember)
{
	RemoveFromInviteList(newMember);

#ifdef _DEBUG
	unsigned i;
	for (i=0; i < privateSlotMembers.Size(); i++)
	{
		if (privateSlotMembers[i]==newMember)
		{
			RakAssert(0);
		}
	}
#endif

	if (privateSlotMembers.Size()<privateSlots)
	{
		privateSlotMembers.Insert(newMember);
		return true;
	}
	return false;
}
void LobbyRoom::AddRoomMemberAsSpectator(LobbyDBSpec::DatabaseKey newMember)
{
	RemoveFromInviteList(newMember);

#ifdef _DEBUG
	unsigned i;
	for (i=0; i < spectators.Size(); i++)
	{
		if (spectators[i]==newMember)
		{
			RakAssert(0);
		}
	}
#endif

	spectators.Insert(newMember);
}
bool LobbyRoom::IsInPublicSlot(LobbyDBSpec::DatabaseKey member) const
{
	unsigned i;
	for (i=0; i < publicSlotMembers.Size(); i++)
		if (publicSlotMembers[i]==member)
			return true;
	return false;
}
bool LobbyRoom::IsInPrivateSlot(LobbyDBSpec::DatabaseKey member) const
{
	unsigned i;
	for (i=0; i < privateSlotMembers.Size(); i++)
		if (privateSlotMembers[i]==member)
			return true;
	return false;
}
bool LobbyRoom::IsInSpectatorSlot(LobbyDBSpec::DatabaseKey member) const
{
	unsigned i;
	for (i=0; i < spectators.Size(); i++)
		if (spectators[i]==member)
			return true;
	return false;
}
void LobbyRoom::SetRoomName(const char *newRoomName)
{
	if (roomName)
		delete [] roomName;
	if (newRoomName && newRoomName[0])
	{
		roomName = new char [strlen(newRoomName)+1];
		strcpy(roomName,newRoomName);
	}
	else
		roomName=0;
}
void LobbyRoom::SetTitleName(const char *newTitleName)
{
	if (titleName)
		delete [] titleName;
	if (newTitleName && newTitleName[0])
	{
		titleName = new char [strlen(newTitleName)+1];
		strcpy(titleName,newTitleName);
	}
	else
		titleName=0;
}
void LobbyRoom::SetRoomPassword(const char *newRoomPassword)
{
	if (roomPassword)
		delete [] roomPassword;
	if (newRoomPassword && newRoomPassword[0])
	{
		roomPassword = new char [strlen(newRoomPassword)+1];
		strcpy(roomPassword,newRoomPassword);
	}
	else
		roomPassword=0;	
}
void LobbyRoom::Clear(void)
{
	if (roomName)
		delete [] roomName;
	roomName=0;
	if (roomPassword)
		delete [] roomPassword;
	roomPassword=0;
	privateSlotMembers.Clear();
	spectators.Clear();
	publicSlotMembers.Clear();
}
void LobbyRoom::WriteToRow(bool writeRoomName)
{
	if (writeRoomName) // Just for efficiency so we don't keep reallocating pointlessly
	{
		rowInTable->cells[DefaultTableColumns::TC_TITLE_NAME]->Set(titleName);
		rowInTable->cells[DefaultTableColumns::TC_ROOM_NAME]->Set(roomName);
	}
	rowInTable->cells[DefaultTableColumns::TC_ROOM_ID]->Set(rowId);
	rowInTable->cells[DefaultTableColumns::TC_ALLOW_SPECTATORS]->Set(allowSpectators);
	rowInTable->cells[DefaultTableColumns::TC_ROOM_HIDDEN]->Set(roomIsHidden);
	rowInTable->cells[DefaultTableColumns::TC_TOTAL_SLOTS]->Set(publicSlots+privateSlots);
	rowInTable->cells[DefaultTableColumns::TC_TOTAL_PUBLIC_SLOTS]->Set(publicSlots);
	rowInTable->cells[DefaultTableColumns::TC_TOTAL_PRIVATE_SLOTS]->Set(privateSlots);
	rowInTable->cells[DefaultTableColumns::TC_OPEN_PUBLIC_SLOTS]->Set(publicSlots-publicSlotMembers.Size() < 0 ? 0 : publicSlots-publicSlotMembers.Size() < 0);
	rowInTable->cells[DefaultTableColumns::TC_OPEN_PRIVATE_SLOTS]->Set(privateSlots-privateSlotMembers.Size() < 0 ? 0 : privateSlots-privateSlotMembers.Size() < 0);
	rowInTable->cells[DefaultTableColumns::TC_USED_SLOTS]->Set(publicSlotMembers.Size() + privateSlotMembers.Size());
	rowInTable->cells[DefaultTableColumns::TC_USED_PUBLIC_SLOTS]->Set(publicSlotMembers.Size());
	rowInTable->cells[DefaultTableColumns::TC_USED_PRIVATE_SLOTS]->Set(privateSlotMembers.Size());
	rowInTable->cells[DefaultTableColumns::TC_USED_SPECTATE_SLOTS]->Set(spectators.Size());
	rowInTable->cells[DefaultTableColumns::TC_LOBBY_ROOM_PTR]->SetPtr(this);
	rowInTable->cells[DefaultTableColumns::TC_TITLE_ID]->Set(titleId);	
}
void LobbyRoom::ClearPrivateSlotInvites(void)
{
	reservedSlotInvites.Clear();
}
bool LobbyRoom::RemoveFromInviteList(LobbyDBSpec::DatabaseKey newMember)
{
	// Remove from the invite list, since they are joining the room.
	unsigned i;
	for (i=0; i < reservedSlotInvites.Size(); i++)
	{
		if (reservedSlotInvites[i]==newMember)
		{
			reservedSlotInvites.RemoveAtIndex(i);
			return true;
		}
	}
	return false;

}
bool LobbyRoom::HasPrivateSlotInvite(LobbyDBSpec::DatabaseKey invitee) const
{
	unsigned i;
	for (i=0; i < reservedSlotInvites.Size(); i++)
	{
		if (reservedSlotInvites[i]==invitee)
			return true;
	}
	return false;
}
bool LobbyRoom::AddPrivateSlotInvite(LobbyDBSpec::DatabaseKey invitee)
{
	if (HasPrivateSlotInvite(invitee)==false)
	{
		if (reservedSlotInvites.Size()<256)
		{
			reservedSlotInvites.Insert(invitee);
			return true;
		}
		else
		{
			// Some kind of bug or hacker or something
			RakAssert(0);
		}
	}
	return false;
}
int LobbyTitleCompByDatabaseKey( TitleValidationDBSpec::DatabaseKey const &key, RakNet::LobbyRoomsTableContainer * const &data )
{
	if (key < data->titleId)
		return -1;
	else if (key==data->titleId)
		return 0;
	return 1;
}

LobbyRoomsTableContainer::LobbyRoomsTableContainer()
{
	roomsTable=0;
	nextRowId=0;
}
LobbyRoomsTableContainer::~LobbyRoomsTableContainer()
{
	LobbyRoom *room;
	if (roomsTable)
	{
		int i;
		DataStructures::Page<unsigned, DataStructures::Table::Row*, _TABLE_BPLUS_TREE_ORDER> *cur = roomsTable->GetListHead();
		while (cur)
		{
			for (i=0; i < cur->size; i++)
			{
				room = (LobbyRoom *) cur->data[i]->cells[DefaultTableColumns::TC_LOBBY_ROOM_PTR]->ptr;
				delete room;
			}

			cur=cur->next;
		}

		delete roomsTable;
	}
}
void LobbyRoomsTableContainer::Initialize(TitleValidationDBSpec::DatabaseKey _titleId)
{
	titleId=_titleId;
	roomsTable = new DataStructures::Table;

	// Add default required columns, must be added first so indices are correct for fast lookup
	unsigned i;
	for (i=0; i < DefaultTableColumns::TC_TABLE_COLUMNS_COUNT; i++)
	{
		roomsTable->AddColumn(DefaultTableColumns::GetColumnName(i), DefaultTableColumns::GetColumnType(i));
	}
}

unsigned int LobbyRoomsTableContainer::GetFreeRowId(void)
{
	if (roomsTable)
	{
		nextRowId++;
		if (roomsTable->GetRowByID(nextRowId)==0)
		{
			return nextRowId-1;
		}
		else
		{
			nextRowId = roomsTable->GetAvailableRowId()+1;
			return nextRowId-1;
		}
	}
	else
	{
		nextRowId++;
		return nextRowId-1;
	}
}
void FriendInvitation_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(_invitor);
	bs->Write(_invitee);
	bs->Write(messageBodyEncodingLanguage);	
	stringCompressor->EncodeString(messageBody,sizeof(messageBody),bs,messageBodyEncodingLanguage);
	stringCompressor->EncodeString(invitorHandle,sizeof(invitorHandle),bs,messageBodyEncodingLanguage);
	stringCompressor->EncodeString(inviteeHandle,sizeof(inviteeHandle),bs,messageBodyEncodingLanguage);
	bs->Write(request);
	bs->Write(accepted);
}
bool FriendInvitation_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(_invitor);
	invitor=&_invitor;
	bs->Read(_invitee);
	invitee=&_invitee;
	bs->Read(messageBodyEncodingLanguage);	
	stringCompressor->DecodeString(messageBody,sizeof(messageBody),bs,messageBodyEncodingLanguage);
	stringCompressor->DecodeString(invitorHandle,sizeof(invitorHandle),bs,messageBodyEncodingLanguage);
	stringCompressor->DecodeString(inviteeHandle,sizeof(inviteeHandle),bs,messageBodyEncodingLanguage);
	bs->Read(request);
	return bs->Read(accepted);
}
void RoomMemberDrop_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(memberAddress);
	bs->Write(_droppedMember);
	bs->Write(wasKicked);
	bs->Write(gotNewModerator);
	if (gotNewModerator)
		bs->Write(_newModerator);
}
bool RoomMemberDrop_PC::Deserialize(RakNet::BitStream *bs)
{
	bool b;
	bs->Read(memberAddress);
	bs->Read(_droppedMember);
	bs->Read(wasKicked);
	b=bs->Read(gotNewModerator);
	if (gotNewModerator)
		b=bs->Read(_newModerator);
	droppedMember=&_droppedMember;
	newModerator=&_newModerator;
	return b;
}
void RoomMemberJoin_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(memberAddress);
	bs->Write(_joinedMember);
	bs->Write(isSpectator);
	bs->Write(isInOpenSlot);
	bs->Write(isInReservedSlot);
}
bool RoomMemberJoin_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(memberAddress);
	bs->Read(_joinedMember);
	joinedMember=&_joinedMember;
	bs->Read(isSpectator);
	bs->Read(isInOpenSlot);
	return bs->Read(isInReservedSlot);
}
void RoomMemberReadyStateChange_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(_member);
	bs->Write(isReady);
}
bool RoomMemberReadyStateChange_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(_member);
	member=&_member;
	return bs->Read(isReady);
}
void RoomSetroomIsHidden_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(isroomIsHidden);
}
bool RoomSetroomIsHidden_PC::Deserialize(RakNet::BitStream *bs)
{
	return bs->Read(isroomIsHidden);;
}
void RoomSetAllowSpectators_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(allowSpectators);
}
bool RoomSetAllowSpectators_PC::Deserialize(RakNet::BitStream *bs)
{
	return bs->Read(allowSpectators);
}
void RoomChangeNumSlots_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WriteCompressed(newPublicSlots);
	bs->WriteCompressed(newPrivateSlots);
}
bool RoomChangeNumSlots_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadCompressed(newPublicSlots);
	return bs->ReadCompressed(newPrivateSlots);
}
void KickedOutOfRoom_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(roomWasDestroyed);
	bs->Write(_member);
	stringCompressor->EncodeString(memberHandle, 512, bs, 0);
}
bool KickedOutOfRoom_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->Read(roomWasDestroyed);
	bs->Read(_member);
	member=&_member;
	return stringCompressor->DecodeString(memberHandle, 512, bs, 0);
}
void NewModerator_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(_member);
}
bool NewModerator_PC::Deserialize(RakNet::BitStream *bs)
{
	bool b;
	b=bs->Read(_member);
	member=&_member;
	return b;
}
void QuickMatchTimeout_PC::Serialize(RakNet::BitStream *bs)
{

}
bool QuickMatchTimeout_PC::Deserialize(RakNet::BitStream *bs)
{
	return true;
}
void StartGameSuccess_PC::Serialize(RakNet::BitStream *bs)
{
	// Fill this out by hand on the server code, too complex here
}
bool StartGameSuccess_PC::Deserialize(RakNet::BitStream *bs)
{
	return true;
}

void ClanDissolved_PC::Serialize(RakNet::BitStream *bs)
{
	bs->Write(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
	bs->Write(memberIds.Size());
	unsigned i;
	for (i=0; i < memberIds.Size(); i++)
	{
		bs->Write(*(memberIds[i]));
	}
}
bool ClanDissolved_PC::Deserialize(RakNet::BitStream *bs)
{
	bool b;
	bs->Read(clanId);
	stringCompressor->DecodeString(&clanHandle, 512, bs, 0);

	unsigned arraySize;
	LobbyClientUserId *lcui;
	unsigned i;
	b=bs->Read(arraySize);
	if (b==false) return false;
	LobbyClientUserId temp;
	RakNet::RakString tempStr;
	for (i=0; i < arraySize; i++)
	{
		lcui = new LobbyClientUserId;
		b=bs->Read(temp);
		*lcui=temp;
		stringCompressor->DecodeString(&tempStr, 512, bs, 0);
		memberIds.Insert(lcui);
	}

	return b;
}
void LeaveClan_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool LeaveClan_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}

void ClanJoinInvitationWithdrawn_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanJoinInvitationWithdrawn_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanJoinInvitationRejected_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanJoinInvitationRejected_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanJoinInvitationAccepted_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);

	bs->Write(userIdIsYou);
	bs->Write(invitationSenderIsYou);
}
bool ClanJoinInvitationAccepted_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	stringCompressor->DecodeString(&clanHandle, 512, bs, 0);

	bs->Read(userIdIsYou);
	return bs->Read(invitationSenderIsYou);
}
void ClanJoinInvitation_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanJoinInvitation_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanMemberJoinRequestWithdrawn_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanMemberJoinRequestWithdrawn_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanMemberJoinRequestRejected_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanMemberJoinRequestRejected_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanMemberJoinRequestAccepted_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);

	bs->Write(userIdIsYou);
	bs->Write(joinRequestAcceptorIsYou);
}
bool ClanMemberJoinRequestAccepted_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	stringCompressor->DecodeString(&clanHandle, 512, bs, 0);

	bs->Read(userIdIsYou);
	return bs->Read(joinRequestAcceptorIsYou);
}
void ClanMemberJoinRequest_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanId);
	stringCompressor->EncodeString(&clanHandle, 512, bs, 0);
}
bool ClanMemberJoinRequest_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanId);
	return stringCompressor->DecodeString(&clanHandle, 512, bs, 0);
}
void ClanMemberRank_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->Write(rank);
}
bool ClanMemberRank_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	return bs->Read(rank);
}
void ClanMemberKicked_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->Write(wasBanned);
	bs->Write(wasKicked);
}
bool ClanMemberKicked_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);

	bs->Read(wasBanned);
	return bs->Read(wasKicked);
}
void ClanMemberUnblacklisted_PC::Serialize(RakNet::BitStream *bs)
{
	bs->WritePtr(userId);
	stringCompressor->EncodeString(&userHandle, 512, bs, 0);

	bs->WritePtr(clanLeaderOrSubleaderId);
	stringCompressor->EncodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);
}
bool ClanMemberUnblacklisted_PC::Deserialize(RakNet::BitStream *bs)
{
	bs->ReadPtr(userId);
	stringCompressor->DecodeString(&userHandle, 512, bs, 0);

	bs->ReadPtr(clanLeaderOrSubleaderId);
	return stringCompressor->DecodeString(&clanLeaderOrSubleaderHandle, 512, bs, 0);
}