#include "LobbyServer.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "StringCompressor.h"
#include "LobbyDBSpec.h"
#include "RankingServerDBSpec.h"
#include "DS_Table.h"
#include "TableSerializer.h"
#include "RakPeerInterface.h"
#include "GetTime.h"
#include <RakAssert.h>

using namespace RakNet;

static const int MAX_BINARY_DATA_LENGTH=10000000;
static const char *DEFAULT_FRIEND_INVITE_BODY="I would like to be your friend.";
static const char *DEFAULT_FRIEND_ACCEPT_BODY="I have accepted your friends invitation.";
static const char *DEFAULT_FRIEND_DECLINE_BODY="I do not wish to be friends with you.";
static const int MAX_ALLOWED_EMAIL_RECIPIENTS=128;
static const int LOBBY_SERVER_MAX_PUBLIC_SLOTS=128;
static const int LOBBY_SERVER_MAX_PRIVATE_SLOTS=128;
static const bool DEFAULT_ASCENDING_SORT_DOWNLOAD_CLANS_DB_QUERY=false;

#ifdef _MSC_VER
#pragma warning( push )
#endif

int UserCompBySysAddr( SystemAddress const &key, LobbyServerUser * const &data )
{
	if (key < data->systemAddress)
		return -1;
	if (key == data->systemAddress)
		return 0;
	return 1;
}
int UserCompById( LobbyDBSpec::DatabaseKey const &key, LobbyServerUser * const &data )
{
	if (key < data->GetID())
		return -1;
	if (key == data->GetID())
		return 0;
	return 1;
}
int ClanCompByClanId( ClanId const &key, LobbyServerClan * const &data )
{
	if (key < data->clanId)
		return -1;
	if (key == data->clanId)
		return 0;
	return 1;
}
int ClanCompByHandle( RakNet::RakString const &key, LobbyServerClan * const &data )
{
	if (key < data->handle)
		return -1;
	if (key == data->handle)
		return 0;
	return 1;
}
LobbyServerUser::LobbyServerUser()
{
	downloadedFriends=false;
	data=0;
	status=LOBBY_USER_STATUS_OFFLINE;
	title=0;
	currentRoom=0;
	titleId=0;
}
LobbyServerUser::~LobbyServerUser()
{
	if (data)
		data->Deref();

	unsigned i;
	for (i=0; i < friends.Size(); i++)
		friends[i]->Deref();

	for (i=0; i < ignoredUsers.Size(); i++)
		ignoredUsers[i]->Deref();

	// Just a reference
	clanMembers.Clear();
}
bool LobbyServerUser::AddFriend(LobbyDBSpec::AddFriend_Data *_friend)
{
	if (IsFriend(_friend->friendId.databaseRowId)==false)
	{
		_friend->AddRef();
		friends.Insert(_friend);
		return true;
	}
	return false;
}
void LobbyServerUser::RemoveFriend(LobbyDBSpec::DatabaseKey friendId)
{
	unsigned i;
	for (i=0; i < friends.Size(); i++)
	{
		if (friends[i]->friendId.databaseRowId==friendId)
		{
			friends[i]->Deref();
			friends.RemoveAtIndex(i);
			break;
		}
	}
}
bool LobbyServerUser::IsFriend(LobbyDBSpec::DatabaseKey friendId)
{
	unsigned i;
	for (i=0; i < friends.Size(); i++)
	{
		if (friends[i]->friendId.databaseRowId==friendId)
			return true;
	}
	return false;
}
bool LobbyServerUser::AddIgnoredUser(LobbyDBSpec::AddToIgnoreList_Data *ignoredUser)
{
	if (IsIgnoredUser(ignoredUser->ignoredUser.databaseRowId)==false)
	{
		ignoredUser->AddRef();
		ignoredUsers.Insert(ignoredUser);
		return true;
	}
	return false;
}
bool LobbyServerUser::IsInClan(void) const
{
	return GetFirstClan()!=0;
}
LobbyServerClan* LobbyServerUser::GetFirstClan(void) const
{
	unsigned i;
	for (i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
			return clanMembers[i]->clan;
	}
	return 0;
}
void LobbyServerUser::RemoveIgnoredUser(LobbyDBSpec::DatabaseKey userId)
{
	unsigned i;
	for (i=0; i < ignoredUsers.Size(); i++)
	{
		if (ignoredUsers[i]->ignoredUser.databaseRowId==userId)
		{
			ignoredUsers[i]->Deref();
			ignoredUsers.RemoveAtIndex(i);
			break;
		}
	}
}
bool LobbyServerUser::IsIgnoredUser(LobbyDBSpec::DatabaseKey userId)
{
	unsigned i;
	for (i=0; i < ignoredUsers.Size(); i++)
	{
		if (ignoredUsers[i]->ignoredUser.databaseRowId==userId)
			return true;
	}
	return false;
}
LobbyServerClan::LobbyServerClan()
{
}
LobbyServerClan::~LobbyServerClan()
{
	for (unsigned i=0; i < clanMembers.Size(); i++)
		delete clanMembers[i];
	clanMembers.Clear();
}
unsigned LobbyServerClan::GetClanMemberIndex(LobbyDBSpec::DatabaseKey userId)
{
	for (unsigned i=0; i < clanMembers.Size(); i++)
		if (clanMembers[i]->userId==userId)
			return i;
	return (unsigned) -1;
}
void LobbyServerClan::DeleteClanMember(unsigned index)
{
	clanMembers.RemoveAtIndexFast(index);
}
LobbyServerClanMember *LobbyServerClan::GetLeader(void) const
{
	unsigned i;
	for (i=0; i < clanMembers.Size(); i++)
	{
		if (clanMembers[i]->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
		{
			return clanMembers[i];
		}
	}
	return 0;
}
void LobbyServerClan::InsertMember(LobbyServerClanMember *lscm)
{
	if (clanMembers.GetIndexOf(lscm)!=-1)
		return;
	clanMembers.Push(lscm);
}
void LobbyServerClan::SetMembersDirty(bool b)
{
	unsigned i;
	for (i=0; i < clanMembers.Size(); i++)
		clanMembers[i]->isDirty=b;
}
LobbyServerClanMember::LobbyServerClanMember()
{
	memberData=0;
	clanJoinsPending=0;
	DataStructures::OrderedList<LobbyServerClanMember*, LobbyServerClanMember*>::IMPLEMENT_DEFAULT_COMPARISON();
}
LobbyServerClanMember::~LobbyServerClanMember()
{
	if (memberData)
		delete memberData;
}
LobbyDBSpec::DatabaseKey LobbyServerUser::GetID(void) const
{
	return data->id.databaseRowId;
}
LobbyServer::LobbyServer()
{
	orderingChannel=0;
	rakPeer=0;
	packetPriority=MEDIUM_PRIORITY;
}
LobbyServer::~LobbyServer()
{
	Clear();
}
void LobbyServer::Clear(void)
{
	ClearUsers();
	ClearTitles();
	ClearLobbyGames();
	ClearClansAndClanMembers();
}
void LobbyServer::OnAttach(RakPeerInterface *peer)
{
	rakPeer=peer;
}
PluginReceiveResult LobbyServer::OnReceive(RakPeerInterface *peer, Packet *packet)
{
	if (packet->data[0]==ID_LOBBY_GENERAL && packet->length>=2)
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
		case LOBBY_MSGID_SEND_IM:
			SendIM_NetMsg(packet);
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
		case LOBBY_MSGID_SET_READY_TO_PLAY_STATUS:
			SetReadyToPlayStatus_NetMsg(packet);
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
		case LOBBY_MSGID_VALIDATE_USER_KEY:
			ValidateUserKey_NetMsg(packet);
			break;
		}
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	else if (packet->data[0]==ID_DISCONNECTION_NOTIFICATION || packet->data[0]==ID_CONNECTION_LOST)
	{
		RemoveUser(packet->systemAddress);
	}

	return RR_CONTINUE_PROCESSING;
}
void LobbyServer::OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress)
{
	RemoveUser(systemAddress);
}
void LobbyServer::OnShutdown(RakPeerInterface *peer)
{
	Clear();
}
void LobbyServer::Update(RakPeerInterface *peer)
{
	RakNetTime time = RakNet::GetTime();
	unsigned i;
	LobbyServerUser *user;
	TitleValidationDBSpec::DatabaseKey titleIdToMatch;

	// Try to match users. I will make this more sophisticated and correct later
	if (quickMatchUsers.Size()>=2)
	{
		if (quickMatchUsers[0]->quickMatchUserRequirement >= (int) quickMatchUsers.Size())
		{
			titleIdToMatch=quickMatchUsers[0]->title->titleId;
			DataStructures::List<LobbyServerUser *> matchedUsers;
			for (i=0; i < quickMatchUsers.Size(); i++)
			{
				if (titleIdToMatch==quickMatchUsers[i]->title->titleId)
					matchedUsers.Insert(quickMatchUsers[i]);
			}

			if ((int) matchedUsers.Size()>=quickMatchUsers[0]->quickMatchUserRequirement)
			{
				// Remove matched users
				i=0;
				while (i<quickMatchUsers.Size())
				{
					if (titleIdToMatch==quickMatchUsers[i]->title->titleId)
					{
						quickMatchUsers.RemoveAtIndex(i);
					}
					else
					{
						i++;
					}
				}

				// Start game with matched users
				RakNet::BitStream bs;
				bs.Write((unsigned char)ID_LOBBY_GENERAL);
				bs.Write((unsigned char)LOBBY_MSGID_NOTIFY_START_GAME);
				bs.Write(true); // Quick match
				bs.Write(matchedUsers[0]->systemAddress); // Arbitrary moderator
				bs.Write(titleIdToMatch);
				bs.WriteCompressed((unsigned short) matchedUsers.Size());
				// No spectators
				bs.WriteCompressed((unsigned short) 0);
				for (i=0; i < matchedUsers.Size(); i++)
					SerializeClientSafeCreateUserData(matchedUsers[i], &bs);
				for (i=0; i < matchedUsers.Size(); i++)
				{
					Send(&bs, matchedUsers[i]->systemAddress);
					matchedUsers[i]->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
				}
			}
		}
	}
	
	i=0;
	while (i<quickMatchUsers.Size())
	{
		if (quickMatchUsers[i]->quickMatchStopWaiting<time)
		{
			user = quickMatchUsers[i];
			RakNet::BitStream bs;
			bs.Write((unsigned char)ID_LOBBY_GENERAL);
			bs.Write((unsigned char)LOBBY_MSGID_NOTIFY_QUICK_MATCH_TIMEOUT);

			QuickMatchTimeout_PC quickMatchTimeoutContext;
			quickMatchTimeoutContext.Serialize(&bs);
			Send(&bs, user->systemAddress);

			RakAssert(user->status==LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH);
			user->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

			quickMatchUsers.RemoveAtIndex(i);

		}
		else
			i++;
	}
}
void LobbyServer::Login_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char userIdStr[512];
	char userPw[512];
	
	stringCompressor->DecodeString(userPw, sizeof(userPw), &bs, 0);
	stringCompressor->DecodeString(userIdStr, sizeof(userIdStr), &bs, 0);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_LOGIN);
	if (GetUserByHandle(userIdStr))
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("This username is already connected.", 512, &reply, 0);
		Send(&reply, packet->systemAddress);
		return;
	}

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		if (lsu->status==LOBBY_USER_STATUS_LOGGING_ON)
		{
			// Already logging on
			reply.Write((unsigned char)LC_RC_IN_PROGRESS);
		}
		else
		{
			// Already logged on
			reply.Write((unsigned char)LC_RC_SUCCESS);
		}
		Send(&reply, packet->systemAddress);
	}
	else
	{
		Login_DBQuery(packet->systemAddress, userIdStr, userPw);
	}
}
void LobbyServer::Login_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res, const char *userPw_in)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_LOGIN);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (res->userFound==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);		
	}
	else if (res->isBanned)
	{
		reply.Write((unsigned char)LC_RC_BANNED_USER_ID);
		stringCompressor->EncodeString(res->banOrSuspensionReason.C_String(),512,&reply,0);
	}
	else if (res->isSuspended)
	{
		reply.Write((unsigned char)LC_RC_SUSPENDED_USER_ID);
		reply.Write(res->suspensionExpiration);
		stringCompressor->EncodeString(res->banOrSuspensionReason.C_String(),512,&reply,0);
	}
	else if (stricmp(res->output.password.C_String(), userPw_in)!=0)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Bad password.",512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(res->id.databaseRowId);
		res->output.Serialize(&reply);

		LobbyServerUser *user = new LobbyServerUser;
		user->data=res;
		user->readyToPlay=false;
		res->AddRef();
		user->pendingClanCount=0;
		user->systemAddress=systemAddress;
		user->connectionTime=RakNet::GetTime();
		user->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
		user->title=0;

		// Inefficient, do it instead when the list is downloaded
		/*
		// Notify friends who had you on their lists, but offline
		unsigned i,j;
		for (i=0; i < users.Size(); i++)
		{
			for (j=0; j < users[i]->friends.Size(); j++)
			{
				if (users[i]->friends[j]->friendId.userId==user->GetID())
				{
					NotifyFriendStatusChange(users[i]->systemAddress, user->GetID(), user->data->id.userHandle.C_String(), FSC_FRIEND_ONLINE);
				}
			}
		}
		*/

		usersBySysAddr.Insert(systemAddress, user, true);
		usersByID.Insert(user->GetID(), user, true);

		GetClans_DBQuery(systemAddress, user->GetID(),0, DEFAULT_ASCENDING_SORT_DOWNLOAD_CLANS_DB_QUERY);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::AddFriend_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddFriend_Data *res, const char *messageBody, unsigned char language)
{
	LobbyServerUser *myUser = GetUserByID(res->id.databaseRowId);
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ACCEPT_ADD_FRIEND_INVITE);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		stringCompressor->EncodeString(&res->friendId.handle,512,&reply,0);
		reply.Write(res->friendId.databaseRowId);
	}
	else if (res->userFound==false || res->friendFound==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);
		stringCompressor->EncodeString(res->queryResult.C_String(),512,&reply,0);
		stringCompressor->EncodeString(&res->friendId.handle,512,&reply,0);
		reply.Write(res->friendId.databaseRowId);
	}
	else if (res->success==false)
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString(res->queryResult.C_String(),512,&reply,0);

		stringCompressor->EncodeString(&res->friendId.handle,512,&reply,0);
		reply.Write(res->friendId.databaseRowId);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);

		LobbyServerUser *friendUser = GetUserByID(res->friendId.databaseRowId);
		LobbyDBSpec::AddFriend_Data *afd;

		if (myUser)
			myUser->AddFriend(res);
		
		afd = new LobbyDBSpec::AddFriend_Data;

		afd->friendId.hasDatabaseRowId=true;
		afd->friendId.databaseRowId=res->friendId.databaseRowId;
		if (friendUser)
			afd->friendId.handle=friendUser->data->id.handle;
		else
			afd->friendId.handle=res->friendId.handle;

		afd->id.hasDatabaseRowId=true;
		afd->id.databaseRowId=res->id.databaseRowId;
		if (myUser)
			afd->id.handle=myUser->data->id.handle;
		else
			afd->id.handle=res->id.handle;

		afd->creationTime=res->creationTime;

		// Send friend notification acceptance to u2
	//	SendIM(u1, u2, messageBody, language);


		// Invite accepted
		FriendInvitation_PC noticeStruct;
		RakNet::BitStream invitationBS;
		invitationBS.Write((unsigned char)ID_LOBBY_GENERAL);
		invitationBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ADD_FRIEND_RESPONSE);
		noticeStruct._invitee=afd->friendId.databaseRowId; // Replying switches lsu and potentialFriend
		noticeStruct._invitor=res->id.databaseRowId;// Replying switches lsu and potentialFriend
		noticeStruct.messageBodyEncodingLanguage=language;
		strncpy(noticeStruct.invitorHandle, afd->id.handle.C_String(), sizeof(noticeStruct.invitorHandle)-1);
		noticeStruct.invitorHandle[sizeof(noticeStruct.invitorHandle)-1]=0;
		strncpy(noticeStruct.inviteeHandle, afd->friendId.handle.C_String(), sizeof(noticeStruct.inviteeHandle)-1);
		noticeStruct.inviteeHandle[sizeof(noticeStruct.inviteeHandle)-1]=0;
		noticeStruct.request=false;
		noticeStruct.accepted=true;
		noticeStruct.Serialize(&invitationBS);
		if (friendUser)
			Send(&invitationBS, friendUser->systemAddress);


		if (friendUser)
			NotifyFriendStatusChange(myUser->systemAddress, friendUser->GetID(), friendUser->data->id.handle.C_String(), FRIEND_STATUS_NEW_FRIEND);
		if (myUser)
			NotifyFriendStatusChange(friendUser->systemAddress, myUser->GetID(), myUser->data->id.handle.C_String(), FRIEND_STATUS_NEW_FRIEND);

		if (friendUser!=0)
			reply.Write(true);
		else
			reply.Write(false);

		stringCompressor->EncodeString(&afd->friendId.handle,512,&reply,0);
		reply.Write(afd->friendId.databaseRowId);
		if (friendUser==0 || friendUser->AddFriend(afd)==false)
			afd->Deref();
	}

	if (myUser)
		Send(&reply,myUser->systemAddress);
}
void LobbyServer::NotifyFriendStatusChange(SystemAddress target, LobbyDBSpec::DatabaseKey friendId, const char *friendHandle, unsigned char status)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_NOTIFY_FRIEND_STATUS);
	reply.Write((unsigned char) status);
	reply.Write(friendId);
	reply.Write(true);
	stringCompressor->EncodeString(friendHandle,512,&reply,0);
	Send(&reply,target);
}
void LobbyServer::RetrievePasswordRecoveryQuestion_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_RETRIEVE_PASSWORD_RECOVERY_QUESTION);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (res->userFound==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);		
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		stringCompressor->EncodeString(res->output.passwordRecoveryQuestion.C_String(),512,&reply,0);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::RetrievePassword_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res, const char *passwordRecoveryAnswer)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_RETRIEVE_PASSWORD);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (res->userFound==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);		
	}
	else if (stricmp(passwordRecoveryAnswer, res->output.passwordRecoveryAnswer.C_String())!=0)
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		stringCompressor->EncodeString(res->output.password.C_String(),512,&reply,0);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::GetFriends_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetFriends_Data *res, SystemAddress systemAddress)
{
	LobbyServerUser *lsu = GetUserByAddress(systemAddress);
	if (lsu)
	{
		lsu->downloadedFriends=true;

		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_FRIENDS);
		if (dbSuccess==false)
		{
			reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
			stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		}
		else
		{
			reply.Write((unsigned char)LC_RC_SUCCESS);
			reply.WriteCompressed(res->friends.Size());
			unsigned i;
			LobbyServerUser *_friend;
			for (i=0; i < res->friends.Size(); i++)
			{
				_friend = GetUserByID(res->friends[i]->friendId.databaseRowId);
				bool friendIsOnline = _friend!=0;
				reply.Write(friendIsOnline);
				reply.Write(res->friends[i]->friendId.databaseRowId);
				stringCompressor->EncodeString(res->friends[i]->friendId.handle.C_String(), 512, &reply, 0);
				lsu->AddFriend(res->friends[i]);

				// Tell our friends that we are online if they have also already downloaded us into their friends list
				if (_friend)
				{
					if (_friend->IsFriend(lsu->GetID()))
					{
						NotifyFriendStatusChange(_friend->systemAddress, lsu->GetID(), lsu->data->id.handle.C_String(), FRIEND_STATUS_FRIEND_ONLINE);
						NotifyFriendStatusChange(lsu->systemAddress, res->friends[i]->friendId.databaseRowId, res->friends[i]->friendId.handle.C_String(), friendIsOnline ? FRIEND_STATUS_FRIEND_ONLINE : FRIEND_STATUS_FRIEND_OFFLINE);
					}
				}

			}
		}
		Send(&reply,systemAddress);
	}
}
void LobbyServer::CreateUser_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::CreateUser_Data *res)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REGISTER_ACCOUNT);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (querySuccess==false)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::UpdateHandle_DBResult(LobbyDBSpec::ChangeUserHandle_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(res->id.databaseRowId);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_CHANGE_HANDLE);
		
		if (res->success)
		{
			// TODO - Update viewers of this user, so they get the handle updated accordingly
			lsu->data->id.handle=res->newHandle;
			reply.Write((unsigned char)LC_RC_SUCCESS);
		}
		else
		{
			reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
			stringCompressor->EncodeString(res->queryResult.C_String(),512,&reply,0);
		}

		Send(&reply,lsu->systemAddress);
		
	}
}
void LobbyServer::GetIgnoreList_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetIgnoreList_Data *res)
{
	LobbyServerUser *lsu = GetUserByAddress(systemAddress);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_IGNORE_LIST);

		if (dbSuccess)
		{
			reply.Write((unsigned char)LC_RC_SUCCESS);

			reply.WriteCompressed(res->ignoredUsers.Size());
			unsigned i;
			for (i=0; i < res->ignoredUsers.Size(); i++)
			{
				reply.Write(res->ignoredUsers[i]->ignoredUser.databaseRowId);
				stringCompressor->EncodeString(res->ignoredUsers[i]->actions.C_String(), 4096, &reply, 0);
				reply.Write(res->ignoredUsers[i]->creationTime);

				// Store the ignore list in memory too
				lsu->AddIgnoredUser(res->ignoredUsers[i]);
			}
		}
		else
		{
			reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
			stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		}

		Send(&reply,lsu->systemAddress);
	}
}
void LobbyServer::AddToIgnoreList_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey ignoredUserId,
										   LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::AddToIgnoreList_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(myId);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_UPDATE_IGNORE_LIST);

		if (dbSuccess==false)
		{
			reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
			stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		}
		else if (querySuccess==false)
		{
			reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
			stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		}
		else
		{
			reply.Write((unsigned char)LC_RC_SUCCESS);
			reply.Write(res->ignoredUser.databaseRowId);
			stringCompressor->EncodeString(res->actions.C_String(), 4096, &reply, 0);
			reply.Write(res->creationTime);


			lsu->AddIgnoredUser(res);
		}

		Send(&reply,lsu->systemAddress);
	}
}
void LobbyServer::RemoveFromIgnoreList_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey unignoredUserId, LobbyDBSpec::DatabaseKey myId)
{
	LobbyServerUser *lsu = GetUserByID(myId);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REMOVE_FROM_IGNORE_LIST);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(unignoredUserId);
		lsu->RemoveIgnoredUser(unignoredUserId);
	}

	Send(&reply,lsu->systemAddress);
}
void LobbyServer::SendEmail_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::SendEmail_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(res->id.databaseRowId);
	if (lsu==0)
		return;

	RakNet::BitStream reply, emailNotify;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_EMAIL);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (querySuccess==false)
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);

		// This won't work because we may have sent to multiple recipients.
		// res->Serialize(&reply);
		// The user should redownload the list of sent emails.

		// Send back that we are successful in sending these emails
		emailNotify.Write((unsigned char)ID_LOBBY_GENERAL);
		emailNotify.Write((unsigned char)LOBBY_MSGID_INCOMING_EMAIL);
		emailNotify.Write(lsu->GetID());
		stringCompressor->EncodeString(lsu->data->id.handle.C_String(), 512, &emailNotify, 0);
		stringCompressor->EncodeString(res->subject.C_String(), 4096, &emailNotify, 0);

		// Send incoming email notifications to recipients.
		LobbyServerUser *emailRecipient;
		unsigned i;
		for (i=0; i < res->to.Size(); i++)
		{
			if (res->to[i].hasDatabaseRowId)
				emailRecipient = GetUserByID(res->to[i].databaseRowId);
			else
				emailRecipient = GetUserByHandle(res->to[i].handle.C_String());

			if (emailRecipient)
			{
				Send(&emailNotify,emailRecipient->systemAddress);
			}			
		}
	}

	Send(&reply,lsu->systemAddress);
}
void LobbyServer::GetEmails_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::GetEmails_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(res->id.databaseRowId);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_EMAILS);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (querySuccess==false)
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(res->inbox);
		reply.WriteCompressed(res->emails.Size());
		unsigned i;
		for (i=0; i < res->emails.Size(); i++)
		{
			res->emails[i]->Serialize(&reply);
		}		
	}
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::DeleteEmail_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DeleteEmail_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(myId);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DELETE_EMAIL);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(res->emailMessageID);
	}
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::UpdateEmailStatus_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::UpdateEmailStatus_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(myId);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_UPDATE_EMAIL_STATUS);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(res->status);
		reply.Write(res->emailMessageID);
		reply.Write(res->wasOpened);
	}
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::AddToActionHistory_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddToActionHistory_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ADD_TO_ACTION_HISTORY);
	LobbyServerUser *lsu = GetUserByAddress(systemAddress);
	if (lsu==0)
		return;
	res->id.hasDatabaseRowId=true;
	res->id.handle=lsu->data->id.handle;
	res->id.databaseRowId=lsu->GetID();
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::GetActionHistory_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetActionHistory_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_ACTION_HISTORY);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::SubmitMatch_DBResult(bool dbSuccess, const char *queryErrorMessage, RankingServerDBSpec::SubmitMatch_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SUBMIT_MATCH);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		//res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::GetRatingForParticipant_DBResult(bool dbSuccess, const char *queryErrorMessage, RankingServerDBSpec::GetRatingForParticipant_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_RATING);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::UpdateClanMember_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::UpdateClanMember_Data *res, unsigned int lobbyMsgId, SystemAddress systemAddress, LobbyDBSpec::DatabaseKey clanSourceMemberId, RakNet::RakString clanSourceMemberHandle)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)lobbyMsgId);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}
	else if (res->failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(res->failureMessage.C_String(),512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}

	LobbyServerClanMember *lsm;
	LobbyServerClan *lsc = GetClanById(res->clanId.databaseRowId);
	if (lsc==0)
	{
		// Create clan in memory
		lsc = new LobbyServerClan;
		lsc->clanId=res->clanId.databaseRowId;
		lsc->handle=res->clanId.handle;
		AddClan(lsc);
	}

	lsm = GetClanMember(lsc, res->userId.databaseRowId);

	if (lsm==0)
	{
		lsm = CreateClanMember(lsc, res->userId.databaseRowId, res);
		lsm->clanSourceMemberId=clanSourceMemberId;
		lsm->clanSourceMemberHandle=clanSourceMemberHandle;
	}
	
	lsm->busyStatus=LobbyServerClanMember::LSCM_NOT_BUSY;
	lsm->clanStatus=res->mEStatus1;

	unsigned i;
	RakNet::BitStream notificationBs;
	notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
	LobbyClientUserId *userIdPtr = &lsm->userId;
	RakNet::RakString userHandle;
	LobbyServerUser *lsu = GetUserByID(lsm->userId);
	if (lsu)
		userHandle = lsu->data->id.handle;
	ClanId *clanIdPtr;
	RakNet::RakString clanHandle;
	clanIdPtr=&lsc->clanId;
	clanHandle=lsc->handle;

	// send LOBBY_MSGID_NOTIFY_ to online clan members for important events
	switch (lobbyMsgId)
	{
	case LOBBY_MSGID_ACCEPT_CLAN_MEMBER_JOIN_REQUEST:
		{
			ClanMemberJoinRequestAccepted_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
			notification.userId=userIdPtr;
			notification.userHandle=userHandle;
			notification.clanLeaderOrSubleaderId=&res->userId.databaseRowId;
			notification.clanLeaderOrSubleaderHandle=GetHandleByID(res->userId.databaseRowId);
			notification.clanId=clanIdPtr;
			notification.clanHandle=clanHandle;

			/// This is sent to the new member, and also to all online members of the clan
			notification.userIdIsYou=true;
			notification.joinRequestAcceptorIsYou=false;
			notification.Serialize(&notificationBs);
			Send(&notificationBs, systemAddress);

			// Send to join request acceptor
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->userId==lsm->clanSourceMemberId)
				{
					notificationBs.Reset();
					notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
					notification.userIdIsYou=false;
					notification.joinRequestAcceptorIsYou=true;
					notification.Serialize(&notificationBs);
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					break;
				}
			}

			// Send to everyone else
			notificationBs.Reset();
			notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
			notification.joinRequestAcceptorIsYou=false;
			notification.Serialize(&notificationBs);
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->userId!=lsm->clanSourceMemberId &&
					lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
			}
		};
		break;
	case LOBBY_MSGID_ACCEPT_CLAN_JOIN_INVITATION:
		{
			ClanJoinInvitationAccepted_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_JOIN_INVITATION);
			notification.userId=userIdPtr;
			notification.userHandle=userHandle;
			notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
			notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
			notification.clanId=clanIdPtr;
			notification.clanHandle=clanHandle;

			/// This is sent to the new member, and also to all online members of the clan
			notification.userIdIsYou=true;
			notification.invitationSenderIsYou=false;
			notification.Serialize(&notificationBs);
			Send(&notificationBs, systemAddress);

			// Send to join request acceptor
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->userId==lsm->clanSourceMemberId)
				{
					notificationBs.Reset();
					notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
					notification.userIdIsYou=false;
					notification.invitationSenderIsYou=true;
					notification.Serialize(&notificationBs);
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					break;
				}
			}

			// Send to everyone else
			notificationBs.Reset();
			notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
			notification.invitationSenderIsYou=false;
			notification.Serialize(&notificationBs);
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->userId!=lsm->clanSourceMemberId &&
					lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
			}
		};
		break;
	case LOBBY_MSGID_SEND_CLAN_JOIN_INVITATION:
		{
			ClanJoinInvitation_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_SEND_CLAN_JOIN_INVITATION);
			notification.clanLeaderOrSubleaderId=&res->userId.databaseRowId;
			notification.clanLeaderOrSubleaderHandle=GetHandleByID(res->userId.databaseRowId);
			notification.clanId=clanIdPtr;
			notification.clanHandle=clanHandle;
			notification.Serialize(&notificationBs);

			// Send to target
			Send(&notificationBs, GetSystemAddressById(res->userId.databaseRowId));
		};
		break;
	case LOBBY_MSGID_SEND_CLAN_MEMBER_JOIN_REQUEST:
		{
			// TODO requiresInvitationsToJoin==false not yet supported

			ClanMemberJoinRequest_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_SEND_CLAN_MEMBER_JOIN_REQUEST);
			notification.userId=userIdPtr;
			notification.userHandle=userHandle;
			notification.clanId=clanIdPtr;
			notification.clanHandle=clanHandle;
			notification.Serialize(&notificationBs);

			// Send to leaders and subleaders
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
			}
		};
		break;
	case LOBBY_MSGID_SET_CLAN_MEMBER_RANK:
		{
			ClanMemberRank_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_SET_CLAN_MEMBER_RANK);
			notification.userId=userIdPtr;
			notification.userHandle=userHandle;
			notification.clanLeaderOrSubleaderId=&res->userId.databaseRowId;
			notification.clanLeaderOrSubleaderHandle=GetHandleByID(res->userId.databaseRowId);
			notification.rank=res->mEStatus1;
			// Send to leaders and subleaders
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
			}
		}
		break;
	case LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER:
		{
			// Send to all leaders, if user is online, also send to all members
			ClanMemberKicked_PC notification;
			notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_CLAN_MEMBER_KICKED);
			notification.userId=userIdPtr;
			notification.userHandle=userHandle;
			notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
			notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
			notification.wasBanned=true;
			notification.wasKicked=true;
			for (i=0; i < lsc->clanMembers.Size(); i++)
			{
				if (lsm || lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
					Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
			}
			break;
		}
	}



	Send(&reply,systemAddress);
}
void LobbyServer::UpdateClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::UpdateClan_Data *res, SystemAddress systemAddress)
{
}
void LobbyServer::CreateClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::CreateClan_Data *res)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CREATE_CLAN);
	LobbyServerUser *lsu = GetUserByID(res->leaderData.userId.databaseRowId);
	if (lsu==0)
		return;

	SystemAddress systemAddress = lsu->systemAddress;
	
	// If successful, unset processing, create clan in memory
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}
	if (res->initialClanData.failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(&res->initialClanData.failureMessage,512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}
	if (res->leaderData.failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(&res->leaderData.failureMessage,512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}

	LobbyServerClan *lsc;
	lsc = new LobbyServerClan;
	lsc->clanId=res->initialClanData.clanId.databaseRowId;
	lsc->handle=res->initialClanData.handle;
	AddClan(lsc);

	LobbyServerClanMember *lscm = new LobbyServerClanMember;
	lscm->userId=res->leaderData.userId.databaseRowId;
	lscm->clanStatus=res->leaderData.mEStatus1;
	lscm->isDirty=false;
	lscm->memberData = new LobbyDBSpec::UpdateClanMember_Data;
	*(lscm->memberData)=res->leaderData;
	AddClanMember(lsc, lscm);

	lsc->InsertMember(lscm);
	
	// Decrement pending clan integer
	lsu->pendingClanCount--;

	// 4. Reply to user. Wait until DB processing is done as this could fail due to duplicate names.
	reply.Write((unsigned char)LC_RC_SUCCESS);
	res->Serialize(&reply);
	Send(&reply,systemAddress);

}
void LobbyServer::ChangeClanHandle_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::ChangeClanHandle_Data *res)
{
	SystemAddress systemAddress = GetLeaderSystemAddress(res->clanId.databaseRowId);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CHANGE_CLAN_HANDLE);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (res->failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_NAME_IN_USE);
		stringCompressor->EncodeString(&res->failureMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);
	}
	Send(&reply,systemAddress);
}
void LobbyServer::DeleteClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DeleteClan_Data *res, SystemAddress systemAddress)
{
}
void LobbyServer::GetClans_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetClans_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLANS);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
	}
	else if (res->failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);
		stringCompressor->EncodeString(&res->failureMessage,512,&reply,0);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		res->Serialize(&reply);

		LobbyServerClan *lsc;
		LobbyServerClanMember *lscm;
		unsigned i,j;
		for (i=0; i < res->clans.Size(); i++)
		{
			lsc=GetClanById(res->clans[i]->clanId.databaseRowId);
			if (lsc==0)
			{
				lsc = new LobbyServerClan;
				lsc->clanId=res->clans[i]->clanId.databaseRowId;
				AddClan(lsc);
			}
			else
			{
				lsc->SetMembersDirty(true);
			}


			lsc->handle=res->clans[i]->clanId.handle;

			for (j=0; j < res->clans[i]->members.Size(); j++)
			{
				// Only load the clan member data if the user is also online, to save memory
				// Operations can still be performed on offline users
				if (GetUserByID(res->clans[i]->members[j]->userId.databaseRowId))
				{
					lscm = GetClanMember(lsc, res->clans[i]->members[j]->userId.databaseRowId);
					if (lscm==0)
					{
						lscm = CreateClanMember(lsc, res->clans[i]->members[j]->userId.databaseRowId, res->clans[i]->members[j]);
					}
					else
					{
						// Insert clan to member if not already there
						lscm->clan=lsc;
					}

					lscm->clanStatus=res->clans[i]->members[j]->mEStatus1;
					lscm->isDirty=false;
					// Insert member to clan if not already there
					lsc->InsertMember(lscm);
				}
			}
			j=0;

			// Validate that all clan members were updated
			while (j < lsc->clanMembers.Size())
			{
				if (lsc->clanMembers[j]->isDirty)
				{
					// This member was in memory, but not in the database, so get rid of them.
					DeleteClanMember(lsc, lsc->clanMembers[j]->userId);
					lsc->clanMembers.RemoveAtIndex(j);
				}
				else
				{
					j++;
				}
			}
		}
	}
	Send(&reply,systemAddress);
}
void LobbyServer::RemoveFromClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::RemoveFromClan_Data *res, SystemAddress systemAddress, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved, unsigned int lobbyMsgId)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)lobbyMsgId);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		reply.WriteCompressed(res->clanId.handle); // clanHandle
		reply.Write(res->clanId);
		reply.WriteCompressed(res->userId.handle); // userHandle
		reply.Write(res->userId);
		Send(&reply,systemAddress);
		return;
	}
	if (res->failureMessage.IsEmpty()==false)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);
		stringCompressor->EncodeString(&res->failureMessage,512,&reply,0);
		reply.WriteCompressed(res->clanId.handle); // clanHandle
		reply.Write(res->clanId);
		reply.WriteCompressed(res->userId.handle); // userHandle
		reply.Write(res->userId);
		Send(&reply,systemAddress);
		return;
	}

	RakNet::BitStream notificationBs;
	LobbyDBSpec::DatabaseKey clanId;
	clanId=res->clanId.databaseRowId;
	ClanId *clanIdPtr;
	clanIdPtr=&clanId;
	RakNet::RakString clanHandle;
	clanHandle=res->clanId.handle;

	reply.Write((unsigned char)LC_RC_SUCCESS);
	stringCompressor->EncodeString(&clanHandle,512,&reply,0);
	reply.Write(clanId);

	LobbyServerClan *lsc = GetClanById(clanId);
	unsigned i;
	if (lsc)
	{
		LobbyServerClanMember *lsm = GetClanMember(lsc, res->userId.databaseRowId);
		if (lsm)
		{
			notificationBs.Reset();
			notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
			LobbyClientUserId *userIdPtr = &lsm->userId;
			RakNet::RakString userHandle;
			LobbyServerUser *lsu = GetUserByID(lsm->userId);
			if (lsu)
				userHandle = lsu->data->id.handle;

			switch (lobbyMsgId)
			{
			case LOBBY_MSGID_LEAVE_CLAN:
				{
					LeaveClan_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_LEAVE_CLAN);
					notification.userId=userIdPtr;
					notification.userHandle=userHandle;
					notification.clanId=clanIdPtr;
					notification.clanHandle=clanHandle;
					notification.Serialize(&notificationBs);

					// Send to all clan members
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						if (lsc->clanMembers[i]!=lsm && lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
							Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					}
				}
				break;
			case LOBBY_MSGID_WITHDRAW_CLAN_JOIN_INVITATION:
				{
					ClanJoinInvitationWithdrawn_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_WITHDRAW_CLAN_JOIN_INVITATION);
					
					notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
					notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
					notification.clanId=clanIdPtr;
					notification.clanHandle=clanHandle;
					notification.Serialize(&notificationBs);

					// Send to target
					Send(&notificationBs, GetSystemAddressById(res->userId.databaseRowId));

					stringCompressor->EncodeString(&lsm->clanSourceMemberHandle,512,&reply,0);
					reply.Write(lsm->clanSourceMemberId);
				};
				break;

			case LOBBY_MSGID_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST:
				{
					ClanMemberJoinRequestWithdrawn_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST);
					notification.userId=userIdPtr;
					notification.userHandle=userHandle;
					notification.clanId=clanIdPtr;
					notification.clanHandle=clanHandle;
					notification.Serialize(&notificationBs);

					// Send to leaders and subleaders
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						if (lsc->clanMembers[i]!=lsm && lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
							Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					}
				};
				break;
			case LOBBY_MSGID_REJECT_CLAN_MEMBER_JOIN_REQUEST:
				{
					ClanMemberJoinRequestRejected_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_REJECT_CLAN_MEMBER_JOIN_REQUEST);
					notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
					notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
					notification.clanId=clanIdPtr;
					notification.clanHandle=clanHandle;
					notification.Serialize(&notificationBs);

					// Send to all leaders, subleaders, and target
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						if (lsc->clanMembers[i]!=lsm && lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
							Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					}
					Send(&notificationBs, GetSystemAddressById(res->userId.databaseRowId));
				};
				break;

				case LOBBY_MSGID_REJECT_CLAN_JOIN_INVITATION:
				{
					ClanJoinInvitationRejected_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_REJECT_CLAN_JOIN_INVITATION);
					notification.userId=userIdPtr;
					notification.userHandle=userHandle;
					notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
					notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
					notification.clanId=clanIdPtr;
					notification.clanHandle=clanHandle;
					notification.Serialize(&notificationBs);

					// Send to leaders and subleaders
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						if (lsc->clanMembers[i]!=lsm && lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
							Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					}
				};
				break;


				case LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER:
				{
					ClanMemberKicked_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_CLAN_MEMBER_KICKED);
					notification.userId=userIdPtr;
					notification.userHandle=userHandle;
					notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
					notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;
					notification.wasBanned=false;
					notification.wasKicked=true;
					
					// Send to all clan members, which should include the now-kicked member
					for (i=0; i < lsc->clanMembers.Size(); i++)
						Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					
					break;
				}

				case LOBBY_MSGID_CLAN_UNBLACKLIST_USER:
				{
					ClanMemberUnblacklisted_PC notification;
					notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_CLAN_MEMBER_UNBLACKLISTED);
					notification.userId=userIdPtr;
					notification.userHandle=userHandle;
					notification.clanLeaderOrSubleaderId=&lsm->clanSourceMemberId;
					notification.clanLeaderOrSubleaderHandle=lsm->clanSourceMemberHandle;

					// Send to leaders and subleaders
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						if (lsc->clanMembers[i]!=lsm && lsc->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
							Send(&notificationBs, GetSystemAddressById(lsc->clanMembers[i]->userId));
					}
					break;
				}
			}


			if (dissolveIfClanLeader)
			{
				if (lsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
				{
					unsigned i,j;
					LobbyServerUser *lsu;
					ClanDissolved_PC clanDissolved;
					LobbyClientUserId *lcui;
					clanDissolved.clanHandle=lsc->handle;
					clanDissolved.clanId=lsc->clanId;

					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						lcui = new LobbyClientUserId;
						*lcui=lsc->clanMembers[i]->userId;
						clanDissolved.memberIds.Insert(lcui);
					}

					if (autoSendEmailIfClanDissolved)
					{
						// Email to send out about dissolved clan
						LobbyDBSpec::SendEmail_Data *emailNotification;
						emailNotification = AllocSendEmail_Data();
						emailNotification->body=RakNet::RakString("Your clan %s has been dissolved by the leader %s. This email has been automatically generated.", lsc->handle.C_String(), GetHandleByID(lsm->userId).C_String());
						emailNotification->subject=RakNet::RakString("Clan %s dissolved.", lsc->handle.C_String());
						emailNotification->id.hasDatabaseRowId=true;
						emailNotification->id.databaseRowId=res->userId.databaseRowId;
						emailNotification->initialRecipientStatus=0;
						emailNotification->initialSenderStatus=0;
						emailNotification->status=0;
						LobbyDBSpec::RowIdOrHandle emailTo;
						emailTo.hasDatabaseRowId=true;

						for (i=0; i < lsc->clanMembers.Size(); i++)
						{
							if (*lcui!=res->userId.databaseRowId)
							{
								emailTo.databaseRowId=*lcui;
								emailNotification->to.Insert(emailTo);
							}
						}

						if (emailNotification->to.Size()>0)
						{
							SendEmail(res->userId.databaseRowId,0,emailNotification,UNASSIGNED_SYSTEM_ADDRESS);
						}
						else
						{
							delete emailNotification;
						}
					}

					// Remove each member, and send them an email if required
					for (i=0; i < lsc->clanMembers.Size(); i++)
					{
						// Dereference from users
						lsu = GetUserByID(lsc->clanMembers[i]->userId);

						if (lsc->clanMembers[i]!=lsm)
						{
							if (lsu)
							{
								notificationBs.Reset();
								notificationBs.Write((unsigned char)ID_LOBBY_GENERAL);
								notificationBs.Write((unsigned char)LOBBY_MSGID_NOTIFY_CLAN_DISSOLVED);
								clanDissolved.Serialize(&notificationBs);

								Send(&notificationBs,lsu->systemAddress);
							}
						}

						if (lsu)
						{
							for (j=0; j < lsu->clanMembers.Size(); j++)
							{
								lsu->clanMembers.RemoveAtIndexFast(j);
								break;
							}
						}

						// delete member
						delete lsc->clanMembers[i];
					}
					lsc->clanMembers.Clear();

					// Delete clan
					clansByHandle.Remove(lsc->handle);
					clansById.Remove(lsc->clanId);
					delete lsc;
					lsc=0;
					lsm=0;

					unsigned cdi;
					for (cdi=0; cdi < clanDissolved.memberIds.Size(); cdi++)
						delete clanDissolved.memberIds[cdi];
				}
			}

			if (lsc && lsm)
				DeleteClanMember(lsc, lsm->userId);
		}
	}

	Send(&reply,systemAddress);
}
void LobbyServer::AddToClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddToClanBoard_Data *res)
{
	LobbyServerUser *lsu = GetUserByID(res->userId.databaseRowId);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ADD_POST_TO_CLAN_BOARD);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,lsu->systemAddress);
		return;
	}

	reply.Write((unsigned char)LC_RC_SUCCESS);
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::RemoveFromClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::RemoveFromClanBoard_Data *res, SystemAddress systemAddress)
{
	LobbyServerUser *lsu = GetUserByAddress(systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REMOVE_POST_FROM_CLAN_BOARD);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,lsu->systemAddress);
		return;
	}

	reply.Write((unsigned char)LC_RC_SUCCESS);
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::GetClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetClanBoard_Data *res, SystemAddress systemAddress, LobbyNetworkOperations op)
{
	LobbyServerUser *lsu = GetUserByAddress(systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)op);

	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,lsu->systemAddress);
		return;
	}

	reply.Write((unsigned char)LC_RC_SUCCESS);
	Send(&reply,lsu->systemAddress);
}
void LobbyServer::ValidateUserKey_DBResult(bool dbSuccess, const char *queryErrorMessage, TitleValidationDBSpec::ValidateUserKey_Data *res, SystemAddress systemAddress)
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_VALIDATE_USER_KEY);
	if (dbSuccess==false)
	{
		reply.Write((unsigned char)LC_RC_DATABASE_FAILURE);
		stringCompressor->EncodeString(queryErrorMessage,512,&reply,0);
		Send(&reply,systemAddress);
		return;
	}

	/// [out] Validation result. 1 = success. -1 = no keys matched. -2 = key marked as invalid (invalidKeyReason will be filled in), -3 = undefined
	if (res->successful==1)
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		LobbyServerUser *lsu = GetUserByAddress(systemAddress);
		lsu->titleId=res->titleId;
		Send(&reply,systemAddress);
	}
	else if (res->successful==-1)
	{
		reply.Write((unsigned char)LC_RC_USER_KEY_UNKNOWN);
		Send(&reply,systemAddress);
	}
	else if (res->successful==-2)
	{
		reply.Write((unsigned char)LC_RC_USER_KEY_INVALID);
		reply.Write(res->invalidKeyReason);
		Send(&reply,systemAddress);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,systemAddress);
	}
}
void LobbyServer::SetTitleLoginId_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SET_TITLE_LOGIN_ID);

	LobbyServerUser *user = GetUserByAddress(packet->systemAddress);
	if (user==0)
		return;

	char titleLoginPW[512];
	int titleLoginPWLength;
	char titleIdStr[512];
	bs.ReadAlignedBytesSafe((char*)titleLoginPW,titleLoginPWLength,512);
	stringCompressor->DecodeString(titleIdStr, sizeof(titleIdStr), &bs, 0);

	TitleValidationDBSpec::AddTitle_Data *title=GetTitleDataByPassword(titleLoginPW, titleLoginPWLength);
	if (title->requireUserKeyToLogon)
	{
		if (user->titleId==0)
			reply.Write((unsigned char)LC_RC_USER_KEY_NOT_SET);
		else if (user->titleId!=title->titleId)
			reply.Write((unsigned char)LC_RC_USER_KEY_WRONG_TITLE);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (title)
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		user->title=title;
	}
	else
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
	}
	Send(&reply,packet->systemAddress);
}
void LobbyServer::Logoff_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RemoveUser(packet->systemAddress);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_LOGOFF);
	Send(&reply,packet->systemAddress);
}
void LobbyServer::RegisterAccount_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyDBSpec::CreateUser_Data *input = AllocCreateUser_Data();
	char titleLoginPW[512];
	int titleLoginPWLength;
	char titleIdStr[512];
	bs.ReadAlignedBytesSafe(titleLoginPW,titleLoginPWLength,512);
	stringCompressor->DecodeString(titleIdStr, sizeof(titleIdStr), &bs, 0);
	input->Deserialize(&bs);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REGISTER_ACCOUNT);

	TitleValidationDBSpec::AddTitle_Data *title=GetTitleDataByPassword(titleLoginPW, titleLoginPWLength);
	if (title)
	{
		if (title->allowClientAccountCreation)
		{
			CreateUser_DBQuery(packet->systemAddress, input);
		}
		else
		{
			reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
			Send(&reply,packet->systemAddress);
			input->Deref();
		}
	}
	else
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
		Send(&reply,packet->systemAddress);
		input->Deref();
	}
}
void LobbyServer::UpdateAccount_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_UPDATE_ACCOUNT);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		if (lsu->title==0)
		{
			reply.Write((unsigned char)LC_RC_UNKNOWN_PERMISSIONS);
		}
		else
		{
			LobbyDBSpec::UpdateUser_Data *input = AllocUpdateUser_Data();
			input->DeserializeBooleans(&bs);
			if (lsu->title->defaultAllowUpdateAccountBalance==false && input->updateAccountBalance==true)
			{
				reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
				stringCompressor->EncodeString("Title default permission AllowUpdateAccountBalance==false",512,&reply,0);
			}
			else if (lsu->title->defaultAllowUpdateAdminLevel==false && input->updateAdminLevel==true)
			{
				reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
				stringCompressor->EncodeString("Title default permission AllowUpdateAdminLevel==false",512,&reply,0);
			}
			else if (lsu->title->defaultAllowUpdateAccountNumber==false && input->updateAccountNumber==true)
			{
				reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
				stringCompressor->EncodeString("Title default permission AllowUpdateAccountNumber==false",512,&reply,0);
			}
			else if (lsu->title->defaultAllowUpdateCCInfo==false && input->updateCCInfo==true)
			{
				reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
				stringCompressor->EncodeString("Title default permission AllowUpdateCCInfo==false",512,&reply,0);
			}
			else
			{
				// Read twice, once into our struct in memory, again into the struct on the stack to pass to the database
				int readOffset = bs.GetReadOffset();
				input->Deserialize(&bs, &input->input, false);
				bs.SetReadOffset(readOffset);
				input->Deserialize(&bs, &lsu->data->output, false);
				reply.Write((unsigned char)LC_RC_SUCCESS);

				// Don't wait for DB reply back, just assume it works
				input->id.hasDatabaseRowId=true;
				input->id.databaseRowId=lsu->data->id.databaseRowId;
				UpdateUser_DBQuery(input);
			}
		}
	}

	Send(&reply,packet->systemAddress);
}
void LobbyServer::ChangeHandle_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char newHandle[512];
	stringCompressor->DecodeString(newHandle, 512, &bs, 0);


	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_CHANGE_HANDLE);
		if (lsu->title==0)
		{
			reply.Write((unsigned char)LC_RC_UNKNOWN_PERMISSIONS);
			Send(&reply,packet->systemAddress);
		}
		else
		{
			if (lsu->title->defaultAllowUpdateHandle)
			{
				UpdateHandle_DBQuery(newHandle, lsu->data->id.databaseRowId);
			}
			else
			{
				reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
				stringCompressor->EncodeString("Title default permission AllowUpdateHandle==false",512,&reply,0);
				Send(&reply,packet->systemAddress);
			}
		}
	}
}
void LobbyServer::RetrievePasswordRecoveryQuestion_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char userIdStr[512];
	stringCompressor->DecodeString(userIdStr, sizeof(userIdStr), &bs, 0);

	RetrievePasswordRecoveryQuestion_DBQuery(packet->systemAddress, userIdStr);
}
void LobbyServer::RetrievePassword_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char userIdStr[512];
	char passwordRecoveryAnswer[512];
	stringCompressor->DecodeString(userIdStr, sizeof(userIdStr), &bs, 0);
	stringCompressor->DecodeString(passwordRecoveryAnswer, sizeof(passwordRecoveryAnswer), &bs, 0);


	RetrievePassword_DBQuery(packet->systemAddress, userIdStr, passwordRecoveryAnswer);
}
void LobbyServer::DownloadFriends_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	bool ascendingSort;
	bs.Read(ascendingSort);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		GetFriends_DBQuery(packet->systemAddress, lsu->GetID(), ascendingSort);
	}
}
void LobbyServer::SendAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	FriendInvitation_PC noticeStruct;
	unsigned char language;
