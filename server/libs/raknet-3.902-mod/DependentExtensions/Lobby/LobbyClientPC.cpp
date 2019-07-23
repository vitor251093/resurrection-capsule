#include "LobbyClientPC.h"
#include "StringCompressor.h"
#include "RakAssert.h"
#include "TableSerializer.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"

using namespace RakNet;

static const int MAX_BINARY_DATA_LENGTH=10000000;


// alloca
#ifdef _CONSOLE_1
#elif defined(_WIN32)
#include <malloc.h>
#else
#include <alloca.h>
#include <stdlib.h>
#endif

#ifdef _MSC_VER
#pragma warning( push )
#endif

LobbyClientUser::LobbyClientUser()
{
	readyToPlay=false;
	userData=0;
}
LobbyClientUser::~LobbyClientUser()
{
	if (userData)
		delete userData;
}
LobbyClientPC::LobbyClientPC()
{
	orderingChannel=0;
	cb=0;
	rakPeer=0;
	serverAddr=UNASSIGNED_SYSTEM_ADDRESS;
	packetPriority=MEDIUM_PRIORITY;
	downloadingRooms=false;
	titleIdStr=0;
	titleLoginPW=0;
	titleLoginPWLength=0;
	InitializeVariables();
}
LobbyClientPC::~LobbyClientPC()
{
	if (titleIdStr)
		delete [] titleIdStr;
	if (titleLoginPW)
		delete [] titleLoginPW;
}
void LobbyClientPC::Login(const char *userHandle, const char *userPassword, SystemAddress lobbyServerAddr)
{
	RakAssert(rakPeer);
	RakAssert(lobbyServerAddr!=UNASSIGNED_SYSTEM_ADDRESS);
	RakAssert(titleLoginPW || titleLoginPWLength==0);
	serverAddr=lobbyServerAddr;

	if (rakPeer->GetIndexFromSystemAddress(lobbyServerAddr)==-1)
	{
		if (cb)
			cb->Login_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	if (lobbyUserStatus==LOBBY_USER_STATUS_LOGGING_ON)
	{
		if (cb)
			cb->Login_Result(LC_RC_IN_PROGRESS);
		return;
	}

	if (IsConnected())
	{
		if (cb)
			cb->Login_Result(LC_RC_SUCCESS);
		return;
	}

	if (lobbyUserStatus!=LOBBY_USER_STATUS_OFFLINE)
	{
		if (cb)
			cb->Login_Result(LC_RC_BUSY);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_LOGIN);
	stringCompressor->EncodeString(userPassword, 512, &bs, 0);
	stringCompressor->EncodeString(userHandle, 512, &bs, 0);
	if (Send(&bs))
		lobbyUserStatus=LOBBY_USER_STATUS_LOGGING_ON;
}
void LobbyClientPC::SetTitleLoginId(const char *_titleIdStr, const char *_titleLoginPW, int _titleLoginPWLength)
{
	if (_titleLoginPW==0 || _titleLoginPWLength==0)
	{
		if (cb)
			cb->SetTitleLoginId_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Login is empty.", 0 ) ));
		return;
	}

	if (titleIdStr)
		delete [] titleIdStr;
	if (titleLoginPW)
		delete [] titleLoginPW;

	if (_titleIdStr && _titleIdStr[0])
	{
		titleIdStr = new char[strlen(_titleIdStr)+1];
		strcpy(titleIdStr,_titleIdStr);
	}
	else
		titleIdStr=0;

	if (_titleLoginPW && _titleLoginPWLength>0)
	{
		titleLoginPW = new char[strlen(_titleLoginPW)];
		memcpy(titleLoginPW,_titleLoginPW,_titleLoginPWLength);
		titleLoginPWLength=_titleLoginPWLength;
	}
	else
	{
		titleLoginPW=0;
		titleLoginPWLength=0;
	}

	if (IsConnected())
	{
		SendSetTitleLoginId();
	}
	else
	{
		if (cb)
			cb->SetTitleLoginId_Result(LC_RC_SUCCESS);
		return;
	}
}
void LobbyClientPC::SendSetTitleLoginId(void)
{
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SET_TITLE_LOGIN_ID);
	bs.WriteAlignedBytesSafe((const char*) titleLoginPW,titleLoginPWLength,512);
	stringCompressor->EncodeString(titleIdStr, 512, &bs, 0);
	Send(&bs);
}
void LobbyClientPC::Logoff(void)
{
	if (lobbyUserStatus==LOBBY_USER_STATUS_OFFLINE)
	{
		if (cb)
			cb->Logoff_Result(LC_RC_SUCCESS);
		return;
	}

	if (lobbyUserStatus==LOBBY_USER_STATUS_LOGGING_OFF)
	{
		if (cb)
			cb->Logoff_Result(LC_RC_IN_PROGRESS);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_LOGOFF);
	Send(&bs);
}
bool LobbyClientPC::IsConnected(void) const
{
	return lobbyUserStatus!=LOBBY_USER_STATUS_OFFLINE && lobbyUserStatus!=LOBBY_USER_STATUS_LOGGING_ON;
}
void LobbyClientPC::RegisterAccount(LobbyDBSpec::CreateUser_Data *input, SystemAddress lobbyServerAddr)
{
	RakAssert(input);

	serverAddr = lobbyServerAddr;

	if (titleLoginPW==0 || titleLoginPWLength==0)
	{
		if (cb)
			cb->RegisterAccount_Result(LC_RC_BAD_TITLE_ID);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_REGISTER_ACCOUNT);
	bs.WriteAlignedBytesSafe(titleLoginPW,titleLoginPWLength,512);
	stringCompressor->EncodeString(titleIdStr, 512, &bs, 0);
	input->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::UpdateAccount(LobbyDBSpec::UpdateUser_Data *input)
{
	RakAssert(input);

	if (IsConnected()==false)
	{
		if (cb)
			cb->UpdateAccount_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_UPDATE_ACCOUNT);
	input->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::ChangeHandle(const char *newHandle)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->ChangeHandle_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CHANGE_HANDLE);
	stringCompressor->EncodeString(newHandle, 512, &bs, 0);
	Send(&bs);
}
void LobbyClientPC::DownloadFriends(bool ascendingSortByDate)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadFriends_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_FRIENDS);
	bs.Write(ascendingSortByDate);
	Send(&bs);
}
void LobbyClientPC::SendAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SendAddFriendInvite_Result(LC_RC_NOT_CONNECTED_TO_SERVER, friendHandle, friendId);
		return;
	}

	if (friendId && IsFriend(*friendId))
	{
		// Already a friend
		if (cb)
			cb->SendAddFriendInvite_Result(LC_RC_SUCCESS, friendHandle, friendId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SEND_ADD_FRIEND_INVITE);
	SerializeHandleOrId(friendHandle,friendId,&bs);
	bs.Write(language);
	stringCompressor->EncodeString(messageBody, 4096, &bs, language);
	Send(&bs);
}
void LobbyClientPC::AcceptAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->AcceptAddFriendInvite_Result(LC_RC_NOT_CONNECTED_TO_SERVER, friendHandle, friendId);
		return;
	}

	if (IsFriend(*friendId))
	{
		// Already a friend
		if (cb)
			cb->AcceptAddFriendInvite_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Already a friend.", 0 ) ), friendHandle, friendId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ACCEPT_ADD_FRIEND_INVITE);
	bs.Write(language);
	stringCompressor->EncodeString(friendHandle, 512, &bs, language);
	bs.Write(*friendId);
	stringCompressor->EncodeString(messageBody, 4096, &bs, language);
	Send(&bs);
}
void LobbyClientPC::DeclineAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DeclineAddFriendInvite_Result(LC_RC_NOT_CONNECTED_TO_SERVER, friendHandle, friendId);
		return;
	}

	if (IsFriend(*friendId))
	{
		// Doesn't make sense to decline a friend inviation from someone who is already your friend
		if (cb)
			cb->DeclineAddFriendInvite_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Specified user is currently a friend.", 0 ) ), friendHandle, friendId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DECLINE_ADD_FRIEND_INVITE);
	bs.Write(language);
	stringCompressor->EncodeString(friendHandle, 512, &bs, language);
	bs.Write(*friendId);
	stringCompressor->EncodeString(messageBody, 4096, &bs, language);
	Send(&bs);
}
const DataStructures::List<LC_Friend *>& LobbyClientPC::GetFriendsList(void) const
{
	return friendsList;
}
bool LobbyClientPC::IsFriend(const char *friendHandle)
{
	unsigned i;
	for (i=0; i < friendsList.Size(); i++)
	{
		if (stricmp(friendsList[i]->handle, friendHandle)==0)
			return true;
	}
	return false;
}
bool LobbyClientPC::IsFriend(LobbyClientUserId *friendId)
{
	return IsFriend(*friendId);
}
void LobbyClientPC::DownloadIgnoreList(bool ascendingSort)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadIgnoreList_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_IGNORE_LIST);
	bs.Write(ascendingSort);
	Send(&bs);
}
void LobbyClientPC::AddToOrUpdateIgnoreList(const char *userHandle, LobbyClientUserId *userId, const char *actionsStr)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->AddToOrUpdateIgnoreList_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_UPDATE_IGNORE_LIST);
	SerializeHandleOrId(userHandle,userId,&bs);
	stringCompressor->EncodeString(actionsStr,4096,&bs,0);
	Send(&bs);
}
void LobbyClientPC::RemoveFromIgnoreList(LobbyClientUserId *userId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->RemoveFromIgnoreList_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_REMOVE_FROM_IGNORE_LIST);
	bs.Write(*userId);
	Send(&bs);
}
const DataStructures::List<LC_Ignored *>& LobbyClientPC::GetIgnoreList(void) const
{
	return ignoreList;
}
void LobbyClientPC::DownloadEmails(bool ascendingSort, bool inbox, unsigned char language)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadEmails_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}


	// Used to automatically redownload sent emails after sending an email
	hasCalledGetEmails=true;
	lastGetEmailsSortIsAscending=ascendingSort;
	lastGetEmailsLangauge=language;

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_EMAILS);
	bs.Write(language);
	bs.Write(ascendingSort);
	bs.Write(inbox);
	Send(&bs);
}
void LobbyClientPC::SendEmail(LobbyDBSpec::SendEmail_Data *input, unsigned char language)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SendEmail_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakAssert(input);
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SEND_EMAIL);
	bs.Write(language);
	input->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::DeleteEmail(EmailId *emailId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DeleteEmail_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakAssert(emailId);
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DELETE_EMAIL);
	bs.Write(*emailId);
	Send(&bs);
}
void LobbyClientPC::UpdateEmailStatus(EmailId *emailId, int newStatus, bool wasOpened)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->UpdateEmailStatus_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakAssert(emailId);
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_UPDATE_EMAIL_STATUS);
	bs.Write(*emailId);
	bs.Write(newStatus);
	bs.Write(wasOpened);
	Send(&bs);
}
const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & LobbyClientPC::GetInboxEmailList( void ) const
{
	return inboxEmails;
}
const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & LobbyClientPC::GetSentEmailList( void ) const
{
	return sentEmails;
}
void LobbyClientPC::SendIM(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SendIM_Result(LC_RC_NOT_CONNECTED_TO_SERVER, userHandle, userId);
		return;
	}
	if ((chatMessage==0 || chatMessage[0]==0) && chatBinaryData==0)
	{
		if (cb)
			cb->SendIM_Result(LobbyClientCBResult(LC_RC_INVALID_INPUT, "Message is empty.", 0 ), userHandle, userId );
		return;
	}
	if (userId && *userId==myUserId)
	{
		if (cb)
			cb->SendIM_Result(LobbyClientCBResult(LC_RC_INVALID_INPUT, "Cannot IM yourself.", 0 ), userHandle, userId );
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SEND_IM);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(languageId);
	stringCompressor->EncodeString(chatMessage,4096,&bs,languageId);
	bs.WriteAlignedBytesSafe((const char*) chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);
	Send(&bs);
}
LobbyUserStatus LobbyClientPC::GetLobbyUserStatus(void) const
{
	return lobbyUserStatus;
}
void LobbyClientPC::RetrievePasswordRecoveryQuestion(const char *userHandle, SystemAddress lobbyServerAddr)
{
	RakAssert(userHandle && userHandle[0]);
	RakAssert(lobbyServerAddr!=UNASSIGNED_SYSTEM_ADDRESS);

	serverAddr = lobbyServerAddr;

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_RETRIEVE_PASSWORD_RECOVERY_QUESTION);
	stringCompressor->EncodeString(userHandle, 512, &bs, 0);
	Send(&bs);
}
void LobbyClientPC::RetrievePassword(const char *userHandle, const char *passwordRecoveryAnswer, SystemAddress lobbyServerAddr)
{
	RakAssert(userHandle && userHandle[0] &&
		passwordRecoveryAnswer && passwordRecoveryAnswer[0]);
	RakAssert(lobbyServerAddr!=UNASSIGNED_SYSTEM_ADDRESS);

	serverAddr = lobbyServerAddr;

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_RETRIEVE_PASSWORD);
	stringCompressor->EncodeString(userHandle, 512, &bs, 0);
	stringCompressor->EncodeString(passwordRecoveryAnswer, 512, &bs, 0);
	Send(&bs);
}
bool LobbyClientPC::IsInRoom(void) const
{
	return lobbyUserStatus==LOBBY_USER_STATUS_IN_ROOM;
}
void LobbyClientPC::CreateRoom(const char *roomName, const char *roomPassword, bool roomIsHidden, int publicSlots, int privateSlots, bool allowSpectators,
							   DataStructures::Table *customProperties)
{
	if (IsInRoom())
	{
		if (cb)
			cb->CreateRoom_Result(LC_RC_ALREADY_IN_ROOM);
		return;
	}
	else if (IsConnected()==false)
	{
		if (cb)
			cb->CreateRoom_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	else if (lobbyUserStatus==LOBBY_USER_STATUS_CREATING_ROOM)
	{
		if (cb)
			cb->CreateRoom_Result(LC_RC_IN_PROGRESS);
		return;
	}
	else if (lobbyUserStatus!=LOBBY_USER_STATUS_IN_LOBBY_IDLE)
	{
		if (cb)
			cb->CreateRoom_Result(LC_RC_BUSY);
		return;
	}
	else if (publicSlots+privateSlots<2)
	{
		if (cb)
			cb->CreateRoom_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Must have at least 2 slots.", 0 ) ));
		return;
	}
	else if (roomName==0 || roomName[0]==0)
	{
		if (cb)
			cb->CreateRoom_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Must specify a room name.", 0 ) ));
		return;
	}
	else if (customProperties)
	{
		// Check that reserved column names were not used
		unsigned i,j;
		for (j=0; j < customProperties->GetColumns().Size(); j++)
		{
			for (i=0; i < DefaultTableColumns::TC_LOBBY_ROOM_PTR; i++)
			{
				if (stricmp(DefaultTableColumns::GetColumnName(i),customProperties->GetColumns()[j].columnName)==0)
				{
					if (cb)
						cb->CreateRoom_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Cannot use reserved column names.", 0 ) ));
					return;
				}
			}
		}
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CREATE_ROOM);
	stringCompressor->EncodeString(roomName, 512, &bs, 0);
	stringCompressor->EncodeString(roomPassword, 512, &bs, 0);
	bs.Write(roomIsHidden);
	bs.WriteCompressed(publicSlots);
	bs.WriteCompressed(privateSlots);
	bs.Write(allowSpectators);
	if (customProperties)
	{
		bs.Write(true);
		TableSerializer::SerializeTable(customProperties, &bs);
	}
	else
	{
		bs.Write(false);
	}

	Send(&bs);
}
void LobbyClientPC::DownloadRooms(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters)
{
	RakNet::BitStream bs;
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadRooms_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	if (downloadingRooms)
	{
		if (cb)
			cb->DownloadRooms_Result(LC_RC_IN_PROGRESS);
		return;
	}

	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_ROOMS);
	TableSerializer::SerializeFilterQueryList(&bs, inclusionFilters, numInclusionFilters, RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS);

	Send(&bs);
	downloadingRooms=true;
}
DataStructures::Table * LobbyClientPC::GetAllRooms(void)
{
	return &allRooms;
}
bool LobbyClientPC::IsDownloadingRoomList(void)
{
	return downloadingRooms;
}
void LobbyClientPC::LeaveRoom(void)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->LeaveRoom_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->LeaveRoom_Result(LC_RC_NOT_IN_ROOM);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_LEAVE_ROOM);
	Send(&bs);
}
const DataStructures::List<LobbyClientUserId *>& LobbyClientPC::GetAllRoomMembers(void) const
{
	RakAssert(IsInRoom());
	static DataStructures::List<LobbyClientUserId *> output;
	output.Clear(true);
	unsigned i;
	for (i=0; i < currentRoom.publicSlotMembers.Size(); i++)
		output.Insert(& currentRoom.publicSlotMembers[i]);
	for (i=0; i < currentRoom.privateSlotMembers.Size(); i++)
		output.Insert(& currentRoom.privateSlotMembers[i]);
	for (i=0; i < currentRoom.spectators.Size(); i++)
		output.Insert(& currentRoom.spectators[i]);
	return output;
}
const DataStructures::List<LobbyClientUserId *>& LobbyClientPC::GetOpenSlotMembers(void) const
{
	RakAssert(IsInRoom());
	static DataStructures::List<LobbyClientUserId *> output;
	output.Clear(true);
	unsigned i;
	for (i=0; i < currentRoom.publicSlotMembers.Size(); i++)
		output.Insert(& currentRoom.publicSlotMembers[i]);
	return output;
}
const DataStructures::List<LobbyClientUserId *>& LobbyClientPC::GetReservedSlotMembers(void) const
{
	RakAssert(IsInRoom());
	static DataStructures::List<LobbyClientUserId *> output;
	output.Clear(true);
	unsigned i;
	for (i=0; i < currentRoom.privateSlotMembers.Size(); i++)
		output.Insert(& currentRoom.privateSlotMembers[i]);
	return output;
}
const DataStructures::List<LobbyClientUserId *>& LobbyClientPC::GetSpectators(void) const
{
	RakAssert(IsInRoom());
	static DataStructures::List<LobbyClientUserId *> output;
	output.Clear(true);
	unsigned i;
	for (i=0; i < currentRoom.spectators.Size(); i++)
		output.Insert(& currentRoom.spectators[i]);
	return output;
}
LobbyClientUserId* LobbyClientPC::GetRoomModerator(void)
{
	RakAssert(IsInRoom());
	return &currentRoom.moderator;
}
LobbyClientRoomId* LobbyClientPC::GetRoomId(void)
{
	RakAssert(IsInRoom());
	return &currentRoom.rowId;
}
unsigned int LobbyClientPC::GetPublicSlotMaxCount(void) const
{
	RakAssert(IsInRoom());
	return currentRoom.publicSlots;
}
unsigned int LobbyClientPC::GetPrivateSlotMaxCount(void) const
{
	RakAssert(IsInRoom());
	return currentRoom.privateSlots;
}
LobbyClientTitleId* LobbyClientPC::GetTitleId(void) const
{
	RakAssert(IsInRoom());
	return (LobbyClientTitleId*) &currentRoom.titleId;
}
const char *LobbyClientPC::GetRoomName(void) const
{
	RakAssert(IsInRoom());
	return currentRoom.roomName;
}
const char *LobbyClientPC::GetTitleName(void) const
{
	RakAssert(IsInRoom());
	return currentRoom.titleName;
}
bool LobbyClientPC::RoomIsHidden(void) const
{
	RakAssert(IsInRoom());
	return currentRoom.roomIsHidden;
}
DataStructures::Table::Row *LobbyClientPC::GetRoomTableRow(void) const
{
	RakAssert(IsInRoom());
	return allRooms.GetRowByID(currentRoom.rowId);
}
void LobbyClientPC::JoinRoom(LobbyClientRoomId* roomId, const char *roomName, const char *roomPassword, bool joinAsSpectator)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->JoinRoom_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom())
	{
		if (cb)
			cb->JoinRoom_Result(LC_RC_ALREADY_IN_ROOM);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_JOIN_ROOM);
	bs.Write(*roomId);
	stringCompressor->EncodeString(roomName,512,&bs,0);
	bs.Write(joinAsSpectator);
	stringCompressor->EncodeString(roomPassword,512,&bs,0);
	Send(&bs);
}
void LobbyClientPC::JoinRoomByFilter(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters, bool joinAsSpectator)
{
	RakNet::BitStream bs;
	if (IsConnected()==false)
	{
		if (cb)
			cb->JoinRoomByFilter_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom())
	{
		if (cb)
			cb->JoinRoomByFilter_Result(LC_RC_ALREADY_IN_ROOM);
		return;
	}

	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_JOIN_ROOM_BY_FILTER);
	TableSerializer::SerializeFilterQueryList(&bs, inclusionFilters, numInclusionFilters, RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS);
	bs.Write(joinAsSpectator);

	Send(&bs);
}
void LobbyClientPC::RoomChat(const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->RoomChat_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->RoomChat_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (chatMessage==0 || chatMessage[0]==0)
	{
		if (cb)
			cb->RoomChat_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Chat message is empty.", 0 ) ));
		return;
	}

	// Just indicate success locally, to avoid a probably unnecessary send
	if (cb)
		cb->RoomChat_Result(LC_RC_SUCCESS);

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ROOM_CHAT);
	bs.Write(languageId);
	stringCompressor->EncodeString(chatMessage, 4096, &bs, languageId);
	bs.WriteAlignedBytesSafe((const char*) chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);
	Send(&bs);
}
bool LobbyClientPC::IsInReservedSlot(LobbyClientUserId *userId)
{
	return currentRoom.IsInPrivateSlot(*userId);
}
bool LobbyClientPC::IsRoomModerator(LobbyClientUserId *userId)
{
	return currentRoom.GetModerator()==*userId;
}
bool LobbyClientPC::IsUserInRoom(LobbyClientUserId *userId) const
{
	return userId && (currentRoom.IsInPrivateSlot(*userId) || currentRoom.IsInPublicSlot(*userId) || currentRoom.IsInSpectatorSlot(*userId));
}
bool LobbyClientPC::RoomAllowsSpectators(void) const
{
	return currentRoom.allowSpectators;
}
void LobbyClientPC::InviteToRoom(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId,
								 const char *chatBinaryData, int chatBinaryLength, bool inviteToPrivateSlot)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->InviteToRoom_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->InviteToRoom_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false && inviteToPrivateSlot)
	{
		if (cb)
			cb->InviteToRoom_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	if (userId && IsUserInRoom(userId)==true)
	{
		if (cb)
			cb->InviteToRoom_Result(LC_RC_USER_ALREADY_IN_ROOM);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_INVITE_TO_ROOM);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(inviteToPrivateSlot);
	bs.Write(languageId);
	stringCompressor->EncodeString(chatMessage, 4096, &bs, languageId);
	bs.WriteAlignedBytesSafe((const char*) chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);
	Send(&bs);
}
void LobbyClientPC::SetReadyToPlayStatus(bool isReady)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SetReadyToPlayStatus_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->SetReadyToPlayStatus_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (readyToPlay==isReady)
	{
		if (cb)
			cb->SetReadyToPlayStatus_Result(LC_RC_SUCCESS);
		return;
	}
	else
	{
		RakNet::BitStream bs;
		bs.Write((unsigned char)ID_LOBBY_GENERAL);
		bs.Write((unsigned char)LOBBY_MSGID_SET_READY_TO_PLAY_STATUS);
		bs.Write(isReady);
		Send(&bs);
	}

	readyToPlay=isReady;

	// Call cannot fail from the server
	if (cb)
		cb->SetReadyToPlayStatus_Result(LC_RC_SUCCESS);

}
void LobbyClientPC::KickRoomMember(LobbyClientUserId *userId)
{
	RakAssert(userId);

	if (IsConnected()==false)
	{
		if (cb)
			cb->KickRoomMember_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->KickRoomMember_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->KickRoomMember_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	if (IsUserInRoom(userId)==false)
	{
		if (cb)
			cb->KickRoomMember_Result(LC_RC_USER_NOT_IN_ROOM);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_KICK_ROOM_MEMBER);
	bs.Write(*userId);
	Send(&bs);	
}
void LobbyClientPC::SetRoomIsHidden(bool roomIsHidden)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SetRoomIsHidden_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->SetRoomIsHidden_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->SetRoomIsHidden_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SET_INVITE_ONLY);
	bs.Write(roomIsHidden);
	Send(&bs);	
}
void LobbyClientPC::SetRoomAllowSpectators(bool spectatorsOn)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SetRoomAllowSpectators_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->SetRoomAllowSpectators_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->SetRoomAllowSpectators_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	if (RoomAllowsSpectators()==spectatorsOn)
	{
		if (cb)
			cb->SetRoomAllowSpectators_Result(LC_RC_SUCCESS);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SET_ROOM_ALLOW_SPECTATORS);
	bs.Write(spectatorsOn);
	Send(&bs);
}
void LobbyClientPC::ChangeNumSlots(int publicSlots, int reservedSlots)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->ChangeNumSlots_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->ChangeNumSlots_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->ChangeNumSlots_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	if (publicSlots+reservedSlots<2)
	{
		if (cb)
			cb->ChangeNumSlots_Result((LobbyClientCBResult(LC_RC_INVALID_INPUT, "Must have at least two slots.", 0 ) ));
		return;
	}
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CHANGE_NUM_SLOTS);
	bs.WriteCompressed(publicSlots);
	bs.WriteCompressed(reservedSlots);
	Send(&bs);
}
void LobbyClientPC::GrantModerator(LobbyClientUserId* userId)
{
	RakAssert(userId);

	if (IsConnected()==false)
	{
		if (cb)
			cb->GrantModerator_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->GrantModerator_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->GrantModerator_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	if (IsUserInRoom(userId)==false)
	{
		if (cb)
			cb->GrantModerator_Result(LC_RC_USER_NOT_IN_ROOM);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_GRANT_MODERATOR);
	bs.Write(*userId);
	Send(&bs);
}
void LobbyClientPC::StartGame(void)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->StartGame_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (IsInRoom()==false)
	{
		if (cb)
			cb->StartGame_Result(LC_RC_NOT_IN_ROOM);
		return;
	}
	if (IsRoomModerator(&myUserId)==false)
	{
		if (cb)
			cb->StartGame_Result(LC_RC_MODERATOR_ONLY);
		return;
	}
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_START_GAME);
	Send(&bs);
}
void LobbyClientPC::SubmitMatch(RankingServerDBSpec::SubmitMatch_Data *input)
{
	RakAssert(input);

	if (IsConnected()==false)
	{
		if (cb)
			cb->SubmitMatch_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SUBMIT_MATCH);
	input->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::DownloadRating(RankingServerDBSpec::GetRatingForParticipant_Data *input)
{
	RakAssert(input);

	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadRating_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_RATING);
	input->Serialize(&bs);
	Send(&bs);
}
RankingServerDBSpec::GetRatingForParticipant_Data * LobbyClientPC::GetRating(void)
{
	return &rating;
}
bool LobbyClientPC::IsWaitingOnQuickMatch(void)
{
	return lobbyUserStatus==LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH;
}
void LobbyClientPC::QuickMatch(int requiredPlayers, int timeoutInSeconds)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->QuickMatch_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (lobbyUserStatus!=LOBBY_USER_STATUS_IN_LOBBY_IDLE)
	{
		if (cb)
			cb->CreateRoom_Result(LC_RC_BUSY);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_QUICK_MATCH);
	bs.WriteCompressed(requiredPlayers);
	bs.WriteCompressed(timeoutInSeconds);
	Send(&bs);
}
void LobbyClientPC::CancelQuickMatch(void)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->CancelQuickMatch_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}
	if (lobbyUserStatus!=LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH)
	{
		if (cb)
			cb->CancelQuickMatch_Result(LC_RC_INVALID_INPUT);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CANCEL_QUICK_MATCH);
	Send(&bs);
}
void LobbyClientPC::DownloadActionHistory(bool ascendingSort)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadActionHistory_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_ACTION_HISTORY);
	bs.Write(ascendingSort);
	Send(&bs);
}
void LobbyClientPC::AddToActionHistory(LobbyDBSpec::AddToActionHistory_Data *input)
{
	RakAssert(input);

	if (IsConnected()==false)
	{
		if (cb)
			cb->AddToActionHistory_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ADD_TO_ACTION_HISTORY);
	input->Serialize(&bs);
	Send(&bs);
}
const DataStructures::List<LobbyDBSpec::AddToActionHistory_Data *> & LobbyClientPC::GetActionHistoryList(void) const
{
	return actionHistory;
}
void LobbyClientPC::CreateClan(LobbyDBSpec::CreateClan_Data *clanData, bool failIfAlreadyInClan)
{
	RakAssert(clanData);

	if (IsConnected()==false)
	{
		if (cb)
			cb->CreateClan_Result(LC_RC_NOT_CONNECTED_TO_SERVER, clanData);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CREATE_CLAN);
	clanData->Serialize(&bs);
	bs.Write(failIfAlreadyInClan);
	Send(&bs);
}
void LobbyClientPC::ChangeClanHandle(ClanId *clanId, const char *newHandle)
{
	if (IsConnected()==false)
	{
		LobbyDBSpec::ChangeClanHandle_Data callResult;
		callResult.clanId.hasDatabaseRowId=true;
		if (clanId)
			callResult.clanId.databaseRowId=*clanId;
		else
			callResult.clanId.databaseRowId=0;
		callResult.newHandle=newHandle;
		if (cb)
			cb->ChangeClanHandle_Result(LC_RC_NOT_CONNECTED_TO_SERVER, &callResult);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CHANGE_CLAN_HANDLE);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	stringCompressor->EncodeString(newHandle,512,&bs,0);
	Send(&bs);
}
void LobbyClientPC::LeaveClan(ClanId *clanId, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->LeaveClan_Result(LC_RC_NOT_CONNECTED_TO_SERVER, "", clanId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_LEAVE_CLAN);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	bs.Write(dissolveIfClanLeader);
	bs.Write(autoSendEmailIfClanDissolved);
	Send(&bs);
}
void LobbyClientPC::DownloadClans(const char *userHandle, LobbyClientUserId* userId, bool ascendingSort)
{
	if (IsConnected()==false)
	{
		LobbyDBSpec::GetClans_Data callResult;
		callResult.ascendingSort=ascendingSort;
		callResult.userId.hasDatabaseRowId=false;
		if (userId)
			callResult.userId.databaseRowId=*userId;
		else
			callResult.userId.databaseRowId=0;
		callResult.userId.handle=userHandle;
		if (cb)
			cb->DownloadClans_Result(LC_RC_NOT_CONNECTED_TO_SERVER,&callResult);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLANS);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(ascendingSort);
	Send(&bs);
}
void LobbyClientPC::SendClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)
{
	if (IsConnected()==false)
	{
		LobbyDBSpec::UpdateClanMember_Data callResult;
		callResult.clanId.hasDatabaseRowId=false;
		if (clanId)
			callResult.clanId.databaseRowId=*clanId;
		else
			callResult.clanId.databaseRowId=0;
		callResult.clanId.handle=userHandle;
		if (userId)
			callResult.userId.databaseRowId=*userId;
		else
			callResult.userId.databaseRowId=0;

		if (cb)
			cb->SendClanJoinInvitation_Result(LC_RC_NOT_CONNECTED_TO_SERVER, &callResult);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SEND_CLAN_JOIN_INVITATION);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	Send(&bs);
}
void LobbyClientPC::WithdrawClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->WithdrawClanJoinInvitation_Result(LC_RC_NOT_CONNECTED_TO_SERVER, "", clanId, userHandle, userId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_WITHDRAW_CLAN_JOIN_INVITATION);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	Send(&bs);
}
void LobbyClientPC::AcceptClanJoinInvitation(LobbyDBSpec::UpdateClanMember_Data *clanMemberData, bool failIfAlreadyInClan)
{
	RakAssert(clanMemberData);
	if (IsConnected()==false)
	{
		if (cb)
			cb->AcceptClanJoinInvitation_Result(LC_RC_NOT_CONNECTED_TO_SERVER, clanMemberData);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ACCEPT_CLAN_JOIN_INVITATION);
	bs.Write(failIfAlreadyInClan);
	clanMemberData->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::RejectClanJoinInvitation(const char *clanHandle, ClanId *clanId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->RejectClanJoinInvitation_Result(LC_RC_NOT_CONNECTED_TO_SERVER, clanHandle, clanId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_REJECT_CLAN_JOIN_INVITATION);
	SerializeHandleOrId(clanHandle,clanId,&bs);
	Send(&bs);
}
void LobbyClientPC::DownloadByClanMemberStatus(LobbyDBSpec::ClanMemberStatus status)
{
	if (IsConnected()==false)
	{
		DataStructures::List<LobbyDBSpec::RowIdOrHandle> clans;
		if (cb)
			cb->DownloadByClanMemberStatus_Result(LC_RC_NOT_CONNECTED_TO_SERVER, status, clans);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_BY_CLAN_MEMBER_STATUS);
	bs.Write(status);
	Send(&bs);
}
void LobbyClientPC::SendClanMemberJoinRequest(LobbyDBSpec::UpdateClanMember_Data *clanMemberData)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SendClanMemberJoinRequest_Result(LC_RC_NOT_CONNECTED_TO_SERVER, clanMemberData);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SEND_CLAN_MEMBER_JOIN_REQUEST);
	clanMemberData->Serialize(&bs);
	Send(&bs);
}
void LobbyClientPC::WithdrawClanMemberJoinRequest(const char *clanHandle, ClanId *clanId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->WithdrawClanMemberJoinRequest_Result(LC_RC_NOT_CONNECTED_TO_SERVER, clanHandle, clanId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST);
	SerializeHandleOrId(clanHandle,clanId,&bs);
	Send(&bs);
}
void LobbyClientPC::AcceptClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool failIfAlreadyInClan)
{
	if (IsConnected()==false)
	{
		LobbyDBSpec::UpdateClanMember_Data callResult;
		callResult.clanId.databaseRowId=clanId ? *clanId : 0;
		callResult.userId.handle=userHandle;
		callResult.userId.databaseRowId=userId ? *userId : 0;

		if (cb)
			cb->AcceptClanMemberJoinRequest_Result(LC_RC_NOT_CONNECTED_TO_SERVER, &callResult);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(failIfAlreadyInClan);
	Send(&bs);
}
void LobbyClientPC::RejectClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->RejectClanMemberJoinRequest_Result(LC_RC_NOT_CONNECTED_TO_SERVER, "", clanId, userHandle, userId);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_REJECT_CLAN_MEMBER_JOIN_REQUEST);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	Send(&bs);
}
void LobbyClientPC::SetClanMemberRank(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, LobbyDBSpec::ClanMemberStatus newRank )
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->SetClanMemberRank_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	if (newRank>LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
	{
		// Can only use to set as leader, subleader, or member
		if (cb)
			cb->SetClanMemberRank_Result(LC_RC_INVALID_INPUT);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_SET_CLAN_MEMBER_RANK);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(newRank);
	Send(&bs);
}
void LobbyClientPC::ClanKickAndBlacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool kick, bool blacklist)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->ClanKickAndBlacklistUser_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	bs.Write(kick);
	bs.Write(blacklist);
	Send(&bs);
}
void LobbyClientPC::ClanUnblacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->ClanUnblacklistUser_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_CLAN_UNBLACKLIST_USER);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	Send(&bs);
}
void LobbyClientPC::AddPostToClanBoard(LobbyDBSpec::AddToClanBoard_Data *postData, bool failIfNotLeader, bool failIfNotMember, bool failIfBlacklisted)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->AddPostToClanBoard_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_ADD_POST_TO_CLAN_BOARD);
	postData->Serialize(&bs);
	bs.Write(failIfNotLeader);
	bs.Write(failIfNotMember);
	bs.Write(failIfBlacklisted);
	Send(&bs);
}
void LobbyClientPC::RemovePostFromClanBoard(ClanId *clanId, ClanBoardPostId *postId, bool allowMemberToRemoveOwnPosts )
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->RemovePostFromClanBoard_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_REMOVE_POST_FROM_CLAN_BOARD);
	SerializeOptionalId((unsigned int*) clanId, &bs);
	bs.Write(*postId);
	bs.Write(allowMemberToRemoveOwnPosts);
	Send(&bs);
}
void LobbyClientPC::DownloadMyClanBoards(bool ascendingSort)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadMyClanBoards_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_MY_CLAN_BOARDS);
	bs.Write(ascendingSort);
	Send(&bs);
}
void LobbyClientPC::DownloadClanBoard(const char *clanHandle, ClanId *clanId, bool ascendingSort)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadClanBoard_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLAN_BOARD);
	SerializeHandleOrId(clanHandle,clanId,&bs);
	bs.Write(ascendingSort);
	Send(&bs);
}
void LobbyClientPC::DownloadClanMember(const char *clanHandle, ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)
{
	if (IsConnected()==false)
	{
		if (cb)
			cb->DownloadClanMember_Result(LC_RC_NOT_CONNECTED_TO_SERVER);
		return;
	}

	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLAN_MEMBER);
	SerializeHandleOrId(clanHandle,clanId,&bs);
	SerializeHandleOrId(userHandle,userId,&bs);
	Send(&bs);
}
void LobbyClientPC::ValidateUserKey(const char *titleIdStr, const char *keyPassword, int keyPasswordLength)
{
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_VALIDATE_USER_KEY);
	stringCompressor->EncodeString(titleIdStr, 512, &bs, 0);
	bs.WriteAlignedBytesSafe((const char*) keyPassword,keyPasswordLength,512);
	Send(&bs);
}
const DataStructures::List<LobbyUserClientDetails *>& LobbyClientPC::GetRecentlyMetUsersList(void) const
{
	return recentlyMetUsers;
}
bool LobbyClientPC::GetUserDetails(LobbyClientUserId *userId, LobbyUserClientDetails *details)
{
	if (*userId==myUserId)
	{
		// Return my own details
		details->userId=&myUserId;
		details->userData=&myself;
		details->systemAddress=rakPeer->GetExternalID(serverAddr);
		details->readyToPlay=readyToPlay;
		return true;
	}

	LobbyClientUser *u = GetLobbyClientUser(*userId);
	if (u)
	{
		details->systemAddress=u->systemAddress;
		details->userId=&u->userId;
		details->userData=u->userData;
		details->readyToPlay=u->readyToPlay;
		return true;
	}
	
	return false;
}
LobbyClientUserId *LobbyClientPC::GerUserIdFromName(const char *name)
{
	if (name==0)
		return 0;

	unsigned i;
	for (i=0; i < lobbyClientUsers.Size(); i++)
		if (strcmp(lobbyClientUsers[i]->userData->handle.C_String(), name)==0)
			return & lobbyClientUsers[i]->userId;

	return 0;
}
LobbyClientUserId * LobbyClientPC::GetMyId(void)
{
	return &myUserId;
}
bool LobbyClientPC::EqualIDs(const LobbyClientUserId* userId1, const LobbyClientUserId* userId2)
{
	RakAssert(userId1);
	RakAssert(userId2);
	return *userId1==*userId2;
}
void LobbyClientPC::SetCallbackInterface(LobbyClientInterfaceCB *cbi)
{
	cb=cbi;
}
void LobbyClientPC::OnAttach(RakPeerInterface *peer)
{
	rakPeer=peer;
}

