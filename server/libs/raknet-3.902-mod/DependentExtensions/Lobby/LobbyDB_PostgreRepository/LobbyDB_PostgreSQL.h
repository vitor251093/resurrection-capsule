/// \file
/// \brief An implementation of the LobbyServer database to use PostgreSQL to store the relevant data
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __LOBBY_SERVER_POSTGRESQL_REPOSITORY_H
#define __LOBBY_SERVER_POSTGRESQL_REPOSITORY_H

#include "LobbyDBSpec.h"
#include "PostgreSQLInterface.h"
#include "FunctionThread.h"

struct pg_conn;
typedef struct pg_conn PGconn;

struct pg_result;
typedef struct pg_result PGresult;

class LobbyDB_PostgreSQL;
class LobbyDBCBInterface;

class LobbyDBFunctor : public RakNet::Functor
{
public:
	LobbyDBFunctor() {lobbyServer=0; callback=0; dbQuerySuccess=false;}
	static char* AllocBytes(int numBytes);
	static void FreeBytes(char *data);

	/// [in] Set this to a pointer to LobbyDB_PostgreSQL
	LobbyDB_PostgreSQL *lobbyServer;
	/// [in] Set this to a pointer to a callback if you wish to get notification calls. Otherwise you will have to override HandleResult()
	LobbyDBCBInterface *callback;
	/// [out] Whether or not the database query succeeded
	bool dbQuerySuccess;
};

class CreateUser_PostgreSQLImpl : public LobbyDBSpec::CreateUser_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static CreateUser_PostgreSQLImpl* Alloc(void);
	static void Free(CreateUser_PostgreSQLImpl *s);

	/// [out] Whether or not the result of the query was OK. Can fail on handle already in use. dbQuerySuccess must be true for this to have meaning.
	bool queryOK;
	RakNet::RakString queryResultString;

	static void EncodeQueryInput(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &paramTypeStr, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateCaptions(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateOther(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdatePermissions(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateAccountNumber(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateAdminLevel(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateAccountBalance(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateCCInfo(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateBinaryData(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdatePersonalInfo(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdateEmailAddr(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
	static void EncodeQueryUpdatePassword(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat);
};

class GetUser_PostgreSQLImpl : public LobbyDBSpec::GetUser_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetUser_PostgreSQLImpl* Alloc(void);
	static void Free(GetUser_PostgreSQLImpl *s);
};

class UpdateUser_PostgreSQLImpl : public LobbyDBSpec::UpdateUser_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static UpdateUser_PostgreSQLImpl* Alloc(void);
	static void Free(UpdateUser_PostgreSQLImpl *s);
};

class DeleteUser_PostgreSQLImpl : public LobbyDBSpec::DeleteUser_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static DeleteUser_PostgreSQLImpl* Alloc(void);
	static void Free(DeleteUser_PostgreSQLImpl *s);
};


class ChangeUserHandle_PostgreSQLImpl : public LobbyDBSpec::ChangeUserHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static ChangeUserHandle_PostgreSQLImpl* Alloc(void);
	static void Free(ChangeUserHandle_PostgreSQLImpl *s);
};


class AddAccountNote_PostgreSQLImpl : public LobbyDBSpec::AddAccountNote_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddAccountNote_PostgreSQLImpl* Alloc(void);
	static void Free(AddAccountNote_PostgreSQLImpl *s);
};

class GetAccountNotes_PostgreSQLImpl : public LobbyDBSpec::GetAccountNotes_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetAccountNotes_PostgreSQLImpl* Alloc(void);
	static void Free(GetAccountNotes_PostgreSQLImpl *s);
};

class AddFriend_PostgreSQLImpl : public LobbyDBSpec::AddFriend_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddFriend_PostgreSQLImpl* Alloc(void);
	static void Free(AddFriend_PostgreSQLImpl *s);
};

class RemoveFriend_PostgreSQLImpl : public LobbyDBSpec::RemoveFriend_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static RemoveFriend_PostgreSQLImpl* Alloc(void);
	static void Free(RemoveFriend_PostgreSQLImpl *s);
};

class GetFriends_PostgreSQLImpl : public LobbyDBSpec::GetFriends_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetFriends_PostgreSQLImpl* Alloc(void);
	static void Free(GetFriends_PostgreSQLImpl *s);
};

class AddToIgnoreList_PostgreSQLImpl : public LobbyDBSpec::AddToIgnoreList_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddToIgnoreList_PostgreSQLImpl* Alloc(void);
	static void Free(AddToIgnoreList_PostgreSQLImpl *s);
};

