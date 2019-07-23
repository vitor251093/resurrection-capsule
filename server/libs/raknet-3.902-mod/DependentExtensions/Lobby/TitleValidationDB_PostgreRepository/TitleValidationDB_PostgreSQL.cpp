/// \file
/// \brief An implementation of TitleValidationDBSpec.h to use PostgreSQL. Title validation is useful for checking server logins and cd keys.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "TitleValidationDB_PostgreSQL.h"
// libpq-fe.h is part of PostgreSQL which must be installed on this computer to use the PostgreRepository
#include "libpq-fe.h"
#include <RakAssert.h>
#include "FormatString.h"

#define PQEXECPARAM_FORMAT_TEXT		0
#define PQEXECPARAM_FORMAT_BINARY	1

using namespace TitleValidationDBSpec;

char* TitleValidationFunctor::AllocBytes(int numBytes) {return new char[numBytes];}
void TitleValidationFunctor::FreeBytes(char *data) {delete [] data;}

TitleValidation_PostgreSQL::TitleValidation_PostgreSQL()
{
	titleValidationCallback=0;
}
TitleValidation_PostgreSQL::~TitleValidation_PostgreSQL()
{
}
bool TitleValidation_PostgreSQL::CreateTitleValidationTables(void)
{
	if (isConnected==false)
		return false;

	const char *createTablesStr =
		"CREATE TABLE titles ("
		"titleId_pk serial UNIQUE,"
		"titleName text,"
		"titlePassword bytea NOT NULL UNIQUE,"
		"allowClientAccountCreation boolean,"
		"lobbyIsGameIndependent boolean,"
		"requireUserKeyToLogon boolean,"
		"defaultAllowUpdateHandle boolean,"
		"defaultAllowUpdateCCInfo boolean,"
		"defaultAllowUpdateAccountNumber boolean,"
		"defaultAllowUpdateAdminLevel boolean,"
		"defaultAllowUpdateAccountBalance boolean,"
		"defaultAllowClientsUploadActionHistory boolean,"
		"defaultPermissionsStr text,"
		"creationDate bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp));"
		"\n "
		"CREATE TABLE userKeys ("
		"userKeyId_pk serial UNIQUE,"
		"titleId_fk integer REFERENCES titles (titleId_pk) ON DELETE CASCADE,"
		"keyPassword bytea UNIQUE,"
		"userId integer,"
		"userIP text,"
		"numRegistrations integer,"
		"invalidKey boolean,"
		"invalidKeyReason text,"
		"binaryData bytea,"
		"creationDate bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp));"		
		;

	PGresult *result;
	bool res = ExecuteBlockingCommand(createTablesStr, &result, false);
	PQclear(result);
	return res;
}
bool TitleValidation_PostgreSQL::DestroyTitleValidationTables(void)
{
	if (isConnected==false)
		return false;

	const char *dropTablesStr =
		"DROP TABLE titles CASCADE; "
		"DROP TABLE userKeys CASCADE;"
		;

	PGresult *result;
	bool res = ExecuteBlockingCommand(dropTablesStr, &result, false);
	PQclear(result);
	return res;
}
void TitleValidation_PostgreSQL::PushFunctor(TitleValidationFunctor *functor, void *context)
{
	functor->titleValidationServer=this;
	functor->callback=titleValidationCallback;
	FunctionThreadDependentClass::PushFunctor(functor,context);
}
void TitleValidation_PostgreSQL::AssignCallback(TitleValidationDBCBInterface *cb)
{
	titleValidationCallback=cb;
}