PluginReceiveResult LobbyClientPC::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	if (packet->data[0]==ID_LOBBY_GENERAL && packet->length>=2 && packet->systemAddress==serverAddr)
	{
		switch (packet->data[1])
		{
		case LOBBY_MSGID_LOGIN:
			Login_NetMsg(packet);
			break;
		case LOBBY_MSGID_SET_TITLE_LOGIN_ID:
			SetTitleLoginId_NetMsg(packet);
			break;
		case LOBBY_MSGID_LOGOFF:
			Logoff_NetMsg(packet);
			break;
		case LOBBY_MSGID_REGISTER_ACCOUNT:
			RegisterAccount_NetMsg(packet);
			break;
		case LOBBY_MSGID_UPDATE_ACCOUNT:
			UpdateAccount_NetMsg(packet);
			break;
		case LOBBY_MSGID_CHANGE_HANDLE:
			ChangeHandle_NetMsg(packet);
			break;
		case LOBBY_MSGID_RETRIEVE_PASSWORD_RECOVERY_QUESTION:
			RetrievePasswordRecoveryQuestion_NetMsg(packet);
			break;
		case LOBBY_MSGID_RETRIEVE_PASSWORD:
			RetrievePassword_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_FRIENDS:
			DownloadFriends_NetMsg(packet);
			break;
		case LOBBY_MSGID_SEND_ADD_FRIEND_INVITE:
			SendAddFriendInvite_NetMsg(packet);
			break;
		case LOBBY_MSGID_ACCEPT_ADD_FRIEND_INVITE:
			AcceptAddFriendInvite_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_FRIEND_STATUS:
			NotifyFriendStatus_NetMsg(packet);
			break;
		case LOBBY_MSGID_DECLINE_ADD_FRIEND_INVITE:
			DeclineAddFriendInvite_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_IGNORE_LIST:
			DownloadIgnoreList_NetMsg(packet);
			break;
		case LOBBY_MSGID_UPDATE_IGNORE_LIST:
			UpdateIgnoreList_NetMsg(packet);
			break;
		case LOBBY_MSGID_REMOVE_FROM_IGNORE_LIST:
			RemoveFromIgnoreList_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_EMAILS:
			DownloadEmails_NetMsg(packet);
			break;
		case LOBBY_MSGID_SEND_EMAIL:
			SendEmail_NetMsg(packet);
			break;
		case LOBBY_MSGID_DELETE_EMAIL:
			DeleteEmail_NetMsg(packet);
			break;
		case LOBBY_MSGID_UPDATE_EMAIL_STATUS:
			UpdateEmailStatus_NetMsg(packet);
			break;
		case LOBBY_MSGID_INCOMING_EMAIL:
			IncomingEmail_NetMsg(packet);
			break;
		case LOBBY_MSGID_SEND_IM:
			SendIM_NetMsg(packet);
			break;
		case LOBBY_MSGID_RECEIVE_IM:
			ReceiveIM_NetMsg(packet);
			break;
		case LOBBY_MSGID_CREATE_ROOM:
			CreateRoom_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_ROOMS:
			DownloadRooms_NetMsg(packet);
			break;
		case LOBBY_MSGID_LEAVE_ROOM:
			LeaveRoom_NetMsg(packet);
			break;
		case LOBBY_MSGID_JOIN_ROOM:
			JoinRoom_NetMsg(packet);
			break;
		case LOBBY_MSGID_JOIN_ROOM_BY_FILTER:
			JoinRoomByFilter_NetMsg(packet);
			break;
		case LOBBY_MSGID_ROOM_CHAT:
			RoomChat_NetMsg(packet);
			break;
		case LOBBY_MSGID_INVITE_TO_ROOM:
			InviteToRoom_NetMsg(packet);
			break;
		case LOBBY_MSGID_KICK_ROOM_MEMBER:
			KickRoomMember_NetMsg(packet);
			break;
		case LOBBY_MSGID_SET_INVITE_ONLY:
			SetRoomIsHidden_NetMsg(packet);
			break;
		case LOBBY_MSGID_SET_ROOM_ALLOW_SPECTATORS:
			SetRoomAllowSpectators_NetMsg(packet);
			break;
		case LOBBY_MSGID_CHANGE_NUM_SLOTS:
			ChangeNumSlots_NetMsg(packet);
			break;
		case LOBBY_MSGID_GRANT_MODERATOR:
			GrantModerator_NetMsg(packet);
			break;
		case LOBBY_MSGID_START_GAME:
			StartGame_NetMsg(packet);
			break;
		case LOBBY_MSGID_SUBMIT_MATCH:
			SubmitMatch_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_RATING:
			DownloadRating_NetMsg(packet);
			break;
		case LOBBY_MSGID_QUICK_MATCH:
			QuickMatch_NetMsg(packet);
			break;
		case LOBBY_MSGID_CANCEL_QUICK_MATCH:
			CancelQuickMatch_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_ACTION_HISTORY:
			DownloadActionHistory_NetMsg(packet);
			break;
		case LOBBY_MSGID_ADD_TO_ACTION_HISTORY:
			AddToActionHistory_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_MEMBER_DROP:
			NotifyRoomMemberDrop_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_MEMBER_JOIN:
			NotifyRoomMemberJoin_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_MEMBER_READY_STATE:
			NotifyRoomMemberReadyState_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_NEW_MODERATOR:
			NotifyRoomAllNewModerator_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_SET_INVITE_ONLY:
			NotifyRoomSetroomIsHidden_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_SET_ALLOW_SPECTATORS:
			NotifyRoomSetAllowSpectators_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_CHANGE_NUM_SLOTS:
			NotifyRoomChangeNumSlots_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_CHAT:
			NotifyRoomChat_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_INVITE:
			NotifyRoomInvite_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ROOM_DESTROYED:
			NotifyRoomDestroyed_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_KICKED_OUT_OF_ROOM:
			NotifyKickedOutOfRoom_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_QUICK_MATCH_TIMEOUT:
			NotifyQuickMatchTimeout_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_START_GAME:
			NotifyStartGame(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ADD_FRIEND_RESPONSE:
			NotifyAddFriendResponse_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_CLAN_DISSOLVED:
			NotifyClanDissolved_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_LEAVE_CLAN:
			NotifyLeaveClan_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_SEND_CLAN_JOIN_INVITATION:
			NotifySendClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_WITHDRAW_CLAN_JOIN_INVITATION:
			NotifyWithdrawClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_JOIN_INVITATION:
			NotifyAcceptClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_REJECT_CLAN_JOIN_INVITATION:
			NotifyRejectClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_SEND_CLAN_MEMBER_JOIN_REQUEST:
			NotifySendClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST:
			NotifyWithdrawClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST:
			NotifyAcceptClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_REJECT_CLAN_MEMBER_JOIN_REQUEST:
			NotifyRejectClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_SET_CLAN_MEMBER_RANK:
			NotifySetClanMemberRank_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_CLAN_MEMBER_KICKED:
			NotifyClanMemberKicked_NetMsg(packet);
			break;
		case LOBBY_MSGID_NOTIFY_CLAN_MEMBER_UNBLACKLISTED:
			NotifyClanMemberUnblacklisted_NetMsg(packet);
			break;
		case LOBBY_MSGID_CREATE_CLAN:
			CreateClan_NetMsg(packet);
			break;
		case LOBBY_MSGID_CHANGE_CLAN_HANDLE:
			ChangeClanHandle_NetMsg(packet);
			break;
		case LOBBY_MSGID_LEAVE_CLAN:
			LeaveClan_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_CLANS:
			DownloadClans_NetMsg(packet);
			break;
		case LOBBY_MSGID_SEND_CLAN_JOIN_INVITATION:
			SendClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_WITHDRAW_CLAN_JOIN_INVITATION:
			WithdrawClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_ACCEPT_CLAN_JOIN_INVITATION:
			AcceptClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_REJECT_CLAN_JOIN_INVITATION:
			RejectClanJoinInvitation_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_BY_CLAN_MEMBER_STATUS:
			DownloadByClanMemberStatus_NetMsg(packet);
			break;
		case LOBBY_MSGID_SEND_CLAN_MEMBER_JOIN_REQUEST:
			SendClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST:
			WithdrawClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_ACCEPT_CLAN_MEMBER_JOIN_REQUEST:
			AcceptClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_REJECT_CLAN_MEMBER_JOIN_REQUEST:
			RejectClanMemberJoinRequest_NetMsg(packet);
			break;
		case LOBBY_MSGID_SET_CLAN_MEMBER_RANK:
			SetClanMemberRank_NetMsg(packet);
			break;
		case LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER:
			ClanKickAndBlacklistUser_NetMsg(packet);
			break;
		case LOBBY_MSGID_CLAN_UNBLACKLIST_USER:
			ClanUnblacklistUser_NetMsg(packet);
			break;
		case LOBBY_MSGID_ADD_POST_TO_CLAN_BOARD:
			AddPostToClanBoard_NetMsg(packet);
			break;
		case LOBBY_MSGID_REMOVE_POST_FROM_CLAN_BOARD:
			RemovePostFromClanBoard_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_MY_CLAN_BOARDS:
			DownloadMyClanBoards_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_CLAN_BOARD:
			DownloadClanBoard_NetMsg(packet);
			break;
		case LOBBY_MSGID_DOWNLOAD_CLAN_MEMBER:
			DownloadClanMember_NetMsg(packet);
			break;
		case LOBBY_MSGID_VALIDATE_USER_KEY:
			ValidateUserKey_NetMsg(packet);
			break;

		}
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	else if (serverAddr==packet->systemAddress &&
		(packet->data[0]==ID_DISCONNECTION_NOTIFICATION ||
		packet->data[0]==ID_CONNECTION_LOST))
	{
		if (cb && IsConnected())
			cb->Login_Result(LC_RC_CONNECTION_FAILURE);

		// Disconnected, reset
		InitializeVariables();
	}
	return RR_CONTINUE_PROCESSING;
}
void LobbyClientPC::OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress)
{

}
void LobbyClientPC::OnShutdown(RakPeerInterface *peer)
{

}
void LobbyClientPC::SetOrderingChannel(char oc)
{
	orderingChannel=oc;
}
void LobbyClientPC::SetSendPriority(PacketPriority pp)
{
	packetPriority=pp;
}
bool LobbyClientPC::Send( const RakNet::BitStream * bitStream )
{
	RakAssert(serverAddr!=UNASSIGNED_SYSTEM_ADDRESS);
	return rakPeer->Send(bitStream, packetPriority, RELIABLE_ORDERED, orderingChannel, serverAddr, false);
}
void LobbyClientPC::InitializeVariables(void)
{
	lobbyUserStatus=LOBBY_USER_STATUS_OFFLINE;
	unsigned i;
	for (i=0; i < friendsList.Size(); i++)
		delete friendsList[i];
	friendsList.Clear();
	for (i=0; i < ignoreList.Size(); i++)
		delete ignoreList[i];
	ignoreList.Clear();	
	ClearEmails(false);
	ClearEmails(true);
	currentRoom.Clear();
	allRooms.Clear();

	ClearLobbyClientUsers();
	ClearActionHistory();
	ClearRecentlyMetUsers();

	hasCalledGetEmails=false;

}
void LobbyClientPC::Login_NetMsg(Packet *packet)
{
	if (lobbyUserStatus!=LOBBY_USER_STATUS_LOGGING_ON)
	{
		RakAssert(0);
		return;
	}

	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_IN_PROGRESS:
	case LC_RC_UNKNOWN_USER_ID:
		// Just return as error
		lobbyUserStatus=LOBBY_USER_STATUS_OFFLINE;
		break;
	case LC_RC_SUSPENDED_USER_ID:
		bs.Read(callbackResult.epochTime);
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		lobbyUserStatus=LOBBY_USER_STATUS_OFFLINE;
		break;
	case LC_RC_BANNED_USER_ID:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		lobbyUserStatus=LOBBY_USER_STATUS_OFFLINE;
		break;
	case LC_RC_SUCCESS:
		bs.Read(myUserId);
		myself.Deserialize(&bs);
		lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
		if (titleLoginPW && titleLoginPWLength>0)
			SendSetTitleLoginId();
		break;
	}

	if (cb)
		cb->Login_Result(callbackResult);
}
void LobbyClientPC::SetTitleLoginId_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	// No data to read right now
	switch (resultCode)
	{
		case LC_RC_BAD_TITLE_ID:
		case LC_RC_USER_KEY_NOT_SET:
			break;
		case LC_RC_SUCCESS:
			break;
	}

	if (cb)
		cb->SetTitleLoginId_Result(callbackResult);
}
void LobbyClientPC::Logoff_NetMsg(Packet *packet)
{
	LobbyClientCBResult callbackResult(LC_RC_SUCCESS,0,0);
	if (cb)
		cb->Logoff_Result(callbackResult);
	lobbyUserStatus=LOBBY_USER_STATUS_OFFLINE;

	InitializeVariables();
}
void LobbyClientPC::RegisterAccount_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	// No data to read right now
	switch (resultCode)
	{
	case LC_RC_PERMISSION_DENIED:
	case LC_RC_BAD_TITLE_ID:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->RegisterAccount_Result(callbackResult);
}
void LobbyClientPC::UpdateAccount_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_UNKNOWN_PERMISSIONS:
	case LC_RC_SUCCESS:
		break;
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->UpdateAccount_Result(callbackResult);
}
void LobbyClientPC::ChangeHandle_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_UNKNOWN_PERMISSIONS:
	case LC_RC_SUCCESS:
		break;
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->ChangeHandle_Result(callbackResult);
}
void LobbyClientPC::RetrievePasswordRecoveryQuestion_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_UNKNOWN_USER_ID:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_SUCCESS:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->RetrievePasswordRecoveryQuestion_Result(callbackResult);
}
void LobbyClientPC::RetrievePassword_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_UNKNOWN_USER_ID:
	case LC_RC_PERMISSION_DENIED:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_SUCCESS:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->RetrievePassword_Result(callbackResult);
}
void LobbyClientPC::DownloadFriends_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		{
			unsigned int friendCount;
			bs.ReadCompressed(friendCount);
			unsigned i;
			bool isOnline;
			LobbyClientUserId id;
			char friendName[512];
			for (i=0; i < friendCount; i++)
			{
				bs.Read(isOnline);
				bs.Read(id);
				stringCompressor->DecodeString(friendName, sizeof(friendName), &bs, 0);
				AddFriend(isOnline,friendName,id);

				// Server does this explicitly in order to prevent duplicate notifications of both when the friend status changes and when they d/l the list
			//	if (cb)
			//		cb->FriendStatus_Notify(friendName, &id, isOnline ? FRIEND_STATUS_FRIEND_ONLINE : FRIEND_STATUS_FRIEND_OFFLINE);
			}

			break;
		}		
	}

	if (cb)
		cb->DownloadFriends_Result(callbackResult);
}
void LobbyClientPC::SendAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DISCONNECTED_USER_ID:
	case LC_RC_SUCCESS:
	case LC_RC_IGNORED:
	case LC_RC_ALREADY_CURRENT_STATE:
		break;
	}

	char userHandle[512];
	LobbyClientUserId userId;
	stringCompressor->DecodeString(userHandle,512,&bs,0);
	bs.Read(userId);

	if (cb)
		cb->SendAddFriendInvite_Result(callbackResult,userHandle,&userId);
}
void LobbyClientPC::AcceptAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char userHandle[512];
	LobbyClientUserId userId;
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		stringCompressor->DecodeString(userHandle,512,&bs,0);
		bs.Read(userId);
		break;
	case LC_RC_SUCCESS:
		bool isOnline;
		bs.Read(isOnline);
		stringCompressor->DecodeString(userHandle, sizeof(userHandle), &bs, 0);
		bs.Read(userId);
		AddFriend(isOnline,userHandle,userId);
		break;
	}


	if (cb)
		cb->AcceptAddFriendInvite_Result(callbackResult,userHandle,&userId);
}
void LobbyClientPC::DeclineAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DISCONNECTED_USER_ID:
	case LC_RC_INVALID_INPUT:
	case LC_RC_SUCCESS:
		break;
	}

	char userHandle[512];
	LobbyClientUserId userId;
	stringCompressor->DecodeString(userHandle,512,&bs,0);
	bs.Read(userId);

	if (cb)
		cb->DeclineAddFriendInvite_Result(callbackResult,userHandle,&userId);
}
void LobbyClientPC::NotifyFriendStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char status;
	bs.Read(status);

	LobbyClientUserId them;
	bs.Read(them);
	bool isOnline;
	char handle[512];
	bs.Read(isOnline);
	stringCompressor->DecodeString(handle,512,&bs,0);

	switch (status)
	{
//	case FRIEND_STATUS_NEW_FRIEND:
//		AddFriend(isOnline, handle, them);
//		break;
	case FRIEND_STATUS_NO_LONGER_FRIENDS:
		RemoveFriend(them);
		break;
	case FRIEND_STATUS_FRIEND_ONLINE:
	case FRIEND_STATUS_FRIEND_OFFLINE:
		break;
	}

	if (cb)
		cb->FriendStatus_Notify(handle, &them, (RakNet::FriendStatus) status);
}
void LobbyClientPC::DownloadIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			unsigned int ignoredUserCount;
			bs.ReadCompressed(ignoredUserCount);
			unsigned i;
			char actions[4096];
			long long creationTime;
			LobbyClientUserId id;
			for (i=0; i < ignoredUserCount; i++)
			{
				bs.Read(id);
				stringCompressor->DecodeString(actions, sizeof(actions), &bs, 0);
				bs.Read(creationTime);
				AddIgnoredUser(id, actions, creationTime);
			}
		}
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->DownloadIgnoreList_Result(callbackResult);
}
void LobbyClientPC::UpdateIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			char actions[4096];
			long long creationTime;
			LobbyClientUserId id;
			bs.Read(id);
			stringCompressor->DecodeString(actions, sizeof(actions), &bs, 0);
			bs.Read(creationTime);
			AddIgnoredUser(id, actions, creationTime);
		}
		break;
	case LC_RC_ALREADY_CURRENT_STATE:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->AddToOrUpdateIgnoreList_Result(callbackResult);
}
void LobbyClientPC::RemoveFromIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		LobbyClientUserId id;
		bs.Read(id);
		RemoveIgnoredUser(id);
		break;
	case LC_RC_ALREADY_CURRENT_STATE:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->RemoveFromIgnoreList_Result(callbackResult);
}
void LobbyClientPC::DownloadEmails_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			unsigned int emailsCount;
			bool inbox;
			bs.Read(inbox);
			bs.ReadCompressed(emailsCount);
			unsigned i;
			ClearEmails(inbox);
			for (i=0; i < emailsCount; i++)
			{
				LobbyDBSpec::SendEmail_Data *email = new LobbyDBSpec::SendEmail_Data;
				email->Deserialize(&bs);
				AddEmail(email,inbox);
			}
		}
		
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->DownloadEmails_Result(callbackResult);
}
void LobbyClientPC::SendEmail_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			// Won't work because we didn't necessarily just send one email
			
