/// \file
/// \brief Base class that implements the networking for the lobby server. Database specific functionality is implemented in a derived class.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RAKNET_LOBBY_SERVER_H
#define __RAKNET_LOBBY_SERVER_H

#include "LobbyPCTypes.h"
#include "PluginInterface.h"
#include "DS_OrderedList.h"
#include "LobbyDBSpec.h"
#include "RakNetTypes.h"
#include "LobbyTypes.h"
#include "PacketPriority.h"
#include "TitleValidationDBSpec.h"
#include "DS_Table.h"
#include "LobbyPCCommon.h"

namespace RakNet
{
	struct LobbyServerUser;
	struct LobbyServerClan;
	struct LobbyRoomsTableContainer;
	struct LobbyServerClanMember;
}

// This can't be in the namespace or VS8 compiler bugs won't find the implementation and say it is an undefined external.
int RAK_DLL_EXPORT UserCompBySysAddr( SystemAddress const &key, RakNet::LobbyServerUser * const &data );
int RAK_DLL_EXPORT UserCompById( LobbyDBSpec::DatabaseKey const &key, RakNet::LobbyServerUser * const &data );

int RAK_DLL_EXPORT ClanCompByClanId( ClanId const &key, RakNet::LobbyServerClan * const &data );
int RAK_DLL_EXPORT ClanCompByHandle( RakNet::RakString const &key, RakNet::LobbyServerClan * const &data );
int RAK_DLL_EXPORT ClanMemberCompByHandle( RakNet::RakString const &key, RakNet::LobbyServerClanMember * const &data );

namespace RakNet
{

	/// Implements the basis for the lobby server for RakNet, independent of any kind of database.
/// Features include user to user interaction, chatting, rooms, and quick match.
/// Just attach as a plugin and go.
/// If you want to interact with the implemented PostgreSQL database, such as adding users or titles, see LobbyServerPostgreSQL
class RAK_DLL_EXPORT LobbyServer :
	public PluginInterface
{
public:
	LobbyServer();
	virtual ~LobbyServer();

	// ------------ PC SPECIFIC --------------
	/// Ordering channel to use with RakPeer::Send()
	void SetOrderingChannel(char oc);
	void SetSendPriority(PacketPriority pp);


	struct PotentialFriend
	{
		LobbyDBSpec::DatabaseKey invitor;
		LobbyDBSpec::DatabaseKey invitee;
	};

protected:
	void Login_NetMsg(Packet *packet);
	void SetTitleLoginId_NetMsg(Packet *packet);
	void Logoff_NetMsg(Packet *packet);
	void RegisterAccount_NetMsg(Packet *packet);
	void UpdateAccount_NetMsg(Packet *packet);
	void ChangeHandle_NetMsg(Packet *packet);
	void RetrievePasswordRecoveryQuestion_NetMsg(Packet *packet);
	void RetrievePassword_NetMsg(Packet *packet);
	void DownloadFriends_NetMsg(Packet *packet);
	void SendAddFriendInvite_NetMsg(Packet *packet);
	void AcceptAddFriendInvite_NetMsg(Packet *packet);
	void DeclineAddFriendInvite_NetMsg(Packet *packet);
	void DownloadIgnoreList_NetMsg(Packet *packet);
	void UpdateIgnoreList_NetMsg(Packet *packet);
	void RemoveFromIgnoreList_NetMsg(Packet *packet);
	void DownloadEmails_NetMsg(Packet *packet);
	void SendEmail_NetMsg(Packet *packet);
	void DeleteEmail_NetMsg(Packet *packet);
	void UpdateEmailStatus_NetMsg(Packet *packet);
	void SendIM_NetMsg(Packet *packet);
	void CreateRoom_NetMsg(Packet *packet);
	void DownloadRooms_NetMsg(Packet *packet);
	void LeaveRoom_NetMsg(Packet *packet);
	void JoinRoom_NetMsg(Packet *packet);
	void JoinRoomByFilter_NetMsg(Packet *packet);
	void RoomChat_NetMsg(Packet *packet);
	void SetReadyToPlayStatus_NetMsg(Packet *packet);
	void InviteToRoom_NetMsg(Packet *packet);
	void KickRoomMember_NetMsg(Packet *packet);
	void SetRoomIsHidden_NetMsg(Packet *packet);
	void SetRoomAllowSpectators_NetMsg(Packet *packet);
	void ChangeNumSlots_NetMsg(Packet *packet);
	void GrantModerator_NetMsg(Packet *packet);
	void StartGame_NetMsg(Packet *packet);
	void SubmitMatch_NetMsg(Packet *packet);
	void DownloadRating_NetMsg(Packet *packet);
	void QuickMatch_NetMsg(Packet *packet);
	void CancelQuickMatch_NetMsg(Packet *packet);
	void DownloadActionHistory_NetMsg(Packet *packet);
	void AddToActionHistory_NetMsg(Packet *packet);
	void CreateClan_NetMsg(Packet *packet);
	void ChangeClanHandle_NetMsg(Packet *packet);
	void LeaveClan_NetMsg(Packet *packet);
	void DownloadClans_NetMsg(Packet *packet);
	void SendClanJoinInvitation_NetMsg(Packet *packet);
	void WithdrawClanJoinInvitation_NetMsg(Packet *packet);
	void AcceptClanJoinInvitation_NetMsg(Packet *packet);
	void RejectClanJoinInvitation_NetMsg(Packet *packet);
	void DownloadByClanMemberStatus_NetMsg(Packet *packet);
	void SendClanMemberJoinRequest_NetMsg(Packet *packet);
	void WithdrawClanMemberJoinRequest_NetMsg(Packet *packet);
	void AcceptClanMemberJoinRequest_NetMsg(Packet *packet);
	void RejectClanMemberJoinRequest_NetMsg(Packet *packet);
	void SetClanMemberRank_NetMsg(Packet *packet);
	void ClanKickAndBlacklistUser_NetMsg(Packet *packet);
	void ClanUnblacklistUser_NetMsg(Packet *packet);
	void AddPostToClanBoard_NetMsg(Packet *packet);
	void RemovePostFromClanBoard_NetMsg(Packet *packet);
	void DownloadMyClanBoards_NetMsg(Packet *packet);
	void DownloadClanBoard_NetMsg(Packet *packet);
	void DownloadClanMember_NetMsg(Packet *packet);
	void ValidateUserKey_NetMsg(Packet *packet);

	virtual LobbyDBSpec::CreateUser_Data* AllocCreateUser_Data(void) const=0;
	virtual LobbyDBSpec::UpdateUser_Data* AllocUpdateUser_Data(void) const=0;
	virtual LobbyDBSpec::SendEmail_Data* AllocSendEmail_Data(void) const=0;
	virtual LobbyDBSpec::AddToActionHistory_Data* AllocAddToActionHistory_Data(void) const=0;
	virtual LobbyDBSpec::UpdateClanMember_Data* AllocUpdateClanMember_Data(void) const=0;
	virtual LobbyDBSpec::UpdateClan_Data* AllocUpdateClan_Data(void) const=0;
	virtual LobbyDBSpec::CreateClan_Data* AllocCreateClan_Data(void) const=0;
	virtual LobbyDBSpec::ChangeClanHandle_Data* AllocChangeClanHandle_Data(void) const=0;
	virtual LobbyDBSpec::DeleteClan_Data* AllocDeleteClan_Data(void) const=0;
	virtual LobbyDBSpec::GetClans_Data* AllocGetClans_Data(void) const=0;
	virtual LobbyDBSpec::RemoveFromClan_Data* AllocRemoveFromClan_Data(void) const=0;
	virtual LobbyDBSpec::AddToClanBoard_Data* AllocAddToClanBoard_Data(void) const=0;
	virtual LobbyDBSpec::RemoveFromClanBoard_Data* AllocRemoveFromClanBoard_Data(void) const=0;
	virtual LobbyDBSpec::GetClanBoard_Data* AllocGetClanBoard_Data(void) const=0;
	virtual RankingServerDBSpec::SubmitMatch_Data* AllocSubmitMatch_Data(void) const=0;
	virtual RankingServerDBSpec::GetRatingForParticipant_Data* AllocGetRatingForParticipant_Data(void) const=0;
	virtual TitleValidationDBSpec::ValidateUserKey_Data* AllocValidateUserKey_Data(void) const=0;

	virtual void Login_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *userPw)=0;
	virtual void RetrievePasswordRecoveryQuestion_DBQuery(SystemAddress systemAddress, const char *userIdStr)=0;
	virtual void RetrievePassword_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *passwordRecoveryAnswer)=0;
	virtual void GetFriends_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort)=0;
	virtual void AddFriend_DBQuery(LobbyDBSpec::DatabaseKey id, LobbyDBSpec::DatabaseKey friendId, const char *messageBody, unsigned char language)=0;
	virtual void CreateUser_DBQuery(SystemAddress systemAddress, LobbyDBSpec::CreateUser_Data* input)=0;
	virtual void UpdateUser_DBQuery(LobbyDBSpec::UpdateUser_Data* input)=0;
	virtual void UpdateHandle_DBQuery(const char *newHandle, LobbyDBSpec::DatabaseKey userId)=0;
	virtual void GetIgnoreList_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool ascendingSort)=0;
	virtual void AddToIgnoreList_DBQuery(bool hasUserId, LobbyDBSpec::DatabaseKey userId, const char *userHandle, LobbyDBSpec::DatabaseKey myId, const char *actionsStr)=0;
	virtual void RemoveFromIgnoreList_DBQuery(LobbyDBSpec::DatabaseKey unignoredUserId, LobbyDBSpec::DatabaseKey myId)=0;
	virtual void GetEmails_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool inbox, bool ascendingSort)=0;
	virtual void SendEmail_DBQuery(LobbyDBSpec::SendEmail_Data *input)=0;
	virtual void DeleteEmail_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId)=0;
	virtual void UpdateEmailStatus_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId, int newStatus, bool wasOpened)=0;
	virtual void DownloadActionHistory_DBQuery(SystemAddress systemAddress, bool ascendingSort, LobbyDBSpec::DatabaseKey myId)=0;
	virtual void AddToActionHistory_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToActionHistory_Data* input)=0;
	virtual void SubmitMatch_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::SubmitMatch_Data* input)=0;
	virtual void GetRating_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::GetRatingForParticipant_Data* input)=0;
	// Clans
	virtual void CreateClan_DBQuery(LobbyDBSpec::CreateClan_Data* input)=0;
	virtual void ChangeClanHandle_DBQuery(LobbyDBSpec::ChangeClanHandle_Data *dbFunctor)=0;
	virtual void RemoveFromClan_DBQuery(SystemAddress systemAddress,LobbyDBSpec::RemoveFromClan_Data *dbFunctor, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved, unsigned int lobbyMsgId)=0;
	virtual void GetClans_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, const char *userHandle, bool ascendingSort)=0;
	virtual void UpdateClanMember_DBQuery(LobbyDBSpec::UpdateClanMember_Data *dbFunctor, unsigned int lobbyMsgId, SystemAddress systemAddress, LobbyDBSpec::DatabaseKey clanSourceMemberId, RakNet::RakString clanSourceMemberHandle)=0;
	virtual void AddPostToClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToClanBoard_Data *addToClanBoard)=0;
	virtual void RemovePostFromClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::RemoveFromClanBoard_Data *removeFromClanBoard)=0;
	virtual void DownloadClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::GetClanBoard_Data *removeFromClanBoard, LobbyNetworkOperations op)=0;
	virtual void DownloadClanMember_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort)=0;

	// Keys
	virtual void ValidateUserKey_DBQuery(SystemAddress systemAddress, TitleValidationDBSpec::ValidateUserKey_Data *dbFunctor)=0;

	virtual void Login_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res, const char *userPw_in);
	virtual void AddFriend_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddFriend_Data *res, const char *messageBody, unsigned char language);
	virtual void RetrievePasswordRecoveryQuestion_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res);
	virtual void RetrievePassword_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetUser_Data *res, const char *passwordRecoveryAnswer);
	virtual void GetFriends_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetFriends_Data *res, SystemAddress systemAddress);
	virtual void CreateUser_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::CreateUser_Data *res);
	virtual void UpdateHandle_DBResult(LobbyDBSpec::ChangeUserHandle_Data *res);
	virtual void GetIgnoreList_DBResult(bool dbSuccess, const char *queryErrorMessage, SystemAddress systemAddress, LobbyDBSpec::GetIgnoreList_Data *res);
	virtual void AddToIgnoreList_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey ignoredUserId, LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::AddToIgnoreList_Data *res);
	virtual void RemoveFromIgnoreList_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey unignoredUserId, LobbyDBSpec::DatabaseKey myId);
	virtual void SendEmail_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::SendEmail_Data *res);
	virtual void GetEmails_DBResult(bool dbSuccess, bool querySuccess, const char *queryErrorMessage, LobbyDBSpec::GetEmails_Data *res);
	virtual void DeleteEmail_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DeleteEmail_Data *res);
	virtual void UpdateEmailStatus_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::UpdateEmailStatus_Data *res);
	virtual void AddToActionHistory_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddToActionHistory_Data *res, SystemAddress systemAddress);
	virtual void GetActionHistory_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetActionHistory_Data *res, SystemAddress systemAddress);
	virtual void SubmitMatch_DBResult(bool dbSuccess, const char *queryErrorMessage, RankingServerDBSpec::SubmitMatch_Data *res, SystemAddress systemAddress);
	virtual void GetRatingForParticipant_DBResult(bool dbSuccess, const char *queryErrorMessage, RankingServerDBSpec::GetRatingForParticipant_Data *res, SystemAddress systemAddress);
	virtual void UpdateClanMember_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::UpdateClanMember_Data *res, unsigned int lobbyMsgId, SystemAddress systemAddress, LobbyDBSpec::DatabaseKey clanSourceMemberId, RakNet::RakString clanSourceMemberHandle);
	virtual void UpdateClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::UpdateClan_Data *res, SystemAddress systemAddress);
	virtual void CreateClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::CreateClan_Data *res);
	virtual void ChangeClanHandle_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::ChangeClanHandle_Data *res);
	virtual void DeleteClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::DeleteClan_Data *res, SystemAddress systemAddress);
	virtual void GetClans_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetClans_Data *res, SystemAddress systemAddress);
	virtual void RemoveFromClan_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::RemoveFromClan_Data *res, SystemAddress systemAddress, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved, unsigned int lobbyMsgId);
	virtual void AddToClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::AddToClanBoard_Data *res);
	virtual void RemoveFromClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::RemoveFromClanBoard_Data *res, SystemAddress systemAddress);
	virtual void GetClanBoard_DBResult(bool dbSuccess, const char *queryErrorMessage, LobbyDBSpec::GetClanBoard_Data *res, SystemAddress systemAddress, LobbyNetworkOperations op);
	virtual void ValidateUserKey_DBResult(bool dbSuccess, const char *queryErrorMessage, TitleValidationDBSpec::ValidateUserKey_Data *res, SystemAddress systemAddress);


	// Plugin interface functions
	virtual void OnAttach(RakPeerInterface *peer);
	PluginReceiveResult OnReceive(RakPeerInterface *peer, Packet *packet);
	virtual void OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress);
	virtual void OnShutdown(RakPeerInterface *peer);
	virtual void Update(RakPeerInterface *peer);

	// Utility functions
	bool Send( const RakNet::BitStream * bitStream, SystemAddress target );

	bool SendIM(LobbyServerUser *sender, LobbyServerUser *recipient, const char *messageBody, unsigned char language);

	// 0 = new friend, implicitly online (+ data), 1 = no longer friends, 2 = friend online, 3 = friend offline

	void NotifyFriendStatusChange(SystemAddress target, LobbyDBSpec::DatabaseKey friendId, const char *friendHandle, unsigned char status);

	// TODO - run nat punchthrough directly, or separately?

	void Clear(void);
	void ClearTitles(void);
	void ClearLobbyGames(void);
	void ClearClansAndClanMembers(void);
	void ClearUsers(void);

	RakPeerInterface *rakPeer;
	void DestroyRoom(LobbyRoom *room);
	virtual void RemoveUser(SystemAddress sysAddr);
	void RemoveRoomMember(LobbyRoom *room, LobbyDBSpec::DatabaseKey memberId, bool *roomIsDead, bool wasKicked, SystemAddress memberAddr);
	void RemoveRoom(LobbyRoom *room);
	void SendToRoomMembers(LobbyRoom *room, RakNet::BitStream *bs, bool hasSkipId=false, LobbyDBSpec::DatabaseKey skipId=0);
	void ForceAllRoomMembersToLobby(LobbyRoom *room);
	LobbyServerUser* GetUserByHandle(const char *handle);
	LobbyServerUser* GetUserByAddress(SystemAddress sysAddr);
	LobbyServerUser* GetUserByID(LobbyDBSpec::DatabaseKey id) const;
	RakNet::RakString GetHandleByID(LobbyDBSpec::DatabaseKey id) const;
	SystemAddress GetSystemAddressById(LobbyDBSpec::DatabaseKey id) const;

	void RemoveFromQuickMatchList(LobbyServerUser *user);
	void SerializeClientSafeCreateUserData(LobbyServerUser *user, RakNet::BitStream *bs);

	// Double sort for fast lookup by either system address or id
	DataStructures::OrderedList<SystemAddress, LobbyServerUser*, UserCompBySysAddr> usersBySysAddr;
	DataStructures::OrderedList<LobbyDBSpec::DatabaseKey, LobbyServerUser*, UserCompById> usersByID;
	// Does not store pointers, just references them in users
	DataStructures::List<LobbyServerUser*> quickMatchUsers;

	// Rooms under particular games
	DataStructures::OrderedList< TitleValidationDBSpec::DatabaseKey, LobbyRoomsTableContainer*, LobbyTitleCompByDatabaseKey> lobbyTitles;

	LobbyRoom* GetLobbyRoomByName(const char *title, TitleValidationDBSpec::DatabaseKey gameId);
	LobbyRoom* GetLobbyRoomByRowId(LobbyClientRoomId rowId, TitleValidationDBSpec::DatabaseKey gameId);
	LobbyRoomsTableContainer* GetLobbyRoomsTableContainerById(TitleValidationDBSpec::DatabaseKey titleId);

	TitleValidationDBSpec::AddTitle_Data* GetTitleDataByPassword(char *titlePassword, int titlePasswordLength);
	TitleValidationDBSpec::AddTitle_Data* GetTitleDataById(TitleValidationDBSpec::DatabaseKey titleId);
	TitleValidationDBSpec::AddTitle_Data* GetTitleDataByTitleName(RakNet::RakString titleName);
	void SendToRoomNotIgnoredNotSelf(LobbyServerUser* lsu, RakNet::BitStream *reply);

	char orderingChannel;
	PacketPriority packetPriority;
	DataStructures::List<TitleValidationDBSpec::AddTitle_Data*> titles;
	DataStructures::List<RankingServerDBSpec::ModifyTrustedIPList_Data*> trustedIpList;

	bool IsTrustedIp(SystemAddress systemAddress, TitleValidationDBSpec::DatabaseKey titleId);

	void SendNewMemberToRoom(LobbyServerUser *lsu, LobbyRoom* room, bool joinAsSpectator);
	void FlagEnteredAndSerializeRoom(LobbyServerUser *lsu, RakNet::BitStream &reply, LobbyRoom* room);
	LobbyServerUser * LobbyServer::DeserializeHandleOrId(char *userHandle, LobbyClientUserId &userId, RakNet::BitStream *bs);
	RakNet::LobbyServerClan * LobbyServer::GetClanByHandleOrId(RakNet::RakString clanHandle, ClanId clanId);
	RakNet::LobbyServerClan * LobbyServer::DeserializeClanHandleOrId(char *clanHandle, ClanId &clanId, RakNet::BitStream *bs);
	LobbyServerClan *ValidateAndDeserializeClan(char *clanHandle, ClanId &clanId, RakNet::BitStream *inputBs, RakNet::BitStream *outputBs );

	bool IsInPotentialFriends(PotentialFriend p);
	void AddToPotentialFriends(PotentialFriend p);
	void RemoveFromPotentialFriends(LobbyDBSpec::DatabaseKey user);
	void RemoveFromPotentialFriends(PotentialFriend p);
	DataStructures::List<PotentialFriend> potentialFriends;

	bool HasClan(ClanId id) const;
	unsigned GetClanIndex(const RakNet::RakString &id) const;
	unsigned GetClanIndex(ClanId id) const;
	LobbyServerClan *GetClanById(ClanId id) const;
	LobbyServerClan *GetClanByHandle(RakNet::RakString handle) const;
	bool HasClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const;
	unsigned GetClanMemberIndex(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const;
	LobbyServerClanMember *GetOptionalClanMember(LobbyServerClan *lsc, LobbyServerUser *lsu) const;
	void SetOptionalBusyStatus(LobbyServerClanMember *targetLsm, int status );
	LobbyServerClanMember *GetClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId) const;
	void AddClan(LobbyServerClan *lsc);
	void AddClanMember(LobbyServerClan *lsc, LobbyServerClanMember *lscm);
	void DeleteClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId);
	LobbyServerClanMember* CreateClanMember(LobbyServerClan *lsc, LobbyDBSpec::DatabaseKey userId, LobbyDBSpec::UpdateClanMember_Data *ucm);
	void DeleteClan(ClanId id);
	void DeleteClan(LobbyServerClan *lsc);
	bool DeserializeClanId(RakNet::BitStream *bs, ClanId &clanId) const;
	bool DeserializeClanIdDefaultSelf(LobbyServerUser *lsu, RakNet::BitStream *bs, ClanId &clanId) const;
	bool ValidateNotSelf(LobbyServerClanMember *lsm1, LobbyServerClanMember *lsm2, RakNet::BitStream *bs);
	bool ValidateOneClan(LobbyServerUser *lsu, RakNet::BitStream *bs);
	bool ValidateLeader(LobbyServerClanMember *lsm, RakNet::BitStream *bs);
	bool ValidateLeaderOrSubleader(LobbyServerClanMember *lsm, RakNet::BitStream *bs);
	bool ValidateNotBlacklisted(LobbyServerClanMember *lsm, RakNet::BitStream *bs);
	bool ValidateNotBusy(LobbyServerClanMember *lsm, RakNet::RakString &memberName, RakNet::RakString &clanName, RakNet::BitStream *bs);

	SystemAddress GetLeaderSystemAddress(ClanId clanId) const;
	void SendEmail(LobbyDBSpec::DatabaseKey dbKey, unsigned char language, LobbyDBSpec::SendEmail_Data *input, SystemAddress systemAddress );

	DataStructures::OrderedList<ClanId, LobbyServerClan*, ClanCompByClanId> clansById; // Holds real pointer
	DataStructures::OrderedList<RakNet::RakString, LobbyServerClan*, ClanCompByHandle> clansByHandle;
};

struct LobbyServerUser
{
	LobbyServerUser();
	~LobbyServerUser();
	LobbyDBSpec::GetUser_Data *data;
	SystemAddress systemAddress;
	RakNetTime connectionTime;
	RakNet::LobbyUserStatus status;
	bool downloadedFriends;
	bool readyToPlay;
	RakNetTime quickMatchStopWaiting;
	int quickMatchUserRequirement;
	TitleValidationDBSpec::AddTitle_Data *title;
	DataStructures::List<LobbyDBSpec::AddFriend_Data *> friends;
	DataStructures::List<LobbyDBSpec::AddToIgnoreList_Data *> ignoredUsers;
	DataStructures::List<LobbyServerClanMember*> clanMembers;
	LobbyRoom *currentRoom;
	int pendingClanCount;

	// Validated CD Key
	LobbyDBSpec::DatabaseKey titleId;

	bool AddFriend(LobbyDBSpec::AddFriend_Data *_friend);
	void RemoveFriend(LobbyDBSpec::DatabaseKey friendId);
	bool IsFriend(LobbyDBSpec::DatabaseKey friendId);

	bool AddIgnoredUser(LobbyDBSpec::AddToIgnoreList_Data *ignoredUser);
	void RemoveIgnoredUser(LobbyDBSpec::DatabaseKey userId);
	bool IsIgnoredUser(LobbyDBSpec::DatabaseKey userId);
	bool IsInClan(void) const;
	LobbyServerClan* GetFirstClan(void) const;

	LobbyDBSpec::DatabaseKey GetID(void) const;
};

struct LobbyServerClanMember;

struct LobbyServerClan
{
	LobbyServerClan();
	~LobbyServerClan();
	ClanId clanId;
	RakNet::RakString handle;
	

	void AddClanMember(LobbyServerClanMember* lscm);
	unsigned GetClanMemberIndex(LobbyDBSpec::DatabaseKey userId);
	void DeleteClanMember(unsigned index);
	LobbyServerClanMember *GetLeader(void) const;

	DataStructures::List<LobbyServerClanMember*> clanMembers;
	void InsertMember(LobbyServerClanMember *lscm);
	void SetMembersDirty(bool b);
};

struct LobbyServerClanMember
{
	LobbyServerClanMember();
	~LobbyServerClanMember();
	bool isDirty; // Used to delete members that are not in the LobbyServerClanList when we download the list
	LobbyDBSpec::DatabaseKey userId; 
	LobbyDBSpec::ClanMemberStatus clanStatus;
	int clanJoinsPending;
	LobbyServerClan *clan;
	enum BusyState
	{
		LSCM_NOT_BUSY,
		LSCM_BUSY_JOIN_INVITATION,
		LSCM_BUSY_WITHDRAW_JOIN_INVITATION,
		LSCM_BUSY_ACCEPTING_JOIN_INVITATION,
		LSCM_BUSY_REJECTING_JOIN_INVITATION,
		LSCM_BUSY_WITHDRAW_JOIN_REQUEST,
		LSCM_BUSY_ACCEPTING_JOIN_REQUEST,
		LSCM_BUSY_REJECTING_JOIN_REQUEST,
		LSCM_BUSY_JOIN_REQUEST,
		LSCM_BUSY_BLACKLISTING,
	} busyStatus;

	LobbyDBSpec::DatabaseKey clanSourceMemberId;
	RakNet::RakString clanSourceMemberHandle;

	LobbyDBSpec::UpdateClanMember_Data *memberData;

//	LobbyDBSpec::DatabaseKey targetMemberId;
//	RakNet::RakString targetMemberHandle;
};

}

#endif