void AddTitle_PostgreSQLImpl::Process(void *context)
{
	if (titlePassword==0 || titlePasswordLength==0)
		return;

	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "INSERT INTO titles (";
	PostgreSQLInterface::EncodeQueryInput("titleName", titleName, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("titlePassword", titlePassword, titlePasswordLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("allowClientAccountCreation", allowClientAccountCreation, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("lobbyIsGameIndependent", lobbyIsGameIndependent, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("requireUserKeyToLogon", requireUserKeyToLogon, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowUpdateHandle", defaultAllowUpdateHandle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowUpdateCCInfo", defaultAllowUpdateCCInfo, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowUpdateAccountNumber", defaultAllowUpdateAccountNumber, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowUpdateAdminLevel", defaultAllowUpdateAdminLevel, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowUpdateAccountBalance", defaultAllowUpdateAccountBalance, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultAllowClientsUploadActionHistory", defaultAllowClientsUploadActionHistory, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("defaultPermissionsStr", defaultPermissionsStr, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(titleValidationServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=titleValidationServer->IsResultSuccessful(result, false);
	PQclear(result);

}
void AddTitle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddTitle_CB(this, wasCancelled, context);
	Deref();
}
AddTitle_PostgreSQLImpl* AddTitle_PostgreSQLImpl::Alloc(void) {return new AddTitle_PostgreSQLImpl;}
void AddTitle_PostgreSQLImpl::Free(AddTitle_PostgreSQLImpl *s) {delete s;}
	
void GetTitles_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "SELECT * FROM titles;";

	result = PQexecParams(titleValidationServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=titleValidationServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	AddTitle_Data *item;
	if (dbQuerySuccess)
	{
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			item = new TitleValidationDBSpec::AddTitle_Data;
			PostgreSQLInterface::PQGetValueFromBinary(&item->titleId, result, i, "titleId_pk");
			PostgreSQLInterface::PQGetValueFromBinary(&item->titleName, result, i, "titleName");
			PostgreSQLInterface::PQGetValueFromBinary(&item->titlePassword, &item->titlePasswordLength, result, i, "titlePassword");
			PostgreSQLInterface::PQGetValueFromBinary(&item->allowClientAccountCreation, result, i, "allowClientAccountCreation");
			PostgreSQLInterface::PQGetValueFromBinary(&item->lobbyIsGameIndependent, result, i, "lobbyIsGameIndependent");
			PostgreSQLInterface::PQGetValueFromBinary(&item->requireUserKeyToLogon, result, i, "requireUserKeyToLogon");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowUpdateHandle, result, i, "defaultAllowUpdateHandle");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowUpdateCCInfo, result, i, "defaultAllowUpdateCCInfo");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowUpdateAccountNumber, result, i, "defaultAllowUpdateAccountNumber");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowUpdateAdminLevel, result, i, "defaultAllowUpdateAdminLevel");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowUpdateAccountBalance, result, i, "defaultAllowUpdateAccountBalance");
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultAllowClientsUploadActionHistory, result, i, "defaultAllowClientsUploadActionHistory");			
			PostgreSQLInterface::PQGetValueFromBinary(&item->defaultPermissionsStr, result, i, "defaultPermissionsStr");
			PostgreSQLInterface::PQGetValueFromBinary(&item->creationDate, result, i, "creationDate");
			
			titles.Insert(item);
		}
	}
}
void GetTitles_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetTitles_CB(this, wasCancelled, context);
	Deref();
}
GetTitles_PostgreSQLImpl* GetTitles_PostgreSQLImpl::Alloc(void) {return new GetTitles_PostgreSQLImpl;}
void GetTitles_PostgreSQLImpl::Free(GetTitles_PostgreSQLImpl *s) {delete s;}
	
