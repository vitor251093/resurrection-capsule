/// \file
/// \brief Interface to the lobby client. Not platform specific
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __LOBBY_CLIENT_INTERFACE_H
#define __LOBBY_CLIENT_INTERFACE_H

#ifdef _CONSOLE_1
#elif defined (_PS3)
#include "LobbyPS3Types.h"
#else
#include "LobbyPCTypes.h"
#endif

#include "LobbyTypes.h"
#include "RankingServerDBSpec.h"
#include "LobbyDBSpec.h"
#include "RankingServerDBSpec.h"
#include "DS_Table.h"
#include "DS_List.h"
#include "RakNetTypes.h"

namespace RakNet
{

struct FriendInvitation_Notification;
struct RoomMemberDrop_Notification;
struct RoomMemberJoin_Notification;
struct RoomMemberReadyStateChange_Notification;
struct RoomSetroomIsHidden_Notification;
struct RoomNewModerator_Notification;
struct RoomSetAllowSpectators_Notification;
struct RoomChangeNumSlots_Notification;
struct KickedOutOfRoom_Notification;
struct QuickMatchTimeout_Notification;
struct StartGame_Notification;
struct ClanDissolved_Notification;
struct LeaveClan_Notification;
struct ClanJoinInvitationWithdrawn_Notification;
struct ClanJoinInvitationRejected_Notification;
struct ClanJoinInvitationAccepted_Notification;
struct ClanJoinInvitation_Notification;
struct ClanMemberJoinRequestWithdrawn_Notification;
struct ClanMemberJoinRequestRejected_Notification;
struct ClanMemberJoinRequestAccepted_Notification;
struct ClanMemberJoinRequest_Notification;
struct ClanMemberRank_Notification;
struct ClanMemberKicked_Notification;
struct ClanMemberUnblacklisted_Notification;
struct LobbyClientInterfaceCB;
struct LobbyUserClientDetails;
class RAK_DLL_EXPORT LC_Friend;
class RAK_DLL_EXPORT LC_Ignored;

/// \brief The client-side interface to the lobby system
/// The Lobby client implements functionality commonly found in game lobbies.
/// It is the counterpart to the LobbyServer class.
/// This interface is system independent, but for PC functionality see LobbyClientPC
/// For full documentation on the PC version, see LobbyClientPC.h
class RAK_DLL_EXPORT LobbyClientInterface
{
public:
	// ------------ NETWORKING --------------
	virtual void Login(const char *userHandle, const char *userPassword, SystemAddress lobbyServerAddr)=0;
	virtual void SetTitleLoginId(const char *titleIdStr, const char *titleLoginPW, int titleLoginPWLength)=0;
	virtual void Logoff(void)=0;

	// ------------ YOURSELF --------------
	virtual void RegisterAccount(LobbyDBSpec::CreateUser_Data *input, SystemAddress lobbyServerAddr)=0;
	virtual void UpdateAccount(LobbyDBSpec::UpdateUser_Data *input)=0;
	virtual void ChangeHandle(const char *newHandle)=0;
	virtual LobbyUserStatus GetLobbyUserStatus(void) const=0;
	virtual void RetrievePasswordRecoveryQuestion(const char *userHandle, SystemAddress lobbyServerAddr)=0;
	virtual void RetrievePassword(const char *userHandle, const char *passwordRecoveryAnswer, SystemAddress lobbyServerAddr)=0;