//			LobbyDBSpec::SendEmail_Data *email = new LobbyDBSpec::SendEmail_Data;
//			email->Deserialize(&bs);
//			AddEmail(email, false);

			// Sort of a hack - redownload emails after sending an email to fix the list.
			if (hasCalledGetEmails)
				DownloadEmails(lastGetEmailsSortIsAscending, false, lastGetEmailsLangauge);

		}
		break;
	case LC_RC_INVALID_INPUT:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->SendEmail_Result(callbackResult);
}
void LobbyClientPC::DeleteEmail_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			LobbyDBSpec::DatabaseKey id;
			bs.Read(id);
			RemoveEmail(id);
		}
		break;
	case LC_RC_INVALID_INPUT:
		break;
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->DeleteEmail_Result(callbackResult);
}
void LobbyClientPC::UpdateEmailStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			LobbyDBSpec::DatabaseKey id;
			int status;
			bool wasOpened;
			bs.Read(status);
			bs.Read(id);
			bs.Read(wasOpened);
			UpdateEmail(id,status,wasOpened);
		}
		break;
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->UpdateEmailStatus_Result(callbackResult);
}
void LobbyClientPC::IncomingEmail_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	LobbyClientUserId senderId;
	char userHandle[512];
	char subject[4096];
	bs.Read(senderId);
	stringCompressor->DecodeString(userHandle, sizeof(userHandle), &bs, 0);
	stringCompressor->DecodeString(subject, sizeof(subject), &bs, 0);
	
	if (cb)
		cb->IncomingEmail_Notify(senderId, userHandle, subject);

	// Sort of a hack - redownload emails after receiving an email notification to fix the list.
	if (hasCalledGetEmails)
		DownloadEmails(lastGetEmailsSortIsAscending, true, lastGetEmailsLangauge);
}