void UpdateUserKey_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (userKeyIsSet)
	{
		query="UPDATE userKeys SET ";
		PostgreSQLInterface::EncodeQueryInput("titleId_fk", titleId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		if (paramTypeStr.IsEmpty()==false) {query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("keyPassword", keyPassword, keyPasswordLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("userId", userId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("userIP", userIP, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("numRegistrations", numRegistrations, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("invalidKey", invalidKey, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("invalidKeyReason", invalidKeyReason, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}
		PostgreSQLInterface::EncodeQueryInput("binaryData", binaryData, binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (paramTypeStr.IsEmpty()==false) {query+=", "; query+=paramTypeStr; query+="="; query+=valueStr; paramTypeStr.Clear(); valueStr.Clear();}

		query+=FormatString(" WHERE userKeyId_pk=%i;", userKeyId);
	}
	else
	{
		query = "INSERT INTO userKeys (";
		PostgreSQLInterface::EncodeQueryInput("titleId_fk", titleId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("keyPassword", keyPassword, keyPasswordLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("userId", userId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("userIP", userIP, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("numRegistrations", numRegistrations, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("invalidKey", invalidKey, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("invalidKeyReason", invalidKeyReason, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("binaryData", binaryData, binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		query+=paramTypeStr;
		query+=") VALUES (";
		query+=valueStr;
		query+=") RETURNING userKeyId_pk;";
	}

	result = PQexecParams(titleValidationServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=titleValidationServer->IsResultSuccessful(result, false);

	if (userKeyIsSet==false)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&userKeyId, result, 0, "userKeyId_pk");
	}

	PQclear(result);
}
void UpdateUserKey_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->UpdateUserKey_CB(this, wasCancelled, context);
	Deref();
}
UpdateUserKey_PostgreSQLImpl* UpdateUserKey_PostgreSQLImpl::Alloc(void) {return new UpdateUserKey_PostgreSQLImpl;}
void UpdateUserKey_PostgreSQLImpl::Free(UpdateUserKey_PostgreSQLImpl *s) {delete s;}

void GetUserKeys_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "SELECT userKeyId_pk,titleId_fk,keyPassword,userId, userIP, numRegistrations, invalidKey, invalidKeyReason, binaryData, creationDate FROM userKeys;";

	result = PQexecParams(titleValidationServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=titleValidationServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	UpdateUserKey_Data *item;
	if (dbQuerySuccess)
	{
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			item = new TitleValidationDBSpec::UpdateUserKey_Data;
			PostgreSQLInterface::PQGetValueFromBinary(&item->userKeyId, result, i, "userKeyId_pk");
			PostgreSQLInterface::PQGetValueFromBinary(&item->titleId, result, i, "titleId_fk");
			PostgreSQLInterface::PQGetValueFromBinary(&item->keyPassword, &item->keyPasswordLength, result, i, "keyPassword");
			PostgreSQLInterface::PQGetValueFromBinary(&item->userId, result, i, "userId");
			PostgreSQLInterface::PQGetValueFromBinary(&item->userIP, result, i, "userIP");
			PostgreSQLInterface::PQGetValueFromBinary(&item->numRegistrations, result, i, "numRegistrations");
			PostgreSQLInterface::PQGetValueFromBinary(&item->invalidKey, result, i, "invalidKey");
			PostgreSQLInterface::PQGetValueFromBinary(&item->invalidKeyReason, result, i, "invalidKeyReason");
			PostgreSQLInterface::PQGetValueFromBinary(&item->binaryData, &item->binaryDataLength, result, i, "binaryData");
			PostgreSQLInterface::PQGetValueFromBinary(&item->creationDate, result, i, "creationDate");

			keys.Insert(item);
		}
	}
}
void GetUserKeys_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetUserKeys_CB(this, wasCancelled, context);
	Deref();
}
GetUserKeys_PostgreSQLImpl* GetUserKeys_PostgreSQLImpl::Alloc(void) {return new GetUserKeys_PostgreSQLImpl;}
void GetUserKeys_PostgreSQLImpl::Free(GetUserKeys_PostgreSQLImpl *s) {delete s;}

void ValidateUserKey_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	successful=-3;

	query = FormatString("SELECT invalidKey, invalidKeyReason, binaryData FROM userKeys WHERE titleId_fk=%i AND ", titleId);
	PostgreSQLInterface::EncodeQueryInput("keyPassword", keyPassword, keyPasswordLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	query+=paramTypeStr;
	query+="=";
	query+=valueStr;
	query+=";";

	result = PQexecParams(titleValidationServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=titleValidationServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	if (dbQuerySuccess)
	{
		if (numRowsReturned==0)
		{
			successful=-1;
			return;
		}

		bool invalidKey;
		PostgreSQLInterface::PQGetValueFromBinary(&invalidKey, result, 0, "invalidKey");
		PostgreSQLInterface::PQGetValueFromBinary(&invalidKeyReason, result, 0, "invalidKeyReason");
		if (invalidKey)
			successful=-2;
		else
			successful=0;
	}
}
void ValidateUserKey_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->ValidateUserKey_CB(this, wasCancelled, context);
	Deref();
}
ValidateUserKey_PostgreSQLImpl* ValidateUserKey_PostgreSQLImpl::Alloc(void) {return new ValidateUserKey_PostgreSQLImpl;}
void ValidateUserKey_PostgreSQLImpl::Free(ValidateUserKey_PostgreSQLImpl *s) {delete s;}