class RemoveFromIgnoreList_PostgreSQLImpl : public LobbyDBSpec::RemoveFromIgnoreList_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static RemoveFromIgnoreList_PostgreSQLImpl* Alloc(void);
	static void Free(RemoveFromIgnoreList_PostgreSQLImpl *s);
};

class GetIgnoreList_PostgreSQLImpl : public LobbyDBSpec::GetIgnoreList_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetIgnoreList_PostgreSQLImpl* Alloc(void);
	static void Free(GetIgnoreList_PostgreSQLImpl *s);
};

class SendEmail_PostgreSQLImpl : public LobbyDBSpec::SendEmail_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static SendEmail_PostgreSQLImpl* Alloc(void);
	static void Free(SendEmail_PostgreSQLImpl *s);
};

class GetEmails_PostgreSQLImpl : public LobbyDBSpec::GetEmails_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetEmails_PostgreSQLImpl* Alloc(void);
	static void Free(GetEmails_PostgreSQLImpl *s);
};

class DeleteEmail_PostgreSQLImpl : public LobbyDBSpec::DeleteEmail_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static DeleteEmail_PostgreSQLImpl* Alloc(void);
	static void Free(DeleteEmail_PostgreSQLImpl *s);
};

class UpdateEmailStatus_PostgreSQLImpl : public LobbyDBSpec::UpdateEmailStatus_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static UpdateEmailStatus_PostgreSQLImpl* Alloc(void);
	static void Free(UpdateEmailStatus_PostgreSQLImpl *s);
};

class GetHandleFromUserId_PostgreSQLImpl : public LobbyDBSpec::GetHandleFromUserId_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetHandleFromUserId_PostgreSQLImpl* Alloc(void);
	static void Free(GetHandleFromUserId_PostgreSQLImpl *s);
};

class GetUserIdFromHandle_PostgreSQLImpl : public LobbyDBSpec::GetUserIdFromHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetUserIdFromHandle_PostgreSQLImpl* Alloc(void);
	static void Free(GetUserIdFromHandle_PostgreSQLImpl *s);
};

class AddDisallowedHandle_PostgreSQLImpl : public LobbyDBSpec::AddDisallowedHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddDisallowedHandle_PostgreSQLImpl* Alloc(void);
	static void Free(AddDisallowedHandle_PostgreSQLImpl *s);
};

class RemoveDisallowedHandle_PostgreSQLImpl : public LobbyDBSpec::RemoveDisallowedHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static RemoveDisallowedHandle_PostgreSQLImpl* Alloc(void);
	static void Free(RemoveDisallowedHandle_PostgreSQLImpl *s);
};

class IsDisallowedHandle_PostgreSQLImpl : public LobbyDBSpec::IsDisallowedHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static IsDisallowedHandle_PostgreSQLImpl* Alloc(void);
	static void Free(IsDisallowedHandle_PostgreSQLImpl *s);
};

class AddToActionHistory_PostgreSQLImpl : public LobbyDBSpec::AddToActionHistory_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddToActionHistory_PostgreSQLImpl* Alloc(void);
	static void Free(AddToActionHistory_PostgreSQLImpl *s);
};

class GetActionHistory_PostgreSQLImpl : public LobbyDBSpec::GetActionHistory_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetActionHistory_PostgreSQLImpl* Alloc(void);
	static void Free(GetActionHistory_PostgreSQLImpl *s);
};

// -------------------------------------------------------------------------------------
// CLANS
// -------------------------------------------------------------------------------------

class UpdateClanMember_PostgreSQLImpl : public LobbyDBSpec::UpdateClanMember_Data, public LobbyDBFunctor
{	
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static UpdateClanMember_PostgreSQLImpl* Alloc(void);
	static void Free(UpdateClanMember_PostgreSQLImpl *s);
};