	// ------------ FRIENDS --------------
	virtual void DownloadFriends(bool ascendingSortByDate)=0;
	virtual void SendAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)=0;
	virtual void AcceptAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)=0;
	virtual void DeclineAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language)=0;
	virtual const DataStructures::List<LC_Friend *>& GetFriendsList(void) const=0;
	virtual bool IsFriend(const char *friendHandle)=0;
	virtual bool IsFriend(LobbyClientUserId *friendId)=0;

	// ------------ IGNORE LIST --------------
	virtual void DownloadIgnoreList(bool ascendingSort)=0;
	virtual void AddToOrUpdateIgnoreList(const char *userHandle, LobbyClientUserId *userId, const char *actionsStr)=0;
	virtual void RemoveFromIgnoreList(LobbyClientUserId *userId)=0;
	virtual const DataStructures::List<LC_Ignored *>& GetIgnoreList(void) const=0;
	
	// ------------ EMAILS --------------
	virtual void DownloadEmails(bool ascendingSort, bool inbox, unsigned char language)=0;
	virtual void SendEmail(LobbyDBSpec::SendEmail_Data *input, unsigned char language)=0;
	virtual void DeleteEmail(EmailId *emailId)=0;
	virtual void UpdateEmailStatus(EmailId *emailId, int newStatus, bool wasOpened)=0;
	virtual const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & GetInboxEmailList( void ) const=0;
	virtual const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & GetSentEmailList( void ) const=0;

	// ------------ INSTANT MESSAGING --------------
	virtual void SendIM(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength)=0;

	// ------------ ROOMS --------------
	// ***  Anyone anytime  ***
	virtual bool IsInRoom(void) const=0;

	// ***  Anyone Outside of a room  ***
	virtual void CreateRoom(const char *roomName, const char *roomPassword, bool roomIsHidden, int publicSlots, int privateSlots, bool allowSpectators,
		DataStructures::Table *customProperties)=0;
	// Use indices for inclusionFilters if possible, See LobbyTypes.h DefaultTableColumns::columnId for predefined indices.See LobbyTypes.h
	virtual void DownloadRooms(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters)=0;
	virtual DataStructures::Table * GetAllRooms(void)=0;
	virtual bool IsDownloadingRoomList(void)=0;
	virtual void LeaveRoom(void)=0;

	// ***  Anyone inside of a room  ***
	virtual const DataStructures::List<LobbyClientUserId *>& GetAllRoomMembers(void) const=0;
	virtual const DataStructures::List<LobbyClientUserId *>& GetOpenSlotMembers(void) const=0;
	virtual const DataStructures::List<LobbyClientUserId *>& GetReservedSlotMembers(void) const=0;
	virtual const DataStructures::List<LobbyClientUserId *>& GetSpectators(void) const=0;
	virtual LobbyClientUserId* GetRoomModerator(void)=0;
	virtual LobbyClientRoomId* GetRoomId(void)=0;
	virtual unsigned int GetPublicSlotMaxCount(void) const=0;
	virtual unsigned int GetPrivateSlotMaxCount(void) const=0;
	virtual LobbyClientTitleId* GetTitleId(void) const=0;
	virtual const char *GetRoomName(void) const=0;
	virtual const char *GetTitleName(void) const=0;
	virtual bool RoomIsHidden(void) const=0;
	virtual DataStructures::Table::Row *GetRoomTableRow(void) const=0;

	virtual void JoinRoom(LobbyClientRoomId* roomId, const char *roomName, const char *roomPassword, bool joinAsSpectator)=0;
	virtual void JoinRoomByFilter(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters, bool joinAsSpectator)=0;
	virtual void RoomChat(const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength)=0;
	virtual bool IsInReservedSlot(LobbyClientUserId *userId)=0;
	virtual bool IsRoomModerator(LobbyClientUserId *userId)=0;
	virtual bool IsUserInRoom(LobbyClientUserId *userId) const=0;
	virtual bool RoomAllowsSpectators(void) const=0;
	// inviteToPrivateSlot only used if moderator.
	// Room must have open slots of the type specified.
	virtual void InviteToRoom(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId,
		const char *chatBinaryData, int chatBinaryLength, bool inviteToPrivateSlot)=0;
	virtual void SetReadyToPlayStatus(bool isReady)=0;

	// ***  Moderators inside of a room  ***
	virtual void KickRoomMember(LobbyClientUserId *userId)=0;
	virtual void SetRoomIsHidden(bool roomIsHidden)=0;
	virtual void SetRoomAllowSpectators(bool spectatorsOn)=0;
	virtual void ChangeNumSlots(int publicSlots, int reservedSlots)=0;
	virtual void GrantModerator(LobbyClientUserId* userId)=0;
	virtual void StartGame(void)=0;

	// ---------- SCORE TRACKING -------------
	virtual void SubmitMatch(RankingServerDBSpec::SubmitMatch_Data *input)=0;
	virtual void DownloadRating(RankingServerDBSpec::GetRatingForParticipant_Data *input)=0;
	virtual RankingServerDBSpec::GetRatingForParticipant_Data * GetRating(void)=0;

	// ------------ QUICK MATCH --------------
	virtual bool IsWaitingOnQuickMatch(void)=0;
	virtual void QuickMatch(int requiredPlayers, int timeoutInSeconds)=0;
	virtual void CancelQuickMatch(void)=0;
	
	// ------------ ACTION HISTORY --------------
	virtual void DownloadActionHistory(bool ascendingSort)=0;
	virtual void AddToActionHistory(LobbyDBSpec::AddToActionHistory_Data *input)=0;
	virtual const DataStructures::List<LobbyDBSpec::AddToActionHistory_Data *> & GetActionHistoryList(void) const=0;

	// ------------ CLANS --------------
	virtual void CreateClan(LobbyDBSpec::CreateClan_Data *clanData, bool failIfAlreadyInClan)=0;
	virtual void ChangeClanHandle(ClanId *clanId, const char *newHandle)=0;
	virtual void LeaveClan(ClanId *clanId, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved)=0;
	virtual void DownloadClans(const char *userHandle, LobbyClientUserId* userId, bool ascendingSort)=0;
	virtual void SendClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)=0;
	virtual void WithdrawClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)=0;
	virtual void AcceptClanJoinInvitation(LobbyDBSpec::UpdateClanMember_Data *clanMemberData, bool failIfAlreadyInClan)=0;
	virtual void RejectClanJoinInvitation(const char *clanHandle, ClanId *clanId)=0;
	virtual void DownloadByClanMemberStatus(LobbyDBSpec::ClanMemberStatus status)=0;
	virtual void SendClanMemberJoinRequest(LobbyDBSpec::UpdateClanMember_Data *clanMemberData)=0;
	virtual void WithdrawClanMemberJoinRequest(const char *clanHandle, ClanId *clanId)=0;
	virtual void AcceptClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool failIfAlreadyInClan)=0;
	virtual void RejectClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)=0;
	virtual void SetClanMemberRank(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, LobbyDBSpec::ClanMemberStatus newRank )=0;
	virtual void ClanKickAndBlacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool kick, bool blacklist)=0;
	virtual void ClanUnblacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)=0;
	virtual void AddPostToClanBoard(LobbyDBSpec::AddToClanBoard_Data *postData, bool failIfNotLeader, bool failIfNotMember, bool failIfBlacklisted)=0;
	virtual void RemovePostFromClanBoard(ClanId *clanId, ClanBoardPostId *postId, bool allowMemberToRemoveOwnPosts )=0;
	virtual void DownloadMyClanBoards(bool ascendingSort)=0;
	virtual void DownloadClanBoard(const char *clanHandle, ClanId *clanId, bool ascendingSort)=0;
	virtual void DownloadClanMember(const char *clanHandle, ClanId *clanId, const char *userHandle, LobbyClientUserId* userId)=0;

	// ------------ USER LOGIN KEYS --------------
	virtual void ValidateUserKey(const char *titleIdStr, const char *keyPassword, int keyPasswordLength)=0;
	
	// ------------ MET USERS --------------
	virtual const DataStructures::List<LobbyUserClientDetails *>& GetRecentlyMetUsersList(void) const=0;

	// ------------ HELPERS --------------
	virtual bool GetUserDetails(LobbyClientUserId *userId, LobbyUserClientDetails *details)=0;
	virtual LobbyClientUserId *GerUserIdFromName(const char *name)=0;
	virtual LobbyClientUserId *GetMyId(void)=0;
	virtual bool EqualIDs(const LobbyClientUserId* userId1, const LobbyClientUserId* userId2)=0;
	virtual void SetCallbackInterface(LobbyClientInterfaceCB *cbi)=0;

	// ------------ PER TICK UPDATE --------------
	// Not necessary on PC
	virtual void Update(void) {}
};