void LobbyClientPC::SendIM_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	char userHandle[512];
	LobbyClientUserId userId;

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
	case LC_RC_UNKNOWN_USER_ID:
	case LC_RC_IGNORED:
		break;
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	stringCompressor->DecodeString(userHandle, 512, &bs, 0);
	bs.Read(userId);

	if (cb)
		cb->SendIM_Result(callbackResult, userHandle, &userId);
}
void LobbyClientPC::ReceiveIM_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	char sender[512];
	char messageBody[4096];
	LobbyClientUserId id;
	unsigned char language;
	bs.Read(language);
	bs.Read(id);
	stringCompressor->DecodeString(sender, sizeof(sender), &bs, language);
	stringCompressor->DecodeString(messageBody, sizeof(messageBody), &bs, language);

	if (cb)
		cb->ReceiveIM_Notify(sender, messageBody, &id);
}
void LobbyClientPC::CreateRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			RakAssert(currentRoom.RoomIsDead()==true);			
			currentRoom.Clear();
			currentRoom.Deserialize(&bs);
			TableSerializer::DeserializeColumns(&bs, &allRooms);
			TableSerializer::DeserializeRow(&bs, &allRooms); // Get your own room row so you know about custom users columns.
			DeserializeAndAddClientSafeUser(&bs);
			currentRoom.rowInTable=allRooms.GetRowByID(currentRoom.rowId);
			currentRoom.WriteToRow(false);
			lobbyUserStatus=LOBBY_USER_STATUS_IN_ROOM;
		}
		break;
	case LC_RC_INVALID_INPUT:
	case LC_RC_BAD_TITLE_ID:
	case LC_RC_NAME_IN_USE:
		break;
	}

	if (cb)
		cb->CreateRoom_Result(callbackResult);
}
void LobbyClientPC::DownloadRooms_NetMsg(Packet *packet)
{
	downloadingRooms=false;

	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			bool queriedTable;
			bs.Read(queriedTable);
			if (queriedTable==true)
			{
				TableSerializer::DeserializeTable(&bs,&allRooms);

				// Update the room row reference
				if (currentRoom.RoomIsDead()==false)
					currentRoom.rowInTable=allRooms.GetRowByID(currentRoom.rowId);
			}
			else
			{
				allRooms.Clear();
			}
		}
		break;
	case LC_RC_BAD_TITLE_ID:
		break;
	}

	if (cb)
		cb->DownloadRooms_Result(callbackResult);
}
void LobbyClientPC::LeaveRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		{
			bool roomIsDead;
			bs.Read(roomIsDead);
			LeaveCurrentRoom(roomIsDead);
		}
		break;
	case LC_RC_NOT_IN_ROOM:
		break;
	}

	if (cb)
		cb->LeaveRoom_Result(callbackResult);
}
void LobbyClientPC::JoinRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		currentRoom.Deserialize(&bs);

		unsigned int openSlotSize, reservedSlotSize, spectatorsSize;
		unsigned i;
		bs.ReadCompressed(openSlotSize);
		for (i=0; i < openSlotSize; i++)
			DeserializeAndAddClientSafeUser(&bs);
		bs.ReadCompressed(reservedSlotSize);
		for (i=0; i < reservedSlotSize; i++)
			DeserializeAndAddClientSafeUser(&bs);
		bs.ReadCompressed(spectatorsSize);
		for (i=0; i < spectatorsSize; i++)
			DeserializeAndAddClientSafeUser(&bs);

		lobbyUserStatus=LOBBY_USER_STATUS_IN_ROOM;

		break;
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_ROOM_FULL:
	case LC_RC_ROOM_ONLY_HAS_RESERVED:
	case LC_RC_ALREADY_IN_ROOM:
	case LC_RC_ROOM_DOES_NOT_EXIST:
		break;
	}

	if (cb)
		cb->JoinRoom_Result(callbackResult);
}
void LobbyClientPC::JoinRoomByFilter_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
		currentRoom.Deserialize(&bs);

		unsigned int openSlotSize, reservedSlotSize, spectatorsSize;
		unsigned i;
		bs.ReadCompressed(openSlotSize);
		for (i=0; i < openSlotSize; i++)
			DeserializeAndAddClientSafeUser(&bs);
		bs.ReadCompressed(reservedSlotSize);
		for (i=0; i < reservedSlotSize; i++)
			DeserializeAndAddClientSafeUser(&bs);
		bs.ReadCompressed(spectatorsSize);
		for (i=0; i < spectatorsSize; i++)
			DeserializeAndAddClientSafeUser(&bs);

		lobbyUserStatus=LOBBY_USER_STATUS_IN_ROOM;

		break;
	case LC_RC_ROOM_DOES_NOT_EXIST:
		break;
	}

	if (cb)
		cb->JoinRoomByFilter_Result(callbackResult);
}
void LobbyClientPC::RoomChat_NetMsg(Packet *packet){}
void LobbyClientPC::InviteToRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_SUCCESS:
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_DISCONNECTED_USER_ID:
	case LC_RC_USER_ALREADY_IN_ROOM:
		break;
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	}

	if (cb)
		cb->InviteToRoom_Result(callbackResult);

}
void LobbyClientPC::KickRoomMember_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
	case LC_RC_DISCONNECTED_USER_ID:
	case LC_RC_USER_NOT_IN_ROOM:
	case LC_RC_INVALID_INPUT:
	case LC_RC_SUCCESS:
		break;
	}

	// LOBBY_MSGID_NOTIFY_ROOM_MEMBER_DROP will arrive to actually drop the guy

	if (cb)
		cb->KickRoomMember_Result(callbackResult);
}
void LobbyClientPC::SetRoomIsHidden_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
		break;
	case LC_RC_SUCCESS:
		{
			bool roomIsHidden;
			bs.Read(roomIsHidden);
			currentRoom.roomIsHidden=roomIsHidden;
			currentRoom.WriteToRow(false);
		}
		break;
	}

	if (cb)
		cb->SetRoomIsHidden_Result(callbackResult);
}
void LobbyClientPC::SetRoomAllowSpectators_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	bool allowSpectators;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
		break;
	case LC_RC_SUCCESS:
		bs.Read(allowSpectators);
		currentRoom.allowSpectators=allowSpectators;
		currentRoom.WriteToRow(false);
		break;
	}

	if (cb)
		cb->SetRoomAllowSpectators_Result(callbackResult);
}
void LobbyClientPC::ChangeNumSlots_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	int publicSlots,privateSlots;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
		break;
	case LC_RC_SUCCESS:
		bs.ReadCompressed(publicSlots);
		bs.ReadCompressed(privateSlots);
		currentRoom.publicSlots=publicSlots;
		currentRoom.privateSlots=privateSlots;
		currentRoom.WriteToRow(false);
		break;
	}

	if (cb)
		cb->ChangeNumSlots_Result(callbackResult);
}
void LobbyClientPC::GrantModerator_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientUserId moderator;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo, sizeof(additionalInfo), &bs, 0);
		break;
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
		break;
	case LC_RC_SUCCESS:
		bs.Read(moderator);
		currentRoom.SetModerator(moderator);
		break;
	}

	if (cb)
		cb->GrantModerator_Result(callbackResult);
}
void LobbyClientPC::StartGame_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_NOT_IN_ROOM:
	case LC_RC_MODERATOR_ONLY:
	case LC_RC_NOT_ENOUGH_PARTICIPANTS:
	case LC_RC_NOT_READY:
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->StartGame_Result(callbackResult);
}
void LobbyClientPC::SubmitMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	case LC_RC_PERMISSION_DENIED:
		break;
	}

	if (cb)
		cb->SubmitMatch_Result(callbackResult);
}
void LobbyClientPC::DownloadRating_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		rating.Deserialize(&bs);
		break;
	case LC_RC_PERMISSION_DENIED:
		break;
	}

	if (cb)
		cb->DownloadRating_Result(callbackResult);
}
void LobbyClientPC::QuickMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_DISCONNECTED_USER_ID:
	case LC_RC_INVALID_INPUT:
		break;
	case LC_RC_SUCCESS:
		lobbyUserStatus=LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH;
		break;
	}


	if (cb)
		cb->QuickMatch_Result(callbackResult);
}
void LobbyClientPC::CancelQuickMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	case LC_RC_INVALID_INPUT:
		break;
	case LC_RC_SUCCESS:
		lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
		break;
	}

	
	if (cb)
		cb->CancelQuickMatch_Result(callbackResult);
}
void LobbyClientPC::DownloadActionHistory_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	LobbyDBSpec::GetActionHistory_Data res;
	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		res.Deserialize(&bs);
		ClearActionHistory();
		unsigned i;
		for (i=0; i < res.history.Size(); i++)
		{
			actionHistory.Insert(res.history[i]);
			res.history[i]->AddRef();
		}
		break;
	}

	if (cb)
		cb->DownloadActionHistory_Result(callbackResult);
}
void LobbyClientPC::AddToActionHistory_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	LobbyDBSpec::AddToActionHistory_Data *res = new LobbyDBSpec::AddToActionHistory_Data;
	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		delete res;
		break;
	case LC_RC_SUCCESS:
		res->Deserialize(&bs);
		actionHistory.Insert(res);
		break;
	case LC_RC_PERMISSION_DENIED:
	case LC_RC_UNKNOWN_PERMISSIONS:
		delete res;
		break;
	default:
		delete res;
	}

	if (cb)
		cb->AddToActionHistory_Result(callbackResult);
}
void LobbyClientPC::CreateClan_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	LobbyDBSpec::CreateClan_Data callResult;

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->CreateClan_Result(callbackResult, &callResult);
}
void LobbyClientPC::ChangeClanHandle_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::ChangeClanHandle_Data callResult;

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_PERMISSION_DENIED:
	case LC_RC_NAME_IN_USE:
	case LC_RC_INVALID_CLAN:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->ChangeClanHandle_Result(callbackResult, &callResult);
}
void LobbyClientPC::LeaveClan_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	RakNet::RakString clanHandle;
	ClanId clanId;

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_UNKNOWN_USER_ID:
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_INVALID_CLAN:
	case LC_RC_SUCCESS:
		break;
	}

	stringCompressor->DecodeString(&clanHandle,512,&bs,0);
	bs.Read(clanId);

	if (cb)
		cb->LeaveClan_Result(callbackResult, clanHandle, &clanId);
}
void LobbyClientPC::DownloadClans_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	
	LobbyDBSpec::GetClans_Data callResult;

	switch (resultCode)
	{
	case LC_RC_INVALID_CLAN:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_UNKNOWN_USER_ID:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->DownloadClans_Result(callbackResult, &callResult);
}
void LobbyClientPC::SendClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	case LC_RC_INVALID_CLAN:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_BUSY:
	case LC_RC_INVALID_INPUT:
	case LC_RC_PERMISSION_DENIED:
	case LC_RC_ALREADY_CURRENT_STATE:
	case LC_RC_BLACKLISTED_FROM_CLAN:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->SendClanJoinInvitation_Result(callbackResult, &callResult);
}
void LobbyClientPC::WithdrawClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	RakNet::RakString clanHandle;
	ClanId clanId;
	RakNet::RakString userHandle;
	LobbyClientUserId userId;

	switch (resultCode)
	{
	case LC_RC_INVALID_CLAN:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_BUSY:
	case LC_RC_INVALID_INPUT:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		stringCompressor->DecodeString(&clanHandle,512,&bs,0);
		bs.Read(clanId);
		stringCompressor->DecodeString(&userHandle,512,&bs,0);
		bs.Read(userId);
		break;
	}

	if (cb)
		cb->WithdrawClanJoinInvitation_Result(callbackResult, clanHandle, &clanId, userHandle, &userId);
}
void LobbyClientPC::AcceptClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	RakNet::RakString clanHandle;

	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	case LC_RC_INVALID_CLAN:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_BUSY:
	case LC_RC_INVALID_INPUT:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->AcceptClanJoinInvitation_Result(callbackResult, &callResult);
}
void LobbyClientPC::RejectClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	RakNet::RakString clanHandle;
	LobbyDBSpec::DatabaseKey clanId;

	switch (resultCode)
	{
	case LC_RC_INVALID_CLAN:
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_BUSY:
	case LC_RC_INVALID_INPUT:
	case LC_RC_PERMISSION_DENIED:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	stringCompressor->DecodeString(&clanHandle,512,&bs,0);
	bs.Read(clanId);

	if (cb)
		cb->RejectClanJoinInvitation_Result(callbackResult, clanHandle, &clanId);
}
void LobbyClientPC::DownloadByClanMemberStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::ClanMemberStatus status;
	unsigned int numMatchingMembers;
	unsigned i;
	LobbyDBSpec::RowIdOrHandle rowIdOrHandle;
	DataStructures::List<LobbyDBSpec::RowIdOrHandle> clans;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_INVALID_CLAN:
	case LC_RC_SUCCESS:
		break;
	}

	bs.Read(status);
	bs.Read(numMatchingMembers);
	for (i=0; i < numMatchingMembers; i++)
	{
		bs.Read(rowIdOrHandle.databaseRowId);
		stringCompressor->DecodeString(&rowIdOrHandle.handle,512,&bs,0);
		clans.Insert(rowIdOrHandle);
	}

	if (cb)
		cb->DownloadByClanMemberStatus_Result(callbackResult, status, clans);
}
void LobbyClientPC::SendClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		stringCompressor->DecodeString(&callResult.clanId.handle,512,&bs,0);
		bs.Read(callResult.clanId.databaseRowId);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->SendClanMemberJoinRequest_Result(callbackResult, &callResult);
}
void LobbyClientPC::WithdrawClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	RakNet::RakString clanHandle;
	LobbyDBSpec::DatabaseKey clanId;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	stringCompressor->DecodeString(&clanHandle,512,&bs,0);
	bs.Read(clanId);
	if (cb)
		cb->WithdrawClanMemberJoinRequest_Result(callbackResult, clanHandle, &clanId);
}
void LobbyClientPC::AcceptClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		stringCompressor->DecodeString(&callResult.clanId.handle,512,&bs,0);
		bs.Read(callResult.clanId.databaseRowId);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->AcceptClanMemberJoinRequest_Result(callbackResult, &callResult);
}
void LobbyClientPC::RejectClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	RakNet::RakString clanHandle;
	ClanId clanId;

	switch (resultCode)
	{
	case LC_RC_DATABASE_FAILURE:
	case LC_RC_UNKNOWN_USER_ID:
	case LC_RC_INVALID_INPUT:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_INVALID_CLAN:
	case LC_RC_SUCCESS:
		break;
	}

	stringCompressor->DecodeString(&clanHandle,512,&bs,0);
	bs.Read(clanId);

	// TODO
	LobbyClientUserId lcui=0;

	if (cb)
		cb->RejectClanMemberJoinRequest_Result(callbackResult, clanHandle, &clanId, "", &lcui);
}
void LobbyClientPC::SetClanMemberRank_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->SetClanMemberRank_Result(callbackResult);
}
void LobbyClientPC::ClanKickAndBlacklistUser_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	// TODO
}
void LobbyClientPC::ClanUnblacklistUser_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	// TODO
}
void LobbyClientPC::AddPostToClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->AddPostToClanBoard_Result(callbackResult);
}
void LobbyClientPC::RemovePostFromClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->RemovePostFromClanBoard_Result(callbackResult);
}
void LobbyClientPC::DownloadMyClanBoards_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->DownloadMyClanBoards_Result(callbackResult);
}
void LobbyClientPC::DownloadClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->DownloadMyClanBoards_Result(callbackResult);
}
void LobbyClientPC::DownloadClanMember_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		break;
	}

	if (cb)
		cb->DownloadClanMember_Result(callbackResult);
}