class UpdateClan_PostgreSQLImpl : public LobbyDBSpec::UpdateClan_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static UpdateClan_PostgreSQLImpl* Alloc(void);
	static void Free(UpdateClan_PostgreSQLImpl *s);
};

class CreateClan_PostgreSQLImpl : public LobbyDBSpec::CreateClan_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static CreateClan_PostgreSQLImpl* Alloc(void);
	static void Free(CreateClan_PostgreSQLImpl *s);
};

class ChangeClanHandle_PostgreSQLImpl : public LobbyDBSpec::ChangeClanHandle_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static ChangeClanHandle_PostgreSQLImpl* Alloc(void);
	static void Free(ChangeClanHandle_PostgreSQLImpl *s);
};

class DeleteClan_PostgreSQLImpl : public LobbyDBSpec::DeleteClan_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static DeleteClan_PostgreSQLImpl* Alloc(void);
	static void Free(DeleteClan_PostgreSQLImpl *s);
};

class GetClans_PostgreSQLImpl : public LobbyDBSpec::GetClans_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetClans_PostgreSQLImpl* Alloc(void);
	static void Free(GetClans_PostgreSQLImpl *s);
};

class RemoveFromClan_PostgreSQLImpl : public LobbyDBSpec::RemoveFromClan_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static RemoveFromClan_PostgreSQLImpl* Alloc(void);
	static void Free(RemoveFromClan_PostgreSQLImpl *s);
};

class AddToClanBoard_PostgreSQLImpl : public LobbyDBSpec::AddToClanBoard_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static AddToClanBoard_PostgreSQLImpl* Alloc(void);
	static void Free(AddToClanBoard_PostgreSQLImpl *s);
};

class RemoveFromClanBoard_PostgreSQLImpl : public LobbyDBSpec::RemoveFromClanBoard_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static RemoveFromClanBoard_PostgreSQLImpl* Alloc(void);
	static void Free(RemoveFromClanBoard_PostgreSQLImpl *s);
};

class GetClanBoard_PostgreSQLImpl : public LobbyDBSpec::GetClanBoard_Data, public LobbyDBFunctor
{
public:
	virtual void Process(void *context); /// Does SQL Processing
	/// Calls the callback specified by LobbyDBFunctor::callback (which you set) or LobbyDB_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);
	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetClanBoard_PostgreSQLImpl* Alloc(void);
	static void Free(GetClanBoard_PostgreSQLImpl *s);
};

/// If you want to forward Functor::HandleResult() the output to a callback (which is what it does by default), you can use this
class RAK_DLL_EXPORT LobbyDBCBInterface
{
public:
	virtual void CreateUser_CB(CreateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetUser_CB(GetUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void UpdateUser_CB(UpdateUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void DeleteUser_CB(DeleteUser_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void ChangeUserHandle_CB(ChangeUserHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddAccountNote_CB(AddAccountNote_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetAccountNotes_CB(GetAccountNotes_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddFriend_CB(AddFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void RemoveFriend_CB(RemoveFriend_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetFriends_CB(GetFriends_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddToIgnoreList_CB(AddToIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void RemoveFromIgnoreList_CB(RemoveFromIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetIgnoreList_CB(GetIgnoreList_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void SendEmail_CB(SendEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetEmails_CB(GetEmails_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void DeleteEmail_CB(DeleteEmail_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void UpdateEmailStatus_CB(UpdateEmailStatus_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetHandleFromUserId_CB(GetHandleFromUserId_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetUserIdFromHandle_CB(GetUserIdFromHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddDisallowedHandle_CB(AddDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void RemoveDisallowedHandle_CB(RemoveDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void IsDisallowedHandle_CB(IsDisallowedHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddToActionHistory_CB(AddToActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetActionHistory_CB(GetActionHistory_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	// CLANS
	virtual void UpdateClanMember_CB(UpdateClanMember_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void UpdateClan_CB(UpdateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void CreateClan_CB(CreateClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void ChangeClanHandle_CB(ChangeClanHandle_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void DeleteClan_CB(DeleteClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetClans_CB(GetClans_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void RemoveFromClan_CB(RemoveFromClan_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void AddToClanBoard_CB(AddToClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void RemoveFromClanBoard_CB(RemoveFromClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetClanBoard_CB(GetClanBoard_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
};

/// Provides a central class for all LobbyServerDatabase functionality using the PostgreSQL interface
/// If you are using more than one class that uses functionThreads, you should maintain this separately and call AssignFunctionThread() to avoid unnecessary threads.
class RAK_DLL_EXPORT LobbyDB_PostgreSQL :
	public PostgreSQLInterface, // Gives the class C++ functions to access PostgreSQL
	public RakNet::FunctionThreadDependentClass // Adds a function thread member.
{
public:
	LobbyDB_PostgreSQL();
	~LobbyDB_PostgreSQL();

	/// Create the tables used by the ranking server, for all applications.  Call this first.
	/// I recommend using UTF8 for the database encoding within PostgreSQL if you are going to store binary data
	/// \return True on success, false on failure.
	bool CreateLobbyServerTables(void);

	/// Destroy the tables used by the ranking server.  Don't call this unless you don't want to use the ranking server anymore, or are testing.
	/// \return True on success, false on failure.
	bool DestroyLobbyServerTables(void);

	/// Push one of the above *_PostgreSQLImpl functors to run.
	/// \param functor A structure allocated on the HEAP (using new) with the input parameters filled in.
	virtual void PushFunctor(LobbyDBFunctor *functor, void *context);

	/// Assigns a callback to get the results of processing.
	/// \param[in] A structure allocated on the HEAP (using new) with the input parameters filled in.
	void AssignCallback(LobbyDBCBInterface *cb);

protected:
	friend class CreateUser_PostgreSQLImpl;
	friend class GetUser_PostgreSQLImpl;
	friend class UpdateUser_PostgreSQLImpl;
	friend class DeleteUser_PostgreSQLImpl;
	friend class ChangeUserHandle_PostgreSQLImpl;
	friend class AddAccountNote_PostgreSQLImpl;
	friend class GetAccountNotes_PostgreSQLImpl;
	friend class AddFriend_PostgreSQLImpl;
	friend class RemoveFriend_PostgreSQLImpl;
	friend class GetFriends_PostgreSQLImpl;
	friend class AddToIgnoreList_PostgreSQLImpl;
	friend class RemoveFromIgnoreList_PostgreSQLImpl;
	friend class GetIgnoreList_PostgreSQLImpl;
	friend class SendEmail_PostgreSQLImpl;
	friend class GetEmails_PostgreSQLImpl;
	friend class DeleteEmail_PostgreSQLImpl;
	friend class UpdateEmailStatus_PostgreSQLImpl;
	friend class GetHandleFromUserId_PostgreSQLImpl;
	friend class GetUserIdFromHandle_PostgreSQLImpl;
	friend class AddDisallowedHandle_PostgreSQLImpl;
	friend class RemoveDisallowedHandle_PostgreSQLImpl;
	friend class IsDisallowedHandle_PostgreSQLImpl;
	friend class AddToActionHistory_PostgreSQLImpl;
	friend class GetActionHistory_PostgreSQLImpl;
	
	// Clans
	friend class UpdateClanMember_PostgreSQLImpl;
	friend class UpdateClan_PostgreSQLImpl;
	friend class CreateClan_PostgreSQLImpl;
	friend class ChangeClanHandle_PostgreSQLImpl;
	friend class DeleteClan_PostgreSQLImpl; 
	friend class GetClans_PostgreSQLImpl;
	friend class RemoveFromClan_PostgreSQLImpl;
	friend class AddToClanBoard_PostgreSQLImpl;
	friend class RemoveFromClanBoard_PostgreSQLImpl;
	friend class GetClanBoard_PostgreSQLImpl;

	LobbyDBCBInterface *lobbyServerCallback;

	bool IsValidUserID(LobbyDBSpec::DatabaseKey userId);
	bool GetUserIDFromHandle(const RakNet::RakString &handle,LobbyDBSpec::DatabaseKey &userId);
	bool GetClanIDFromHandle(const RakNet::RakString &handle,LobbyDBSpec::DatabaseKey &clanId);
	bool GetHandleFromClanID(RakNet::RakString &handle,LobbyDBSpec::DatabaseKey &clanId);
};

#endif