//	LobbyClientUserId friendId;
	
	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyServerUser *potentialFriend;
	LobbyClientUserId userId;
	char userHandle[512];
	potentialFriend=DeserializeHandleOrId(userHandle,userId,&bs);

	bs.Read(language);
	stringCompressor->DecodeString(noticeStruct.messageBody, sizeof(noticeStruct.messageBody), &bs, language);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_ADD_FRIEND_INVITE);

	if (potentialFriend==0)
	{
		reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);
	}
	else if (lsu==potentialFriend)
	{
		// Cannot be a friend to yourself!
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
	}
	else
	{
		if (lsu->IsIgnoredUser(potentialFriend->GetID()))
		{
			reply.Write((unsigned char)LC_RC_IGNORED);
		}
		else
		{
			if (lsu->IsFriend(potentialFriend->GetID())==false)
			{
				RakNet::BitStream invitationBS;
				invitationBS.Write((unsigned char)ID_LOBBY_GENERAL);
				invitationBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ADD_FRIEND_RESPONSE);
				noticeStruct._invitee=potentialFriend->GetID();
				noticeStruct._invitor=lsu->GetID();
				noticeStruct.messageBodyEncodingLanguage=language;
				strncpy(noticeStruct.invitorHandle, lsu->data->id.handle.C_String(), sizeof(noticeStruct.invitorHandle)-1);
				noticeStruct.invitorHandle[sizeof(noticeStruct.invitorHandle)-1]=0;
				strncpy(noticeStruct.inviteeHandle, potentialFriend->data->id.handle.C_String(), sizeof(noticeStruct.inviteeHandle)-1);
				noticeStruct.inviteeHandle[sizeof(noticeStruct.inviteeHandle)-1]=0;
				noticeStruct.request=true;
				noticeStruct.Serialize(&invitationBS);
				Send(&invitationBS, potentialFriend->systemAddress);

				// Potential friend acceptance list
				PotentialFriend p;
				p.invitor=lsu->data->id.databaseRowId;
				p.invitee=potentialFriend->data->id.databaseRowId;
				AddToPotentialFriends(p);
			}

			reply.Write((unsigned char)LC_RC_SUCCESS);
		}
	}

	stringCompressor->EncodeString(&potentialFriend->data->id.handle,512,&reply,0);
	reply.Write(potentialFriend->GetID());

	Send(&reply,packet->systemAddress);

}
void LobbyServer::AcceptAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyClientUserId friendId;
	unsigned char messageBodyEncodingLanguage;
	char messageBody[4096];
	bs.Read(messageBodyEncodingLanguage);
	char friendHandle[512];
	stringCompressor->DecodeString(friendHandle, sizeof(friendHandle), &bs, messageBodyEncodingLanguage);
	bs.Read(friendId);
	stringCompressor->DecodeString(messageBody, sizeof(messageBody), &bs, messageBodyEncodingLanguage);
	if (messageBody[0]==0)
	{
		RakAssert(DEFAULT_FRIEND_ACCEPT_BODY[0]);
		strncpy(messageBody, DEFAULT_FRIEND_ACCEPT_BODY, sizeof(messageBody)-1 );
		messageBody[sizeof(messageBody)-1]=0;
	}

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_ACCEPT_ADD_FRIEND_INVITE);

		LobbyServerUser *potentialFriend = GetUserByID(friendId);
		if (potentialFriend==0)
		{
			potentialFriend = GetUserByHandle(friendHandle);
			if (potentialFriend==0)
			{
				// Not online
				reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);

				stringCompressor->EncodeString("",512,&reply,0);
				reply.Write(friendId);
				Send(&reply,packet->systemAddress);
				return;
			}
			friendId=potentialFriend->GetID();
		}
		
		if (lsu->IsFriend(friendId))
		{
			// Already a friend
			reply.Write((unsigned char)LC_RC_ALREADY_CURRENT_STATE);

			stringCompressor->EncodeString(&potentialFriend->data->id.handle,512,&reply,0);
			reply.Write(potentialFriend->GetID());
			Send(&reply,packet->systemAddress);
		}
		else
		{
			PotentialFriend p;
			p.invitor=friendId;
			p.invitee=lsu->data->id.databaseRowId;
			if (IsInPotentialFriends(p))
			{
				// No longer a potential friend if you are actually a friend. Also should be removed if the db query failed anyway.
				RemoveFromPotentialFriends(p);

				// Start the database query, send success IM when done.
				AddFriend_DBQuery(lsu->GetID(), potentialFriend->GetID(), messageBody, messageBodyEncodingLanguage);
			}
			else
			{
				// No pending friend invitations to accept
				reply.Write((unsigned char)LC_RC_NOT_INVITED);
				stringCompressor->EncodeString("",512,&reply,0);
				stringCompressor->EncodeString(&potentialFriend->data->id.handle,512,&reply,0);
				reply.Write(potentialFriend->GetID());
				Send(&reply,packet->systemAddress);
			}
		}
	}

}
void LobbyServer::DeclineAddFriendInvite_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	FriendInvitation_PC noticeStruct;
	LobbyClientUserId friendId;
	bs.Read(noticeStruct.messageBodyEncodingLanguage);
	char friendHandle[512];
	stringCompressor->DecodeString(friendHandle, sizeof(friendHandle), &bs, noticeStruct.messageBodyEncodingLanguage);
	bs.Read(friendId);
	stringCompressor->DecodeString(noticeStruct.messageBody, sizeof(noticeStruct.messageBody), &bs, noticeStruct.messageBodyEncodingLanguage);
	if (noticeStruct.messageBody[0]==0)
	{
		RakAssert(DEFAULT_FRIEND_DECLINE_BODY[0]);
		strncpy(noticeStruct.messageBody, DEFAULT_FRIEND_DECLINE_BODY, sizeof(noticeStruct.messageBody)-1 );
		noticeStruct.messageBody[sizeof(noticeStruct.messageBody)-1]=0;
	}

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_DECLINE_ADD_FRIEND_INVITE);

		LobbyServerUser *potentialFriend = GetUserByID(friendId);
		if (potentialFriend==0)
		{
			potentialFriend = GetUserByHandle(friendHandle);
			if (potentialFriend==0)
			{
				// Not online
				reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);
				stringCompressor->EncodeString("",512,&reply,0);
				reply.Write(friendId);
				Send(&reply,packet->systemAddress);
				return;
			}
			friendId=potentialFriend->GetID();
		}

		if (lsu->IsFriend(friendId))
		{
			// Currently a friend
			reply.Write((unsigned char)LC_RC_INVALID_INPUT);
			stringCompressor->EncodeString(&potentialFriend->data->id.handle,512,&reply,0);
			reply.Write(friendId);
		}
		else
		{
			PotentialFriend p;
			p.invitor=friendId;
			p.invitee=lsu->data->id.databaseRowId;
			if (IsInPotentialFriends(p))
			{
				// No longer a potential friend if you are actually a friend. Also should be removed if the db query failed anyway.
				RemoveFromPotentialFriends(p);

//				SendIM(lsu, potentialFriend, messageBody, language);


				// Invite rejected
				RakNet::BitStream invitationBS;
				invitationBS.Write((unsigned char)ID_LOBBY_GENERAL);
				invitationBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ADD_FRIEND_RESPONSE);
				noticeStruct._invitee=lsu->GetID(); // Replying switches lsu and potentialFriend
				noticeStruct._invitor=potentialFriend->GetID();// Replying switches lsu and potentialFriend
				strncpy(noticeStruct.invitorHandle, potentialFriend->data->id.handle.C_String(), sizeof(noticeStruct.invitorHandle)-1);
				noticeStruct.invitorHandle[sizeof(noticeStruct.invitorHandle)-1]=0;
				strncpy(noticeStruct.inviteeHandle, lsu->data->id.handle.C_String(), sizeof(noticeStruct.inviteeHandle)-1);
				noticeStruct.inviteeHandle[sizeof(noticeStruct.inviteeHandle)-1]=0;
				noticeStruct.request=false;
				noticeStruct.accepted=false;
				noticeStruct.Serialize(&invitationBS);
				Send(&invitationBS, potentialFriend->systemAddress);


				reply.Write((unsigned char)LC_RC_SUCCESS);
			}
			else
			{
				// No pending friend invitations to accept
				reply.Write((unsigned char)LC_RC_INVALID_INPUT);
			}
			stringCompressor->EncodeString(&potentialFriend->data->id.handle,512,&reply,0);
			reply.Write(friendId);
		}

		Send(&reply,packet->systemAddress);
	}
}
void LobbyServer::DownloadIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	bool ascendingSort;
	bs.Read(ascendingSort);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	// Do DB query to download the ignore list
	GetIgnoreList_DBQuery(packet->systemAddress, lsu->GetID(), ascendingSort);	

}
void LobbyServer::UpdateIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char actionsStr[4096];

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyServerUser *ignoredUser;
	LobbyClientUserId userId;
	char userHandle[512];
	
	bool hasUserId;
	stringCompressor->DecodeString(userHandle,512,&bs,0);
	bs.Read(hasUserId);
	if (hasUserId)
	{
		bs.Read(userId);
	}
	else
	{
		if (userHandle[0]==0)
		{
			// Nobody specified
			RakNet::BitStream reply;
			reply.Write((unsigned char)ID_LOBBY_GENERAL);
			reply.Write((unsigned char)LOBBY_MSGID_UPDATE_IGNORE_LIST);
			reply.Write((unsigned char)LC_RC_INVALID_INPUT);
			Send(&reply,packet->systemAddress);
			return;
		}

		ignoredUser = GetUserByHandle(userHandle);
		if (ignoredUser)
		{
			hasUserId=true;
			userId=ignoredUser->GetID();
		}
	}

	stringCompressor->DecodeString(actionsStr, 4096, &bs, 0);

	if (hasUserId && lsu->IsIgnoredUser(userId))
	{
		// This guy is already ignored
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_UPDATE_IGNORE_LIST);
		reply.Write((unsigned char)LC_RC_ALREADY_CURRENT_STATE);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Do DB query to update the ignore list for a particular user
	AddToIgnoreList_DBQuery(hasUserId, userId, userHandle, lsu->GetID(), actionsStr);

}
void LobbyServer::RemoveFromIgnoreList_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyClientUserId userId;
	bs.Read(userId);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	if (lsu->IsIgnoredUser(userId)==false)
	{
		// This guy is not ignored to begin with
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_REMOVE_FROM_IGNORE_LIST);
		reply.Write((unsigned char)LC_RC_ALREADY_CURRENT_STATE);
		Send(&reply,packet->systemAddress);
		return;
	}

	// if not ignored, just return reply.Write((unsigned char)LC_RC_INVALID_INPUT);

	// Do DB query to remove a user from the ignore list
	RemoveFromIgnoreList_DBQuery(userId, lsu->GetID());
}
void LobbyServer::DownloadEmails_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	bool ascendingSort, inbox;
	unsigned char language;
	bs.Read(language);
	bs.Read(ascendingSort);
	bs.Read(inbox);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	GetEmails_DBQuery( packet->systemAddress, lsu->GetID(), inbox, ascendingSort);
}
void LobbyServer::SendEmail_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	unsigned char language;
	bs.Read(language);

	LobbyDBSpec::SendEmail_Data *input;
	input = AllocSendEmail_Data();
	input->Deserialize(&bs);

	SendEmail(lsu->GetID(), language, input, packet->systemAddress);
}
void LobbyServer::DeleteEmail_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	EmailId emailId;

	bs.Read(emailId);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	DeleteEmail_DBQuery(lsu->GetID(), emailId);
}
void LobbyServer::UpdateEmailStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	int newStatus;
	EmailId emailId;
	bool wasOpened;

	bs.Read(emailId);
	bs.Read(newStatus);
	bs.Read(wasOpened);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	UpdateEmailStatus_DBQuery(lsu->GetID(), emailId, newStatus, wasOpened);
}
void LobbyServer::SendIM_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char *chatBinaryData=0;
	int chatBinaryLength;
	unsigned char languageId;
	char chatMessage[4096];

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;


	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_IM);
	
	LobbyServerUser *target;
	LobbyClientUserId userId;
	char userHandle[512];
	target=DeserializeHandleOrId(userHandle,userId,&bs);
		
	if (target==0)
	{
		reply.Write((unsigned char)LC_RC_UNKNOWN_USER_ID);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (target==lsu)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Cannot IM yourself.", 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	
	bs.Read(languageId);
	stringCompressor->DecodeString(chatMessage, 4096, &bs, languageId);
	bs.ReadAlignedBytesSafeAlloc(&chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);

	if (chatMessage[0]==0 && chatBinaryLength==0)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Both chat message and binary data empty.", 512, &reply, 0);
	}
	else if (target->IsIgnoredUser(lsu->GetID()))
	{
		reply.Write((unsigned char)LC_RC_IGNORED);
	}
	else
	{
		SendIM(lsu, target, chatMessage, languageId);

		reply.Write((unsigned char)LC_RC_SUCCESS);
	}
	
	stringCompressor->EncodeString(&target->data->id.handle, 512, &reply, 0);
	reply.Write(target->GetID());
	Send(&reply,packet->systemAddress);
	delete [] chatBinaryData;
}
void LobbyServer::CreateRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char roomName[512];
	char roomPassword[512];
	bool roomIsHidden;
	int publicSlots, privateSlots;
	bool allowSpectators;
	bool hasCustomProperties;
	DataStructures::Table customProperties;

	stringCompressor->DecodeString(roomName, 512, &bs, 0);
	stringCompressor->DecodeString(roomPassword, 512, &bs, 0);
	bs.Read(roomIsHidden);
	bs.ReadCompressed(publicSlots);
	bs.ReadCompressed(privateSlots);
	bs.Read(allowSpectators);
	bs.Read(hasCustomProperties);
	if (hasCustomProperties)
	{
		TableSerializer::DeserializeTable(&bs, &customProperties);
	}

	if (publicSlots > LOBBY_SERVER_MAX_PUBLIC_SLOTS)
		publicSlots=LOBBY_SERVER_MAX_PUBLIC_SLOTS;
	if (privateSlots > LOBBY_SERVER_MAX_PRIVATE_SLOTS)
		privateSlots=LOBBY_SERVER_MAX_PRIVATE_SLOTS;


	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CREATE_ROOM);

	// Check validity
	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;
	if (lsu->status!=LOBBY_USER_STATUS_IN_LOBBY_IDLE ||
		publicSlots+privateSlots<2 ||
		roomName[0]==0)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (customProperties.GetColumns().Size() > RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (lsu->title==0)
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (hasCustomProperties)
	{
		// Check that reserved column names were not used
		unsigned i,j;
		for (j=0; j < customProperties.GetColumns().Size(); j++)
		{
			for (i=0; i < DefaultTableColumns::TC_LOBBY_ROOM_PTR; i++)
			{
				if (stricmp(DefaultTableColumns::GetColumnName(i),customProperties.GetColumns()[j].columnName)==0)
				{
					reply.Write((unsigned char)LC_RC_INVALID_INPUT);
					Send(&reply,packet->systemAddress);
					return;
				}
			}
		}

		if (customProperties.GetRowCount()!=1)
		{
			reply.Write((unsigned char)LC_RC_INVALID_INPUT);
			Send(&reply,packet->systemAddress);
			return;
		}
	}

	// Check that room name is not already in use.
	LobbyRoom* lobbyRoom = GetLobbyRoomByName(roomName, lsu->title->titleId);
	if (lobbyRoom)
	{
		reply.Write((unsigned char)LC_RC_NAME_IN_USE);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyRoomsTableContainer* lobbyTitle = GetLobbyRoomsTableContainerById(lsu->title->titleId);
	if (lobbyTitle==0)
	{
		// Create this game
		lobbyTitle = new LobbyRoomsTableContainer;
		lobbyTitle->Initialize(lsu->title->titleId);
		lobbyTitles.Insert(lsu->title->titleId, lobbyTitle, true);
	}

	unsigned int rowId = lobbyTitle->GetFreeRowId();
	DataStructures::Table::Row *roomsTableRow = lobbyTitle->roomsTable->AddRow(rowId);

	// Add user-specified columns that do not already exist
	if (hasCustomProperties)
	{
		unsigned int j, columnIndex;
		for (j=0; j < customProperties.GetColumns().Size(); j++)
		{
			columnIndex = lobbyTitle->roomsTable->ColumnIndex(customProperties.ColumnName(j));
			if (columnIndex==-1)
			{
				if (lobbyTitle->roomsTable->GetColumnCount() < RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS+DefaultTableColumns::TC_TABLE_COLUMNS_COUNT)
					columnIndex = lobbyTitle->roomsTable->AddColumn(customProperties.ColumnName(j), customProperties.GetColumnType(j));
				else
				{
					// Too many user columns! Don't add this column after all
					continue;
				}
			}

			// Add user rows values
			DataStructures::Table::Row *userRow = customProperties.GetRowByIndex(0,0);
			*(roomsTableRow->cells[columnIndex])=*(userRow->cells[j]);
		}
	}

	// Add required rows
	lobbyRoom = new LobbyRoom;

	// Setup the room
	lobbyRoom->rowId=rowId;
	lobbyRoom->publicSlots=publicSlots;
	lobbyRoom->privateSlots=privateSlots;
	lobbyRoom->titleId=lsu->title->titleId;
	lobbyRoom->SetRoomName(roomName);
	lobbyRoom->SetTitleName(lsu->title->titleName.C_String());
	lobbyRoom->SetRoomPassword(roomPassword);
	lobbyRoom->allowSpectators=allowSpectators;
	lobbyRoom->roomIsHidden=roomIsHidden;
	lobbyRoom->rowInTable=roomsTableRow;
	lobbyRoom->AddRoomMemberToFirstAvailableSlot(lsu->GetID()); // Adds you to an open slot if possible, otherwise a reserved slot
	lobbyRoom->SetModerator(lsu->GetID());
	lobbyRoom->rowInTable = roomsTableRow;

	// Write the room updates to the row in the table for the queries
	lobbyRoom->WriteToRow(true);

	lsu->readyToPlay=false;
	lsu->currentRoom=lobbyRoom;
	lsu->status=LOBBY_USER_STATUS_IN_ROOM;

	reply.Write((unsigned char)LC_RC_SUCCESS);
	lobbyRoom->Serialize(&reply);
	DataStructures::List<int> skipColumnIndices;
	skipColumnIndices.Insert(DefaultTableColumns::TC_LOBBY_ROOM_PTR);
	TableSerializer::SerializeColumns(lobbyTitle->roomsTable,&reply,skipColumnIndices);
	TableSerializer::SerializeRow(roomsTableRow,lobbyRoom->rowId,lobbyTitle->roomsTable->GetColumns(),&reply,skipColumnIndices);
	SerializeClientSafeCreateUserData(lsu,&reply);
	Send(&reply,packet->systemAddress);
}
void LobbyServer::DownloadRooms_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_ROOMS);

	if (lsu->title==0)
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyRoomsTableContainer* lobbyTitle = GetLobbyRoomsTableContainerById(lsu->title->titleId);
	if (lobbyTitle==0 || lobbyTitle->roomsTable->GetRowCount()==0)
	{
		// No rooms available
		reply.Write((unsigned char)LC_RC_SUCCESS);
		reply.Write(false);
		Send(&reply,packet->systemAddress);
		return;
	}
	
	DataStructures::Table::FilterQuery *query;
	unsigned int numQueries;
	TableSerializer::DeserializeFilterQueryList(&bs, &query, &numQueries, RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS);

	// All columns except the one with the internal pointer
	unsigned columnIds[DefaultTableColumns::TC_TABLE_COLUMNS_COUNT+RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS];
	unsigned i,columnIdCount;
	for (i=0,columnIdCount=0; i < lobbyTitle->roomsTable->GetColumnCount(); i++)
	{
		if (i!=DefaultTableColumns::TC_LOBBY_ROOM_PTR)
			columnIds[columnIdCount++]=i;
	}

	DataStructures::Table resultTable;
	// Run this query filter on the user's current selected game.
	// No filters means return all rows.
	lobbyTitle->roomsTable->QueryTable(columnIds,columnIdCount,query,numQueries,0,0,&resultTable);

	// Return possibly rooms available
	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.Write(true);
	TableSerializer::SerializeTable(&resultTable,&reply);
	Send(&reply,packet->systemAddress);

	TableSerializer::DeallocateQueryList(query, numQueries);
}
void LobbyServer::LeaveRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_LEAVE_ROOM);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Leave room
	bool roomIsDead;
	RemoveRoomMember(lsu->currentRoom, lsu->GetID(), &roomIsDead, false, lsu->systemAddress);
	lsu->currentRoom=0;

	// Set user state to in lobby
	lsu->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

	// TODO - Update members in lobby

	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.Write(roomIsDead);
	Send(&reply,packet->systemAddress);

}
void LobbyServer::JoinRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	char roomPassword[512], roomName[512];
	LobbyClientRoomId roomRowId;
	bool joinAsSpectator;

	bs.Read(roomRowId);
	stringCompressor->DecodeString(roomName,512,&bs,0);
	bs.Read(joinAsSpectator);
	stringCompressor->DecodeString(roomPassword,512,&bs,0);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_JOIN_ROOM);
	if (lsu->currentRoom)
	{
		reply.Write((unsigned char)LC_RC_ALREADY_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyRoom* room = GetLobbyRoomByRowId(roomRowId, lsu->title->titleId);
	if (room==0)
	{
		room = GetLobbyRoomByName(roomName, lsu->title->titleId);
		if (room==0)
		{
			reply.Write((unsigned char)LC_RC_ROOM_DOES_NOT_EXIST);
			Send(&reply,packet->systemAddress);
			return;
		}
	}

	if (room->roomPassword && strcmp(roomPassword, room->roomPassword)!=0)
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Wrong password.", 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	bool gotIntoRoom;
	if (joinAsSpectator)
	{
		if (room->allowSpectators==false)
		{
			reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
			stringCompressor->EncodeString("This room does not allow spectators", 512, &reply, 0);
			Send(&reply,packet->systemAddress);
			return;
		}

		room->AddRoomMemberAsSpectator(lsu->GetID());
		gotIntoRoom=true;
	}
	else
	{
		if (room->HasPrivateSlotInvite(lsu->GetID()))
			gotIntoRoom=room->AddRoomMemberToFirstAvailableSlot(lsu->GetID());
		else
			gotIntoRoom=room->AddRoomMemberToPublicSlot(lsu->GetID());

	}

	if (gotIntoRoom)
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		FlagEnteredAndSerializeRoom(lsu, reply, room);		
	}
	else
	{
		if (room->GetAvailablePrivateSlots()>0)
		{
			reply.Write((unsigned char)LC_RC_ROOM_ONLY_HAS_RESERVED);
		}
		else
		{
			reply.Write((unsigned char)LC_RC_ROOM_FULL);
		}
	}

	Send(&reply,packet->systemAddress);

	if (gotIntoRoom)
	{
		// Serialize that the room has an additional member, to all members currently in the room, except yourself
		SendNewMemberToRoom(lsu, room, joinAsSpectator);


		// Update the table of rooms
		room->WriteToRow(false);
	}
}
void LobbyServer::SendNewMemberToRoom(LobbyServerUser *lsu, LobbyRoom* room, bool joinAsSpectator)
{
	// Serialize new room state to all members
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_MEMBER_JOIN);

	RoomMemberJoin_PC join;
	join.memberAddress=lsu->systemAddress;
	join._joinedMember=lsu->GetID();
	join.isSpectator=joinAsSpectator;
	join.isInOpenSlot=room->IsInPublicSlot(join._joinedMember);
	join.isInReservedSlot=room->IsInPrivateSlot(join._joinedMember);
	join.Serialize(&bs);

	// Serialize the new member's data to all other members
	SerializeClientSafeCreateUserData(lsu,&bs);

	SendToRoomMembers(room, &bs, true, lsu->GetID());
}
void LobbyServer::FlagEnteredAndSerializeRoom(LobbyServerUser *lsu, RakNet::BitStream &reply, LobbyRoom* room)
{
	lsu->readyToPlay=false;
	lsu->currentRoom=room;
	lsu->status=LOBBY_USER_STATUS_IN_ROOM;

	room->Serialize(&reply);

	// Serialize each individuals member's data, including your own
	unsigned i;
	LobbyServerUser* user;

	reply.WriteCompressed(room->publicSlotMembers.Size());
	for (i=0; i < room->publicSlotMembers.Size(); i++)
	{
		user=GetUserByID(room->publicSlotMembers[i]);
		RakAssert(user);
		SerializeClientSafeCreateUserData(user,&reply);
	}

	reply.WriteCompressed(room->privateSlotMembers.Size());
	for (i=0; i < room->privateSlotMembers.Size(); i++)
	{
		user=GetUserByID(room->privateSlotMembers[i]);
		RakAssert(user);
		SerializeClientSafeCreateUserData(user,&reply);
	}

	reply.WriteCompressed(room->spectators.Size());
	for (i=0; i < room->spectators.Size(); i++)
	{
		user=GetUserByID(room->spectators[i]);
		RakAssert(user);
		SerializeClientSafeCreateUserData(user,&reply);
	}
}
void LobbyServer::JoinRoomByFilter_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;
	bool joinAsSpectator;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_JOIN_ROOM_BY_FILTER);

	if (lsu->title==0)
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyRoomsTableContainer* lobbyTitle = GetLobbyRoomsTableContainerById(lsu->title->titleId);
	if (lobbyTitle==0 || lobbyTitle->roomsTable->GetRowCount()==0)
	{
		// No rooms available
		reply.Write((unsigned char)LC_RC_ROOM_DOES_NOT_EXIST);
		Send(&reply,packet->systemAddress);
		return;
	}

	DataStructures::Table::FilterQuery *query;
	unsigned int numQueries;
	TableSerializer::DeserializeFilterQueryList(&bs, &query, &numQueries, RAKNET_LOBBY_MAX_USER_DEFINED_COLUMNS);

	bs.Read(joinAsSpectator);

	DataStructures::Table resultTable;
	// Run this query filter on the user's current selected game.
	// No filters means return all rows.
	lobbyTitle->roomsTable->QueryTable(0,0,query,numQueries,0,0,&resultTable);

	// Find the first room with no password and at least one open slot
	unsigned i;
	for (i=0; i < resultTable.GetRowCount(); i++)
	{
		DataStructures::Table::Row *row = resultTable.GetRowByIndex(i,0);
		if (row->cells[DefaultTableColumns::TC_OPEN_PUBLIC_SLOTS]>0)
		{
			LobbyRoom* room = (LobbyRoom*) row->cells[DefaultTableColumns::TC_LOBBY_ROOM_PTR]->ptr;
			if (room->roomPassword==0 &&
				(joinAsSpectator==false || room->allowSpectators==true))
			{
				if (joinAsSpectator)
					room->AddRoomMemberAsSpectator(lsu->GetID());
				else
					room->AddRoomMemberToPublicSlot(lsu->GetID());

				// Found a room, return success
				reply.Write((unsigned char)LC_RC_SUCCESS);
				FlagEnteredAndSerializeRoom(lsu,reply,room);
				Send(&reply,packet->systemAddress);

				// Serialize that the room has an additional member, to all members currently in the room, except yourself
				SendNewMemberToRoom(lsu, room, joinAsSpectator);

				// Update the table of rooms, to reflect the new member
				room->WriteToRow(false);
				return;
			}

		}
	}

	reply.Write((unsigned char)LC_RC_ROOM_DOES_NOT_EXIST);
	Send(&reply,packet->systemAddress);
}
void LobbyServer::RoomChat_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	unsigned char languageId;
	char chatMessage[4096];
	char *chatBinaryData=0;
	int chatBinaryLength;

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	if (lsu->currentRoom==0)
		return; // Intentionally no reply

	bs.Read(languageId);
	stringCompressor->DecodeString(chatMessage, 4096, &bs, languageId);
	bs.ReadAlignedBytesSafeAlloc(&chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_CHAT);
	reply.Write(lsu->GetID());
	stringCompressor->EncodeString(lsu->data->id.handle.C_String(), 512, &reply, 0);
	reply.Write(languageId);
	stringCompressor->EncodeString(chatMessage, 4096, &reply, languageId);
	reply.WriteAlignedBytesSafe(chatBinaryData,chatBinaryLength, MAX_BINARY_DATA_LENGTH);

	SendToRoomNotIgnoredNotSelf(lsu, &reply);
	
	delete [] chatBinaryData;
}
void LobbyServer::SetReadyToPlayStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);
	bool isReady;

	bs.Read(isReady);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	if (lsu->currentRoom==0)
		return;

	lsu->readyToPlay=isReady;

	// Broadcast to all in room
	RakNet::BitStream broadcast;
	broadcast.Write((unsigned char)ID_LOBBY_GENERAL);
	broadcast.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_MEMBER_READY_STATE);
	RoomMemberReadyStateChange_PC output;
	output._member=lsu->GetID();
	output.isReady=isReady;
	output.Serialize(&broadcast);
	SendToRoomMembers(lsu->currentRoom, &broadcast, true, lsu->GetID());
}
void LobbyServer::InviteToRoom_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	unsigned char languageId;
	char chatMessage[4096];
	char *chatBinaryData=0;
	int chatBinaryLength;
	bool inviteToPrivateSlot;

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_INVITE_TO_ROOM);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerUser *invitee;
	LobbyClientUserId userId;
	char userHandle[512];
	invitee=DeserializeHandleOrId(userHandle,userId,&bs);

	bs.Read(inviteToPrivateSlot);

	if (invitee==0)
	{
		reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (invitee->currentRoom == lsu->currentRoom)
	{
		reply.Write((unsigned char)LC_RC_USER_ALREADY_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (inviteToPrivateSlot==false || lsu->currentRoom->AddPrivateSlotInvite(invitee->GetID()))
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);

		bs.Read(languageId);
		stringCompressor->DecodeString(chatMessage, 4096, &bs, languageId);
		bs.ReadAlignedBytesSafeAlloc(&chatBinaryData, chatBinaryLength, MAX_BINARY_DATA_LENGTH);

		RakNet::BitStream targetMessage;
		targetMessage.Write((unsigned char)ID_LOBBY_GENERAL);
		targetMessage.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_INVITE);
		targetMessage.Write(inviteToPrivateSlot);
		targetMessage.Write(lsu->GetID());
		targetMessage.WriteAlignedBytesSafe(chatBinaryData,chatBinaryLength, MAX_BINARY_DATA_LENGTH);
		stringCompressor->EncodeString(lsu->data->id.handle.C_String(), 512, &targetMessage, 0);
		targetMessage.Write(languageId);
		stringCompressor->EncodeString(chatMessage, 4096, &targetMessage, languageId);
		targetMessage.Write(lsu->currentRoom->rowId);
		stringCompressor->EncodeString(lsu->currentRoom->roomName, 512, &targetMessage, 0);
		Send(&targetMessage, invitee->systemAddress);
		delete [] chatBinaryData;
	}
	else
	{
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Room invite queue full, probably a bug. Please email support.", 512, &reply, 0);
	}


	Send(&reply,packet->systemAddress);
}
void LobbyServer::KickRoomMember_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyClientUserId userId;
	bs.Read(userId);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_KICK_ROOM_MEMBER);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}
	
	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerUser *targetUser = GetUserByID(userId);
	if (targetUser==0)
	{
		reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (targetUser->currentRoom != lsu->currentRoom)
	{
		reply.Write((unsigned char)LC_RC_USER_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu==targetUser)
	{
		// Can't kick yourself!
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Kick OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	Send(&reply,packet->systemAddress);

	bool roomIsDead;
	RemoveRoomMember(targetUser->currentRoom, targetUser->GetID(), &roomIsDead, true, targetUser->systemAddress);
	targetUser->currentRoom=0;

	// Set user state to in lobby
	targetUser->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

	// TODO - Update members in lobby

	// Tell the guy he was kicked out, and by who
	RakNet::BitStream kickNoticeBS;
	kickNoticeBS.Write((unsigned char)ID_LOBBY_GENERAL);
	kickNoticeBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_KICKED_OUT_OF_ROOM);
	KickedOutOfRoom_PC noticeStruct;
	noticeStruct.roomWasDestroyed=roomIsDead;
	noticeStruct._member=lsu->GetID();
	strncpy(noticeStruct.memberHandle, lsu->data->id.handle.C_String(), sizeof(noticeStruct.memberHandle-1));
	noticeStruct.Serialize(&kickNoticeBS);
	Send(&kickNoticeBS, targetUser->systemAddress);
}
void LobbyServer::SetRoomIsHidden_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	bool roomIsHidden;
	bs.Read(roomIsHidden);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SET_INVITE_ONLY);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Action OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.Write(roomIsHidden);
	Send(&reply,packet->systemAddress);

	if (lsu->currentRoom->roomIsHidden!=roomIsHidden)
	{
		lsu->currentRoom->roomIsHidden=roomIsHidden;
		lsu->currentRoom->WriteToRow();

		RakNet::BitStream noticeBS;
		noticeBS.Write((unsigned char)ID_LOBBY_GENERAL);
		noticeBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_SET_INVITE_ONLY);
		RoomSetroomIsHidden_PC output;
		output.isroomIsHidden=roomIsHidden;
		output.Serialize(&noticeBS);
		SendToRoomMembers(lsu->currentRoom, &noticeBS, true, lsu->GetID());
	}
}
void LobbyServer::SetRoomAllowSpectators_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	bool allowSpectators;
	bs.Read(allowSpectators);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SET_ROOM_ALLOW_SPECTATORS);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Action OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.Write(allowSpectators);
	Send(&reply,packet->systemAddress);

	if (lsu->currentRoom->allowSpectators!=allowSpectators)
	{
		lsu->currentRoom->allowSpectators=allowSpectators;
		lsu->currentRoom->WriteToRow();

		RakNet::BitStream noticeBS;
		noticeBS.Write((unsigned char)ID_LOBBY_GENERAL);
		noticeBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_SET_ALLOW_SPECTATORS);
		RoomSetAllowSpectators_PC output;
		output.allowSpectators=allowSpectators;
		output.Serialize(&noticeBS);
		SendToRoomMembers(lsu->currentRoom, &noticeBS, true, lsu->GetID());
	}
}
void LobbyServer::ChangeNumSlots_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	int publicSlots;
	int privateSlots;

	bs.ReadCompressed(publicSlots);
	bs.ReadCompressed(privateSlots);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CHANGE_NUM_SLOTS);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Action OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.WriteCompressed(publicSlots);
	reply.WriteCompressed(privateSlots);
	Send(&reply,packet->systemAddress);

	if (lsu->currentRoom->publicSlots!=publicSlots ||
		lsu->currentRoom->privateSlots!=privateSlots)
	{
		lsu->currentRoom->publicSlots=publicSlots;
		lsu->currentRoom->privateSlots=privateSlots;
		lsu->currentRoom->WriteToRow();

		RakNet::BitStream noticeBS;
		noticeBS.Write((unsigned char)ID_LOBBY_GENERAL);
		noticeBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_CHANGE_NUM_SLOTS);
		RoomChangeNumSlots_PC output;
		output.newPublicSlots=publicSlots;
		output.newPrivateSlots=privateSlots;
		output.Serialize(&noticeBS);
		SendToRoomMembers(lsu->currentRoom, &noticeBS, true, lsu->GetID());
	}
}
void LobbyServer::GrantModerator_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyClientUserId userId;
	bs.Read(userId);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_GRANT_MODERATOR);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerUser *targetUser = GetUserByID(userId);
	if (targetUser==0)
	{
		reply.Write((unsigned char)LC_RC_DISCONNECTED_USER_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (targetUser->currentRoom != lsu->currentRoom)
	{
		reply.Write((unsigned char)LC_RC_USER_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu==targetUser)
	{
		// Can't change moderator to yourself
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("You are already the moderator.", 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (targetUser->currentRoom->IsInSpectatorSlot(targetUser->GetID()))
	{
		// Can't make a spectator a moderator
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Spectators cannot be moderators.", 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Grant OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	reply.Write(userId);
	Send(&reply,packet->systemAddress);

	lsu->currentRoom->SetModerator(userId);

	RakNet::BitStream noticeBS;
	noticeBS.Write((unsigned char)ID_LOBBY_GENERAL);
	noticeBS.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_NEW_MODERATOR);
	NewModerator_PC output;
	output._member=userId;
	output.Serialize(&noticeBS);
	SendToRoomMembers(lsu->currentRoom, &noticeBS, true, lsu->GetID());

}
void LobbyServer::StartGame_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_START_GAME);
	if (lsu->currentRoom==0)
	{
		reply.Write((unsigned char)LC_RC_NOT_IN_ROOM);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->currentRoom->GetModerator()!=lsu->GetID())
	{
		reply.Write((unsigned char)LC_RC_MODERATOR_ONLY);
		Send(&reply,packet->systemAddress);
		return;
	}

	// At least 2 players, and all users ready
	if (lsu->currentRoom->publicSlotMembers.Size()+lsu->currentRoom->privateSlotMembers.Size()<2)
	{
		reply.Write((unsigned char)LC_RC_NOT_ENOUGH_PARTICIPANTS);
		Send(&reply,packet->systemAddress);
		return;
	}

	unsigned i;
	LobbyServerUser *roomMember;
	bool allReady=true;
	for (i=0; i < lsu->currentRoom->publicSlotMembers.Size(); i++)
	{
		roomMember = GetUserByID(lsu->currentRoom->publicSlotMembers[i]);
		RakAssert(roomMember);
		if (roomMember->readyToPlay==false)
		{
			allReady=false;
			break;
		}
	}

	if (allReady)
	{
		for (i=0; i < lsu->currentRoom->privateSlotMembers.Size(); i++)
		{
			roomMember = GetUserByID(lsu->currentRoom->privateSlotMembers[i]);
			RakAssert(roomMember);
			if (roomMember->readyToPlay==false)
			{
				allReady=false;
				break;
			}
		}
	}

	if (allReady==false)
	{
		reply.Write((unsigned char)LC_RC_NOT_READY);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Action OK
	reply.Write((unsigned char)LC_RC_SUCCESS);
	Send(&reply,packet->systemAddress);

	// Notify all members of game start
	RakNet::BitStream gameStartNotification;
	gameStartNotification.Write((unsigned char)ID_LOBBY_GENERAL);
	gameStartNotification.Write((unsigned char)LOBBY_MSGID_NOTIFY_START_GAME);
	gameStartNotification.Write(false); // Not quick match
	gameStartNotification.Write(lsu->systemAddress); // We are the moderator
	gameStartNotification.Write(lsu->title->titleId);
	gameStartNotification.WriteCompressed((unsigned short) (lsu->currentRoom->publicSlotMembers.Size() + lsu->currentRoom->privateSlotMembers.Size()) );
	gameStartNotification.WriteCompressed((unsigned short) lsu->currentRoom->spectators.Size());
	LobbyServerUser *user;
	for (i=0; i < lsu->currentRoom->publicSlotMembers.Size(); i++)
	{
		user=GetUserByID(lsu->currentRoom->publicSlotMembers[i]);
		SerializeClientSafeCreateUserData(user, &gameStartNotification);
	}
	for (i=0; i < lsu->currentRoom->privateSlotMembers.Size(); i++)
	{
		user=GetUserByID(lsu->currentRoom->privateSlotMembers[i]);
		SerializeClientSafeCreateUserData(user, &gameStartNotification);
	}
	for (i=0; i < lsu->currentRoom->spectators.Size(); i++)
	{
		user=GetUserByID(lsu->currentRoom->spectators[i]);
		SerializeClientSafeCreateUserData(user, &gameStartNotification);
	}
	SendToRoomMembers(lsu->currentRoom, &gameStartNotification, false, 0);

	// Everyone is back to the lobby. They probably should just disconnect.
	//DestroyRoom(lsu->currentRoom);
	LobbyRoom *room = lsu->currentRoom;
	ForceAllRoomMembersToLobby(room);
	RemoveRoom(room);
}
void LobbyServer::SubmitMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RankingServerDBSpec::SubmitMatch_Data *input = AllocSubmitMatch_Data();
	input->Deserialize(&bs);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	if (IsTrustedIp(packet->systemAddress, lsu->title->titleId)==false)
	{
		RakNet::BitStream reply;
		reply.Write((unsigned char)ID_LOBBY_GENERAL);
		reply.Write((unsigned char)LOBBY_MSGID_SUBMIT_MATCH);
		reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		Send(&reply,packet->systemAddress);
	}
	else
		SubmitMatch_DBQuery(packet->systemAddress, input);
}
void LobbyServer::DownloadRating_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RankingServerDBSpec::GetRatingForParticipant_Data *input = AllocGetRatingForParticipant_Data();;
	input->Deserialize(&bs);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	GetRating_DBQuery(packet->systemAddress, input);
}
void LobbyServer::QuickMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	int requiredSlots, timeoutInSeconds;
	bs.ReadCompressed(requiredSlots);
	bs.ReadCompressed(timeoutInSeconds);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_QUICK_MATCH);

	if (lsu->status==LOBBY_USER_STATUS_IN_ROOM)
	{
		reply.Write((unsigned char)LC_RC_BUSY);
	}
	else if (lsu->status==LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH)
	{
		reply.Write((unsigned char)LC_RC_IN_PROGRESS);
	}
	else if (lsu->status!=LOBBY_USER_STATUS_IN_LOBBY_IDLE)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
	}
	else if (timeoutInSeconds < 1)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
	}
	else if (requiredSlots<2 || requiredSlots>128)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
	}
	else if (lsu->title==0)
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
	}
	else
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);

		// Limit to 30 seconds until I get a more fair algorithm in place
		if (timeoutInSeconds > 30)
			timeoutInSeconds = 30;

		lsu->status=LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH;

		// Add to quick match list
