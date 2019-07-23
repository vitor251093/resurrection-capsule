#include "LobbyServer_PostgreSQL.h"
#include "LobbyDB_PostgreSQL.h"
#include "RankingServer_PostgreSQL.h"
#include "TitleValidationDB_PostgreSQL.h"
#include "RakAssert.h"

using namespace RakNet;

void DeleteSysAddrContext(FunctionThread::FunctorAndContext fac)
{
	if (fac.context)
		delete (SystemAddress*)fac.context;
}
bool CancelWithSysAddr(FunctionThread::FunctorAndContext fac, void *userData)
{
	SystemAddress sysAddr1 = *((SystemAddress*) fac.context);
	SystemAddress sysAddr2 = *((SystemAddress*) userData);
	return sysAddr1==sysAddr2;
}
LobbyServerPostgreSQL::LobbyServerPostgreSQL()
{
	lobbyDb=0;
	rankingDb=0;
	titleValidationDb=0;
}
LobbyServerPostgreSQL::~LobbyServerPostgreSQL()
{
	if (lobbyDb)
		delete lobbyDb;
	if (rankingDb)
		delete rankingDb;
	if (titleValidationDb)
		delete titleValidationDb;
}
void LobbyServerPostgreSQL::Shutdown(void)
{
	functionThread->StopThreads(true);
}
void LobbyServerPostgreSQL::Startup(void)
{
	StartFunctionThread();

	if (lobbyDb==0)
		lobbyDb = new LobbyDB_PostgreSQL;
	if (rankingDb==0)
		rankingDb = new RankingServer_PostgreSQL;
	if (titleValidationDb==0)
		titleValidationDb = new TitleValidation_PostgreSQL;

	lobbyDb->AssignFunctionThread(functionThread);
	rankingDb->AssignFunctionThread(functionThread);
	titleValidationDb->AssignFunctionThread(functionThread);

	// I pass a SystemAddress pointer as the context in many cases
	// This will automatically delete them after the functor is done.
	functionThread->SetPostResultFunction(DeleteSysAddrContext);
	
	lobbyDb->AssignCallback(this);
	rankingDb->AssignCallback(this);
	titleValidationDb->AssignCallback(this);

	// Download all the titles
	GetTitles_PostgreSQLImpl *functor1 = GetTitles_PostgreSQLImpl::Alloc();
	PushFunctor(functor1);

	// Download allowed IPs for uploading ranking data
	GetTrustedIPList_PostgreSQLImpl*functor2 = GetTrustedIPList_PostgreSQLImpl::Alloc();
	PushFunctor(functor2);
}
bool LobbyServerPostgreSQL::ConnectToDB(const char *connectionString)
{
	if (lobbyDb==0)
		lobbyDb = new LobbyDB_PostgreSQL;
	if (rankingDb==0)
		rankingDb = new RankingServer_PostgreSQL;
	if (titleValidationDb==0)
		titleValidationDb = new TitleValidation_PostgreSQL;

	bool b = lobbyDb->Connect(connectionString);
	if (b)
	{
		rankingDb->AssignConnection(lobbyDb->GetPGConn());
		titleValidationDb->AssignConnection(lobbyDb->GetPGConn());
	}
	return b;
}
void LobbyServerPostgreSQL::CreateTables(void)
{
	lobbyDb->CreateLobbyServerTables();
	rankingDb->CreateRankingServerTables();
	titleValidationDb->CreateTitleValidationTables();
}
void LobbyServerPostgreSQL::DestroyTables(void)
{
	lobbyDb->DestroyLobbyServerTables();
	rankingDb->DestroyRankingServerTables();
	titleValidationDb->DestroyTitleValidationTables();
}
void LobbyServerPostgreSQL::PushFunctor(TitleValidationFunctor *functor, void *context)
{
	titleValidationDb->PushFunctor(functor, context);
}
void LobbyServerPostgreSQL::PushFunctor(LobbyDBFunctor *functor, void *context)
{
	lobbyDb->PushFunctor(functor, context);
}
void LobbyServerPostgreSQL::PushFunctor(RankingServerFunctor *functor, void *context)
{
	rankingDb->PushFunctor(functor, context);
}
void LobbyServerPostgreSQL::Update(RakPeerInterface *peer)
{
	LobbyServer::Update(peer);
	functionThread->CallResultHandlers();
}
LobbyDBSpec::CreateUser_Data* LobbyServerPostgreSQL::AllocCreateUser_Data(void) const
{
	return new CreateUser_PostgreSQLImpl;
}
LobbyDBSpec::UpdateUser_Data* LobbyServerPostgreSQL::AllocUpdateUser_Data(void) const
{
	return new UpdateUser_PostgreSQLImpl;
}
LobbyDBSpec::SendEmail_Data* LobbyServerPostgreSQL::AllocSendEmail_Data(void) const
{
	return new SendEmail_PostgreSQLImpl;
}
LobbyDBSpec::AddToActionHistory_Data* LobbyServerPostgreSQL::AllocAddToActionHistory_Data(void) const
{
	return new AddToActionHistory_LS;
}
LobbyDBSpec::UpdateClanMember_Data* LobbyServerPostgreSQL::AllocUpdateClanMember_Data(void) const
{
	return new UpdateClanMember_LS;
}
LobbyDBSpec::UpdateClan_Data* LobbyServerPostgreSQL::AllocUpdateClan_Data(void) const
{
	return new UpdateClan_PostgreSQLImpl;
}
LobbyDBSpec::CreateClan_Data* LobbyServerPostgreSQL::AllocCreateClan_Data(void) const
{
	return new CreateClan_PostgreSQLImpl;
}
LobbyDBSpec::ChangeClanHandle_Data* LobbyServerPostgreSQL::AllocChangeClanHandle_Data(void) const
{
	return new ChangeClanHandle_PostgreSQLImpl;
}
LobbyDBSpec::DeleteClan_Data* LobbyServerPostgreSQL::AllocDeleteClan_Data(void) const
{
	return new DeleteClan_PostgreSQLImpl;
}
LobbyDBSpec::GetClans_Data* LobbyServerPostgreSQL::AllocGetClans_Data(void) const
{
	return new GetClans_PostgreSQLImpl;
}
LobbyDBSpec::RemoveFromClan_Data* LobbyServerPostgreSQL::AllocRemoveFromClan_Data(void) const
{
	return new RemoveFromClan_LS;
}
LobbyDBSpec::AddToClanBoard_Data* LobbyServerPostgreSQL::AllocAddToClanBoard_Data(void) const
{
	return new AddToClanBoard_PostgreSQLImpl;
}
LobbyDBSpec::RemoveFromClanBoard_Data* LobbyServerPostgreSQL::AllocRemoveFromClanBoard_Data(void) const
{
	return new RemoveFromClanBoard_LS;
}
LobbyDBSpec::GetClanBoard_Data* LobbyServerPostgreSQL::AllocGetClanBoard_Data(void) const
{
	return new GetClanBoard_LS;
}
RankingServerDBSpec::SubmitMatch_Data* LobbyServerPostgreSQL::AllocSubmitMatch_Data(void) const
{
	return new SubmitMatch_LS;
}
RankingServerDBSpec::GetRatingForParticipant_Data* LobbyServerPostgreSQL::AllocGetRatingForParticipant_Data(void) const
{
	return new GetRatingForParticipant_LS;
}
TitleValidationDBSpec::ValidateUserKey_Data* LobbyServerPostgreSQL::AllocValidateUserKey_Data(void) const
{
	return new ValidateUserKey_LS;
}
void LobbyServerPostgreSQL::Login_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *userPw)
{
	GetUser_LS *query = new GetUser_LS;
	query->userPw_in=userPw;
	query->getCCInfo=true;
	query->getBinaryData=true;
	query->getPersonalInfo=true;
	query->getEmailAddr=true;
	query->getPassword=true;
	query->id.hasDatabaseRowId=false;
	query->id.handle=userIdStr;
	query->action=GetUser_LS::GET_USER_LS_LOGIN;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::RetrievePasswordRecoveryQuestion_DBQuery(SystemAddress systemAddress, const char *userIdStr)
{
	GetUser_LS *query = new GetUser_LS;
	query->getCCInfo=false;
	query->getBinaryData=false;
	query->getPersonalInfo=false;
	query->getEmailAddr=false;
	query->getPassword=true;
	query->id.hasDatabaseRowId=false;
	query->id.handle=userIdStr;
	query->action=GetUser_LS::GET_USER_LS_RETRIEVE_PASSWORD_RECOVERY_QUESTION;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}

void LobbyServerPostgreSQL::RetrievePassword_DBQuery(SystemAddress systemAddress, const char *userIdStr, const char *passwordRecoveryAnswer)
{
	GetUser_LS *query = new GetUser_LS;
	query->getCCInfo=false;
	query->getBinaryData=false;
	query->getPersonalInfo=false;
	query->getEmailAddr=false;
	query->getPassword=true;
	query->id.hasDatabaseRowId=false;
	query->id.handle=userIdStr;
	query->action=GetUser_LS::GET_USER_LS_RETRIEVE_PASSWORD;
	query->passwordRecoveryAnswer=passwordRecoveryAnswer;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::GetFriends_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort)
{
	GetFriends_PostgreSQLImpl *query = new GetFriends_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=userId;
	query->ascendingSort=ascendingSort;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::AddFriend_DBQuery(LobbyDBSpec::DatabaseKey id, LobbyDBSpec::DatabaseKey friendId, const char *messageBody, unsigned char language)
{
	AddFriend_LS *query = new AddFriend_LS;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=id;
	query->friendId.hasDatabaseRowId=true;
	query->friendId.databaseRowId=friendId;
	query->language=language;
	strcpy(query->messageBody, messageBody);
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::CreateUser_DBQuery(SystemAddress systemAddress, LobbyDBSpec::CreateUser_Data* input)
{
	CreateUser_PostgreSQLImpl *query = (CreateUser_PostgreSQLImpl *) input;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::UpdateUser_DBQuery(LobbyDBSpec::UpdateUser_Data* input)
{
	UpdateUser_PostgreSQLImpl *query = (UpdateUser_PostgreSQLImpl *) input;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::UpdateHandle_DBQuery(const char *newHandle, LobbyDBSpec::DatabaseKey userId)
{
	ChangeUserHandle_PostgreSQLImpl *query = new ChangeUserHandle_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=userId;
	query->newHandle=newHandle;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::GetIgnoreList_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool ascendingSort)
{
	GetIgnoreList_PostgreSQLImpl *query = new GetIgnoreList_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=myId;
	query->ascendingSort=ascendingSort;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::AddToIgnoreList_DBQuery(bool hasUserId, LobbyDBSpec::DatabaseKey userId, const char *userHandle, LobbyDBSpec::DatabaseKey myId, const char *actionsStr)
{
	AddToIgnoreList_PostgreSQLImpl *query = new AddToIgnoreList_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=myId;
	query->ignoredUser.hasDatabaseRowId=hasUserId;
	query->ignoredUser.databaseRowId=userId;
	query->ignoredUser.handle=userHandle;
	query->actions=actionsStr;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::RemoveFromIgnoreList_DBQuery(LobbyDBSpec::DatabaseKey unignoredUserId, LobbyDBSpec::DatabaseKey myId)
{
	RemoveFromIgnoreList_PostgreSQLImpl *query = new RemoveFromIgnoreList_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=myId;
	query->ignoredUser.hasDatabaseRowId=true;
	query->ignoredUser.databaseRowId=unignoredUserId;
	PushFunctor(query, 0);
}

void LobbyServerPostgreSQL::GetEmails_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey myId, bool inbox, bool ascendingSort)
{
	GetEmails_PostgreSQLImpl *query = new GetEmails_PostgreSQLImpl;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=myId;
	query->inbox=inbox;
	query->ascendingSort=ascendingSort;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::SendEmail_DBQuery(LobbyDBSpec::SendEmail_Data *input)
{
	SendEmail_PostgreSQLImpl *query = (SendEmail_PostgreSQLImpl*) input;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::DeleteEmail_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId)
{
	DeleteEmail_LS *query = new DeleteEmail_LS;
	query->emailMessageID=emailId;
	query->myId=myId;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::UpdateEmailStatus_DBQuery(LobbyDBSpec::DatabaseKey myId, LobbyDBSpec::DatabaseKey emailId, int newStatus, bool wasOpened)
{
	UpdateEmailStatus_LS *query = new UpdateEmailStatus_LS;
	query->emailMessageID=emailId;
	query->myId=myId;
	query->status=newStatus;
	query->wasOpened=wasOpened;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::DownloadActionHistory_DBQuery(SystemAddress systemAddress, bool ascendingSort, LobbyDBSpec::DatabaseKey myId)
{
	DownloadActionHistory_LS *query = new DownloadActionHistory_LS;
	query->systemAddress=systemAddress;
	query->ascendingSort=ascendingSort;
	query->id.hasDatabaseRowId=true;
	query->id.databaseRowId=myId;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::AddToActionHistory_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToActionHistory_Data* input)
{
	AddToActionHistory_LS *query = (AddToActionHistory_LS*) input;
	query->systemAddress=systemAddress;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::SubmitMatch_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::SubmitMatch_Data* input)
{
	SubmitMatch_LS *query = (SubmitMatch_LS*) input;
	query->systemAddress=systemAddress;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::GetRating_DBQuery(SystemAddress systemAddress, RankingServerDBSpec::GetRatingForParticipant_Data* input)
{
	GetRatingForParticipant_LS *query = (GetRatingForParticipant_LS*) input;
	query->systemAddress=systemAddress;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::CreateClan_DBQuery(LobbyDBSpec::CreateClan_Data* input)
{
	CreateClan_PostgreSQLImpl *query = (CreateClan_PostgreSQLImpl*) input;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::ChangeClanHandle_DBQuery(LobbyDBSpec::ChangeClanHandle_Data *dbFunctor)
{
	ChangeClanHandle_PostgreSQLImpl *query = (ChangeClanHandle_PostgreSQLImpl*) dbFunctor;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::RemoveFromClan_DBQuery(SystemAddress systemAddress,LobbyDBSpec::RemoveFromClan_Data *dbFunctor, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved, unsigned int lobbyMsgId)
{
	RemoveFromClan_LS *query = (RemoveFromClan_LS*) dbFunctor;
	query->systemAddress=systemAddress;
	query->dissolveIfClanLeader=dissolveIfClanLeader;
	query->autoSendEmailIfClanDissolved=autoSendEmailIfClanDissolved;
	query->lobbyMsgId=lobbyMsgId;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::GetClans_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, const char *userHandle, bool ascendingSort)
{
	GetClans_LS *query = new GetClans_LS;
	query->systemAddress=systemAddress;
	query->ascendingSort=ascendingSort;
	if (userId!=0)
	{
		query->userId.hasDatabaseRowId=true;
		query->userId.databaseRowId=userId;
		query->userId.handle=userHandle;
	}
	else
		query->userId.handle=userHandle;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}
void LobbyServerPostgreSQL::UpdateClanMember_DBQuery(LobbyDBSpec::UpdateClanMember_Data *dbFunctor, unsigned int lobbyMsgId, SystemAddress systemAddress, LobbyDBSpec::DatabaseKey clanSourceMemberId, RakNet::RakString clanSourceMemberHandle)
{
	UpdateClanMember_LS *query = (UpdateClanMember_LS*) dbFunctor;
	query->lobbyMsgId=lobbyMsgId;
	query->systemAddress=systemAddress;
	query->clanSourceMemberId=clanSourceMemberId;
	query->clanSourceMemberHandle=clanSourceMemberHandle;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::AddPostToClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::AddToClanBoard_Data *addToClanBoard)
{
	AddToClanBoard_PostgreSQLImpl *query = (AddToClanBoard_PostgreSQLImpl*) addToClanBoard;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::RemovePostFromClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::RemoveFromClanBoard_Data *removeFromClanBoard)
{
	RemoveFromClanBoard_LS *query = (RemoveFromClanBoard_LS*) removeFromClanBoard;
	query->systemAddress=systemAddress;
	PushFunctor(query, 0);
}
void LobbyServerPostgreSQL::DownloadClanBoard_DBQuery(SystemAddress systemAddress, LobbyDBSpec::GetClanBoard_Data *removeFromClanBoard, LobbyNetworkOperations op)
{
	GetClanBoard_LS *query = (GetClanBoard_LS*) removeFromClanBoard;
	query->systemAddress=systemAddress;
	query->op=op;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);

}
void LobbyServerPostgreSQL::DownloadClanMember_DBQuery(SystemAddress systemAddress, LobbyDBSpec::DatabaseKey userId, bool ascendingSort){}
void LobbyServerPostgreSQL::ValidateUserKey_DBQuery(SystemAddress systemAddress, TitleValidationDBSpec::ValidateUserKey_Data *dbFunctor)
{
	ValidateUserKey_LS *query = (ValidateUserKey_LS*) dbFunctor;
	query->systemAddress=systemAddress;
	SystemAddress *context = new SystemAddress; // Deallocated by functionThread->SetPostResultFunction(DeleteSysAddrContext);
	*context=systemAddress;
	PushFunctor(query, context);
}

// Ranking server DB callbacks
void LobbyServerPostgreSQL::SubmitMatch_CB(SubmitMatch_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	SubmitMatch_LS *ls = (SubmitMatch_LS*) callResult;
	if (wasCancelled==false)
	{
		SubmitMatch_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}
void LobbyServerPostgreSQL::ModifyTrustedIPList_CB(ModifyTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{

}
void LobbyServerPostgreSQL::GetTrustedIPList_CB(GetTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (callResult->dbQuerySuccess)
	{
		// Take the pointers, they won't be deleted thanks to AddRef
		unsigned i;
		for (i=0; i < callResult->ipList.Size(); i++)
		{
			trustedIpList.Insert(callResult->ipList[i]);
			callResult->ipList[i]->AddRef();
		}
	}
	else
	{
		//printf("Warning: callResult->dbQuerySuccess==false in GetTrustedIPList_CB\n");
	} 	
}
void LobbyServerPostgreSQL::GetRatingForParticipant_CB(GetRatingForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	GetRatingForParticipant_LS *ls = (GetRatingForParticipant_LS*) callResult;
	if (wasCancelled==false)
	{
		GetRatingForParticipant_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}

void LobbyServerPostgreSQL::GetRatingForParticipants_CB(GetRatingForParticipants_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::GetHistoryForParticipant_CB(GetHistoryForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}


// Lobby DB callbacks
void LobbyServerPostgreSQL::CreateUser_CB(CreateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	CreateUser_PostgreSQLImpl *res = (CreateUser_PostgreSQLImpl *) callResult;
	if (wasCancelled==false)
		CreateUser_DBResult(res->dbQuerySuccess, res->queryOK, res->dbQuerySuccess ? res->queryResultString.C_String() : lobbyDb->GetLastError(), *((SystemAddress*)context), res);
}

void LobbyServerPostgreSQL::GetUser_CB(GetUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	GetUser_LS *res = (GetUser_LS *) callResult;
	if (wasCancelled==false)
	{
		if (res->action==GetUser_LS::GET_USER_LS_LOGIN)
			Login_DBResult(res->dbQuerySuccess, lobbyDb->GetLastError(), *((SystemAddress*)context), res, res->userPw_in.C_String());
		else if (res->action==GetUser_LS::GET_USER_LS_RETRIEVE_PASSWORD_RECOVERY_QUESTION)
			RetrievePasswordRecoveryQuestion_DBResult(res->dbQuerySuccess, lobbyDb->GetLastError(), *((SystemAddress*)context), res);
		else if (res->action==GetUser_LS::GET_USER_LS_RETRIEVE_PASSWORD)
			RetrievePassword_DBResult(res->dbQuerySuccess, lobbyDb->GetLastError(), *((SystemAddress*)context), res, res->passwordRecoveryAnswer.C_String());
		else
		{
			RakAssert(0);
		}
	}
}
void LobbyServerPostgreSQL::UpdateUser_CB(UpdateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::DeleteUser_CB(DeleteUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::ChangeUserHandle_CB(ChangeUserHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		UpdateHandle_DBResult(callResult);
	}
}

void LobbyServerPostgreSQL::AddAccountNote_CB(AddAccountNote_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::GetAccountNotes_CB(GetAccountNotes_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::AddFriend_CB(AddFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		AddFriend_LS *ls = (AddFriend_LS*) callResult;
		AddFriend_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->messageBody, ls->language);
	}
}

void LobbyServerPostgreSQL::RemoveFriend_CB(RemoveFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::GetFriends_CB(GetFriends_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		GetFriends_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, *((SystemAddress*) context));
	}
}

void LobbyServerPostgreSQL::AddToIgnoreList_CB(AddToIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		AddToIgnoreList_DBResult(callResult->dbQuerySuccess, callResult->success, callResult->dbQuerySuccess ? callResult->queryResult.C_String() : lobbyDb->GetLastError(),
			callResult->ignoredUser.databaseRowId, callResult->id.databaseRowId, callResult);
	}
}

void LobbyServerPostgreSQL::RemoveFromIgnoreList_CB(RemoveFromIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		RemoveFromIgnoreList_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(),
			callResult->ignoredUser.databaseRowId, callResult->id.databaseRowId);
	}
}

void LobbyServerPostgreSQL::GetIgnoreList_CB(GetIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		GetIgnoreList_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), *((SystemAddress*)context), callResult);
	}
}

void LobbyServerPostgreSQL::SendEmail_CB(SendEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		SendEmail_DBResult(callResult->dbQuerySuccess, callResult->validParameters,
			callResult->dbQuerySuccess ? callResult->failureMessage.C_String() : lobbyDb->GetLastError(),
			callResult);
	}
}

void LobbyServerPostgreSQL::GetEmails_CB(GetEmails_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		GetEmails_DBResult(callResult->dbQuerySuccess, callResult->validParameters,
			callResult->dbQuerySuccess ? callResult->failureMessage.C_String() : lobbyDb->GetLastError(),
			callResult);
	}
}

void LobbyServerPostgreSQL::DeleteEmail_CB(DeleteEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	DeleteEmail_LS *ls = (DeleteEmail_LS*) callResult;

	if (wasCancelled==false)
	{
		DeleteEmail_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), ls->myId, callResult);
	}
}

void LobbyServerPostgreSQL::UpdateEmailStatus_CB(UpdateEmailStatus_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	UpdateEmailStatus_LS *ls = (UpdateEmailStatus_LS*) callResult;

	if (wasCancelled==false)
	{
		UpdateEmailStatus_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), ls->myId, callResult);
	}
}

void LobbyServerPostgreSQL::GetHandleFromUserId_CB(GetHandleFromUserId_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::GetUserIdFromHandle_CB(GetUserIdFromHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::AddDisallowedHandle_CB(AddDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::RemoveDisallowedHandle_CB(RemoveDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::IsDisallowedHandle_CB(IsDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::AddToActionHistory_CB(AddToActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	AddToActionHistory_LS *ls = (AddToActionHistory_LS*) callResult;
	if (wasCancelled==false)
	{
		AddToActionHistory_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}

void LobbyServerPostgreSQL::GetActionHistory_CB(GetActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}
void LobbyServerPostgreSQL::UpdateClanMember_CB(UpdateClanMember_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	UpdateClanMember_LS *ls = (UpdateClanMember_LS*) callResult;
	if (wasCancelled==false)
	{
		UpdateClanMember_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->lobbyMsgId, ls->systemAddress, ls->clanSourceMemberId, ls->clanSourceMemberHandle);
	}
}
void LobbyServerPostgreSQL::UpdateClan_CB(UpdateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}
void LobbyServerPostgreSQL::CreateClan_CB(CreateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		CreateClan_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult);
	}
}
void LobbyServerPostgreSQL::ChangeClanHandle_CB(ChangeClanHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
		ChangeClanHandle_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult);
}
void LobbyServerPostgreSQL::DeleteClan_CB(DeleteClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}
void LobbyServerPostgreSQL::GetClans_CB(GetClans_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	GetClans_LS *ls = (GetClans_LS*) callResult;
	if (wasCancelled==false)
	{
		GetClans_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}
void LobbyServerPostgreSQL::RemoveFromClan_CB(RemoveFromClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	RemoveFromClan_LS *ls = (RemoveFromClan_LS*) callResult;
	if (wasCancelled==false)
	{
		RemoveFromClan_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress, ls->dissolveIfClanLeader, ls->autoSendEmailIfClanDissolved, ls->lobbyMsgId);
	}
}
void LobbyServerPostgreSQL::AddToClanBoard_CB(AddToClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (wasCancelled==false)
	{
		AddToClanBoard_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult);
	}
	
}
void LobbyServerPostgreSQL::RemoveFromClanBoard_CB(RemoveFromClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	RemoveFromClanBoard_LS *ls = (RemoveFromClanBoard_LS*) callResult;
	if (wasCancelled==false)
	{
		RemoveFromClanBoard_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}
void LobbyServerPostgreSQL::GetClanBoard_CB(GetClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	GetClanBoard_LS *ls = (GetClanBoard_LS*) callResult;
	if (wasCancelled==false)
	{
		GetClanBoard_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress, ls->op);
	}
}

// Title validation DB callbacks
void LobbyServerPostgreSQL::AddTitle_CB(AddTitle_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (callResult->dbQuerySuccess)
	{
		titles.Insert(callResult);
		callResult->AddRef();
	}
	else
	{
		printf("Add Title DB Query failure. %s", titleValidationDb->GetLastError());
	}
	
}

void LobbyServerPostgreSQL::GetTitles_CB(GetTitles_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	if (callResult->dbQuerySuccess)
	{
		// Take the pointers, they won't be deleted thanks to AddRef
		unsigned i;
		for (i=0; i < callResult->titles.Size(); i++)
		{
			titles.Insert(callResult->titles[i]);
			callResult->titles[i]->AddRef();
		}
	}
	else
	{
//		printf("ERROR: Lobby server has no titles. Be sure to add titles using the AddTitle_PostgreSQLImpl functor.\n");
//		printf("DB Error: %s\n", titleValidationDb->GetLastError());
		// Can't run without titles
		Shutdown();
	}
}

void LobbyServerPostgreSQL::UpdateUserKey_CB(UpdateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}

void LobbyServerPostgreSQL::GetUserKeys_CB(GetUserKeys_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
}
void LobbyServerPostgreSQL::ValidateUserKey_CB(ValidateUserKey_PostgreSQLImpl *callResult, bool wasCancelled, void *context)
{
	ValidateUserKey_LS *ls = (ValidateUserKey_LS*) callResult;
	if (wasCancelled==false)
	{
		ValidateUserKey_DBResult(callResult->dbQuerySuccess, lobbyDb->GetLastError(), callResult, ls->systemAddress);
	}
}
void LobbyServerPostgreSQL::RemoveUser(SystemAddress sysAddr)
{
	LobbyServer::RemoveUser(sysAddr);
	// No need to process input functors for a user that is no longer online.
	CancelFunctorsWithSysAddr(sysAddr);
}
void LobbyServerPostgreSQL::CancelFunctorsWithSysAddr(SystemAddress sysAddr)
{
	functionThread->CancelFunctorsWithContext(CancelWithSysAddr, &sysAddr);
}