struct LobbyClientCBResult
{
	LobbyClientCBResult() {additionalInfo=0; epochTime=0;}
	LobbyClientCBResult(LobbyClientCBResultCode rc, char *add=0, long long et=0 ) {resultCode=rc; additionalInfo=add; epochTime=et;};
	void DebugPrintf(void);
	void DebugSprintf(char *output);
	LobbyClientCBResultCode resultCode;
	char *additionalInfo; // Certain error codes have additional text info
	long long epochTime; // Certain error codes contain time info
};

enum FriendStatus
{
	FRIEND_STATUS_NEW_FRIEND,
	FRIEND_STATUS_NO_LONGER_FRIENDS,
	FRIEND_STATUS_FRIEND_ONLINE,
	FRIEND_STATUS_FRIEND_OFFLINE,
};

struct LobbyClientInterfaceCB
{
	/// --- FUNCTION CALL RESULTS ---
	// Use ErrorCodeDescription::ToEnglish(result) to get a meaningful description of each error code.
	virtual void Login_Result(LobbyClientCBResult result){};
	virtual void SetTitleLoginId_Result(LobbyClientCBResult result){};
	virtual void Logoff_Result(LobbyClientCBResult result){};
	virtual void RegisterAccount_Result(LobbyClientCBResult result){};
	virtual void UpdateAccount_Result(LobbyClientCBResult result){};
	virtual void ChangeHandle_Result(LobbyClientCBResult result){};
	virtual void RetrievePasswordRecoveryQuestion_Result(LobbyClientCBResult result){};
	virtual void RetrievePassword_Result(LobbyClientCBResult result){};
	virtual void DownloadFriends_Result(LobbyClientCBResult result){};
	virtual void SendAddFriendInvite_Result(LobbyClientCBResult result, const char *friendHandle, LobbyClientUserId *friendId){};
	virtual void AcceptAddFriendInvite_Result(LobbyClientCBResult result, const char *friendHandle, LobbyClientUserId *friendId){};
	virtual void DeclineAddFriendInvite_Result(LobbyClientCBResult result, const char *friendHandle, LobbyClientUserId *friendId){};
	virtual void DownloadIgnoreList_Result(LobbyClientCBResult result){};
	virtual void AddToOrUpdateIgnoreList_Result(LobbyClientCBResult result){};
	virtual void RemoveFromIgnoreList_Result(LobbyClientCBResult result){};
	virtual void DownloadEmails_Result(LobbyClientCBResult result){};
	virtual void SendEmail_Result(LobbyClientCBResult result){};
	virtual void DeleteEmail_Result(LobbyClientCBResult result){};
	virtual void UpdateEmailStatus_Result(LobbyClientCBResult result){};
	virtual void SendIM_Result(LobbyClientCBResult result, const char *userHandle, LobbyClientUserId *userId){};
	virtual void CreateRoom_Result(LobbyClientCBResult result){};
	virtual void DownloadRooms_Result(LobbyClientCBResult result){};
	virtual void LeaveRoom_Result(LobbyClientCBResult result){};
	virtual void JoinRoom_Result(LobbyClientCBResult result){};
	virtual void JoinRoomByFilter_Result(LobbyClientCBResult result){};
	virtual void RoomChat_Result(LobbyClientCBResult result){};
	virtual void InviteToRoom_Result(LobbyClientCBResult result){};
	virtual void SetReadyToPlayStatus_Result(LobbyClientCBResult result){};
	virtual void KickRoomMember_Result(LobbyClientCBResult result){};
	virtual void SetRoomIsHidden_Result(LobbyClientCBResult result){};
	virtual void SetRoomAllowSpectators_Result(LobbyClientCBResult result){};
	virtual void ChangeNumSlots_Result(LobbyClientCBResult result){};
	virtual void GrantModerator_Result(LobbyClientCBResult result){};
	virtual void StartGame_Result(LobbyClientCBResult result){};
	virtual void SubmitMatch_Result(LobbyClientCBResult result){};
	virtual void DownloadRating_Result(LobbyClientCBResult result){};
	virtual void QuickMatch_Result(LobbyClientCBResult result){};
	virtual void CancelQuickMatch_Result(LobbyClientCBResult result){};
	virtual void DownloadActionHistory_Result(LobbyClientCBResult result){};
	virtual void AddToActionHistory_Result(LobbyClientCBResult result){};
	virtual void CreateClan_Result(LobbyClientCBResult result, LobbyDBSpec::CreateClan_Data *callResult){};
	virtual void ChangeClanHandle_Result(LobbyClientCBResult result, LobbyDBSpec::ChangeClanHandle_Data *callResult){};
	virtual void LeaveClan_Result(LobbyClientCBResult result, RakNet::RakString clanHandle, ClanId *clanId){};
	virtual void DownloadClans_Result(LobbyClientCBResult result, LobbyDBSpec::GetClans_Data *callResult){};
	virtual void SendClanJoinInvitation_Result(LobbyClientCBResult result, LobbyDBSpec::UpdateClanMember_Data *callResult){};
	virtual void WithdrawClanJoinInvitation_Result(LobbyClientCBResult result, RakNet::RakString clanHandle, ClanId *clanId, RakNet::RakString userHandle, LobbyClientUserId *userId){};
	virtual void AcceptClanJoinInvitation_Result(LobbyClientCBResult result, LobbyDBSpec::UpdateClanMember_Data *callResult){};
	virtual void RejectClanJoinInvitation_Result(LobbyClientCBResult result, const char *clanHandle, ClanId *clanId){};
	virtual void DownloadByClanMemberStatus_Result(LobbyClientCBResult result, LobbyDBSpec::ClanMemberStatus status, DataStructures::List<LobbyDBSpec::RowIdOrHandle> &clans){};
	virtual void SendClanMemberJoinRequest_Result(LobbyClientCBResult result, LobbyDBSpec::UpdateClanMember_Data *callResult){};
	virtual void WithdrawClanMemberJoinRequest_Result(LobbyClientCBResult result, const char *clanHandle, ClanId *clanId){};
	virtual void AcceptClanMemberJoinRequest_Result(LobbyClientCBResult result, LobbyDBSpec::UpdateClanMember_Data *callResult){};
	virtual void RejectClanMemberJoinRequest_Result(LobbyClientCBResult result, RakNet::RakString clanHandle, ClanId *clanId, RakNet::RakString userHandle, LobbyClientUserId *userId){};
	virtual void SetClanMemberRank_Result(LobbyClientCBResult result){};
	virtual void ClanKickAndBlacklistUser_Result(LobbyClientCBResult result){};
	virtual void ClanUnblacklistUser_Result(LobbyClientCBResult result){};
	virtual void AddPostToClanBoard_Result(LobbyClientCBResult result){};
	virtual void RemovePostFromClanBoard_Result(LobbyClientCBResult result){};
	virtual void DownloadMyClanBoards_Result(LobbyClientCBResult result){};
	virtual void DownloadClanBoard_Result(LobbyClientCBResult result){};
	virtual void DownloadClanMember_Result(LobbyClientCBResult result){};
	virtual void ValidateUserKey_Result(LobbyClientCBResult result){};


	/// --- EVENT_NOTIFICATIONS ---
	virtual void ReceiveIM_Notify(const char *senderHandle, const char *body, LobbyClientUserId* senderId){};
	virtual void FriendStatus_Notify(const char *friendName, LobbyClientUserId *friendId, FriendStatus status){};
	virtual void IncomingEmail_Notify(LobbyClientUserId senderId, const char *senderHandle, const char *subject){};
	virtual void FriendInvitation_Notify(FriendInvitation_Notification *notification){};
	virtual void RoomMemberDrop_Notify(RoomMemberDrop_Notification *notification){};
	virtual void RoomMemberJoin_Notify(RoomMemberJoin_Notification *notification){};
	virtual void RoomMemberReadyStateChange_Notify(RoomMemberReadyStateChange_Notification *notification){};
	virtual void RoomNewModerator_Notify(RoomNewModerator_Notification *notification){};
	virtual void RoomSetIsHidden_Notify(RoomSetroomIsHidden_Notification *notification){};
	virtual void RoomSetAllowSpectators_Notify(RoomSetAllowSpectators_Notification *notification){};
	virtual void RoomChangeNumSlots_Notify(RoomChangeNumSlots_Notification *notification){};
	virtual void ChatMessage_Notify(LobbyClientUserId *senderId, const char *senderHandle, const char *message, void *binaryData, unsigned int binaryDataLength){};
	virtual void RoomInvite_Notify(LobbyClientUserId *senderId, const char *senderHandle, const char *message, void *binaryData, unsigned int binaryDataLength, LobbyClientRoomId *roomId, const char *roomName, bool privateSlotInvitation){};
	virtual void RoomDestroyed_Notify(void){};
	virtual void KickedOutOfRoom_Notify(KickedOutOfRoom_Notification *notification){};
	virtual void QuickMatchTimeout_Notify(QuickMatchTimeout_Notification *notification){};
	virtual void StartGame_Notify(StartGame_Notification *notification){};
	virtual void ClanDissolved_Notify(ClanDissolved_Notification *notification){};
	virtual void LeaveClan_Notify(LeaveClan_Notification *notification){};
	virtual void ClanJoinInvitationWithdrawn_Notify(ClanJoinInvitationWithdrawn_Notification *notification){};
	virtual void ClanJoinInvitationRejected_Notify(ClanJoinInvitationRejected_Notification *notification){};
	virtual void ClanJoinInvitationAccepted_Notify(ClanJoinInvitationAccepted_Notification *notification){};
	virtual void ClanJoinInvitation_Notify(ClanJoinInvitation_Notification *notification){};
	virtual void ClanMemberJoinRequestWithdrawn_Notify(ClanMemberJoinRequestWithdrawn_Notification *notification){};
	virtual void ClanMemberJoinRequestRejected_Notify(ClanMemberJoinRequestRejected_Notification *notification){};
	virtual void ClanMemberJoinRequestAccepted_Notify(ClanMemberJoinRequestAccepted_Notification *notification){};
	virtual void ClanMemberJoinRequest_Notify(ClanMemberJoinRequest_Notification *notification){};
	virtual void ClanMemberRank_Notify(ClanMemberRank_Notification *notification){};
	virtual void ClanMemberKicked_Notify(ClanMemberKicked_Notification *notification){};
	virtual void ClanMemberUnblacklisted_Notify(ClanMemberUnblacklisted_Notification *notification){};

	// --- TEMP ---
	// Fix later
//	virtual void ScoreSubmitted(unsigned int newRank) {}
//	virtual void GotScore(void *memberId, char *onlineName, unsigned int rank, unsigned int highestRank, unsigned long long score ) {}
//	virtual void FriendRemovedFromFriendsList(void *friendId, const char *friendName) {}
};


struct LobbyUserClientDetails
{
	// Note to self: Remove alloca from LobbyClientPC::NotifyQuickMatchSuccess_NetMsg if I add types with constructors / destructors

	LobbyClientUserId* userId;

	SystemAddress systemAddress;

	bool readyToPlay;

	// Currently only handle, binary data, captions, and admin level are filled in. Other fields will have invalid or NULL values
	// Pointers are deleted when the user leaves the room, or you leave the room. Copy out the data if you want to store it past that time.
	// Better yet, recall the function when you need it, data will remain stable until next receive call.
	LobbyDBSpec::CreateUser_Data *userData;
};

struct FriendInvitation_Notification
{
	unsigned char messageBodyEncodingLanguage;
	char messageBody[4096];
	char invitorHandle[512];
	char inviteeHandle[512];
	LobbyClientUserId *invitor;
	LobbyClientUserId *invitee;

	// If request is true, this is a query to be a friend.
	// If request is false, this is a response (accepted==true or accepted==false)
	bool request;
	bool accepted;
};