#ifdef _DEBUG
		unsigned i;
		for (i=0; i < quickMatchUsers.Size(); i++)
			if (quickMatchUsers[i]==lsu)
				RakAssert(0);
#endif
		quickMatchUsers.Insert(lsu);
		lsu->quickMatchStopWaiting=RakNet::GetTime()+timeoutInSeconds*1000;
		lsu->quickMatchUserRequirement=requiredSlots;
	}
	Send(&reply,packet->systemAddress);
}
void LobbyServer::CancelQuickMatch_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CANCEL_QUICK_MATCH);
	if (lsu->status==LOBBY_USER_STATUS_WAITING_ON_QUICK_MATCH)
	{
		reply.Write((unsigned char)LC_RC_SUCCESS);
		lsu->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;

		// Remove from quick match list
		RemoveFromQuickMatchList(lsu);
		
	}
	else
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
	}

	
	Send(&reply,packet->systemAddress);
}
void LobbyServer::DownloadActionHistory_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	bool ascendingSort;
	bs.Read(ascendingSort);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	DownloadActionHistory_DBQuery(packet->systemAddress, ascendingSort, lsu->GetID());
}
void LobbyServer::AddToActionHistory_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyDBSpec::AddToActionHistory_Data* input = AllocAddToActionHistory_Data();
	input->Deserialize(&bs);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ADD_TO_ACTION_HISTORY);

	if ((lsu->title && lsu->title->defaultAllowClientsUploadActionHistory) ||
		IsTrustedIp(packet->systemAddress, lsu->title->titleId))
	{
		input->id.databaseRowId=lsu->GetID();
		input->id.hasDatabaseRowId=true;
		AddToActionHistory_DBQuery(packet->systemAddress, input);
	}
	else
	{
		if (lsu->title==0)
			reply.Write((unsigned char)LC_RC_UNKNOWN_PERMISSIONS);
		else
			reply.Write((unsigned char)LC_RC_PERMISSION_DENIED);
		Send(&reply,packet->systemAddress);
	}
}
void LobbyServer::CreateClan_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyDBSpec::CreateClan_Data *createClan = AllocCreateClan_Data();
	bool failIfAlreadyInClan;
	createClan->Deserialize(&bs);
	bs.Read(failIfAlreadyInClan);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CREATE_CLAN);

	LobbyServerClan* clan = lsu->GetFirstClan();

	// 1. Fail if (already in clan or pending clan integer>0) and failIfAlreadyInClan==true
	if (failIfAlreadyInClan && clan)
	{
		reply.Write((unsigned char)LC_RC_ALREADY_CURRENT_STATE);
		stringCompressor->EncodeString(RakString("Already in clan %s.", clan->handle.C_String()), 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (failIfAlreadyInClan && lsu->pendingClanCount!=0)
	{
		reply.Write((unsigned char)LC_RC_ALREADY_CURRENT_STATE);
		stringCompressor->EncodeString("Clan join already in progress.", 512, &reply, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	// 2. Increment pending clan integer.
	lsu->pendingClanCount++;

	createClan->leaderData.userId.hasDatabaseRowId=true;
	createClan->leaderData.userId.databaseRowId=lsu->GetID();
	createClan->leaderData.updateMEStatus1=true;
	createClan->leaderData.mEStatus1=LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER;

	// 3. Process LobbyDBSpec::CreateClan_Data to database
	CreateClan_DBQuery(createClan);
}
void LobbyServer::ChangeClanHandle_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CHANGE_CLAN_HANDLE);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *lsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateLeader(lsm, &reply)==false)
	{
		Send(&reply,packet->systemAddress);
		return;
	}

	char newHandle[512];
	stringCompressor->DecodeString(newHandle, 512, &bs, 0);

	LobbyDBSpec::ChangeClanHandle_Data *dbFunctor = AllocChangeClanHandle_Data();
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->newHandle=newHandle;
	ChangeClanHandle_DBQuery(dbFunctor);
}
void LobbyServer::LeaveClan_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	bool dissolveIfClanLeader;
	bool autoSendEmailIfClanDissolved;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_LEAVE_CLAN);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	bs.Read(dissolveIfClanLeader);
	bs.Read(autoSendEmailIfClanDissolved);

	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetClanMember(lsc, lsu->GetID());
	if (targetLsm==0 || targetLsm->clanStatus>=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
	{
		reply.Write(LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(RakString("Not a member of clan %s.",lsc->handle.C_String()), 512, &reply, 0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->userId.databaseRowId=lsu->GetID();
	dbFunctor->userId.hasDatabaseRowId=true;

	targetLsm->clanSourceMemberId=lsu->data->id.databaseRowId;
	targetLsm->clanSourceMemberHandle=lsu->data->id.handle;

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, dissolveIfClanLeader, autoSendEmailIfClanDissolved, LOBBY_MSGID_LEAVE_CLAN);

}
void LobbyServer::DownloadClans_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyClientUserId userId=0;
	char userHandle[512];
	LobbyServerUser *target;
	bool ascendingSort;
	target=DeserializeHandleOrId(userHandle,userId,&bs);
	bs.Read(ascendingSort);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLANS);

	// If target==0 && userId==0 && userHandle==0 then target=lsu
	if (userHandle[0]==0 && userId==0)
	{
		target=lsu;
		userId=target->GetID();
	}
	else if (target)
		userId=target->GetID();

	/// Do DB query. Don't just use memory in case it changed from an external source.
	GetClans_DBQuery(packet->systemAddress, userId, userHandle, ascendingSort);
}
void LobbyServer::SendClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_CLAN_JOIN_INVITATION);

	ClanId clanId;
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);

	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());

	// If target==lsu, can't invite yourself
	if (ValidateNotSelf(targetLsm, myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// If not leader or subleader, return failure
	if (ValidateLeaderOrSubleader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (ValidateNotBusy(targetLsm, RakString(userHandle), lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (targetLsm!=0)
	{
		if (targetLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
		{
			reply.Write((unsigned char) LC_RC_BLACKLISTED_FROM_CLAN);
			stringCompressor->EncodeString(RakString("%s must be unblacklisted from clan %s first.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		}
		else if (targetLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
		{
			reply.Write((unsigned char) LC_RC_ALREADY_CURRENT_STATE);
			stringCompressor->EncodeString(RakString("%s has already requested to join clan %s.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		}
		else if (targetLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN)
		{
			reply.Write((unsigned char) LC_RC_ALREADY_CURRENT_STATE);
			stringCompressor->EncodeString(RakString("%s has already been invited to join clan %s.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		}
		else
		{
			reply.Write((unsigned char) LC_RC_ALREADY_CURRENT_STATE);
			stringCompressor->EncodeString(RakString("%s is already a member of clan %s.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		}
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_JOIN_INVITATION);
		
	LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=userId!=0;
	dbFunctor->mEStatus1=LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN;

	UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_SEND_CLAN_JOIN_INVITATION, lsu->systemAddress, myLsm->userId, GetHandleByID(myLsm->userId));

}
void LobbyServer::WithdrawClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_WITHDRAW_CLAN_JOIN_INVITATION);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);

	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());

	// If target==lsu, can't invite yourself
	if (ValidateNotSelf(targetLsm, myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// If not leader or subleader, return failure
	if (ValidateLeaderOrSubleader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (ValidateNotBusy(targetLsm, RakString(userHandle), lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate invited
	if (targetLsm && targetLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN)
	{
		reply.Write((unsigned char) LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(RakString("Cannot withdraw invitation to %s for clan %s as they were not invited to begin with.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_WITHDRAW_JOIN_INVITATION);

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=userId!=0;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN;

	if (targetLsm)
	{
		targetLsm->clanSourceMemberId=myLsm->userId;
		targetLsm->clanSourceMemberHandle=GetHandleByID(myLsm->userId);
	}	

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_WITHDRAW_CLAN_JOIN_INVITATION);

}
void LobbyServer::AcceptClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ACCEPT_CLAN_JOIN_INVITATION);


	LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
	bool failIfAlreadyInClan;
	bs.Read(failIfAlreadyInClan);
	dbFunctor->Deserialize(&bs);

	LobbyServerClan *lsc = GetClanByHandleOrId(dbFunctor->clanId.handle, dbFunctor->clanId.databaseRowId);
	/*
	if (lsc==0)
	{
		reply.Write((unsigned char) LC_RC_INVALID_CLAN);
		if (updateClanMember->clanId.handle.IsEmpty()==false)
			stringCompressor->EncodeString(RakNet::RakString("Unknown clan %s", updateClanMember->clanId.handle.C_String()), 512, &reply, 0);
		else
			stringCompressor->EncodeString(RakNet::RakString("Unknown clan with index %i", updateClanMember->clanId.databaseRowId), 512, &reply, 0);

		delete updateClanMember;
		Send(&reply,packet->systemAddress);
		return;
	}
	*/

	if (dbFunctor->clanId.handle.IsEmpty() && dbFunctor->clanId.hasDatabaseRowId==false)
	{
		reply.Write((unsigned char) LC_RC_INVALID_CLAN);
		if (dbFunctor->clanId.handle.IsEmpty())
			stringCompressor->EncodeString("No clan specified.\n", 512, &reply, 0);
		else
			stringCompressor->EncodeString("",512,&reply,0);
		stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
		reply.Write(dbFunctor->clanId.hasDatabaseRowId);

		delete dbFunctor;
		Send(&reply,packet->systemAddress);
		return;
	}

	if (failIfAlreadyInClan && ValidateOneClan(lsu,&reply)==false)
	{
		stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
		reply.Write(dbFunctor->clanId.hasDatabaseRowId);
		delete dbFunctor;
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm=0;

	// Validate invited to begin with
	unsigned i;
	if (lsc)
	{
		for (i=0; i < lsc->clanMembers.Size(); i++)
		{
			if (lsc->clanMembers[i]->userId==lsu->GetID())
			{
				if (lsc->clanMembers[i]->clanStatus < LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are already a member of clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
					reply.Write(dbFunctor->clanId.hasDatabaseRowId);
					Send(&reply,packet->systemAddress);
					delete dbFunctor;
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are not invited to clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
					reply.Write(dbFunctor->clanId.hasDatabaseRowId);
					Send(&reply,packet->systemAddress);
					delete dbFunctor;
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are blacklisted from clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
					reply.Write(dbFunctor->clanId.hasDatabaseRowId);
					Send(&reply,packet->systemAddress);
					delete dbFunctor;
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN)
				{
					targetLsm=lsc->clanMembers[i];
					break;
				}
			}			
		}

		if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
		{
			stringCompressor->EncodeString(&dbFunctor->clanId.handle,512,&reply,0);
			reply.Write(dbFunctor->clanId.hasDatabaseRowId);
			Send(&reply,packet->systemAddress);
			delete dbFunctor;
			return;
		}
	}
	

	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_ACCEPTING_JOIN_INVITATION);
	
	dbFunctor->clanId.hasDatabaseRowId=dbFunctor->clanId.databaseRowId!=0;
	dbFunctor->userId.databaseRowId=lsu->GetID();
	dbFunctor->userId.hasDatabaseRowId=true;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN;
	dbFunctor->mEStatus1=LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER;
	dbFunctor->updateMEStatus1=true;

	UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_ACCEPT_CLAN_JOIN_INVITATION, lsu->systemAddress, lsu->GetID(), lsu->data->id.handle);
}
void LobbyServer::RejectClanJoinInvitation_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyClientUserId clanId;
	char clanHandle[512];
	LobbyServerClan *lsc=DeserializeClanHandleOrId(clanHandle,clanId,&bs);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REJECT_CLAN_JOIN_INVITATION);

	if (clanHandle[0]==0 && clanId==0)
	{
		reply.Write((unsigned char) LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("No clan specified.\n", 512, &reply, 0);
		stringCompressor->EncodeString(clanHandle,512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate invited to begin with
	unsigned i;
	LobbyServerClanMember *targetLsm=0;
	if (lsc)
	{
		for (i=0; i < lsc->clanMembers.Size(); i++)
		{
			if (lsc->clanMembers[i]->userId==lsu->GetID())
			{
				if (lsc->clanMembers[i]->clanStatus < LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are already a member of clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are not invited to clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are blacklisted from clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN)
				{
					targetLsm=lsc->clanMembers[i];
					break;
				}
			}
		}

		if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
		{
			Send(&reply,packet->systemAddress);
			stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
			reply.Write(lsc->clanId);
			return;
		}
	}


	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_REJECTING_JOIN_INVITATION);

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->clanId.handle=clanHandle;
	dbFunctor->clanId.hasDatabaseRowId=dbFunctor->clanId.databaseRowId!=0;
	dbFunctor->userId.databaseRowId=lsu->GetID();
	dbFunctor->userId.hasDatabaseRowId=true;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN;

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_REJECT_CLAN_JOIN_INVITATION);
}
void LobbyServer::DownloadByClanMemberStatus_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyDBSpec::ClanMemberStatus status;
	bs.Read(status);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_BY_CLAN_MEMBER_STATUS);

	DataStructures::List<LobbyServerClanMember*> matchingMembers;
	
	unsigned i;
	for (i=0; i < lsu->clanMembers.Size(); i++)
	{
		if (lsu->clanMembers[i]->clanStatus==status)
			matchingMembers.Push(lsu->clanMembers[i]);
	}

	reply.Write(status);
	reply.Write(matchingMembers.Size());
	for (i=0; i < matchingMembers.Size(); i++)
	{
		reply.Write(matchingMembers[i]->clan->clanId);
		stringCompressor->EncodeString(&matchingMembers[i]->clan->handle, 512, &reply, 0);
	}

	reply.Write((unsigned char) LC_RC_SUCCESS);
	Send(&reply,packet->systemAddress);

}
void LobbyServer::SendClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
	dbFunctor->Deserialize(&bs);
	LobbyServerClan *lsc = GetClanByHandleOrId(dbFunctor->clanId.handle, dbFunctor->clanId.databaseRowId);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_CLAN_MEMBER_JOIN_REQUEST);

	if (dbFunctor->clanId.handle.IsEmpty()==true && dbFunctor->clanId.databaseRowId==0)
	{
		reply.Write((unsigned char) LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("No clan specified.\n", 512, &reply, 0);
		stringCompressor->EncodeString(&dbFunctor->clanId.handle, 512, &reply, 0);
		reply.Write(dbFunctor->clanId.databaseRowId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate invited to begin with
	unsigned i;
	LobbyServerClanMember *targetLsm=0;
	if (lsc)
	{
		for (i=0; i < lsc->clanMembers.Size(); i++)
		{
			if (lsc->clanMembers[i]->userId==lsu->GetID())
			{
				if (lsc->clanMembers[i]->clanStatus < LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are already a member of clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You have already requested to join clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You are blacklisted from clan %s.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}

				if (lsc->clanMembers[i]->clanStatus == LobbyDBSpec::CLAN_MEMBER_STATUS_INVITED_TO_JOIN)
				{
					bs.Write((unsigned char) LC_RC_INVALID_INPUT);
					stringCompressor->EncodeString(RakString("You have already been invited to join clan %s. Accept the invitation to join.", lsc->handle.C_String()), 512, &bs, 0);
					stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
					reply.Write(lsc->clanId);
					Send(&reply,packet->systemAddress);
					return;
				}
			}
		}

		if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
		{
			stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
			reply.Write(lsc->clanId);
			Send(&reply,packet->systemAddress);
			return;
		}
	}


	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_JOIN_REQUEST);

	dbFunctor->userId.databaseRowId=lsu->GetID();
	dbFunctor->userId.hasDatabaseRowId=true;
	dbFunctor->mEStatus1=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN;

	UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_SEND_CLAN_MEMBER_JOIN_REQUEST, lsu->systemAddress, 0, "");
}
void LobbyServer::WithdrawClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyClientUserId clanId;
	char clanHandle[512];
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("", 512, &reply, 0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("", 512, &reply, 0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateNotBusy(myLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate invited
	if (myLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
	{
		reply.Write((unsigned char) LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(RakString("Cannot withdraw request for %s to join clan %s as they did not request to begin with.",lsu->data->id.handle.C_String(), lsc->handle.C_String()), 512, &reply, 0);
		stringCompressor->EncodeString(&lsc->handle, 512, &reply, 0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}


	SetOptionalBusyStatus(myLsm, LobbyServerClanMember::LSCM_BUSY_WITHDRAW_JOIN_REQUEST);

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->clanId.handle=clanHandle;
	dbFunctor->clanId.hasDatabaseRowId=lsu->GetID()!=0;
	dbFunctor->userId.databaseRowId=lsu->GetID();
	dbFunctor->userId.hasDatabaseRowId=true;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN;

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_WITHDRAW_CLAN_MEMBER_JOIN_REQUEST);
}
void LobbyServer::AcceptClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ACCEPT_CLAN_MEMBER_JOIN_REQUEST);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);
	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	bool failIfAlreadyInClan;
	bs.Read(failIfAlreadyInClan);

	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (failIfAlreadyInClan && ValidateOneClan(target,&reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (ValidateLeader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate requested
	if (myLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
	{
		reply.Write((unsigned char) LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(RakString("Cannot accept join request for %s to join clan %s as they did not request to begin with.",target->data->id.handle.C_String(), lsc->handle.C_String()), 512, &reply, 0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_ACCEPTING_JOIN_REQUEST);

	LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=userId!=0;
	dbFunctor->mEStatus1=LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER;

	UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_ACCEPT_CLAN_MEMBER_JOIN_REQUEST, lsu->systemAddress, 0, "");
}
void LobbyServer::RejectClanMemberJoinRequest_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	LobbyClientUserId userId;
	char userHandle[512];
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REJECT_CLAN_MEMBER_JOIN_REQUEST);
	bool deserializedClan = DeserializeClanIdDefaultSelf(lsu, &bs, clanId);
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);
	if (deserializedClan==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0); // Reason
		reply.WriteCompressed(""); // clanHandle
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0); // Reason
		reply.WriteCompressed(""); // clanHandle
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);

	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(clanId);
		if (target)
			reply.WriteCompressed(target->data->id.handle);
		else
			reply.WriteCompressed((char*)userHandle);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (ValidateLeader(myLsm, &reply)==false)
	{
		reply.Write(clanId);
		if (target)
			reply.WriteCompressed(target->data->id.handle);
		else
			reply.WriteCompressed((char*)userHandle);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Validate requested
	if (targetLsm && targetLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
	{
		reply.Write((unsigned char) LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString(RakString("Cannot reject join request from %s to join clan %s as they did not request to begin with.",target->data->id.handle.C_String(), lsc->handle.C_String()), 512, &reply, 0);
		reply.Write(clanId);
		if (target)
			reply.WriteCompressed(target->data->id.handle);
		else
			reply.WriteCompressed((char*)userHandle);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_REJECTING_JOIN_REQUEST);

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->clanId.handle=lsc->handle;
	dbFunctor->clanId.hasDatabaseRowId=true;
	if (target)
		dbFunctor->userId.databaseRowId=target->GetID();
	else
		dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=dbFunctor->userId.databaseRowId!=0;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN;

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_REJECT_CLAN_MEMBER_JOIN_REQUEST);
}
void LobbyServer::SetClanMemberRank_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SET_CLAN_MEMBER_RANK);

	ClanId clanId;
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		stringCompressor->EncodeString("",512,&reply,0);
		reply.Write(clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);
	LobbyDBSpec::ClanMemberStatus newRank;
	bs.Read(newRank);

	if (newRank>LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER)
		return;

	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateLeader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	if (targetLsm && targetLsm->clanStatus==newRank)
	{
		reply.Write(LC_RC_ALREADY_CURRENT_STATE);
		stringCompressor->EncodeString("Already current rank",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (lsu->GetID()==userId)
	{
		reply.Write(LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Cannot change your own rank",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
	dbFunctor->clanId.hasDatabaseRowId=true;
	dbFunctor->clanId.databaseRowId=clanId;
	if (target)
		dbFunctor->userId.databaseRowId=target->GetID();
	else
		dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=dbFunctor->userId.databaseRowId!=0;
	dbFunctor->updateMEStatus1=true;
	dbFunctor->hasRequiredLastStatus=false;
	dbFunctor->mEStatus1=newRank;

	UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_SET_CLAN_MEMBER_RANK, lsu->systemAddress, myLsm->userId, GetHandleByID(myLsm->userId));
}
void LobbyServer::ClanKickAndBlacklistUser_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}
	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);
	bool kick;
	bool blacklist;
	bs.Read(kick);
	bs.Read(blacklist);
	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	if (kick==false && blacklist==false)
	{
		reply.Write(LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("No action requested",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateLeaderOrSubleader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}


	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(clanId);
		if (target)
			reply.WriteCompressed(target->data->id.handle);
		else
			reply.WriteCompressed((char*)userHandle);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (myLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER && targetLsm && targetLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
	{
		reply.Write(LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Subleader cannot kick or ban the leader",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	// Tag reply pending, to avoid contention
	SetOptionalBusyStatus(targetLsm, LobbyServerClanMember::LSCM_BUSY_BLACKLISTING);

	if (blacklist)
	{
		// Set to blacklisted, add if not already there.
		LobbyDBSpec::UpdateClanMember_Data *dbFunctor = AllocUpdateClanMember_Data();
		dbFunctor->clanId.hasDatabaseRowId=true;
		dbFunctor->clanId.databaseRowId=clanId;
		if (target)
			dbFunctor->userId.databaseRowId=target->GetID();
		else
			dbFunctor->userId.databaseRowId=userId;
		dbFunctor->userId.handle=userHandle;
		dbFunctor->userId.hasDatabaseRowId=dbFunctor->userId.databaseRowId!=0;
		dbFunctor->updateMEStatus1=true;
		if (myLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
		{
			dbFunctor->hasRequiredLastStatus=false;
		}
		else
		{
			dbFunctor->hasRequiredLastStatus=true;
			dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER;
		}

		UpdateClanMember_DBQuery(dbFunctor, LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER, lsu->systemAddress, myLsm->userId, GetHandleByID(myLsm->userId));
	}
	else if (kick)
	{
		// Remove from clan only

		LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
		dbFunctor->clanId.databaseRowId=clanId;
		dbFunctor->clanId.handle=lsc->handle;
		dbFunctor->clanId.hasDatabaseRowId=true;
		if (target)
			dbFunctor->userId.databaseRowId=target->GetID();
		else
			dbFunctor->userId.databaseRowId=userId;
		dbFunctor->userId.handle=userHandle;
		dbFunctor->userId.hasDatabaseRowId=dbFunctor->userId.databaseRowId!=0;
		dbFunctor->hasRequiredLastStatus=false;
		if (myLsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
		{
			dbFunctor->hasRequiredLastStatus=false;
		}
		else
		{
			dbFunctor->hasRequiredLastStatus=true;
			dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_MEMBER;
		}

		RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_CLAN_KICK_AND_BLACKLIST_USER);
	}

}
void LobbyServer::ClanUnblacklistUser_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_CLAN_UNBLACKLIST_USER);
	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}
	LobbyClientUserId userId;
	char userHandle[512];
	LobbyServerUser *target=DeserializeHandleOrId(userHandle,userId,&bs);
	// If no clan, return failure
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		stringCompressor->EncodeString("",512,&reply,0);
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}
	LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
	if (ValidateLeader(myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *targetLsm = GetOptionalClanMember(lsc, target);
	if (ValidateNotBusy(targetLsm, lsu->data->id.handle, lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(clanId);
		if (target)
			reply.WriteCompressed(target->data->id.handle);
		else
			reply.WriteCompressed((char*)userHandle);
		Send(&reply,packet->systemAddress);
		return;
	}

	// If target==lsu, can't invite yourself
	if (ValidateNotSelf(targetLsm, myLsm, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (ValidateNotBusy(targetLsm, RakString(userHandle), lsc->handle, &reply)==false)
	{
		stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	if (targetLsm!=0)
	{
		if (targetLsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
		{
			reply.Write((unsigned char) LC_RC_BLACKLISTED_FROM_CLAN);
			stringCompressor->EncodeString(RakString("%s is not blacklisted from clan %s.",userHandle, lsc->handle.C_String()), 512, &reply, 0);
		}

		reply.Write(lsc->clanId);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyDBSpec::RemoveFromClan_Data *dbFunctor = AllocRemoveFromClan_Data();
	dbFunctor->clanId.databaseRowId=clanId;
	dbFunctor->clanId.handle=lsc->handle;
	dbFunctor->clanId.hasDatabaseRowId=true;
	if (target)
		dbFunctor->userId.databaseRowId=target->GetID();
	else
		dbFunctor->userId.databaseRowId=userId;
	dbFunctor->userId.handle=userHandle;
	dbFunctor->userId.hasDatabaseRowId=dbFunctor->userId.databaseRowId!=0;
	dbFunctor->hasRequiredLastStatus=true;
	dbFunctor->requiredLastMEStatus=LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED;

	RemoveFromClan_DBQuery(packet->systemAddress, dbFunctor, false, false, LOBBY_MSGID_CLAN_UNBLACKLIST_USER);

}
void LobbyServer::AddPostToClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_ADD_POST_TO_CLAN_BOARD);
	LobbyDBSpec::AddToClanBoard_Data *addToClanBoard = AllocAddToClanBoard_Data();
	addToClanBoard->Deserialize(&bs);
	LobbyServerClan *lsc;
	lsc = GetClanByHandle(addToClanBoard->clanId.handle);
	if (lsc==0)
		lsc = GetClanById(addToClanBoard->clanId.databaseRowId);
	bool failIfNotLeader;
	bool failIfNotMember;
	bool failIfBlacklisted;
	bs.Read(failIfNotLeader);
	bs.Read(failIfNotMember);
	bs.Read(failIfBlacklisted);

	// If one of 3 failure checks, fail
	if (lsc)
	{
		LobbyServerClanMember *myLsm = GetClanMember(lsc, lsu->GetID());
		if (failIfNotMember && myLsm==0)
		{
			reply.Write(LC_RC_PERMISSION_DENIED);
			reply.Write("Only clan members can post.");
			stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
			reply.Write(lsc->clanId);
			Send(&reply,packet->systemAddress);
			delete addToClanBoard;
			return;
		}

		if (failIfNotLeader && myLsm && ValidateLeader(myLsm, &reply)==false)
		{
			stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
			reply.Write(lsc->clanId);
			Send(&reply,packet->systemAddress);
			delete addToClanBoard;
			return;
		}

		if (failIfBlacklisted && ValidateNotBlacklisted(myLsm, &reply)==false)
		{
			stringCompressor->EncodeString(&lsc->handle,512,&reply,0);
			reply.Write(lsc->clanId);
			Send(&reply,packet->systemAddress);
			delete addToClanBoard;
			return;
		}
	}

	addToClanBoard->userId.databaseRowId=lsu->GetID();
	addToClanBoard->userId.hasDatabaseRowId=true;

	AddPostToClanBoard_DBQuery(packet->systemAddress, addToClanBoard);
}
void LobbyServer::RemovePostFromClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	ClanId clanId;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_REMOVE_POST_FROM_CLAN_BOARD);

	if (DeserializeClanIdDefaultSelf(lsu, &bs, clanId)==false)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}
	ClanBoardPostId postId;
	bs.Read(postId);
	bool allowMemberToRemoveOwnPosts;
	bs.Read(allowMemberToRemoveOwnPosts);

	// If unknown clan, fail
	LobbyServerClan *lsc = GetClanById(clanId);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyDBSpec::RemoveFromClanBoard_Data *removeFromClanBoard = AllocRemoveFromClanBoard_Data();
	removeFromClanBoard->postId=postId;

	RemovePostFromClanBoard_DBQuery(packet->systemAddress, removeFromClanBoard);

}
void LobbyServer::DownloadMyClanBoards_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	bool ascendingSort;
	bs.Read(ascendingSort);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_MY_CLAN_BOARDS);

	unsigned i;
	LobbyServerClan *lsc;
	LobbyDBSpec::GetClanBoard_Data *getClanBoard;
	for (i=0; i < lsu->clanMembers.Size(); i++)
	{
		lsc = lsu->clanMembers[i]->clan;
		getClanBoard = AllocGetClanBoard_Data();
		getClanBoard->ascendingSort=ascendingSort;
		getClanBoard->clanId.databaseRowId=lsc->clanId;
		getClanBoard->clanId.hasDatabaseRowId=true;

		DownloadClanBoard_DBQuery(packet->systemAddress, getClanBoard, LOBBY_MSGID_DOWNLOAD_MY_CLAN_BOARDS);
	}
	
	reply.Write(LC_RC_SUCCESS);
	reply.Write(lsu->clanMembers.Size());
	Send(&reply,packet->systemAddress);
}
void LobbyServer::DownloadClanBoard_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLAN_BOARD);

	LobbyClientUserId clanId;
	char clanHandle[512];
	LobbyServerClan *lsc=DeserializeClanHandleOrId(clanHandle,clanId,&bs);
	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}
	bool ascendingSort;
	bs.Read(ascendingSort);

	// Do DB Query for this one clan
	LobbyDBSpec::GetClanBoard_Data *getClanBoard;
	getClanBoard = AllocGetClanBoard_Data();
	getClanBoard->ascendingSort=ascendingSort;
	getClanBoard->clanId.databaseRowId=lsc->clanId;
	getClanBoard->clanId.hasDatabaseRowId=true;
	DownloadClanBoard_DBQuery(packet->systemAddress, getClanBoard, LOBBY_MSGID_DOWNLOAD_CLAN_BOARD);
}
void LobbyServer::DownloadClanMember_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	LobbyServerUser *lsu = GetUserByAddress(packet->systemAddress);
	if (lsu==0)
		return;

	LobbyClientUserId clanId;
	char clanHandle[512];
	LobbyServerClan *lsc=DeserializeClanHandleOrId(clanHandle,clanId,&bs);
	LobbyClientUserId userId;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_DOWNLOAD_CLAN_MEMBER);

	if (lsc==0)
	{ 
		reply.Write(LC_RC_INVALID_CLAN);
		Send(&reply,packet->systemAddress);
		return;
	}

	LobbyServerClanMember *lsm = GetClanMember(lsc, userId);
	if (lsm)
	{
		reply.Write(LC_RC_SUCCESS);
		lsm->memberData->Serialize(&reply);

	}
	else
	{
		reply.Write(LC_RC_UNKNOWN_USER_ID);
	}
	Send(&reply,packet->systemAddress);
}
void LobbyServer::ValidateUserKey_NetMsg(Packet *packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(char) * 2);

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_VALIDATE_USER_KEY);

	LobbyServerUser *user = GetUserByAddress(packet->systemAddress);
	if (user==0)
		return;

	char keyPassword[512];
	int keyPasswordLength;
	char titleIdStr[512];
	stringCompressor->DecodeString(titleIdStr, sizeof(titleIdStr), &bs, 0);
	bs.ReadAlignedBytesSafe((char*)keyPassword,keyPasswordLength,512);

	if (keyPasswordLength>sizeof(keyPassword))
	{
		reply.Write((unsigned char)LC_RC_BAD_TITLE_ID);
		Send(&reply,packet->systemAddress);
		return;
	}

	TitleValidationDBSpec::AddTitle_Data *title=GetTitleDataByTitleName(titleIdStr);
	if (title==0)
	{
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("keyPasswordLength>512", 512, &bs, 0);
		Send(&reply,packet->systemAddress);
		return;
	}

	TitleValidationDBSpec::ValidateUserKey_Data *dbFunctor = AllocValidateUserKey_Data();
	dbFunctor->titleId=title->titleId;
	memcpy(dbFunctor->keyPassword,keyPassword,sizeof(keyPassword));
	dbFunctor->keyPassword=keyPassword;
	dbFunctor->keyPasswordLength=keyPasswordLength;
	
	ValidateUserKey_DBQuery(packet->systemAddress, dbFunctor);

}
void LobbyServer::ClearUsers(void)
{
	unsigned i;
	for (i=0; i < usersBySysAddr.Size(); i++)
	{
		usersBySysAddr[i]->data->Deref();
		delete usersBySysAddr[i];
	}
	usersBySysAddr.Clear(true);
	usersByID.Clear(true);
}
void LobbyServer::DestroyRoom(LobbyRoom *room)
{
	// Notify all remaining members (mostly spectators?) that the room is closed
	RakNet::BitStream bs;
	bs.Write((unsigned char)ID_LOBBY_GENERAL);
	bs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_DESTROYED);
	SendToRoomMembers(room, &bs);
	ForceAllRoomMembersToLobby(room);

	RemoveRoom(room);
}
void LobbyServer::RemoveUser(SystemAddress sysAddr)
{
	unsigned friendsIndex;
	unsigned index;
	bool objectExists;
	index = usersBySysAddr.GetIndexFromKey(sysAddr, &objectExists);
	if (objectExists)
	{
		LobbyServerUser *lsu = usersBySysAddr[index];
		RemoveFromPotentialFriends(lsu->data->id.databaseRowId);

		for (friendsIndex=0; friendsIndex < lsu->friends.Size(); friendsIndex++)
		{
			LobbyServerUser* _friend = GetUserByID(lsu->friends[friendsIndex]->friendId.databaseRowId);
			if (_friend)
			{
				NotifyFriendStatusChange(_friend->systemAddress, lsu->GetID(), lsu->data->id.handle.C_String(), FRIEND_STATUS_FRIEND_OFFLINE);
			}
		}

		// Remove from the room
		if (lsu->currentRoom)
		{
			bool roomIsDead;
			RemoveRoomMember(lsu->currentRoom, lsu->GetID(), &roomIsDead, false, lsu->systemAddress);
		}

		LobbyServerClanMember *lscm;
		unsigned clanIndex, subIndex;
		for (clanIndex=0; clanIndex < lsu->clanMembers.Size(); clanIndex++)
		{
			lscm = lsu->clanMembers[clanIndex];
			for (subIndex=0; subIndex < lscm->clan->clanMembers.Size(); subIndex++)
			{
				if (lscm->clan->clanMembers[subIndex]==lscm)
				{
					lscm->clan->clanMembers.RemoveAtIndexFast(subIndex);
					if (lscm->clan->clanMembers.Size()==0)
						DeleteClan(lscm->clan);
					delete lscm;
					break;
				}
			}

		}

		usersByID.Remove(usersBySysAddr[index]->GetID());
		delete usersBySysAddr[index];
		usersBySysAddr.RemoveAtIndex(index);
		

		RemoveFromQuickMatchList(lsu);
	}
}
void LobbyServer::RemoveRoomMember(LobbyRoom *room, LobbyDBSpec::DatabaseKey memberId, bool *roomIsDead, bool wasKicked, SystemAddress memberAddr)
{
	// Note to self: Don't forget to update the member's room pointer, if necessary.

	*roomIsDead=false;
	if (room==0)
		return;
	bool gotNewModerator;
	room->RemoveRoomMember(memberId, &gotNewModerator);
	if (room->RoomIsDead())
	{
		DestroyRoom(room);
	}
	else
	{
		// Serialize new room state to all members
		RakNet::BitStream bs;
		bs.Write((unsigned char)ID_LOBBY_GENERAL);
		bs.Write((unsigned char)LOBBY_MSGID_NOTIFY_ROOM_MEMBER_DROP);

		RoomMemberDrop_PC drop;
		drop.memberAddress=memberAddr;
		drop._droppedMember=memberId;
		drop.wasKicked=wasKicked;
		drop.gotNewModerator=gotNewModerator;
		drop._newModerator=room->GetModerator();
		drop.Serialize(&bs);

		SendToRoomMembers(room, &bs);

		// Update the table of rooms
		room->WriteToRow(false);
	}
}
void LobbyServer::SendToRoomMembers(LobbyRoom *room, RakNet::BitStream *bs, bool hasSkipId, LobbyDBSpec::DatabaseKey skipId)
{
	LobbyServerUser* user;

	unsigned i;
	for (i=0; i < room->publicSlotMembers.Size(); i++)
	{
		if (hasSkipId && skipId==room->publicSlotMembers[i])
			continue;
		user=GetUserByID(room->publicSlotMembers[i]);
		Send(bs, user->systemAddress);
	}
	for (i=0; i < room->privateSlotMembers.Size(); i++)
	{
		if (hasSkipId && skipId==room->privateSlotMembers[i])
			continue;
		user=GetUserByID(room->privateSlotMembers[i]);
		Send(bs, user->systemAddress);
	}

	for (i=0; i < room->spectators.Size(); i++)
	{
		if (hasSkipId && skipId==room->spectators[i])
			continue;
		user=GetUserByID(room->spectators[i]);
		Send(bs, user->systemAddress);
	}
}
void LobbyServer::ForceAllRoomMembersToLobby(LobbyRoom *room)
{
	LobbyServerUser* user;

	unsigned i;
	for (i=0; i < room->publicSlotMembers.Size(); i++)
	{
		user=GetUserByID(room->publicSlotMembers[i]);
		user->currentRoom=0;
		user->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
	}
	for (i=0; i < room->privateSlotMembers.Size(); i++)
	{
		user=GetUserByID(room->privateSlotMembers[i]);
		user->currentRoom=0;
		user->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
	}

	for (i=0; i < room->spectators.Size(); i++)
	{
		user=GetUserByID(room->spectators[i]);
		user->currentRoom=0;
		user->status=LOBBY_USER_STATUS_IN_LOBBY_IDLE;
	}
}
void LobbyServer::RemoveRoom(LobbyRoom *room)
{
	// Remove the room from the table
	LobbyRoomsTableContainer* lobbyTitle;
	bool objectExists;
	unsigned index = lobbyTitles.GetIndexFromKey(room->titleId, &objectExists);
	RakAssert(objectExists);
	lobbyTitle=lobbyTitles[index];
	lobbyTitle->roomsTable->RemoveRow(room->rowId);

	if (lobbyTitle->roomsTable->GetRowCount()==0)
	{
		// If roomsTable is empty, delete it, and the lobbyGame, and remove the lobbyGame pointer from the lobbyGames list.
		// (Destructor does all this)
		delete lobbyTitle;
	}

	lobbyTitles.RemoveAtIndex(index);

	// Delete the room.
	delete room;

	// TODO - update room lists of users in lobby
}
LobbyServerUser* LobbyServer::GetUserByAddress(SystemAddress sysAddr)
{
	LobbyServerUser *out;
	if (usersBySysAddr.GetElementFromKey(sysAddr, out))
		return out;
	return 0;
}
LobbyServerUser* LobbyServer::GetUserByHandle(const char *handle)
{
	unsigned i;
	for (i=0; i < usersBySysAddr.Size(); i++)
		if (stricmp(usersBySysAddr[i]->data->id.handle.C_String(),handle)==0)
			return usersBySysAddr[i];
	return 0;
}
LobbyServerUser* LobbyServer::GetUserByID(LobbyDBSpec::DatabaseKey id) const
{
	LobbyServerUser *out;
	if (usersByID.GetElementFromKey(id, out))
		return out;
	return 0;
}
RakNet::RakString LobbyServer::GetHandleByID(LobbyDBSpec::DatabaseKey id) const
{
	LobbyServerUser *out;
	if (usersByID.GetElementFromKey(id, out))
		return out->data->id.handle;
	return "";
}
SystemAddress LobbyServer::GetSystemAddressById(LobbyDBSpec::DatabaseKey id) const
{
	LobbyServerUser *out;
	if (usersByID.GetElementFromKey(id, out))
		return out->systemAddress;
	return UNASSIGNED_SYSTEM_ADDRESS;
}
void LobbyServer::SetOrderingChannel(char oc)
{
	orderingChannel=oc;
}
void LobbyServer::SetSendPriority(PacketPriority pp)
{
	packetPriority=pp;
}
bool LobbyServer::Send( const RakNet::BitStream * bitStream, SystemAddress target )
{
	return rakPeer->Send(bitStream, packetPriority, RELIABLE_ORDERED, orderingChannel, target, false);
}
void LobbyServer::ClearTitles(void)
{
	unsigned i;
	for (i=0; i < titles.Size(); i++)
		titles[i]->Deref();
	
	titles.Clear(true);
}
void LobbyServer::ClearLobbyGames(void)
{
	unsigned i;
	for (i=0; i < lobbyTitles.Size(); i++)
	{
		delete lobbyTitles[i];
	}

	lobbyTitles.Clear(true);
}
void LobbyServer::ClearClansAndClanMembers(void)
{
	unsigned i;
	for (i=0; i < clansById.Size(); i++)
		DeleteClan(clansById[i]);
	clansById.Clear(true);
	clansByHandle.Clear(true);
}
LobbyRoom* LobbyServer::GetLobbyRoomByName(const char *title, TitleValidationDBSpec::DatabaseKey titleId)
{
	LobbyRoomsTableContainer* lobbyGame = GetLobbyRoomsTableContainerById(titleId);
	if (lobbyGame==0)
		return 0;

	if (lobbyGame->roomsTable==0)
		return 0;

	DataStructures::Table resultTable;
	unsigned columnIndexSubset=DefaultTableColumns::TC_LOBBY_ROOM_PTR;
	DataStructures::Table::FilterQuery filter;
	DataStructures::Table::Cell filterCell;
	filterCell.Set(title);
	filter.cellValue=&filterCell;
	filter.columnIndex=DefaultTableColumns::TC_ROOM_NAME;
	filter.operation=DataStructures::Table::QF_EQUAL;

	lobbyGame->roomsTable->QueryTable(&columnIndexSubset, 1, &filter, 1, 0, 0, &resultTable);
	if (resultTable.GetRowCount()>0)
	{
		DataStructures::Table::Row *row = resultTable.GetRowByIndex(0,0);
		LobbyRoom* lobbyRoom = (LobbyRoom*) row->cells[0]->ptr;
		return lobbyRoom;
	}
	return 0;
}
LobbyRoom* LobbyServer::GetLobbyRoomByRowId(LobbyClientRoomId rowId, TitleValidationDBSpec::DatabaseKey titleId)
{
	LobbyRoomsTableContainer* lobbyGame = GetLobbyRoomsTableContainerById(titleId);
	if (lobbyGame==0)
		return 0;

	if (lobbyGame->roomsTable==0)
		return 0;

	DataStructures::Table::Row *row = lobbyGame->roomsTable->GetRowByID(rowId);
	if (row==0)
		return 0;

	LobbyRoom* lobbyRoom = (LobbyRoom*) row->cells[DefaultTableColumns::TC_LOBBY_ROOM_PTR]->ptr;
	return lobbyRoom;
}
LobbyRoomsTableContainer* LobbyServer::GetLobbyRoomsTableContainerById(TitleValidationDBSpec::DatabaseKey gameId)
{
	bool objectExists;
	unsigned index = lobbyTitles.GetIndexFromKey(gameId, &objectExists);
	if (objectExists)
		return lobbyTitles[index];
	return 0;
}
TitleValidationDBSpec::AddTitle_Data* LobbyServer::GetTitleDataByPassword(char *titlePassword, int titlePasswordLength)
{
	unsigned i;
	for (i=0; i < titles.Size(); i++)
	{
		if (titles[i]->titlePasswordLength==titlePasswordLength &&
			memcmp(titles[i]->titlePassword, titlePassword, titlePasswordLength)==0)
			return titles[i];
	}
	return 0;
}
TitleValidationDBSpec::AddTitle_Data* LobbyServer::GetTitleDataById(TitleValidationDBSpec::DatabaseKey titleId)
{
	unsigned i;
	for (i=0; i < titles.Size(); i++)
	{
		if (titles[i]->titleId==titleId)
			return titles[i];
	}
	return 0;
}
TitleValidationDBSpec::AddTitle_Data* LobbyServer::GetTitleDataByTitleName(RakNet::RakString titleName)
{
	unsigned i;
	for (i=0; i < titles.Size(); i++)
	{
		if (titles[i]->titleName==titleName)
			return titles[i];
	}
	return 0;
}
bool LobbyServer::SendIM(LobbyServerUser *sender, LobbyServerUser *recipient, const char *messageBody, unsigned char language)
{
	if (messageBody==0 || messageBody[0]==0)
		return false;

	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_RECEIVE_IM);
	reply.Write(language);
	reply.Write(sender->data->id.databaseRowId);
	stringCompressor->EncodeString(sender->data->id.handle.C_String(), 512, &reply, language);
	stringCompressor->EncodeString(messageBody, 4096, &reply, language);
	Send(&reply, recipient->systemAddress);	
	return true;
}
bool LobbyServer::IsInPotentialFriends(LobbyServer::PotentialFriend p)
{
	unsigned i;
	for (i=0; i < potentialFriends.Size(); i++)
	{
		if (potentialFriends[i].invitee==p.invitee &&
			potentialFriends[i].invitor==p.invitor)
			return true;
	}
	return false;
}
void LobbyServer::AddToPotentialFriends(LobbyServer::PotentialFriend p)
{
	if (IsInPotentialFriends(p))
		return;
	potentialFriends.Insert(p);
}
void LobbyServer::RemoveFromPotentialFriends(LobbyServer::PotentialFriend p)
{
	unsigned i=0;
	while (i < potentialFriends.Size())
	{
		if ((potentialFriends[i].invitee==p.invitee &&
			potentialFriends[i].invitor==p.invitor) ||
			(potentialFriends[i].invitor==p.invitee &&
			potentialFriends[i].invitee==p.invitor)
			)
		{
			potentialFriends.RemoveAtIndex(i);
		}
		else
			i++;
	}
}
void LobbyServer::RemoveFromPotentialFriends(LobbyDBSpec::DatabaseKey user)
{
	unsigned i=0;
	while (i < potentialFriends.Size())
	{
		if (potentialFriends[i].invitee==user || potentialFriends[i].invitor==user)
		{
			potentialFriends.RemoveAtIndex(i);
		}
		else
			i++;
	}
}

void LobbyServer::RemoveFromQuickMatchList(LobbyServerUser *user)
{
	unsigned i;
	for (i=0; i < quickMatchUsers.Size(); i++)
	{
		if (quickMatchUsers[i]==user)
		{
			quickMatchUsers.RemoveAtIndex(i);
			break;
		}
	}
}
unsigned LobbyServer::GetClanIndex(const RakNet::RakString &id) const
{
	bool objectExists;
	unsigned index = clansByHandle.GetIndexFromKey(id,&objectExists);
	if (objectExists)
		return index;
	return (unsigned) -1;
}
unsigned LobbyServer::GetClanIndex(ClanId id) const
{
	bool objectExists;
	unsigned index = clansById.GetIndexFromKey(id,&objectExists);
	if (objectExists)
		return index;
	return (unsigned) -1;
}
void LobbyServer::AddClan(LobbyServerClan *lsc)
{
	clansById.Insert(lsc->clanId,lsc,true);
	clansByHandle.Insert(lsc->handle,lsc,true);
}
void LobbyServer::AddClanMember(LobbyServerClan *lsc, LobbyServerClanMember *lscm)
{
	lscm->clan=lsc;
	lsc->clanMembers.Insert(lscm);
	LobbyServerUser *lsu = GetUserByID(lscm->userId);
	if (lsu)
		lsu->clanMembers.Insert(lscm);
}
void LobbyServer::DeleteClan(ClanId id)
{
	unsigned index = GetClanIndex(id);
	if (index!=-1)
	{
		LobbyServerClan *lsc;
		lsc=clansById[index];
		DeleteClan(lsc);
		clansById.RemoveAtIndex(index);
	}
}
LobbyServerClanMember* LobbyServer::CreateClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId, LobbyDBSpec::UpdateClanMember_Data *ucm)
{
	LobbyServerClanMember*lscm = new LobbyServerClanMember;
	lscm->userId=userId;
	lscm->memberData = new LobbyDBSpec::UpdateClanMember_Data;
	*(lscm->memberData)=*ucm;
	AddClanMember(lsc, lscm);
	return lscm;
}
bool LobbyServer::DeserializeClanId(RakNet::BitStream *bs, ClanId &clanId) const
{
	ClanId c;
	if (bs->Read(c)==false)
		return false;

	if (c!=0)
	{
		clanId=c;
		return true;
	}
	return false;
}
bool LobbyServer::DeserializeClanIdDefaultSelf(LobbyServerUser *lsu, RakNet::BitStream *bs, ClanId &clanId) const
{
	ClanId c;
	if (bs->Read(c)==false)
		return false;

	if (c!=0)
	{
		clanId=c;
		return true;
	}

	unsigned i;
	for (i=0; i < lsu->clanMembers.Size(); i++)
	{
		if (lsu->clanMembers[i]->clanStatus<LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
		{
			clanId = lsu->clanMembers[i]->clan->clanId;
			return true;
		}
	}
	return false;
}
bool LobbyServer::ValidateNotSelf(LobbyServerClanMember *lsm1, LobbyServerClanMember *lsm2, RakNet::BitStream *bs)
{
	if (lsm1==lsm2)
	{
		bs->Write((unsigned char) LC_RC_INVALID_INPUT);
		stringCompressor->EncodeString("Cannot do this to yourself.", 512, bs, 0);
		return false;
	}
	return true;
}
bool LobbyServer::ValidateOneClan(LobbyServerUser *lsu, RakNet::BitStream *bs)
{
	unsigned i,clanCount=0;
	for (i=0; i < lsu->clanMembers.Size(); i++)
	{
		if (lsu->clanMembers[i]->clanStatus < LobbyDBSpec::CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN)
		{
			clanCount++;
			if (clanCount>1)
			{
				bs->Write((unsigned char) LC_RC_INVALID_INPUT);
				stringCompressor->EncodeString("Already in a clan.", 512, bs, 0);
				return false;
			}
		}
	}

	return true;
}
bool LobbyServer::ValidateLeader(LobbyServerClanMember *lsm, RakNet::BitStream *bs)
{
	if (lsm==0 || lsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER)
	{
		bs->Write((unsigned char) LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Must be clan leader.", 512, bs, 0);
		return false;
	}
	return true;
}
bool LobbyServer::ValidateLeaderOrSubleader(LobbyServerClanMember *lsm, RakNet::BitStream *bs)
{
	if (lsm==0 || (lsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_LEADER && lsm->clanStatus!=LobbyDBSpec::CLAN_MEMBER_STATUS_SUBLEADER))
	{
		bs->Write((unsigned char) LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Must be clan leader or subleader.", 512, bs, 0);
		return false;
	}
	return true;
}
bool LobbyServer::ValidateNotBlacklisted(LobbyServerClanMember *lsm, RakNet::BitStream *bs)
{
	if (lsm && lsm->clanStatus==LobbyDBSpec::CLAN_MEMBER_STATUS_BLACKLISTED)
	{
		bs->Write((unsigned char) LC_RC_PERMISSION_DENIED);
		stringCompressor->EncodeString("Blacklisted.", 512, bs, 0);
		return false;
	}
	return true;
}
bool LobbyServer::ValidateNotBusy(LobbyServerClanMember *lsm, RakNet::RakString &memberName, RakNet::RakString &clanName, RakNet::BitStream *bs)
{
	if (lsm==0)
		return true;

	switch (lsm->busyStatus)
	{
	case LobbyServerClanMember::LSCM_NOT_BUSY:
		return true;
	case LobbyServerClanMember::LSCM_BUSY_JOIN_INVITATION:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Processing a join invitation for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_WITHDRAW_JOIN_INVITATION:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Processing a withdraw join invitation for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_ACCEPTING_JOIN_INVITATION:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Accepting a join invitation for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_REJECTING_JOIN_INVITATION:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Rejecting a join invitation for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_WITHDRAW_JOIN_REQUEST:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Withdrawing a join request for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_ACCEPTING_JOIN_REQUEST:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Accepting a join request for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_REJECTING_JOIN_REQUEST:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("Rejecting a join request for %s to clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_JOIN_REQUEST:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("%s is already requesting to join clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	case LobbyServerClanMember::LSCM_BUSY_BLACKLISTING:
		bs->Write(LC_RC_BUSY);
		stringCompressor->EncodeString(RakString("%s is being kicked or blacklisted for clan %s.",memberName.C_String(), clanName.C_String()), 512, bs, 0);
		return false;
	}
	return false;
}
SystemAddress LobbyServer::GetLeaderSystemAddress(ClanId clanId) const
{
	SystemAddress systemAddress = UNASSIGNED_SYSTEM_ADDRESS;
	LobbyServerClan *lobbyServerClan = GetClanById(clanId);
	if (lobbyServerClan)
	{
		LobbyServerClanMember *leader = lobbyServerClan->GetLeader();
		if (leader)
		{
			LobbyServerUser *user = GetUserByID(leader->userId);
			if (user)
			{
				systemAddress=user->systemAddress;
			}
		}
	}
	return systemAddress;
}
void LobbyServer::SendEmail(LobbyDBSpec::DatabaseKey dbKey, unsigned char language, LobbyDBSpec::SendEmail_Data *input, SystemAddress systemAddress )
{
	RakNet::BitStream reply;
	reply.Write((unsigned char)ID_LOBBY_GENERAL);
	reply.Write((unsigned char)LOBBY_MSGID_SEND_EMAIL);
	if (input->to.Size()==0 || input->to.Size()>MAX_ALLOWED_EMAIL_RECIPIENTS)
	{
		// No recipients!
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,systemAddress);
		delete input;
		return;
	}
	else if (input->subject.IsEmpty() && input->body.IsEmpty())
	{
		// Empty email
		reply.Write((unsigned char)LC_RC_INVALID_INPUT);
		Send(&reply,systemAddress);
		delete input;
		return;
	}

	input->id.hasDatabaseRowId=true;
	input->id.databaseRowId=dbKey;

	SendEmail_DBQuery(input);
}
void LobbyServer::DeleteClan(LobbyServerClan *lsc)
{
	for (unsigned clanMemberIndex=0; clanMemberIndex < lsc->clanMembers.Size(); clanMemberIndex++)
	{
		LobbyServerUser *user = GetUserByID(lsc->clanMembers[clanMemberIndex]->userId);
		if (user)
		{
			for (unsigned j=0; j < user->clanMembers.Size(); j++)
			{
				if (user->clanMembers[j]==lsc->clanMembers[clanMemberIndex])
				{
					user->clanMembers.RemoveAtIndexFast(j);
					break;
				}
			}
		}
	}

	clansByHandle.RemoveAtIndex(GetClanIndex(lsc->handle));
	clansById.RemoveAtIndex(GetClanIndex(lsc->clanId));
	delete lsc;
}
void LobbyServer::DeleteClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId)
{
	unsigned clanMemberIndex = lsc->GetClanMemberIndex(userId);
	if (clanMemberIndex!=-1)
	{
		LobbyServerClanMember *lsm=lsc->clanMembers[clanMemberIndex];
		LobbyServerUser *user = GetUserByID(lsm->userId);
		if (user)
		{
			for (unsigned j=0; j < user->clanMembers.Size(); j++)
			{
				if (user->clanMembers[j]==lsm)
				{
					user->clanMembers.RemoveAtIndexFast(j);
					break;
				}
			}
		}

		lsc->DeleteClanMember(clanMemberIndex);
		delete lsm;
		
		if (lsc->clanMembers.Size()==0)
		{
			clansByHandle.RemoveAtIndex(GetClanIndex(lsc->handle));
			delete lsc;
		}
	}
}
bool LobbyServer::HasClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const
{
	return lsc->GetClanMemberIndex(userId)!=(unsigned)-1;
}
LobbyServerClanMember *LobbyServer::GetOptionalClanMember(LobbyServerClan *lsc, LobbyServerUser *lsu) const
{
	if (lsu==0)
		return 0;
	return GetClanMember(lsc, lsu->GetID());
}
void LobbyServer::SetOptionalBusyStatus(LobbyServerClanMember *targetLsm, int status )
{
	if (targetLsm)
		targetLsm->busyStatus=(LobbyServerClanMember::BusyState) status;
}
unsigned LobbyServer::GetClanMemberIndex(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const
{
	return lsc->GetClanMemberIndex(userId);
}
LobbyServerClanMember *LobbyServer::GetClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const
{
	unsigned index = lsc->GetClanMemberIndex(userId);
	if (index==(unsigned)-1)
		return 0;
	return lsc->clanMembers[index];
}
LobbyServerClan *LobbyServer::GetClanById(ClanId id) const
{
	unsigned index = GetClanIndex(id);
	if (index!=(unsigned)-1)
		return clansById[index];
	return 0;
}
LobbyServerClan *LobbyServer::GetClanByHandle(RakNet::RakString handle) const
{
	bool objectExists;
	unsigned index = clansByHandle.GetIndexFromKey(handle, &objectExists);
	if (objectExists)
		return clansByHandle[index];
	return 0;	
}
void LobbyServer::SerializeClientSafeCreateUserData(LobbyServerUser *user, RakNet::BitStream *bs)
{
	stringCompressor->EncodeString(user->data->id.handle.C_String(),512,bs,0);
	bs->Write(user->GetID());
	user->data->output.SerializeBinaryData(bs);
	user->data->output.SerializeCaptions(bs);
	user->data->output.SerializeAdminLevel(bs);
	bs->Write(user->systemAddress);
	bs->Write(user->readyToPlay);
}

void LobbyServer::SendToRoomNotIgnoredNotSelf(LobbyServerUser* lsu, RakNet::BitStream *reply)
{
	LobbyRoom *room = lsu->currentRoom;
	unsigned i;
	LobbyServerUser* user;

	for (i=0; i < room->publicSlotMembers.Size(); i++)
	{
		if (lsu->GetID()==room->publicSlotMembers[i])
			continue;
		user=GetUserByID(room->publicSlotMembers[i]);
		if (user->IsIgnoredUser(lsu->GetID())==false)
			Send(reply, user->systemAddress);
	}
	for (i=0; i < room->privateSlotMembers.Size(); i++)
	{
		if (lsu->GetID()==room->privateSlotMembers[i])
			continue;
		user=GetUserByID(room->privateSlotMembers[i]);
		if (user->IsIgnoredUser(lsu->GetID())==false)
			Send(reply, user->systemAddress);
	}

	for (i=0; i < room->spectators.Size(); i++)
	{
		if (lsu->GetID()==room->spectators[i])
			continue;
		user=GetUserByID(room->spectators[i]);
		if (user->IsIgnoredUser(lsu->GetID())==false)
			Send(reply, user->systemAddress);
	}
}

LobbyServerUser * LobbyServer::DeserializeHandleOrId(char *userHandle, LobbyClientUserId &userId, RakNet::BitStream *bs)
{
	LobbyServerUser *target;
	bool hasUserId;
	stringCompressor->DecodeString(userHandle,512,bs,0);
	bs->Read(hasUserId);
	if (hasUserId)
	{
		bs->Read(userId);
		target = GetUserByID(userId);
	}
	else
	{
		userId=0;
		if (userHandle[0]==0)
			return 0;

		target = GetUserByHandle(userHandle);
	}
	return target;
}
LobbyServerClan * LobbyServer::GetClanByHandleOrId(RakNet::RakString clanHandle, ClanId clanId)
{
	if (clanId)
		return GetClanById(clanId);
	return GetClanByHandle(clanHandle);
}
LobbyServerClan * LobbyServer::DeserializeClanHandleOrId(char *clanHandle, ClanId &clanId, RakNet::BitStream *bs)
{
	LobbyServerClan *target;
	bool hasUserId;
	stringCompressor->DecodeString(clanHandle,512,bs,0);
	bs->Read(hasUserId);
	if (hasUserId)
	{
		bs->Read(clanId);
		target = GetClanById(clanId);
	}
	else
	{
		clanId=0;
		if (clanHandle[0]==0)
			return 0;

		target = GetClanByHandle(clanHandle);
	}
	return target;
}
LobbyServerClan *LobbyServer::ValidateAndDeserializeClan(char *clanHandle, ClanId &clanId, RakNet::BitStream *inputBs, RakNet::BitStream *outputBs )
{
	LobbyServerClan *clan = LobbyServer::DeserializeClanHandleOrId(clanHandle, clanId, inputBs);
	if (clan==0)
	{
		outputBs->Write((unsigned char) LC_RC_INVALID_CLAN);
		if (clanId!=0)
			stringCompressor->EncodeString(RakNet::RakString("Unknown clan %s", clanHandle), 512, outputBs, 0);
		else
			stringCompressor->EncodeString(RakNet::RakString("Unknown clan with index %i", clanId), 512, outputBs, 0);
	}

	return clan;
}
bool LobbyServer::IsTrustedIp(SystemAddress systemAddress, TitleValidationDBSpec::DatabaseKey titleId )
{
	unsigned i;
	for (i=0; i < trustedIpList.Size(); i++)
	{
		if (trustedIpList[i]->systemAddress.binaryAddress==systemAddress.binaryAddress &&
			trustedIpList[i]->gameDbId.primaryKey==titleId)
			return true;
	}
	return false;
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif
