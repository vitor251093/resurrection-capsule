/// \file
/// \brief Base class that implements the networking for the lobby server. Database specific functionality is implemented in a derived class.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RAKNET_LOBBY_SERVER_POSTGRESQL_H
#define __RAKNET_LOBBY_SERVER_POSTGRESQL_H

#include "LobbyServer.h"
#include "FunctionThread.h"
#include "TitleValidationDB_PostgreSQL.h"
#include "RankingServer_PostgreSQL.h"
#include "LobbyDB_PostgreSQL.h"

class LobbyDB_PostgreSQL;
class RankingServer_PostgreSQL;
class TitleValidation_PostgreSQL;

namespace RakNet
{

/// Implementation of the database functionality for the LobbyServer.
/// Implements callbacks for the ranking server, lobby, and title validation databases
/// Database interaction samples are:
/// \sa LobbyDB_PostgreSQLTest
/// \sa TitleValidationDB_PostgreSQLTest
/// \sa RankingServerDBTest
class RAK_DLL_EXPORT LobbyServerPostgreSQL :
	public LobbyServer,
	public FunctionThreadDependentClass,
	public RankingServerCBInterface,
	public LobbyDBCBInterface,
	public TitleValidationDBCBInterface
{
public:
	LobbyServerPostgreSQL();
	virtual ~LobbyServerPostgreSQL();
	void Shutdown(void);
	void Startup(void);
	bool ConnectToDB(const char *connectionString);
	void CreateTables(void);
	void DestroyTables(void);

	void PushFunctor(TitleValidationFunctor *functor, void *context=0);
	void PushFunctor(LobbyDBFunctor *functor, void *context=0);
	void PushFunctor(RankingServerFunctor *functor, void *context=0);

protected:
	// Plugin interfaces
	virtual void Update(RakPeerInterface *peer);

	// Ranking server DB callbacks
	virtual void SubmitMatch_CB(SubmitMatch_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void ModifyTrustedIPList_CB(ModifyTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetTrustedIPList_CB(GetTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetRatingForParticipant_CB(GetRatingForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetRatingForParticipants_CB(GetRatingForParticipants_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetHistoryForParticipant_CB(GetHistoryForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context);

	// Lobby DB callbacks
	virtual void CreateUser_CB(CreateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetUser_CB(GetUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void UpdateUser_CB(UpdateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void DeleteUser_CB(DeleteUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void ChangeUserHandle_CB(ChangeUserHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddAccountNote_CB(AddAccountNote_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetAccountNotes_CB(GetAccountNotes_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddFriend_CB(AddFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void RemoveFriend_CB(RemoveFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetFriends_CB(GetFriends_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddToIgnoreList_CB(AddToIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void RemoveFromIgnoreList_CB(RemoveFromIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetIgnoreList_CB(GetIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void SendEmail_CB(SendEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetEmails_CB(GetEmails_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void DeleteEmail_CB(DeleteEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void UpdateEmailStatus_CB(UpdateEmailStatus_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetHandleFromUserId_CB(GetHandleFromUserId_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetUserIdFromHandle_CB(GetUserIdFromHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddDisallowedHandle_CB(AddDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void RemoveDisallowedHandle_CB(RemoveDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void IsDisallowedHandle_CB(IsDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddToActionHistory_CB(AddToActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetActionHistory_CB(GetActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	// Clans
	virtual void UpdateClanMember_CB(UpdateClanMember_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void UpdateClan_CB(UpdateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void CreateClan_CB(CreateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void ChangeClanHandle_CB(ChangeClanHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void DeleteClan_CB(DeleteClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetClans_CB(GetClans_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void RemoveFromClan_CB(RemoveFromClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void AddToClanBoard_CB(AddToClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void RemoveFromClanBoard_CB(RemoveFromClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetClanBoard_CB(GetClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context);

	// Title validation DB callbacks
	virtual void AddTitle_CB(AddTitle_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetTitles_CB(GetTitles_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void UpdateUserKey_CB(UpdateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void GetUserKeys_CB(GetUserKeys_PostgreSQLImpl *callResult, bool wasCancelled, void *context);
	virtual void ValidateUserKey_CB(ValidateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context);

	virtual LobbyDBSpec::CreateUser_Data* AllocCreateUser_Data(void) const;
	virtual LobbyDBSpec::UpdateUser_Data* AllocUpdateUser_Data(void) const;
	virtual LobbyDBSpec::SendEmail_Data* AllocSendEmail_Data(void) const;
	virtual LobbyDBSpec::AddToActionHistory_Data* AllocAddToActionHistory_Data(void) const;
	virtual LobbyDBSpec::UpdateClanMember_Data* AllocUpdateClanMember_Data(void) const;
	virtual LobbyDBSpec::UpdateClan_Data* AllocUpdateClan_Data(void) const;
	virtual LobbyDBSpec::CreateClan_Data* AllocCreateClan_Data(void) const;
	virtual LobbyDBSpec::ChangeClanHandle_Data* AllocChangeClanHandle_Data(void) const;
	virtual LobbyDBSpec::DeleteClan_Data* AllocDeleteClan_Data(void) const;
	virtual LobbyDBSpec::GetClans_Data* AllocGetClans_Data(void) const;
	virtual LobbyDBSpec::RemoveFromClan_Data* AllocRemoveFromClan_Data(void) const;
	virtual LobbyDBSpec::AddToClanBoard_Data* AllocAddToClanBoard_Data(void) const;
	virtual LobbyDBSpec::RemoveFromClanBoard_Data* AllocRemoveFromClanBoard_Data(void) const;
	virtual LobbyDBSpec::GetClanBoard_Data* AllocGetClanBoard_Data(void) const;
	virtual RankingServerDBSpec::SubmitMatch_Data* AllocSubmitMatch_Data(void) const;
	virtual RankingServerDBSpec::GetRatingForParticipant_Data* AllocGetRatingForParticipant_Data(void) const;
	virtual TitleValidationDBSpec::ValidateUserKey_Data* AllocValidateUserKey_Data(void) const;

	virtual void Login_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *userPw);
	virtual void RetrievePasswordRecoveryQuestion_DBQuery(SystemAddress systemAddress, const char *userIdStr);
	virtual void RetrievePassword_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *passwordRecoveryAnswer);
	virtual void GetFriends_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort);
	virtual void AddFriend_DBQuery(LobbyDBSpec::DatabaseKey id, LobbyDBSpec::DatabaseKey friendId, const char *messageBody, unsigned char language);
	virtual void CreateUser_DBQuery(SystemAddress systemAddress, LobbyDBSpec::CreateUser_Data* input);
	virtual void UpdateUser_DBQuery(LobbyDBSpec::UpdateUser_Data* input);
	virtual void UpdateHandle_DBQuery(const char *newHandle, LobbyDBSpec::DatabaseKey userId);
	virtual void GetIgnoreList_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool ascendingSort);
	virtual void AddToIgnoreList_DBQuery(bool hasUserId, LobbyDBSpec::DatabaseKey userId, const char *userHandle, LobbyDBSpec::DatabaseKey myId, const char *actionsStr);
	virtual void RemoveFromIgnoreList_DBQuery(LobbyDBSpec::DatabaseKey unignoredUserId, LobbyDBSpec::DatabaseKey myId);
	virtual void GetEmails_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool inbox, bool ascendingSort);
	virtual void SendEmail_DBQuery(LobbyDBSpec::SendEmail_Data *input);
	virtual void DeleteEmail_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId);
	virtual void UpdateEmailStatus_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId, int newStatus, bool wasOpened);
	virtual void DownloadActionHistory_DBQuery(SystemAddress systemAddress, bool ascendingSort, LobbyDBSpec::DatabaseKey myId);
	virtual void AddToActionHistory_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToActionHistory_Data* input);
	virtual void SubmitMatch_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::SubmitMatch_Data* input);
	virtual void GetRating_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::GetRatingForParticipant_Data* input);
	// Clans
	virtual void CreateClan_DBQuery(LobbyDBSpec::CreateClan_Data* input);
	virtual void ChangeClanHandle_DBQuery(LobbyDBSpec::ChangeClanHandle_Data *dbFunctor);
	virtual void RemoveFromClan_DBQuery(SystemAddress systemAddress,LobbyDBSpec::RemoveFromClan_Data *dbFunctor, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved, unsigned int lobbyMsgId);
	virtual void GetClans_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, const char *userHandle, bool ascendingSort);
	virtual void UpdateClanMember_DBQuery(LobbyDBSpec::UpdateClanMember_Data *dbFunctor, unsigned int lobbyMsgId, SystemAddress systemAddress, LobbyDBSpec::DatabaseKey clanSourceMemberId, RakNet::RakString clanSourceMemberHandle);
	virtual void AddPostToClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToClanBoard_Data *addToClanBoard);
	virtual void RemovePostFromClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::RemoveFromClanBoard_Data *removeFromClanBoard);
	virtual void DownloadClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::GetClanBoard_Data *removeFromClanBoard, LobbyNetworkOperations op);
	virtual void DownloadClanMember_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort);
	virtual void ValidateUserKey_DBQuery(SystemAddress systemAddress, TitleValidationDBSpec::ValidateUserKey_Data *dbFunctor);

	void CancelFunctorsWithSysAddr(SystemAddress sysAddr);
	virtual void RemoveUser(SystemAddress sysAddr);

	struct GetUser_LS : public GetUser_PostgreSQLImpl	{
		RakNet::RakString userPw_in;
		RakNet::RakString passwordRecoveryAnswer;
		enum
		{
			GET_USER_LS_LOGIN,
			GET_USER_LS_RETRIEVE_PASSWORD_RECOVERY_QUESTION,
			GET_USER_LS_RETRIEVE_PASSWORD,
		} action;
	};

	struct AddFriend_LS : public AddFriend_PostgreSQLImpl	{
		char messageBody[4096];
		unsigned char language;
	};

	struct DeleteEmail_LS : public DeleteEmail_PostgreSQLImpl	{
		LobbyDBSpec::DatabaseKey myId;
	};

	struct UpdateEmailStatus_LS : public UpdateEmailStatus_PostgreSQLImpl {
		LobbyDBSpec::DatabaseKey myId;
	};
	
	struct DownloadActionHistory_LS : public GetActionHistory_PostgreSQLImpl {
		SystemAddress systemAddress;
	};

	struct AddToActionHistory_LS : public AddToActionHistory_PostgreSQLImpl {
		SystemAddress systemAddress;
	};

	struct UpdateClanMember_LS : public UpdateClanMember_PostgreSQLImpl {
		SystemAddress systemAddress;
		unsigned int lobbyMsgId;
		LobbyDBSpec::DatabaseKey clanSourceMemberId;
		RakNet::RakString clanSourceMemberHandle;
	};

	struct ValidateUserKey_LS : public ValidateUserKey_PostgreSQLImpl {
		SystemAddress systemAddress;
	};

	struct RemoveFromClan_LS : public RemoveFromClan_PostgreSQLImpl {
		SystemAddress systemAddress;
		bool dissolveIfClanLeader;
		bool autoSendEmailIfClanDissolved;
		unsigned int lobbyMsgId;
	};

	struct RemoveFromClanBoard_LS : public  RemoveFromClanBoard_PostgreSQLImpl {
		SystemAddress systemAddress;
	};

	struct SubmitMatch_LS : public SubmitMatch_PostgreSQLImpl {
		SystemAddress systemAddress;
	};

	struct GetRatingForParticipant_LS : public GetRatingForParticipant_PostgreSQLImpl {
		SystemAddress systemAddress;
	};
	struct GetClans_LS : public GetClans_PostgreSQLImpl {
		SystemAddress systemAddress;
	};
	struct GetClanBoard_LS : public GetClanBoard_PostgreSQLImpl {
		SystemAddress systemAddress;
		LobbyNetworkOperations op;
	};
	
	// Database interfaces
	LobbyDB_PostgreSQL *lobbyDb;
	RankingServer_PostgreSQL *rankingDb;
	TitleValidation_PostgreSQL *titleValidationDb;
};

}

#endif