struct RoomMemberDrop_Notification
{
	LobbyClientUserId *droppedMember;
	bool wasKicked;
	bool gotNewModerator;
	LobbyClientUserId *newModerator;
	SystemAddress memberAddress;
};

struct RoomMemberJoin_Notification
{
	LobbyClientUserId *joinedMember;
	bool isSpectator;
	bool isInOpenSlot;
	bool isInReservedSlot;
	SystemAddress memberAddress;
};

struct RoomMemberReadyStateChange_Notification
{
	LobbyClientUserId *member;
	bool isReady;
};

struct RoomSetroomIsHidden_Notification
{
	bool isroomIsHidden;
};

struct RoomNewModerator_Notification
{
	LobbyClientUserId *member;
};

struct RoomSetAllowSpectators_Notification
{
	bool allowSpectators;
};

struct RoomChangeNumSlots_Notification
{
	unsigned int newPublicSlots;
	unsigned int newPrivateSlots;
};

struct KickedOutOfRoom_Notification
{
	LobbyClientUserId *member;
	char memberHandle[512];
	bool roomWasDestroyed;
};

struct QuickMatchTimeout_Notification
{
	// No data right now
};

struct StartGame_Notification
{
	bool wasQuickMatch;
	SystemAddress moderator;
	SystemAddress yourAddress;
	DataStructures::List<LobbyUserClientDetails*> matchParticipants;
	DataStructures::List<LobbyUserClientDetails*> matchSpectators;
	LobbyClientTitleId *titleId;
};

struct ClanDissolved_Notification
{
	ClanId clanId;
	RakNet::RakString clanHandle;
	DataStructures::List<LobbyClientUserId *> memberIds;
};

/// Someone has left this clan
struct LeaveClan_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// You were invited to a clan, but this invitation has been revoked
struct ClanJoinInvitationWithdrawn_Notification
{
	/// Leader or subleader of the clan that performed this action
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// A clan leader invited someone to your clan, and they turned the offer down.
/// Send to all clan leaders and subleaders
struct ClanJoinInvitationRejected_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that originally sent the invitation (not necessarily you)
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// A clan leader invited someone to your clan, and they accepted it and have joined the clan
/// This is sent to the new member, and also to all online members of the clan
struct ClanJoinInvitationAccepted_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that originally sent the invitation (not necessarily you)
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;

	/// If this is sent to you as the joining member, this is true
	bool userIdIsYou;

	/// If this is sent to you as the one that originally accepted the request, this is true
	bool invitationSenderIsYou;
};

/// A leader or subleader in a clan has invited you to join that clan
struct ClanJoinInvitation_Notification
{
	/// Leader or subleader that is sending the invitation
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// Someone has petitioned to join your clan, but changed their mind and have withdrawn that petition
struct ClanMemberJoinRequestWithdrawn_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// A clan leader or subleader has rejected your petition to join their clan
/// This is sent to the petitioner, and also to all online members of the clan
struct ClanMemberJoinRequestRejected_Notification
{
	/// Leader or subleader that turned you down
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

/// A clan leader or subleader has accepted a petition to join their clan
/// This is sent to the new member, and also to all online members of the clan
struct ClanMemberJoinRequestAccepted_Notification
{
	/// New member of the clan (possibly you)
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that accepted the join request (possibly you)
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;

	/// If this is sent to you as the joining member, this is true
	bool userIdIsYou;

	/// If this is sent to you as the one that originally accepted the request, this is true
	bool joinRequestAcceptorIsYou;
};

/// Someone has asked to join your clan
struct ClanMemberJoinRequest_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Clan this action is in reference to
	ClanId *clanId;
	RakNet::RakString clanHandle;
};

struct ClanMemberRank_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that changed the rank
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// New rank
	LobbyDBSpec::ClanMemberStatus rank;
};

struct ClanMemberKicked_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that changed the rank
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;

	/// Banned means they can no longer interact with the clan
	bool wasBanned;
	/// Kicked means they are no longer in the clan
	bool wasKicked;
};

struct ClanMemberUnblacklisted_Notification
{
	/// User that this action was referring to
	LobbyClientUserId *userId;
	RakNet::RakString userHandle;

	/// Leader or subleader that changed the rank
	LobbyClientUserId *clanLeaderOrSubleaderId;
	RakNet::RakString clanLeaderOrSubleaderHandle;
};


// Represents a friend of yours
class RAK_DLL_EXPORT LC_Friend
{
public:
	LC_Friend();
	~LC_Friend();

	/// Identifer of the friend.
	LobbyClientUserId *id;
	
	/// Is the friend currently online?
	bool isOnline;

	/// Friend's handle (name)
	char *handle;
};

// Represents an ignored user
class RAK_DLL_EXPORT LC_Ignored
{
public:
	LC_Ignored();
	~LC_Ignored();
	LobbyClientUserId *id;
	char *actionString;
	long long creationTime;
};

}

#endif