void LobbyClientPC::ValidateUserKey_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	unsigned char resultCode;
	bs.Read(resultCode);
	char additionalInfo[512];
	additionalInfo[0]=0;
	LobbyClientCBResult callbackResult((LobbyClientCBResultCode)resultCode,additionalInfo,0);
	LobbyDBSpec::UpdateClanMember_Data callResult;

	switch (resultCode)
	{
	default:
		stringCompressor->DecodeString(additionalInfo,sizeof(additionalInfo),&bs,0);
		break;
	case LC_RC_SUCCESS:
		callResult.Deserialize(&bs);
		break;
	}

	if (cb)
		cb->ValidateUserKey_Result(callbackResult);
}
void LobbyClientPC::NotifyRoomMemberDrop_NetMsg(Packet *packet)
{
	RakAssert(IsInRoom());

	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomMemberDrop_PC dataStruct;
	dataStruct.Deserialize(&bs);
	
	bool b;
	b=currentRoom.RemoveRoomMember(dataStruct._droppedMember, &b);
	RakAssert(b);
	if (dataStruct.gotNewModerator)
		currentRoom.SetModerator(dataStruct._newModerator);

	// Clear this member's data
	ClearLobbyClientUser(dataStruct._droppedMember);

	currentRoom.WriteToRow(false);

	// Notify game
	if (cb)
		cb->RoomMemberDrop_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomMemberJoin_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomMemberJoin_PC dataStruct;
	dataStruct.Deserialize(&bs);

	if (dataStruct.isInOpenSlot)
		currentRoom.AddRoomMemberToPublicSlot(dataStruct._joinedMember);
	else if (dataStruct.isInReservedSlot)
		currentRoom.AddRoomMemberToPrivateSlot(dataStruct._joinedMember);
	else
		currentRoom.AddRoomMemberAsSpectator(dataStruct._joinedMember);

	currentRoom.WriteToRow(false);

	// Read this member's data
	DeserializeAndAddClientSafeUser(&bs);

	// Notify game
	if (cb)
		cb->RoomMemberJoin_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomMemberReadyState_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomMemberReadyStateChange_PC dataStruct;
	dataStruct.Deserialize(&bs);

	LobbyClientUser *user = GetLobbyClientUser(*dataStruct.member);
	if (user)
		user->readyToPlay = dataStruct.isReady;
	
	// Notify game
	if (cb)
		cb->RoomMemberReadyStateChange_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomAllNewModerator_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	NewModerator_PC dataStruct;
	dataStruct.Deserialize(&bs);

	currentRoom.SetModerator(dataStruct._member);

	// Notify game
	if (cb)
		cb->RoomNewModerator_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomSetroomIsHidden_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomSetroomIsHidden_PC dataStruct;
	dataStruct.Deserialize(&bs);

	currentRoom.roomIsHidden=dataStruct.isroomIsHidden;

	// Notify game
	if (cb)
		cb->RoomSetIsHidden_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomSetAllowSpectators_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomSetAllowSpectators_PC dataStruct;
	dataStruct.Deserialize(&bs);

	currentRoom.allowSpectators=dataStruct.allowSpectators;

	// Notify game
	if (cb)
		cb->RoomSetAllowSpectators_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomChangeNumSlots_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	RoomChangeNumSlots_PC dataStruct;
	dataStruct.Deserialize(&bs);

	currentRoom.publicSlots=dataStruct.newPublicSlots;
	currentRoom.publicSlots=dataStruct.newPrivateSlots;

	// Notify game
	if (cb)
		cb->RoomChangeNumSlots_Notify(&dataStruct);
}
void LobbyClientPC::NotifyRoomChat_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	LobbyClientUserId id;
	char handle[512];
	unsigned char languageId;
	char chatMessage[4096];
	char *chatBinaryData=0;
	int chatBinaryLength;
	bs.Read(id);
	stringCompressor->DecodeString(handle, sizeof(handle), &bs, 0);
	bs.Read(languageId);
	stringCompressor->DecodeString(chatMessage, sizeof(chatMessage), &bs, languageId);
	bs.ReadAlignedBytesSafeAlloc(&chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);

	// Notify game
	if (cb)
		cb->ChatMessage_Notify(&id, handle, chatMessage, chatBinaryData, chatBinaryLength);
}
void LobbyClientPC::NotifyRoomInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	LobbyClientUserId id;
	LobbyClientRoomId roomId;
	char handle[512];
	char roomName[512];
	unsigned char languageId;
	char chatMessage[4096];
	char *chatBinaryData=0;
	int chatBinaryLength;
	bool inviteToPrivateSlot;
	bs.Read(inviteToPrivateSlot);
	bs.Read(id);
	bs.ReadAlignedBytesSafeAlloc(&chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);
	stringCompressor->DecodeString(handle, sizeof(handle), &bs, 0);
	bs.Read(languageId);
	stringCompressor->DecodeString(chatMessage, sizeof(chatMessage), &bs, languageId);
	bs.Read(roomId);
	stringCompressor->DecodeString(roomName, sizeof(roomName), &bs, languageId);

	// Notify game
	if (cb)
		cb->RoomInvite_Notify(&id, handle, chatMessage, chatBinaryData, chatBinaryLength, &roomId, roomName, inviteToPrivateSlot);
}
void LobbyClientPC::NotifyRoomDestroyed_NetMsg(Packet *packet)
{
	LeaveCurrentRoom(true);

	// Notify game
	if (cb)
		cb->RoomDestroyed_Notify();
}
void LobbyClientPC::NotifyKickedOutOfRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	KickedOutOfRoom_PC noticeStruct;
	noticeStruct.Deserialize(&bs);

	LeaveCurrentRoom(noticeStruct.roomWasDestroyed);

	// Notify game
	if (cb)
		cb->KickedOutOfRoom_Notify(&noticeStruct);
}
void LobbyClientPC::NotifyQuickMatchTimeout_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	QuickMatchTimeout_PC noticeStruct;
	noticeStruct.Deserialize(&bs);

	RakAssert(lobbyUserStatus==LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH);
	lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

	// Notify game
	if (cb)
		cb->QuickMatchTimeout_Notify(&noticeStruct);
}
void LobbyClientPC::NotifyStartGame(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	StartGame_Notification notification;
	bs.Read(notification.wasQuickMatch);
	bs.Read(notification.moderator);
	notification.yourAddress=rakPeer->GetExternalID(packet->systemAddress);
	LobbyClientTitleId titleId;
	bs.Read(titleId);
	notification.titleId=&titleId;
	unsigned short numParticipants,numSpectators;
	bs.ReadCompressed(numParticipants);
	bs.ReadCompressed(numSpectators);
	unsigned i;
	LobbyUserClientDetails *detail;
	LobbyClientUserId *idArray = (LobbyClientUserId *) alloca(sizeof(LobbyClientUserId)*(numParticipants+numSpectators));
	for (i=0; i < numParticipants; i++)
	{
		detail = (LobbyUserClientDetails *) alloca(sizeof(LobbyUserClientDetails));
		// Crashes, I guess alloca doesn't work with constructors/destructors
//		detail->userData = (LobbyDBSpec::CreateUser_Data *) alloca(sizeof(LobbyDBSpec::CreateUser_Data));
		detail->userData = new LobbyDBSpec::CreateUser_Data;
		detail->userId=idArray+i;
		DeserializeClientSafeCreateUserData(detail->userId, &detail->systemAddress, detail->userData, &detail->readyToPlay, &bs);
		notification.matchParticipants.Insert(detail);

		// Also add to recently met users
		detail = new LobbyUserClientDetails;
		memcpy(detail, notification.matchParticipants[notification.matchParticipants.Size()-1], sizeof(LobbyUserClientDetails));
		detail->userData = new LobbyDBSpec::CreateUser_Data;
		memcpy(detail->userData, notification.matchParticipants[notification.matchParticipants.Size()-1]->userData, sizeof(LobbyDBSpec::CreateUser_Data));
		recentlyMetUsers.Insert(detail);
	}

	for (i=0; i < numSpectators; i++)
	{
		detail = (LobbyUserClientDetails *) alloca(sizeof(LobbyUserClientDetails));
		detail->userData = new LobbyDBSpec::CreateUser_Data;
		detail->userId=idArray+i+numParticipants;
		DeserializeClientSafeCreateUserData(detail->userId, &detail->systemAddress, detail->userData, &detail->readyToPlay, &bs);
		notification.matchSpectators.Insert(detail);
	}
	
	// Notify game
	if (cb)
		cb->StartGame_Notify(&notification);

//	lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

	// Quick match is not in a room to begin with
	if (notification.wasQuickMatch==false)
		LeaveCurrentRoom(true);
	else
		lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

	// Needed because alloca doesn't work (above)
	for (i=0; (unsigned short) i < numParticipants+numSpectators; i++)
	{
		delete notification.matchParticipants[i]->userData;
	}
}
void LobbyClientPC::NotifyAddFriendResponse_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	FriendInvitation_PC noticeStruct;
	noticeStruct.Deserialize(&bs);

	if (noticeStruct.request==false && noticeStruct.accepted==true)
		AddFriend(true,noticeStruct.inviteeHandle,noticeStruct._invitee);

	// Notify game
	if (cb)
		cb->FriendInvitation_Notify(&noticeStruct);
}
void LobbyClientPC::NotifyClanDissolved_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanDissolved_PC noticeStruct;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanDissolved_Notify(&noticeStruct);

	unsigned cdi;
	for (cdi=0; cdi < noticeStruct.memberIds.Size(); cdi++)
		delete noticeStruct.memberIds[cdi];
}
void LobbyClientPC::NotifyLeaveClan_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LeaveClan_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->LeaveClan_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyWithdrawClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanJoinInvitationWithdrawn_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanJoinInvitationWithdrawn_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
}
void LobbyClientPC::NotifySendClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanJoinInvitation_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanJoinInvitation_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
}
void LobbyClientPC::NotifyAcceptClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanJoinInvitationAccepted_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanJoinInvitationAccepted_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyRejectClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanJoinInvitationRejected_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanJoinInvitationRejected_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifySendClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberJoinRequest_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberJoinRequest_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyWithdrawClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberJoinRequestWithdrawn_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberJoinRequestWithdrawn_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyAcceptClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberJoinRequestAccepted_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberJoinRequestAccepted_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyRejectClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberJoinRequestRejected_PC noticeStruct;
	noticeStruct.clanId = new ClanId;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberJoinRequestRejected_Notify(&noticeStruct);

	delete noticeStruct.clanId;
	delete noticeStruct.clanLeaderOrSubleaderId;
}

void LobbyClientPC::NotifySetClanMemberRank_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberRank_PC noticeStruct;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberRank_Notify(&noticeStruct);

	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyClanMemberKicked_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberKicked_PC noticeStruct;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberKicked_Notify(&noticeStruct);

	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
void LobbyClientPC::NotifyClanMemberUnblacklisted_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	ClanMemberUnblacklisted_PC noticeStruct;
	noticeStruct.clanLeaderOrSubleaderId = new LobbyClientUserId;
	noticeStruct.userId = new LobbyClientUserId;
	noticeStruct.Deserialize(&bs);

	// Notify game
	if (cb)
		cb->ClanMemberUnblacklisted_Notify(&noticeStruct);

	delete noticeStruct.clanLeaderOrSubleaderId;
	delete noticeStruct.userId;
}
bool LobbyClientPC::IsFriend(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < friendsList.Size(); i++)
	{
		if (*(friendsList[i]->id)==id)
			return true;
	}
	return false;
}
void LobbyClientPC::AddFriend(bool isOnline, const char *friendName, LobbyClientUserId id)
{
	if (IsFriend(id))
		return;
	LC_Friend *f = new LC_Friend;
	f->handle = new char [strlen(friendName)+1];
	strcpy(f->handle, friendName);
	f->id=new LobbyClientUserId(id);
	f->isOnline=isOnline;
	friendsList.Insert(f);
}
bool LobbyClientPC::RemoveFriend(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < friendsList.Size(); i++)
	{
		if (*(friendsList[i]->id)==id)
		{
			delete friendsList[i];
			friendsList.RemoveAtIndex(i);
			return true;
		}
	}
	return false;
}
void LobbyClientPC::AddIgnoredUser(LobbyClientUserId id, const char *actions, long long creationTime)
{
	if (IsIgnoredUser(id))
		return;
	LC_Ignored *f = new LC_Ignored;
	f->actionString = new char [strlen(actions)+1];
	strcpy(f->actionString, actions);
	f->id=new LobbyClientUserId(id);
	f->creationTime=creationTime;
	ignoreList.Insert(f);
}
bool LobbyClientPC::RemoveIgnoredUser(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < ignoreList.Size(); i++)
	{
		if (*(ignoreList[i]->id)==id)
		{
			delete ignoreList[i];
			ignoreList.RemoveAtIndex(i);
			return true;
		}
	}
	return false;
}
bool LobbyClientPC::IsIgnoredUser(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < ignoreList.Size(); i++)
	{
		if (*(ignoreList[i]->id)==id)
			return true;
	}
	return false;
}
void LobbyClientPC::AddEmail(LobbyDBSpec::SendEmail_Data *email, bool inbox)
{
	if (inbox)
		inboxEmails.Insert(email);
	else
		sentEmails.Insert(email);
}
void LobbyClientPC::DeserializeClientSafeCreateUserData(LobbyClientUserId *userId, SystemAddress *systemAddress, LobbyDBSpec::CreateUser_Data *data, bool *readyToPlay, RakNet::BitStream *bs)
{
	char userHandle[512];
	stringCompressor->DecodeString(userHandle,512,bs,0);
	data->handle=userHandle;
	bs->Read(*userId);
	data->DeserializeBinaryData(bs);
	data->DeserializeCaptions(bs);
	data->DeserializeAdminLevel(bs);
	bs->Read(*systemAddress);
	bs->Read(*readyToPlay);
}
LobbyClientUser * LobbyClientPC::GetLobbyClientUser(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < lobbyClientUsers.Size(); i++)
		if (lobbyClientUsers[i]->userId==id)
			return lobbyClientUsers[i];
	return 0;
}
void LobbyClientPC::AddLobbyClientUser(LobbyClientUser *user)
{
	RakAssert(GetLobbyClientUser(user->userId)==0);
	lobbyClientUsers.Insert(user);
}
void LobbyClientPC::ClearActionHistory(void)
{
	unsigned i;
	for (i=0; i < actionHistory.Size(); i++)
		delete actionHistory[i];
	actionHistory.Clear();
}
void LobbyClientPC::ClearRecentlyMetUsers(void)
{
	unsigned i;
	for (i=0; i < recentlyMetUsers.Size(); i++)
	{
		delete recentlyMetUsers[i]->userData;
		delete recentlyMetUsers[i];
	}
	recentlyMetUsers.Clear();
}
void LobbyClientPC::ClearLobbyClientUsers(void)
{
	unsigned i;
	for (i=0; i < lobbyClientUsers.Size(); i++)
		delete lobbyClientUsers[i];
	lobbyClientUsers.Clear();
}
void LobbyClientPC::ClearLobbyClientUser(LobbyClientUserId id)
{
	unsigned i;
	for (i=0; i < lobbyClientUsers.Size(); i++)
	{
		if (lobbyClientUsers[i]->userId==id)
		{
			delete lobbyClientUsers[i];
			lobbyClientUsers.RemoveAtIndex(i);
			return;
		}
	}
}
void LobbyClientPC::ClearEmails(bool inbox)
{
	unsigned i;
	if (inbox)
	{
		for (i=0; i < inboxEmails.Size(); i++)
			delete inboxEmails[i];
		inboxEmails.Clear(true);
	}
	else
	{
		for (i=0; i < sentEmails.Size(); i++)
			delete sentEmails[i];
		sentEmails.Clear(true);
	}
	
}
void LobbyClientPC::UpdateEmail(EmailId id, int status, bool wasOpened)
{
	unsigned i;
	for (i=0; i < inboxEmails.Size(); i++)
	{
		if (inboxEmails[i]->emailMessageID==id)
		{
			inboxEmails[i]->status=status;
			inboxEmails[i]->wasOpened=wasOpened;
			return;
		}
	}
	for (i=0; i < sentEmails.Size(); i++)
	{
		if (sentEmails[i]->emailMessageID==id)
		{
			sentEmails[i]->status=status;
			sentEmails[i]->wasOpened=wasOpened;
			return;
		}
	}
}
void LobbyClientPC::RemoveEmail(EmailId id)
{
	unsigned i;
	for (i=0; i < inboxEmails.Size(); i++)
	{
		if (inboxEmails[i]->emailMessageID==id)
		{
			delete inboxEmails[i];
			inboxEmails.RemoveAtIndex(i);
			return;
		}
	}
	for (i=0; i < sentEmails.Size(); i++)
	{
		if (sentEmails[i]->emailMessageID==id)
		{
			delete sentEmails[i];
			sentEmails.RemoveAtIndex(i);
			return;
		}
	}
}
void LobbyClientPC::LeaveCurrentRoom(bool roomIsDead)
{
	RakAssert(IsInRoom());
	if (roomIsDead)
		allRooms.RemoveRow(currentRoom.rowId);
	currentRoom.Clear();
	lobbyUserStatus=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
	readyToPlay=false;

	// Clear all member's data
	ClearLobbyClientUsers();
}

void LobbyClientPC::DeserializeAndAddClientSafeUser(RakNet::BitStream *bs)
{
	LobbyClientUser *newUser = new LobbyClientUser;
	newUser->userData=new LobbyDBSpec::CreateUser_Data;

	DeserializeClientSafeCreateUserData(&newUser->userId, &newUser->systemAddress, newUser->userData, & newUser->readyToPlay, bs);
	AddLobbyClientUser(newUser);
}

void LobbyClientPC::SerializeHandleOrId(const char *userHandle, LobbyClientUserId *userId, RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(userHandle,512,bs,0);
	if (userId)
	{
		bs->Write(true);
		bs->Write(*userId);
	}
	else
		bs->Write(false);
}
void LobbyClientPC::SerializeOptionalId(unsigned int *id, RakNet::BitStream *bs)
{
	if (id)
		bs->Write(*id);
	else
		bs->Write((unsigned int) 0);
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif
