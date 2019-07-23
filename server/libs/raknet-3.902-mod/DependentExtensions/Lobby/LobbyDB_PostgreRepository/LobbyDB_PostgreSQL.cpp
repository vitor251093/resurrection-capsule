/// \file
/// \brief An implementation of the LobbyServer database to use PostgreSQL to store the relevant data
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "LobbyDB_PostgreSQL.h"
// libpq-fe.h is part of PostgreSQL which must be installed on this computer to use the PostgreRepository
#include "libpq-fe.h"
#include <RakAssert.h>
#include "FormatString.h"

#define PQEXECPARAM_FORMAT_TEXT		0
#define PQEXECPARAM_FORMAT_BINARY	1

using namespace LobbyDBSpec;

char* LobbyDBFunctor::AllocBytes(int numBytes) {return new char[numBytes];}
void LobbyDBFunctor::FreeBytes(char *data) {delete [] data;}

LobbyDB_PostgreSQL::LobbyDB_PostgreSQL()
{
	lobbyServerCallback=0;
}
LobbyDB_PostgreSQL::~LobbyDB_PostgreSQL()
{
}
bool LobbyDB_PostgreSQL::CreateLobbyServerTables(void)
{
	if (isConnected==false)
		return false;

	const char *createTablesStr =
		"BEGIN;\n"
		"\n"
		"CREATE LANGUAGE plpgsql;\n"
		"CREATE TABLE users (\n"
		"userId_pk serial PRIMARY KEY UNIQUE,\n"
		"handleID_fk integer UNIQUE,\n"
		"firstName text,\n"
		"middleName text,\n"
		"lastName text,\n"
		"race text,\n"
		"sex text,\n"
		"homeAddress1 text,\n"
		"homeAddress2 text,\n"
		"homeCity text,\n"
		"homeState text,\n"
		"homeProvince text,\n"
		"homeCountry text,\n"
		"homeZipCode text,\n"
		"billingAddress1 text,\n"
		"billingAddress2 text,\n"
		"billingCity text,\n"
		"billingState text,\n"
		"billingProvince text,\n"
		"billingCountry text,\n"
		"billingZipCode text,\n"
		"emailAddress text,\n"
		"password text,\n"
		"passwordRecoveryQuestion text,\n"
		"passwordRecoveryAnswer text,\n"
		"caption1 text,\n"
		"caption2 text,\n"
		"caption3 text,\n"
		"dateOfBirth text,\n"
		"accountNumber integer,\n"
		"creditCardNumber text,\n"
		"creditCardExpiration text,\n"
		"creditCardCVV text,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"adminLevel text,\n"
		"permissions text,\n"
		"isBanned boolean,\n"
		"isSuspended boolean,\n"
		"banOrSuspensionReason text,\n"
		"suspensionExpiration bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"accountBalance float4,\n"
		"sourceIPAddress text,\n"
		"binaryData bytea\n"
		");\n"
		"\n"
		"CREATE TABLE accountNotes (\n"
		"userID_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"moderatorId integer,\n"
		"moderatorUserName text,\n"
		"time bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"type text,\n"
		"subject text,\n"
		"body text);\n"
		"\n"
		"CREATE TABLE friends (\n"
		"userID_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"friendUserId_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp));\n"
		"\n"
		"CREATE TABLE ignoreList (\n"
		"userID_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"ignoreUserID_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"actions text);\n"
		"\n"
		"CREATE TABLE emails (\n"
		"emailMessageID_pk serial PRIMARY KEY UNIQUE,\n"
		"userIDOwner_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"userIDOther_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"subject text,\n"
		"body text,\n"
		"date bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"attachment bytea,\n"
		"wasOpened boolean DEFAULT FALSE,\n"
		"inbox boolean,\n"
		"status integer DEFAULT 0);\n"
		"\n"
		"CREATE TABLE handles (\n"
		"handleID_pk serial PRIMARY KEY UNIQUE NOT NULL,\n"
		"userID_fk integer UNIQUE NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"handle text UNIQUE NOT NULL);\n"
		"CREATE UNIQUE INDEX handles_lower_handle_idx ON handles (lower(handle));"
		"\n"
		"CREATE TABLE clans (\n"
		"clanId_pk serial PRIMARY KEY UNIQUE,\n"
		"int_1 integer,\n"
		"int_2 integer,\n"
		"int_3 integer,\n"
		"int_4 integer,\n"
		"float_1 float4,\n"
		"float_2 float4,\n"
		"float_3 float4,\n"
		"float_4 float4,\n"
		"requiresInvitationsToJoin boolean,\n"
		"handleID_fk integer UNIQUE,\n"
		"description_1 text,\n"
		"description_2 text,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"binaryData bytea);\n"
		"\n"
		"CREATE TABLE clanHandles (\n"
		"handleID_pk serial PRIMARY KEY UNIQUE NOT NULL,\n"
		"clanID_fk integer UNIQUE NOT NULL REFERENCES clans (clanId_pk) ON DELETE CASCADE,\n"
		"handle text UNIQUE NOT NULL);\n"
		"CREATE UNIQUE INDEX clanHandles_lower_handle_idx ON clanHandles (lower(handle));"
		"\n"
		"CREATE TABLE disallowedHandles (\n"
		"handle text UNIQUE NOT NULL);\n"
		"CREATE UNIQUE INDEX disallowedHandles_lower_handle_idx ON disallowedHandles (lower(handle));"
		"\n"
		"CREATE TABLE actionHistory (\n"
		"userID_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"actionTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"actionTaken text,\n"
		"description text,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"sourceIPAddress text);\n"
		"\n"
		"CREATE TABLE clanMembers (\n"
		"clanMemberId_pk serial PRIMARY KEY UNIQUE NOT NULL,\n"
		"clanId_fk integer NOT NULL REFERENCES clans (clanId_pk) ON DELETE CASCADE,\n"
		"userId_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"mEStatus1 integer NOT NULL,\n"
		"int_1 integer,\n"
		"int_2 integer,\n"
		"int_3 integer,\n"
		"int_4 integer,\n"
		"float_1 float4,\n"
		"float_2 float4,\n"
		"float_3 float4,\n"
		"float_4 float4,\n"
		"description_1 text,\n"
		"description_2 text,\n"
		"lastUpdate bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"binaryData bytea,\n"
		"UNIQUE (userId_fk, clanId_fk)\n"
		");\n"
		"\n"
		"CREATE TABLE clanBoardPosts (\n"
		"postId_pk serial PRIMARY KEY UNIQUE,\n"
		"clanId_fk integer NOT NULL REFERENCES clans (clanId_pk) ON DELETE CASCADE,\n"
		"userId_fk integer NOT NULL REFERENCES users (userId_pk) ON DELETE CASCADE,\n"
		"subject text,\n"
		"body text,\n"
		"int_1 integer,\n"
		"int_2 integer,\n"
		"int_3 integer,\n"
		"int_4 integer,\n"
		"float_1 float4,\n"
		"float_2 float4,\n"
		"float_3 float4,\n"
		"float_4 float4,\n"
		"creationTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),\n"
		"binaryData bytea\n"
		");\n"
		"\n"
		"create or replace function IsUsedHandle(h text) returns boolean as $$\n"
		"declare\n"
		"	num_matches integer;\n"
		"begin\n"
		"	num_matches := COUNT(*) from handles where (lower(handles.handle)) = (lower(h));\n"
		"	return num_matches > 0;\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"create or replace function IsUsedClanHandle(h text) returns boolean as $$\n"
		"declare\n"
		"	num_matches integer;\n"
		"begin\n"
		"	num_matches := COUNT(*) from clanHandles where (lower(clanHandles.handle)) = (lower(h));\n"
		"	return num_matches > 0;\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"create or replace function IsDisallowedHandle(h text) returns boolean as $$\n"
		"declare\n"
		"	num_matches integer;\n"
		"begin\n"
		"	num_matches := COUNT(*) from disallowedHandles where (lower(disallowedHandles.handle)) = (lower(h));\n"
		"	return num_matches > 0;\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"create or replace function GetHandleFromUserId(userId integer) returns text as $$\n"
		"declare\n"
		"	output_row text;\n"
		"begin\n"
		"	return (SELECT handle FROM handles WHERE handles.userid_fk=userId LIMIT 1);\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"create or replace function GetUserIdFromHandle(handle_in text) returns integer as $$\n"
		"declare\n"
		"begin\n"
		"	return (SELECT userId_fk FROM handles WHERE (lower(handles.handle))=(lower(handle_in)) LIMIT 1);\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"CREATE OR REPLACE function ChangeUserHandle(\n"
		"			userId_in integer,\n"
		"			handle_in text) returns text\n"
		"		as $$\n"
		"		declare\n"
		"		begin\n"
		"		IF IsUsedHandle(handle_in) THEN\n"
		"		return 'Error: Handle already in use.';\n"
		"		ELSIF IsDisallowedHandle(handle_in) THEN\n"
		"		return 'Error: Handle is not allowed.';\n"
		"		END IF;\n"
		"		\n"
		"		UPDATE handles set handle=handle_in WHERE userId_fk=userId_in;\n"
		"		\n"
		"		return 'OK';\n"
		"		end;\n"
		"		$$ LANGUAGE plpgsql;\n"
		"\n"
		"CREATE OR REPLACE function ChangeClanHandle(\n"
		"			clanId_in integer,\n"
		"			handle_in text) returns text\n"
		"		as $$\n"
		"		declare\n"
		"		begin\n"
		"		IF IsUsedClanHandle(handle_in) THEN\n"
		"		return 'Error: Handle already in use.';\n"
		"		ELSIF IsDisallowedHandle(handle_in) THEN\n"
		"		return 'Error: Handle is not allowed.';\n"
		"		END IF;\n"
		"		\n"
		"		UPDATE clanHandles set handle=handle_in WHERE clanId_fk=clanId_in;\n"
		"		\n"
		"		return 'OK';\n"
		"		end;\n"
		"		$$ LANGUAGE plpgsql;\n"
		"\n"
		"CREATE OR REPLACE function CreateClanOnValidHandle(\n"
		"	handle_ina text,\n"
		"	int_1_ina integer,\n"
		"	int_2_ina integer,\n"
		"	int_3_ina integer,\n"
		"	int_4_ina integer,\n"
		"	float_1_ina float4,\n"
		"	float_2_ina float4,\n"
		"	float_3_ina float4,\n"
		"	float_4_ina float4,\n"
		"	requiresInvitationsToJoin_ina boolean,\n"
		"	description_1_ina text,\n"
		"	description_2_ina text,\n"
		"	binaryData_ina bytea,\n"
		"	userId_inb integer,\n"
		"	mEStatus1_inb integer,\n"
		"	int_1_inb integer,\n"
		"	int_2_inb integer,\n"
		"	int_3_inb integer,\n"
		"	int_4_inb integer,\n"
		"	float_1_inb float4,\n"
		"	float_2_inb float4,\n"
		"	float_3_inb float4,\n"
		"	float_4_inb float4,\n"
		"	description_1_inb text,\n"
		"	description_2_inb text,\n"
		"	binaryData_inb bytea\n"
		") returns text\n"
		"as $$\n"
		"declare\n"
		"	clanId integer;\n"
		"	handleID integer;\n"
		"begin\n"
		"IF IsUsedClanHandle(handle_ina) THEN\n"
		"return 'Error: Handle already in use.';\n"
		"ELSIF IsDisallowedHandle(handle_ina) THEN\n"
		"return 'Error: Handle is not allowed.';\n"
		"END IF;\n"
		"\n"
		"INSERT INTO clans (int_1,int_2,int_3,int_4,float_1,float_2,float_3,float_4,requiresInvitationsToJoin,description_1,description_2,binaryData)\n"
		" VALUES (int_1_ina,int_2_ina,int_3_ina,int_4_ina,float_1_ina,float_2_ina,float_3_ina,float_4_ina,requiresInvitationsToJoin_ina,description_1_ina,description_2_ina,binaryData_ina)\n"
		" RETURNING clanId_pk INTO clanId;\n"
		"\n"
		"INSERT INTO clanHandles (clanId_fk, handle) VALUES (clanId, handle_ina) RETURNING handleID_pk INTO handleID;\n"
		"\n"
		"INSERT INTO clanMembers (clanId_fk, userId_fk, mEStatus1,int_1,int_2,int_3,int_4,float_1,float_2,float_3,float_4,description_1,description_2,binaryData)\n"
		"VALUES (clanId, userId_inb, mEStatus1_inb,int_1_inb,int_2_inb,int_3_inb,int_4_inb,float_1_inb,float_2_inb,float_3_inb,float_4_inb,description_1_inb,description_2_inb,binaryData_inb);\n"
		"\n"
		"UPDATE clans SET handleId_fk=handleID WHERE clans.clanId_pk=clanId;\n"
		"\n"
		"return 'OK' || clanId;\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"CREATE OR REPLACE function CreateUserOnValidHandle(\n"
		"	handle_in text,\n"
		"	firstName_in text,\n"
		"	middleName_in text,\n"
		"	lastName_in text,\n"
		"	race_in text,\n"
		"	sex_in text,\n"
		"	homeAddress1_in text,\n"
		"	homeAddress2_in text,\n"
		"	homeCity_in text,\n"
		"	homeState_in text,\n"
		"	homeProvince_in text,\n"
		"	homeCountry_in text,\n"
		"	homeZipCode_in text,\n"
		"	billingAddress1_in text,\n"
		"	billingAddress2_in text,\n"
		"	billingCity_in text,\n"
		"	billingState_in text,\n"
		"	billingProvince_in text,\n"
		"	billingCountry_in text,\n"
		"	billingZipCode_in text,\n"
		"	emailAddress_in text,\n"
		"	password_in text,\n"
		"	passwordRecoveryQuestion_in text,\n"
		"	passwordRecoveryAnswer_in text,\n"
		"	caption1_in text,\n"
		"	caption2_in text,\n"
		"	caption3_in text,\n"
		"	dateOfBirth_in text,\n"
		"	accountNumber_in integer,\n"
		"	creditCardNumber_in text,\n"
		"	creditCardExpiration_in text,\n"
		"	creditCardCVV_in text,\n"
		"	adminLevel_in text,\n"
		"	permissions_in text,\n"
		"	accountBalance_in float4,\n"
		"	sourceIPAddress_in text,\n"
		"	binaryData_in bytea) returns text\n"
		"as $$\n"
		"declare\n"
		"	userId integer;\n"
		"	handleID integer;\n"
		"begin\n"
		"IF IsUsedHandle(handle_in) THEN\n"
		"return 'Error: Handle already in use.';\n"
		"ELSIF IsDisallowedHandle(handle_in) THEN\n"
		"return 'Error: Handle is not allowed.';\n"
		"END IF;\n"
		"\n"
		"INSERT INTO users (firstName,middleName,lastName,race,sex,homeAddress1,homeAddress2,homeCity,homeState,homeProvince,homeCountry,homeZipCode,billingAddress1,billingAddress2,billingCity,billingState,billingProvince,billingCountry,billingZipCode,emailAddress,password,passwordRecoveryQuestion,passwordRecoveryAnswer,caption1,caption2,caption3,dateOfBirth,accountNumber,creditCardNumber,creditCardExpiration,creditCardCVV,adminLevel,permissions,accountBalance,sourceIPAddress,binaryData)\n"
		" VALUES (firstName_in,middleName_in,lastName_in,race_in,sex_in,homeAddress1_in,homeAddress2_in,homeCity_in,homeState_in,homeProvince_in,homeCountry_in,homeZipCode_in,billingAddress1_in,billingAddress2_in,billingCity_in,billingState_in,billingProvince_in,billingCountry_in,billingZipCode_in,emailAddress_in,password_in,passwordRecoveryQuestion_in,passwordRecoveryAnswer_in,caption1_in,caption2_in,caption3_in,dateOfBirth_in,accountNumber_in,creditCardNumber_in,creditCardExpiration_in,creditCardCVV_in,adminLevel_in,permissions_in,accountBalance_in,sourceIPAddress_in,binaryData_in)\n"
		" RETURNING userId_pk INTO userId;\n"
		"\n"
		"INSERT INTO handles (userID_fk, handle) VALUES (userId, handle_in) RETURNING handleID_pk INTO handleID;\n"
		"\n"
		"UPDATE users SET handleID_fk=handleID WHERE users.userId_pk=userId;\n"
		"\n"
		"return 'OK';\n"
		"end;\n"
		"$$ LANGUAGE plpgsql;\n"
		"\n"
		"CREATE OR REPLACE FUNCTION deleteClanOnNoMembers() RETURNS trigger as $$\n"
		"DECLARE\n"
		"numMembers integer;\n"
		"BEGIN\n"
		"	PERFORM * FROM clanMembers WHERE clanMembers.clanId_fk=OLD.clanId_fk FOR UPDATE OF clanMembers;\n"
		"SELECT INTO numMembers COUNT(*) from clanMembers WHERE clanMembers.clanId_fk=OLD.clanId_fk and clanMembers.mEStatus1 < 10000;\n"
		"IF numMembers=0 THEN\n"
		"	DELETE FROM clans where clanId_pk=OLD.clanId_fk;\n"
		"END IF;\n"
		"return OLD;"
		"END;\n"
		"\n"
		"$$ LANGUAGE plpgsql;\n"
		"CREATE TRIGGER deleteClanOnNoMembersTrigger AFTER DELETE ON clanMembers\n"
		"FOR EACH ROW EXECUTE PROCEDURE deleteClanOnNoMembers();\n"
		"\n"
		"COMMIT;\n";

	PGresult *result;
	bool res = ExecuteBlockingCommand(createTablesStr, &result, true);
	PQclear(result);
	return res;
}
bool LobbyDB_PostgreSQL::DestroyLobbyServerTables(void)
{
	if (isConnected==false)
		return false;

	const char *LobbyServerDBDropTablesStr1 =
		"DROP TRIGGER deleteClanOnNoMembersTrigger ON clanMembers CASCADE;\n"
		"DROP TABLE users CASCADE;\n"
		"DROP TABLE accountNotes CASCADE;\n"
		"DROP TABLE friends CASCADE;\n"
		"DROP TABLE ignoreList CASCADE;\n"
		"DROP TABLE emails CASCADE;\n"
		"DROP TABLE handles CASCADE;\n"
		"DROP TABLE clanHandles CASCADE;\n"
		"DROP TABLE disallowedHandles CASCADE;\n"
		"DROP TABLE actionHistory CASCADE;\n"
		"DROP TABLE clans CASCADE;\n"
		"DROP TABLE clanMembers CASCADE;\n"
		"DROP TABLE clanBoardPosts CASCADE;\n"
		"DROP FUNCTION IsUsedHandle(h text) CASCADE;\n"
		"DROP FUNCTION IsUsedClanHandle(h text) CASCADE;\n"
		"DROP FUNCTION IsDisallowedHandle(h text) CASCADE;\n"
		"DROP FUNCTION GetHandleFromUserId(userId integer) CASCADE;\n"
		"DROP FUNCTION GetUserIdFromHandle(handle_in text) CASCADE;\n"
		"DROP FUNCTION ChangeUserHandle(userId_in integer,handle_in text) CASCADE;\n"
		"DROP FUNCTION ChangeClanHandle(clanId_in integer,handle_in text) CASCADE;\n"
		"DROP FUNCTION DeleteClanOnNoMembers() CASCADE;\n"
		"DROP FUNCTION CreateClanOnValidHandle(handle_ina text, int_1_ina integer, int_2_ina integer, int_3_ina integer, int_4_ina integer, float_1_ina real, float_2_ina real, float_3_ina real, float_4_ina real, requiresinvitationstojoin_ina boolean, description_1_ina text, description_2_ina text, binarydata_ina bytea, userid_inb integer, mestatus1_inb integer, int_1_inb integer, int_2_inb integer, int_3_inb integer, int_4_inb integer, float_1_inb real, float_2_inb real, float_3_inb real, float_4_inb real, description_1_inb text, description_2_inb text, binarydata_inb bytea) CASCADE;\n"
		"DROP FUNCTION CreateUserOnValidHandle(handle_in text, firstname_in text, middlename_in text, lastname_in text, race_in text, sex_in text, homeaddress1_in text, homeaddress2_in text, homecity_in text, homestate_in text, homeprovince_in text, homecountry_in text, homezipcode_in text, billingaddress1_in text, billingaddress2_in text, billingcity_in text, billingstate_in text, billingprovince_in text, billingcountry_in text, billingzipcode_in text, emailaddress_in text, password_in text, passwordrecoveryquestion_in text, passwordrecoveryanswer_in text, caption1_in text, caption2_in text, caption3_in text, dateofbirth_in text, accountnumber_in integer, creditcardnumber_in text, creditcardexpiration_in text, creditcardcvv_in text, adminlevel_in text, permissions_in text, accountbalance_in real, sourceipaddress_in text, binarydata_in bytea) CASCADE;\n";

	PGresult *result;
	bool res = ExecuteBlockingCommand(LobbyServerDBDropTablesStr1, &result, false);
	PQclear(result);

	// Update - it's just slow and the GUI doesn't refresh
	/*
	// For some reason, if these last two lines are in the above query, the functions don't get deleted although no errors are reported.
	const char *LobbyServerDBDropTablesStr2 =
		"DROP FUNCTION CreateClanOnValidHandle(handle_ina text, int_1_ina integer, int_2_ina integer, int_3_ina integer, int_4_ina integer, float_1_ina real, float_2_ina real, float_3_ina real, float_4_ina real, requiresinvitationstojoin_ina boolean, description_1_ina text, description_2_ina text, binarydata_ina bytea, userid_inb integer, mestatus1_inb integer, int_1_inb integer, int_2_inb integer, int_3_inb integer, int_4_inb integer, float_1_inb real, float_2_inb real, float_3_inb real, float_4_inb real, description_1_inb text, description_2_inb text, binarydata_inb bytea) CASCADE;\n"
		"DROP FUNCTION CreateUserOnValidHandle(handle_in text, firstname_in text, middlename_in text, lastname_in text, race_in text, sex_in text, homeaddress1_in text, homeaddress2_in text, homecity_in text, homestate_in text, homeprovince_in text, homecountry_in text, homezipcode_in text, billingaddress1_in text, billingaddress2_in text, billingcity_in text, billingstate_in text, billingprovince_in text, billingcountry_in text, billingzipcode_in text, emailaddress_in text, password_in text, passwordrecoveryquestion_in text, passwordrecoveryanswer_in text, caption1_in text, caption2_in text, caption3_in text, dateofbirth_in text, accountnumber_in integer, creditcardnumber_in text, creditcardexpiration_in text, creditcardcvv_in text, adminlevel_in text, permissions_in text, accountbalance_in real, sourceipaddress_in text, binarydata_in bytea) CASCADE;\n";
	res = ExecuteBlockingCommand(LobbyServerDBDropTablesStr2, &result, false);
	PQclear(result);
*/
	return res;
}
void LobbyDB_PostgreSQL::PushFunctor(LobbyDBFunctor *functor, void *context)
{
	functor->lobbyServer=this;
	functor->callback=lobbyServerCallback;
	FunctionThreadDependentClass::PushFunctor(functor,context);
}
void LobbyDB_PostgreSQL::AssignCallback(LobbyDBCBInterface *cb)
{
	lobbyServerCallback=cb;
}
bool LobbyDB_PostgreSQL::IsValidUserID(DatabaseKey userId)
{
	PGresult *result;
	if (ExecuteBlockingCommand(FormatString("SELECT COUNT(*) from users where users.userId_pk = %i",userId), &result, false)==false)
	{
		PQclear(result);
		return false;
	}

	char *outputStr=PQgetvalue(result, 0, 0);
	long long count;
	memcpy(&count, outputStr, sizeof(count));
	PostgreSQLInterface::EndianSwapInPlace((char*)&count, sizeof(count));
	if (count==0)
	{
		PQclear(result);
		return false;
	}
	
	PQclear(result);
	return true;
}
bool LobbyDB_PostgreSQL::GetUserIDFromHandle(const RakNet::RakString &handle,DatabaseKey &userId)
{
	if (handle.IsEmpty())
		return false;

	PGresult *result;
	char *paramData[1];
	int paramLength[1];
	int paramFormat[1];
	paramData[0]=(char*)handle.C_String();
	paramLength[0]=(int) handle.GetLength();
	paramFormat[0]=PQEXECPARAM_FORMAT_TEXT;

	result = PQexecParams(pgConn, "SELECT userID_fk from handles where (lower(handles.handle))=(lower($1::text));",1,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	bool dbQuerySuccess=IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	if (dbQuerySuccess && numRowsReturned>0)
	{
		if (PostgreSQLInterface::PQGetValueFromBinary(&userId, result, 0, "userID_fk"))
		{
			PQclear(result);
			return true;
		}
	}
	PQclear(result);
	return false;
}
bool LobbyDB_PostgreSQL::GetClanIDFromHandle(const RakNet::RakString &handle,DatabaseKey &clanId)
{
	if (handle.IsEmpty())
		return false;

	PGresult *result;
	char *paramData[1];
	int paramLength[1];
	int paramFormat[1];
	paramData[0]=(char*)handle.C_String();
	paramLength[0]=(int) handle.GetLength();
	paramFormat[0]=PQEXECPARAM_FORMAT_TEXT;

	result = PQexecParams(pgConn, "SELECT clanId_fk from clanHandles where (lower(clanHandles.handle))=(lower($1::text));",1,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	bool dbQuerySuccess=IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	if (dbQuerySuccess && numRowsReturned>0)
	{
		if (PostgreSQLInterface::PQGetValueFromBinary(&clanId, result, 0, "clanId_fk"))
		{
			PQclear(result);
			return true;
		}
	}
	PQclear(result);
	return false;
}
bool LobbyDB_PostgreSQL::GetHandleFromClanID(RakNet::RakString &handle,DatabaseKey &clanId)
{
	if (clanId==0)
		return false;

	PGresult *result;
	if (ExecuteBlockingCommand(FormatString("SELECT handle from clanHandles where clanId_fk = %i",clanId), &result, false)==false)
	{
		PQclear(result);
		return false;
	}

	PostgreSQLInterface::PQGetValueFromBinary(&handle, result, 0, "handle");
	PQclear(result);
	return true;

}
void CreateUser_PostgreSQLImpl::EncodeQueryInput(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &paramTypeStr, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryInput("handle", inputData->handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("firstName", inputData->firstName, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("middleName", inputData->middleName, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("lastName", inputData->lastName, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("race", inputData->race, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("sex", inputData->sex, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeAddress1", inputData->homeAddress1, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeAddress2", inputData->homeAddress2, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeCity", inputData->homeCity, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeState", inputData->homeState, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeProvince", inputData->homeProvince, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeCountry", inputData->homeCountry, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("homeZipCode", inputData->homeZipCode, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingAddress1", inputData->billingAddress1, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingAddress2", inputData->billingAddress2, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingCity", inputData->billingCity, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingState", inputData->billingState, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingProvince", inputData->billingProvince, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingCountry", inputData->billingCountry, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("billingZipCode", inputData->billingZipCode, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("emailAddress", inputData->emailAddress, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("password", inputData->password, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("passwordRecoveryQuestion", inputData->passwordRecoveryQuestion, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("passwordRecoveryAnswer", inputData->passwordRecoveryAnswer, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("caption1", inputData->caption1, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("caption2", inputData->caption2, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("caption3", inputData->caption3, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("dateOfBirth", inputData->dateOfBirth, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("accountNumber", inputData->accountNumber, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("creditCardNumber", inputData->creditCardNumber, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("creditCardExpiration", inputData->creditCardExpiration, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("creditCardCVV", inputData->creditCardCVV, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("adminLevel", inputData->adminLevel, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("permissions", inputData->permissions, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("accountBalance", inputData->accountBalance, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("sourceIPAddress", inputData->sourceIPAddress, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("binaryData", inputData->binaryData, inputData->binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateCaptions(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("caption1", inputData->caption1, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("caption2", inputData->caption2, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("caption3", inputData->caption3, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateCCInfo(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("billingAddress1", inputData->billingAddress1, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingAddress2", inputData->billingAddress2, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingCity", inputData->billingCity, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingState", inputData->billingState, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingProvince", inputData->billingProvince, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingCountry", inputData->billingCountry, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("billingZipCode", inputData->billingZipCode, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("creditCardNumber", inputData->creditCardNumber, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("creditCardExpiration", inputData->creditCardExpiration, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("creditCardCVV", inputData->creditCardCVV, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("accountBalance", inputData->accountBalance, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateBinaryData(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("binaryData", inputData->binaryData, inputData->binaryDataLength, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdatePersonalInfo(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("firstName", inputData->firstName, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("middleName", inputData->middleName, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("lastName", inputData->lastName, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("race", inputData->race, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("sex", inputData->sex, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeAddress1", inputData->homeAddress1, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeAddress2", inputData->homeAddress2, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeCity", inputData->homeCity, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeState", inputData->homeState, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeProvince", inputData->homeProvince, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeCountry", inputData->homeCountry, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("homeZipCode", inputData->homeZipCode, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("dateOfBirth", inputData->dateOfBirth, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateEmailAddr(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("emailAddress", inputData->emailAddress, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateOther(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("adminLevel", inputData->adminLevel, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("sourceIPAddress", inputData->sourceIPAddress, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateAccountNumber(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("accountNumber", inputData->accountNumber, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateAdminLevel(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("adminLevel", inputData->accountNumber, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdateAccountBalance(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("accountBalance", inputData->accountNumber, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdatePermissions(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("permissions", inputData->permissions, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::EncodeQueryUpdatePassword(LobbyDBSpec::CreateUser_Data *inputData, RakNet::RakString &valueStr, int &numParams, char **paramData, int *paramLength, int *paramFormat)
{
	PostgreSQLInterface::EncodeQueryUpdate("password", inputData->password, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("passwordRecoveryQuestion", inputData->passwordRecoveryQuestion, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryUpdate("passwordRecoveryAnswer", inputData->passwordRecoveryAnswer, valueStr, numParams, paramData, paramLength, paramFormat );
}
void CreateUser_PostgreSQLImpl::Process(void *context)
{
	if (handle.IsEmpty())
	{
		dbQuerySuccess=true;
		queryOK=false;
		queryResultString = "Handle cannot be an empty string.";
		return;
	}

	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	EncodeQueryInput(this, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat);

	query = "select * FROM CreateUserOnValidHandle(";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess)
	{
		char *valueStr = PQgetvalue(result, 0, 0);
		if (valueStr[0]=='O' && valueStr[1]=='K')
			queryOK=true;
		else
			queryOK=false;
		queryResultString = valueStr;
	}
	else
	{
		queryOK=false;
		queryResultString = lobbyServer->GetLastError();
	}
	PQclear(result);

}
void CreateUser_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->CreateUser_CB(this, wasCancelled, context);
	Deref();
}
CreateUser_PostgreSQLImpl* CreateUser_PostgreSQLImpl::Alloc(void) {return new CreateUser_PostgreSQLImpl;}
void CreateUser_PostgreSQLImpl::Free(CreateUser_PostgreSQLImpl *s) {delete s;}

void GetUser_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "SELECT ";
	if (getCCInfo)
		query+="users.creditCardNumber, users.creditCardExpiration, users.creditCardCVV, users.billingAddress1, users.billingAddress2, users.billingCity, users.billingState, users.billingProvince, users.billingCountry, users.billingZipCode, users.accountBalance, ";
	if (getBinaryData)
		query+="users.binaryData, ";
	if (getPersonalInfo)
		query+="users.firstName, users.middleName, users.lastName, users.race, users.sex, users.homeAddress1, users.homeAddress2, users.homeCity, users.homeState, users.homeProvince, users.homeCountry, users.homeZipCode, users.dateOfBirth, ";
	if (getEmailAddr)
		query+="users.emailAddress, ";
	if (getPassword)
		query+="users.password, users.passwordRecoveryQuestion, users.passwordRecoveryAnswer, ";
	query+="users.userId_pk, users.caption1, users.caption2, users.caption3, users.accountNumber, users.creationTime, users.adminLevel, users.permissions, users.isBanned, users.isSuspended, users.suspensionExpiration, users.banOrSuspensionReason, users.sourceIPAddress";
	query+=", handles.handle ";
	query+="FROM users, handles "
		"WHERE ";
	if (id.hasDatabaseRowId)
	{
		query+=FormatString("users.userId_pk = %i",id.databaseRowId);
	}
	else
	{
		query+="handleID_fk=(SELECT handleID_pk FROM handles WHERE (lower(handle)) = (lower(";
		paramTypeStr.Clear();
		valueStr.Clear();
		PostgreSQLInterface::EncodeQueryInput("handle", id.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
	}
	query+=" AND handles.userID_fk=users.userId_pk;";

//	printf("%s\n",query.C_String());
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);
	int numRowsReturned = PQntuples(result);
	userFound = numRowsReturned>0;
	if (dbQuerySuccess && userFound)
	{
		if (id.hasDatabaseRowId==false)
		{
			output.handle=id.handle;
		}	
		PostgreSQLInterface::PQGetValueFromBinary(&id.databaseRowId, result, 0, "userId_pk");
		PostgreSQLInterface::PQGetValueFromBinary(&id.handle, result, 0, "handle");
		output.handle=id.handle;
		PostgreSQLInterface::PQGetValueFromBinary(&creationTime, result, 0, "creationTime");
		PostgreSQLInterface::PQGetValueFromBinary(&output.firstName, result, 0, "firstName");
		PostgreSQLInterface::PQGetValueFromBinary(&output.middleName, result, 0, "middleName");
		PostgreSQLInterface::PQGetValueFromBinary(&output.lastName, result, 0, "lastName");
		PostgreSQLInterface::PQGetValueFromBinary(&output.race, result, 0, "race");
		PostgreSQLInterface::PQGetValueFromBinary(&output.sex, result, 0, "sex");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeAddress1, result, 0, "homeAddress1");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeAddress2, result, 0, "homeAddress2");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeCity, result, 0, "homeCity");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeState, result, 0, "homeState");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeProvince, result, 0, "homeProvince");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeCountry, result, 0, "homeCountry");
		PostgreSQLInterface::PQGetValueFromBinary(&output.homeZipCode, result, 0, "homeZipCode");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingAddress1, result, 0, "billingAddress1");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingAddress2, result, 0, "billingAddress2");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingCity, result, 0, "billingCity");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingState, result, 0, "billingState");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingProvince, result, 0, "billingProvince");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingCountry, result, 0, "billingCountry");
		PostgreSQLInterface::PQGetValueFromBinary(&output.billingZipCode, result, 0, "billingZipCode");
		PostgreSQLInterface::PQGetValueFromBinary(&output.emailAddress, result, 0, "emailAddress");
		PostgreSQLInterface::PQGetValueFromBinary(&output.password, result, 0, "password");
		PostgreSQLInterface::PQGetValueFromBinary(&output.passwordRecoveryQuestion, result, 0, "passwordRecoveryQuestion");
		PostgreSQLInterface::PQGetValueFromBinary(&output.passwordRecoveryAnswer, result, 0, "passwordRecoveryAnswer");
		PostgreSQLInterface::PQGetValueFromBinary(&output.caption1, result, 0, "caption1");
		PostgreSQLInterface::PQGetValueFromBinary(&output.caption2, result, 0, "caption2");
		PostgreSQLInterface::PQGetValueFromBinary(&output.caption3, result, 0, "caption3");
		PostgreSQLInterface::PQGetValueFromBinary(&output.dateOfBirth, result, 0, "dateOfBirth");
		PostgreSQLInterface::PQGetValueFromBinary(&output.accountNumber, result, 0, "accountNumber");
		PostgreSQLInterface::PQGetValueFromBinary(&output.creditCardNumber, result, 0, "creditCardNumber");
		PostgreSQLInterface::PQGetValueFromBinary(&output.creditCardExpiration, result, 0, "creditCardExpiration");
		PostgreSQLInterface::PQGetValueFromBinary(&output.creditCardCVV, result, 0, "creditCardCVV");
		PostgreSQLInterface::PQGetValueFromBinary(&output.adminLevel, result, 0, "adminLevel");
		PostgreSQLInterface::PQGetValueFromBinary(&output.permissions, result, 0, "permissions");
		PostgreSQLInterface::PQGetValueFromBinary(&isBanned, result, 0, "isBanned");
		PostgreSQLInterface::PQGetValueFromBinary(&isSuspended, result, 0, "isSuspended");
		PostgreSQLInterface::PQGetValueFromBinary(&suspensionExpiration, result, 0, "suspensionExpiration");
		PostgreSQLInterface::PQGetValueFromBinary(&banOrSuspensionReason, result, 0, "banOrSuspensionReason");
		PostgreSQLInterface::PQGetValueFromBinary(&creationTime, result, 0, "creationTime");
		PostgreSQLInterface::PQGetValueFromBinary(&output.accountBalance, result, 0, "accountBalance");
		PostgreSQLInterface::PQGetValueFromBinary(&output.sourceIPAddress, result, 0, "sourceIPAddress");
		PostgreSQLInterface::PQGetValueFromBinary(&output.binaryData, &output.binaryDataLength, result, 0, "binaryData");
	}
	PQclear(result);
}
void GetUser_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetUser_CB(this, wasCancelled, context);
	Deref();
}
GetUser_PostgreSQLImpl* GetUser_PostgreSQLImpl::Alloc(void) {return new GetUser_PostgreSQLImpl;}
void GetUser_PostgreSQLImpl::Free(GetUser_PostgreSQLImpl *s) {delete s;}
void UpdateUser_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (updateCaptions==false && updateCCInfo==false && updateBinaryData==false && updatePersonalInfo==false && updateEmailAddr==false &&
		updatePassword==false && updateOtherInfo==false && updatePermissions==false && updateAccountNumber==false && updateAdminLevel==false && updateAccountBalance==false)
	{
		dbQuerySuccess=true;
		return;
	}

	if (updateCaptions)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateCaptions(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateCCInfo)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateCCInfo(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateBinaryData)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateBinaryData(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updatePersonalInfo)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdatePersonalInfo(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateEmailAddr)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateEmailAddr(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updatePassword)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdatePassword(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateOtherInfo)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateOther(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updatePermissions)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdatePermissions(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateAccountNumber)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateAccountNumber(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateAdminLevel)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateAdminLevel(&input, valueStr, numParams, paramData, paramLength, paramFormat);
	if (updateAccountBalance)
		CreateUser_PostgreSQLImpl::EncodeQueryUpdateAccountBalance(&input, valueStr, numParams, paramData, paramLength, paramFormat);

	query = "UPDATE users SET ";
	query+=valueStr;
	query+=" WHERE ";

	if (id.hasDatabaseRowId)
	{
		query+=FormatString("users.userId_pk = %i ",id.databaseRowId);
	}
	else
	{
		query+="handleID_fk=(SELECT handleID_pk FROM handles WHERE (lower(handle)) = (lower(";
		paramTypeStr.Clear();
		valueStr.Clear();
		PostgreSQLInterface::EncodeQueryInput("handle", id.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
	}
	query+=";";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void UpdateUser_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->UpdateUser_CB(this, wasCancelled, context);
	Deref();
}
UpdateUser_PostgreSQLImpl* UpdateUser_PostgreSQLImpl::Alloc(void) {return new UpdateUser_PostgreSQLImpl;}
void UpdateUser_PostgreSQLImpl::Free(UpdateUser_PostgreSQLImpl *s) {delete s;}

void DeleteUser_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "DELETE FROM users where userId_pk=";
	if (id.hasDatabaseRowId)
		query+=FormatString("%i", id.databaseRowId);
	else
	{
		query+="(SELECT handleID_pk FROM handles WHERE (lower(handle)) = (lower(";
		PostgreSQLInterface::EncodeQueryInput("handle", id.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
	}
	query+=";";
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void DeleteUser_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->DeleteUser_CB(this, wasCancelled, context);
	Deref();
}
DeleteUser_PostgreSQLImpl* DeleteUser_PostgreSQLImpl::Alloc(void) {return new DeleteUser_PostgreSQLImpl;}
void DeleteUser_PostgreSQLImpl::Free(DeleteUser_PostgreSQLImpl *s) {delete s;}

void ChangeUserHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (newHandle.IsEmpty())
	{
		dbQuerySuccess=true;
		success=false;
		queryResult = "New handle cannot be an empty string.";
		return;
	}

	if (id.hasDatabaseRowId==false && lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
	{
		dbQuerySuccess=true;
		success=false;
		queryResult = "Cannot find id.userHandle.";
		return;
	}

	PostgreSQLInterface::EncodeQueryInput("userId_fk", id.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("handle", newHandle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

	query = "select * FROM ChangeUserHandle(";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess)
	{
		char *valueStr = PQgetvalue(result, 0, 0);
		if (valueStr[0]=='O' && valueStr[1]=='K')
			success=true;
		else
			success=false;
		queryResult = valueStr;
	}
	else
	{
		success=false;
		queryResult = lobbyServer->GetLastError();
	}
	PQclear(result);

}
void ChangeUserHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->ChangeUserHandle_CB(this, wasCancelled, context);
	Deref();
}
ChangeUserHandle_PostgreSQLImpl* ChangeUserHandle_PostgreSQLImpl::Alloc(void) {return new ChangeUserHandle_PostgreSQLImpl;}
void ChangeUserHandle_PostgreSQLImpl::Free(ChangeUserHandle_PostgreSQLImpl *s) {delete s;}

void AddAccountNote_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (id.hasDatabaseRowId)
	{
		if (lobbyServer->IsValidUserID(id.databaseRowId)==false)
		{
			userFound=false;
			return;
		}
	}
	else
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
		{
			userFound=false;
			return;
		}
	}

	query = "INSERT INTO accountNotes (";

	if (writeModeratorId)
		PostgreSQLInterface::EncodeQueryInput("moderatorId", moderatorId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("userID_fk", id.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("moderatorUsername", moderatorUsername, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("type", type, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("subject", subject, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("body", body, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );

	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void AddAccountNote_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddAccountNote_CB(this, wasCancelled, context);
	Deref();
}
AddAccountNote_PostgreSQLImpl* AddAccountNote_PostgreSQLImpl::Alloc(void) {return new AddAccountNote_PostgreSQLImpl;}
void AddAccountNote_PostgreSQLImpl::Free(AddAccountNote_PostgreSQLImpl *s) {delete s;}

void GetAccountNotes_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "SELECT moderatorId,moderatorUsername,type,subject,body,time FROM accountNotes WHERE ";
	if (id.hasDatabaseRowId)
	{
		query+=FormatString("userID_fk = %i ",id.databaseRowId);
	}
	else
	{
		query+="userID_fk=(SELECT userID_fk FROM handles WHERE (lower(handle)) = (lower(";
		paramTypeStr.Clear();
		valueStr.Clear();
		PostgreSQLInterface::EncodeQueryInput("handle", id.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) ) ";
	}

	query+="ORDER BY time ";
	if (ascendingSort)
		query+="ASC";
	else
		query+="DESC";
	query+=";";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	AddAccountNote_Data *addAccountNote;
	if (dbQuerySuccess)
	{
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			addAccountNote = new LobbyDBSpec::AddAccountNote_Data;
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->moderatorId, result, i, "moderatorId");
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->moderatorUsername, result, i, "moderatorUsername");
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->type, result, i, "type");
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->subject, result, i, "subject");
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->body, result, i, "body");
			PostgreSQLInterface::PQGetValueFromBinary(&addAccountNote->time, result, i, "time");

			accountNotes.Insert(addAccountNote);
		}
	}
}
void GetAccountNotes_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetAccountNotes_CB(this, wasCancelled, context);
	Deref();
}
GetAccountNotes_PostgreSQLImpl* GetAccountNotes_PostgreSQLImpl::Alloc(void) {return new GetAccountNotes_PostgreSQLImpl;}
void GetAccountNotes_PostgreSQLImpl::Free(GetAccountNotes_PostgreSQLImpl *s) {delete s;}

void AddFriend_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	dbQuerySuccess=true;

	if (id.hasDatabaseRowId)
	{
		if (lobbyServer->IsValidUserID(id.databaseRowId)==false)
		{
			success=false;
			queryResult="User ID not found.";
			return;
		}
	}
	else
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
		{
			success=false;
			queryResult="User handle not found.";
			return;
		}
	}

	if (friendId.hasDatabaseRowId)
	{
		if (lobbyServer->IsValidUserID(friendId.databaseRowId)==false)
		{
			success=false;
			queryResult="Friend ID not found.";
			return;
		}
	}
	else
	{
		if (lobbyServer->GetUserIDFromHandle(friendId.handle, friendId.databaseRowId)==false)
		{
			success=false;
			queryResult="Friend handle not found.";
			return;
		}
	}

	// Can't add yourself as a friend!
	if (friendId.databaseRowId==id.databaseRowId)
	{
		success=false;
		queryResult="Can't add yourself as a friend.";
		return;
	}

	// If already a friend, just fail
	if (lobbyServer->ExecuteBlockingCommand(FormatString("SELECT COUNT(*) from friends where (userID_fk = %i AND friendUserId_fk = %i) OR (userID_fk = %i AND friendUserId_fk = %i)",id.databaseRowId, friendId.databaseRowId, friendId.databaseRowId, id.databaseRowId ), &result, false)==false)
	{
		dbQuerySuccess=false;
		PQclear(result);
		return;
	}
	char *outputStr=PQgetvalue(result, 0, 0);
	if (outputStr[0]!='0')
	{
		success=false;
		queryResult="Already friends";
		PQclear(result);
		return;
	}
	PQclear(result);

	query = "INSERT INTO friends (";

	PostgreSQLInterface::EncodeQueryInput("userID_fk", id.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("friendUserId_fk", friendId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );

	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);

	queryResult="Success";
	success=true;
}
void AddFriend_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddFriend_CB(this, wasCancelled, context);
	Deref();
}
AddFriend_PostgreSQLImpl* AddFriend_PostgreSQLImpl::Alloc(void) {return new AddFriend_PostgreSQLImpl;}
void AddFriend_PostgreSQLImpl::Free(AddFriend_PostgreSQLImpl *s) {delete s;}

void RemoveFriend_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;

	dbQuerySuccess=false;
	if (removeFriendInput.id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(removeFriendInput.id.handle, removeFriendInput.id.databaseRowId)==false)
			return;
	}

	if (removeFriendInput.friendId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(removeFriendInput.friendId.handle, removeFriendInput.friendId.databaseRowId)==false)
			return;
	}

	dbQuerySuccess=lobbyServer->ExecuteBlockingCommand(FormatString(
		"DELETE FROM friends where (userID_fk=%i AND friendUserId_fk=%i) OR (userID_fk=%i AND friendUserId_fk=%i)"
		,removeFriendInput.id.databaseRowId, removeFriendInput.friendId.databaseRowId, removeFriendInput.friendId.databaseRowId, removeFriendInput.id.databaseRowId ), &result, false);

	PQclear(result);
}
void RemoveFriend_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->RemoveFriend_CB(this, wasCancelled, context);
	Deref();
}
RemoveFriend_PostgreSQLImpl* RemoveFriend_PostgreSQLImpl::Alloc(void) {return new RemoveFriend_PostgreSQLImpl;}
void RemoveFriend_PostgreSQLImpl::Free(RemoveFriend_PostgreSQLImpl *s) {delete s;}

void GetFriends_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle,id.databaseRowId)==false)
		{
			dbQuerySuccess=false;
			return;
		}
	}

	query = FormatString("SELECT tbl2.*, handles.handle FROM \n"
	"	(SELECT userID_fk as userId,friendUserId_fk as friendId,creationTime FROM friends WHERE (userID_fk=%i) \n"
	"	UNION \n"
	"	SELECT friendUserId_fk,userID_fk,creationTime FROM friends WHERE (friendUserId_fk=%i)) as tbl2, handles \n"
	"	WHERE handles.userId_fk=tbl2.friendId ORDER BY creationTime \n", id.databaseRowId, id.databaseRowId);

	if (ascendingSort)
		query+="ASC";
	else
		query+="DESC";
	query+=";";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	AddFriend_Data *addFriend;
	if (dbQuerySuccess)
	{
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			addFriend = new LobbyDBSpec::AddFriend_Data;
			PostgreSQLInterface::PQGetValueFromBinary(&addFriend->id.databaseRowId, result, i, "userId");
			if (id.hasDatabaseRowId==false)
				addFriend->id.handle=id.handle;
			PostgreSQLInterface::PQGetValueFromBinary(&addFriend->friendId.handle, result, i, "handle");
			PostgreSQLInterface::PQGetValueFromBinary(&addFriend->friendId.databaseRowId, result, i, "friendId");
			PostgreSQLInterface::PQGetValueFromBinary(&addFriend->creationTime, result, i, "creationTime");
			
			friends.Insert(addFriend);
		}
	}
}
void GetFriends_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetFriends_CB(this, wasCancelled, context);
	Deref();
}
GetFriends_PostgreSQLImpl* GetFriends_PostgreSQLImpl::Alloc(void) {return new GetFriends_PostgreSQLImpl;}
void GetFriends_PostgreSQLImpl::Free(GetFriends_PostgreSQLImpl *s) {delete s;}

void AddToIgnoreList_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	dbQuerySuccess=true;

	if (id.hasDatabaseRowId)
	{
		if (lobbyServer->IsValidUserID(id.databaseRowId)==false)
		{
			success=false;
			queryResult="User ID not found.";
			return;
		}
	}
	else
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
		{
			success=false;
			queryResult="User handle not found.";
			return;
		}
	}

	if (ignoredUser.hasDatabaseRowId)
	{
		if (lobbyServer->IsValidUserID(ignoredUser.databaseRowId)==false)
		{
			success=false;
			queryResult="Ignored user ID not found.";
			return;
		}
	}
	else
	{
		if (lobbyServer->GetUserIDFromHandle(ignoredUser.handle, ignoredUser.databaseRowId)==false)
		{
			success=false;
			queryResult="Ignored user handle not found.";
			return;
		}
	}

	// Can't add yourself as a friend!
	if (ignoredUser.databaseRowId==id.databaseRowId)
	{
		success=false;
		queryResult="Can't ignore yourself.";
		return;
	}

	// If already ignored, just fail
	if (lobbyServer->ExecuteBlockingCommand(FormatString("SELECT COUNT(*) from ignoreList where (userID_fk = %i AND ignoreUserID_fk = %i) OR (userID_fk = %i AND ignoreUserID_fk = %i)",id.databaseRowId, ignoredUser.databaseRowId, ignoredUser.databaseRowId, id.databaseRowId ), &result, false)==false)
	{
		dbQuerySuccess=false;
		PQclear(result);
		return;
	}
	char *outputStr=PQgetvalue(result, 0, 0);
	if (outputStr[0]!='0')
	{
		success=false;
		queryResult="This user is already ignored.";
		PQclear(result);
		return;
	}
	PQclear(result);

	query = "INSERT INTO ignoreList (";

	PostgreSQLInterface::EncodeQueryInput("userID_fk", id.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("ignoreUserID_fk", ignoredUser.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("actions", actions, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );

	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);

	queryResult="Success";
	success=true;
}
void AddToIgnoreList_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddToIgnoreList_CB(this, wasCancelled, context);
	Deref();
}
AddToIgnoreList_PostgreSQLImpl* AddToIgnoreList_PostgreSQLImpl::Alloc(void) {return new AddToIgnoreList_PostgreSQLImpl;}
void AddToIgnoreList_PostgreSQLImpl::Free(AddToIgnoreList_PostgreSQLImpl *s) {delete s;}

void RemoveFromIgnoreList_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;

	dbQuerySuccess=false;
	if (id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
			return;
	}

	if (ignoredUser.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(ignoredUser.handle, ignoredUser.databaseRowId)==false)
			return;
	}

	dbQuerySuccess=lobbyServer->ExecuteBlockingCommand(FormatString(
		"DELETE FROM ignoreList where userID_fk=%i AND ignoreUserID_fk=%i"
		,id.databaseRowId, ignoredUser.databaseRowId, ignoredUser.databaseRowId, id.databaseRowId ), &result, false);

	PQclear(result);
}
void RemoveFromIgnoreList_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->RemoveFromIgnoreList_CB(this, wasCancelled, context);
	Deref();
}
RemoveFromIgnoreList_PostgreSQLImpl* RemoveFromIgnoreList_PostgreSQLImpl::Alloc(void) {return new RemoveFromIgnoreList_PostgreSQLImpl;}
void RemoveFromIgnoreList_PostgreSQLImpl::Free(RemoveFromIgnoreList_PostgreSQLImpl *s) {delete s;}

void GetIgnoreList_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle,id.databaseRowId)==false)
		{
			dbQuerySuccess=false;
			return;
		}
	}
	query = FormatString("SELECT tbl1.*, handles.handle FROM \n"
		"(SELECT userID_fk,ignoreUserID_fk,creationTime FROM ignoreList WHERE (userID_fk=%i)) as tbl1, handles \n"
		"WHERE handles.userId_fk=tbl1.ignoreUserID_fk \n"
		"ORDER BY creationTime ", id.databaseRowId, id.databaseRowId, id.databaseRowId);

	if (ascendingSort)
		query+="ASC";
	else
		query+="DESC";
	query+=";";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	int numRowsReturned = PQntuples(result);
	LobbyDBSpec::AddToIgnoreList_Data *addToIgnoreList;
	if (dbQuerySuccess)
	{
		int i;
		for (i=0; i < numRowsReturned; i++)
		{
			addToIgnoreList = new LobbyDBSpec::AddToIgnoreList_Data;
			PostgreSQLInterface::PQGetValueFromBinary(&addToIgnoreList->id.databaseRowId, result, i, "userID_fk");
			if (id.hasDatabaseRowId==false)
				addToIgnoreList->id.handle=id.handle;
			PostgreSQLInterface::PQGetValueFromBinary(&addToIgnoreList->ignoredUser.handle, result, i, "handle");
			PostgreSQLInterface::PQGetValueFromBinary(&addToIgnoreList->ignoredUser.databaseRowId, result, i, "ignoreUserID_fk");
			PostgreSQLInterface::PQGetValueFromBinary(&addToIgnoreList->creationTime, result, i, "creationTime");

			ignoredUsers.Insert(addToIgnoreList);
		}
	}
}
void GetIgnoreList_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetIgnoreList_CB(this, wasCancelled, context);
	Deref();
}
GetIgnoreList_PostgreSQLImpl* GetIgnoreList_PostgreSQLImpl::Alloc(void) {return new GetIgnoreList_PostgreSQLImpl;}
void GetIgnoreList_PostgreSQLImpl::Free(GetIgnoreList_PostgreSQLImpl *s) {delete s;}

void SendEmail_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	dbQuerySuccess=false;
	validParameters=false;

	// Empty email
	if (subject.IsEmpty() && body.IsEmpty() && (attachment==0 || attachmentLength==0))
	{
		failureMessage="No subject, body, or attachments.";
		return;
	}

	if (id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown user %s", id.handle.C_String());
			return;
		}
	}

	if (to.Size()==0)
	{
		failureMessage="No recipients specified.";
		return;
	}

	unsigned i;
	for (i=0; i < to.Size(); i++)
	{
		valueStr.Clear();
		paramTypeStr.Clear();
		query.Clear();
		numParams=0;

		// Sent emails box
		query="INSERT INTO emails (userIDOwner_fk,userIDOther_fk,";
		valueStr+=FormatString("%i, ", id.databaseRowId);
		if (to[i].hasDatabaseRowId)
		{
			valueStr+=FormatString("%i, ", to[i].databaseRowId);
		}
		else
		{
			valueStr+=FormatString("(SELECT userID_fk FROM handles where (lower(handle))=(lower($%i::text))", numParams+1);
			paramData[numParams]=(char*) to[i].handle.C_String();
			paramLength[numParams]=(int) strlen(to[i].handle.C_String());
			paramFormat[numParams]=PQEXECPARAM_FORMAT_TEXT;
			numParams++;
			valueStr+="), ";
		}
		PostgreSQLInterface::EncodeQueryInput("subject", subject, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("body", body, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("attachment", attachment, attachmentLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("status", initialSenderStatus, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("wasOpened", wasOpened, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		query+=paramTypeStr;
		query+=", inbox) VALUES (";
		query+=valueStr;
		query+=", false);"; // Send

		//printf("%s\n", query.C_String());
		result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
		dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);
		PQclear(result);

		// recipient box
		valueStr.Clear();
		paramTypeStr.Clear();
		query.Clear();
		numParams=0;
		query+="INSERT INTO emails (userIDOther_fk,userIDOwner_fk,"; // Switch other and owner, so each user is the owner of one copy
		valueStr=FormatString("%i, ", id.databaseRowId);
		if (to[i].hasDatabaseRowId)
		{
			valueStr+=FormatString("%i, ", to[i].databaseRowId);
		}
		else
		{
			valueStr+=FormatString("(SELECT userID_fk FROM handles where (lower(handle))=(lower($%i::text))", numParams+1);
			paramData[numParams]=(char*) to[i].handle.C_String();
			paramLength[numParams]=(int) strlen(to[i].handle.C_String());
			paramFormat[numParams]=PQEXECPARAM_FORMAT_TEXT;
			numParams++;
			valueStr+="), ";
		}
		PostgreSQLInterface::EncodeQueryInput("subject", subject, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("body", body, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("attachment", attachment, attachmentLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("status", initialRecipientStatus, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		// Leave out wasOpened, defaults to false
		query+=paramTypeStr;
		query+=", inbox) VALUES (";
		query+=valueStr;
		query+=", true);\n"; // Receive

		//printf("%s\n", query.C_String());
		result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
		dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);
		PQclear(result);
	}

	validParameters=true;
}
void SendEmail_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->SendEmail_CB(this, wasCancelled, context);
	Deref();
}
SendEmail_PostgreSQLImpl* SendEmail_PostgreSQLImpl::Alloc(void) {return new SendEmail_PostgreSQLImpl;}
void SendEmail_PostgreSQLImpl::Free(SendEmail_PostgreSQLImpl *s) {delete s;}

void GetEmails_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	validParameters=false;
	dbQuerySuccess=false;

	if (id.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(id.handle, id.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown user %s", id.handle.C_String());
			return;
		}
	}

	query = FormatString(
		"SELECT emails.userIDOwner_fk, emails.emailMessageID_pk, emails.userIDOther_fk, emails.subject, emails.body, emails.date, emails.attachment, emails.wasOpened, emails.inbox, emails.status, handles.handle \n"
		"FROM emails, handles \n"
		"WHERE emails.userIDOwner_fk=%i AND emails.inbox=", id.databaseRowId);

	if (inbox)
		query+="true";
	else
		query+="false";

	query+=" AND emails.userIDOther_fk=handles.userID_fk \n";

	if (ascendingSort)
		query+="ORDER BY date ASC;";
	else
		query+="ORDER BY date DESC;";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);

	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}

	int numRowsReturned = PQntuples(result);
	SendEmail_Data *email;
	int i;
	for (i=0; i < numRowsReturned; i++)
	{
		email = new LobbyDBSpec::SendEmail_Data;
		PostgreSQLInterface::PQGetValueFromBinary(&email->id.databaseRowId, result, i, "userIDOther_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&email->id.handle, result, i, "handle");
		PostgreSQLInterface::PQGetValueFromBinary(&email->emailMessageID, result, i, "emailMessageID_pk");
		PostgreSQLInterface::PQGetValueFromBinary(&email->subject, result, i, "subject");
		PostgreSQLInterface::PQGetValueFromBinary(&email->body, result, i, "body");
		PostgreSQLInterface::PQGetValueFromBinary(&email->creationTime, result, i, "date");
		PostgreSQLInterface::PQGetValueFromBinary(&email->attachment, &email->attachmentLength, result, i, "attachment");
		PostgreSQLInterface::PQGetValueFromBinary(&email->status, result, i, "status");
		PostgreSQLInterface::PQGetValueFromBinary(&email->wasOpened, result, i, "wasOpened");
		email->initialSenderStatus=0;
		email->initialRecipientStatus=0;
		email->validParameters=true;
		emails.Insert(email);
	}

	PQclear(result);
	validParameters=true;
	dbQuerySuccess=true;
}
void GetEmails_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetEmails_CB(this, wasCancelled, context);
	Deref();
}
GetEmails_PostgreSQLImpl* GetEmails_PostgreSQLImpl::Alloc(void) {return new GetEmails_PostgreSQLImpl;}
void GetEmails_PostgreSQLImpl::Free(GetEmails_PostgreSQLImpl *s) {delete s;}

void DeleteEmail_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	dbQuerySuccess=lobbyServer->ExecuteBlockingCommand(FormatString(
		"DELETE FROM emails WHERE emailmessageid_pk=%i", emailMessageID), &result, false);

	PQclear(result);
}
void DeleteEmail_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->DeleteEmail_CB(this, wasCancelled, context);
	Deref();
}
DeleteEmail_PostgreSQLImpl* DeleteEmail_PostgreSQLImpl::Alloc(void) {return new DeleteEmail_PostgreSQLImpl;}
void DeleteEmail_PostgreSQLImpl::Free(DeleteEmail_PostgreSQLImpl *s) {delete s;}

void UpdateEmailStatus_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	dbQuerySuccess=lobbyServer->ExecuteBlockingCommand(FormatString(
		"UPDATE emails SET status=%i, wasOpened=%s WHERE emailmessageid_pk=%i", status, wasOpened ? "true" : "false", emailMessageID), &result, false);

	PQclear(result);
}
void UpdateEmailStatus_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->UpdateEmailStatus_CB(this, wasCancelled, context);
	Deref();
}
UpdateEmailStatus_PostgreSQLImpl* UpdateEmailStatus_PostgreSQLImpl::Alloc(void) {return new UpdateEmailStatus_PostgreSQLImpl;}
void UpdateEmailStatus_PostgreSQLImpl::Free(UpdateEmailStatus_PostgreSQLImpl *s) {delete s;}


void GetHandleFromUserId_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	dbQuerySuccess=lobbyServer->ExecuteBlockingCommand(FormatString(
		"SELECT handle FROM handles WHERE handles.userID_fk=%i;",userId), &result, false);
	int numRowsReturned = PQntuples(result);
	success=numRowsReturned>0;
	if (success)
		PostgreSQLInterface::PQGetValueFromBinary(&handle, result, 0, "handle");
	PQclear(result);
}
void GetHandleFromUserId_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetHandleFromUserId_CB(this, wasCancelled, context);
	Deref();
}
GetHandleFromUserId_PostgreSQLImpl* GetHandleFromUserId_PostgreSQLImpl::Alloc(void) {return new GetHandleFromUserId_PostgreSQLImpl;}
void GetHandleFromUserId_PostgreSQLImpl::Free(GetHandleFromUserId_PostgreSQLImpl *s) {delete s;}

void GetUserIdFromHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (handle.IsEmpty())
	{
		success=false;
		dbQuerySuccess=true;
		return;
	}

	query = "SELECT userId_fk FROM handles WHERE (lower(handles.handle))=(lower($1::text));";
	PostgreSQLInterface::EncodeQueryInput("handle", handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);
	if (dbQuerySuccess==false)
	{
		success=false;
		PQclear(result);
		return;
	}
	int numRowsReturned = PQntuples(result);
	success=numRowsReturned>0;
	if (success)
		PostgreSQLInterface::PQGetValueFromBinary(&userId, result, 0, "userId_fk");

	PQclear(result);
}
void GetUserIdFromHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetUserIdFromHandle_CB(this, wasCancelled, context);
	Deref();
}
GetUserIdFromHandle_PostgreSQLImpl* GetUserIdFromHandle_PostgreSQLImpl::Alloc(void) {return new GetUserIdFromHandle_PostgreSQLImpl;}
void GetUserIdFromHandle_PostgreSQLImpl::Free(GetUserIdFromHandle_PostgreSQLImpl *s) {delete s;}

void AddDisallowedHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (handle.IsEmpty())
		return;

	query="INSERT INTO disallowedHandles (";
	PostgreSQLInterface::EncodeQueryInput("handle", handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void AddDisallowedHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddDisallowedHandle_CB(this, wasCancelled, context);
	Deref();
}
AddDisallowedHandle_PostgreSQLImpl* AddDisallowedHandle_PostgreSQLImpl::Alloc(void) {return new AddDisallowedHandle_PostgreSQLImpl;}
void AddDisallowedHandle_PostgreSQLImpl::Free(AddDisallowedHandle_PostgreSQLImpl *s) {delete s;}

void RemoveDisallowedHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query="DELETE FROM disallowedHandles where (lower(handle))=(lower(";
	PostgreSQLInterface::EncodeQueryInput("handle", handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	query+=valueStr;
	query+=")) ;";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void RemoveDisallowedHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->RemoveDisallowedHandle_CB(this, wasCancelled, context);
	Deref();
}
RemoveDisallowedHandle_PostgreSQLImpl* RemoveDisallowedHandle_PostgreSQLImpl::Alloc(void) {return new RemoveDisallowedHandle_PostgreSQLImpl;}
void RemoveDisallowedHandle_PostgreSQLImpl::Free(RemoveDisallowedHandle_PostgreSQLImpl *s) {delete s;}

void IsDisallowedHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query="SELECT COUNT(*) as count from disallowedHandles where (lower(handle))=(lower(";
	PostgreSQLInterface::EncodeQueryInput("handle", handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	query+=valueStr;
	query+=")) ;";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess)
	{
		char *outputStr=PQgetvalue(result, 0, 0);
		long long count;
		memcpy(&count, outputStr, sizeof(count));
		PostgreSQLInterface::EndianSwapInPlace((char*)&count, sizeof(count));
		exists=count>0;
	}
	else
		exists=false;
	PQclear(result);
}
void IsDisallowedHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->IsDisallowedHandle_CB(this, wasCancelled, context);
	Deref();
}
IsDisallowedHandle_PostgreSQLImpl* IsDisallowedHandle_PostgreSQLImpl::Alloc(void) {return new IsDisallowedHandle_PostgreSQLImpl;}
void IsDisallowedHandle_PostgreSQLImpl::Free(IsDisallowedHandle_PostgreSQLImpl *s) {delete s;}

void AddToActionHistory_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	// Nothing to write!
	if (actionTaken.IsEmpty() && description.IsEmpty() && sourceIpAddress.IsEmpty() && actionTime.IsEmpty())
		return;

	query="INSERT INTO actionHistory (userID_fk, ";
	PostgreSQLInterface::EncodeQueryInput("actionTime", actionTime, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("actionTaken", actionTaken, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("description", description, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("sourceIPAddress", sourceIpAddress, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	query+=paramTypeStr;
	query+=") VALUES (";
	if (id.hasDatabaseRowId)
		query+=FormatString("%i, ", id.databaseRowId);
	else
	{
		query+=FormatString("(SELECT userID_fk FROM handles where (lower(handle))=(lower($%i::text))", numParams+1);
		paramData[numParams]=(char*) id.handle.C_String();
		paramLength[numParams]=(int) strlen(id.handle.C_String());
		paramFormat[numParams]=PQEXECPARAM_FORMAT_TEXT;
		numParams++;
		query+="), ";
	}
	query+=valueStr;
	query+=") RETURNING creationTime;"; // Send

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);

	if (dbQuerySuccess)
		PostgreSQLInterface::PQGetValueFromBinary(&creationTime, result, 0, "creationTime");

	PQclear(result);
}
void AddToActionHistory_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddToActionHistory_CB(this, wasCancelled, context);
	Deref();
}
AddToActionHistory_PostgreSQLImpl* AddToActionHistory_PostgreSQLImpl::Alloc(void) {return new AddToActionHistory_PostgreSQLImpl;}
void AddToActionHistory_PostgreSQLImpl::Free(AddToActionHistory_PostgreSQLImpl *s) {delete s;}

void GetActionHistory_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query="SELECT userID_fk, actionTime, actionTaken, description, sourceIPAddress, creationTime FROM actionHistory WHERE (userID_fk=";
	if (id.hasDatabaseRowId)
		query+=FormatString("%i", id.databaseRowId);
	else
	{
		query+=FormatString("(SELECT userID_fk FROM handles where (lower(handle))=(lower($%i::text))", numParams+1);
		paramData[numParams]=(char*) id.handle.C_String();
		paramLength[numParams]=(int) strlen(id.handle.C_String());
		paramFormat[numParams]=PQEXECPARAM_FORMAT_TEXT;
		numParams++;
		query+=")";
	}
	query+=valueStr;
	query+=") ORDER BY creationTime "; // Send

	if (ascendingSort)
		query+="ASC";
	else
		query+="DESC";
	query+=";";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, true);
	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}

	int numRowsReturned = PQntuples(result);
	AddToActionHistory_Data *data;
	int i;
	for (i=0; i < numRowsReturned; i++)
	{
		data = new LobbyDBSpec::AddToActionHistory_Data;
		PostgreSQLInterface::PQGetValueFromBinary(&data->id.databaseRowId, result, i, "userID_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&data->actionTime, result, i, "actionTime");
		PostgreSQLInterface::PQGetValueFromBinary(&data->actionTaken, result, i, "actionTaken");
		PostgreSQLInterface::PQGetValueFromBinary(&data->description, result, i, "description");
		PostgreSQLInterface::PQGetValueFromBinary(&data->sourceIpAddress, result, i, "sourceIPAddress");
		PostgreSQLInterface::PQGetValueFromBinary(&data->creationTime, result, i, "creationTime");
		history.Insert(data);
	}

	PQclear(result);

}
void GetActionHistory_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetActionHistory_CB(this, wasCancelled, context);
	Deref();
}
GetActionHistory_PostgreSQLImpl* GetActionHistory_PostgreSQLImpl::Alloc(void) {return new GetActionHistory_PostgreSQLImpl;}
void GetActionHistory_PostgreSQLImpl::Free(GetActionHistory_PostgreSQLImpl *s) {delete s;}

void UpdateClanMember_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];
	
	dbQuerySuccess=true;

	if (userId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(userId.handle, userId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown user %s", userId.handle.C_String());
			return;
		}

		userId.hasDatabaseRowId=true;
	}

	if (clanId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetClanIDFromHandle(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan %s", clanId.handle.C_String());
			return;
		}

		clanId.hasDatabaseRowId=true;
	}
	else
	{
		if (lobbyServer->GetHandleFromClanID(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan with ID %i", clanId.databaseRowId);
			return;
		}
	}

	// Specified a user ID, must be a query to add new
	if (userId.databaseRowId!=0 ||
		userId.handle.IsEmpty()==false)
	{
		isNewRow=lobbyServer->IsValidUserID(userId.databaseRowId)==false;
	}
	else
		isNewRow=false;

	if (isNewRow)
	{
		PostgreSQLInterface::EncodeQueryInput("userId_fk", userId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("mEStatus1", mEStatus1, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("int_1", integers[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("int_2", integers[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("int_3", integers[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("int_4", integers[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("float_1", floats[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("float_2", floats[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("float_3", floats[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("float_4", floats[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("description_1", descriptions[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("description_2", descriptions[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
		PostgreSQLInterface::EncodeQueryInput("clanId_fk", clanId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
		PostgreSQLInterface::EncodeQueryInput("binaryData", binaryData, binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

		query = "INSERT INTO clanMembers (";

		query+=paramTypeStr;
		query+=") VALUES (";
		query+=valueStr;
		query+=")";
	}
	else
	{
		if (updateMEStatus1)
			PostgreSQLInterface::EncodeQueryUpdate("mEStatus1", mEStatus1, valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateInts[0])
			PostgreSQLInterface::EncodeQueryUpdate("int_1", integers[0], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateInts[1])
			PostgreSQLInterface::EncodeQueryUpdate("int_2", integers[1], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateInts[2])
			PostgreSQLInterface::EncodeQueryUpdate("int_3", integers[2], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateInts[3])
			PostgreSQLInterface::EncodeQueryUpdate("int_4", integers[3], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateFloats[0])
			PostgreSQLInterface::EncodeQueryUpdate("float_1", floats[0], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateFloats[1])
			PostgreSQLInterface::EncodeQueryUpdate("float_2", floats[1], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateFloats[2])
			PostgreSQLInterface::EncodeQueryUpdate("float_3", floats[2], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateFloats[3])
			PostgreSQLInterface::EncodeQueryUpdate("float_4", floats[3], valueStr, numParams, paramData, paramLength, paramFormat );
		if (updateDescriptions[0])
			PostgreSQLInterface::EncodeQueryUpdate("description_1", descriptions[0], valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (updateDescriptions[1])
			PostgreSQLInterface::EncodeQueryUpdate("description_2", descriptions[1], valueStr, numParams, paramData, paramLength, paramFormat, false );
		if (updateBinaryData)
			PostgreSQLInterface::EncodeQueryUpdate("binaryData", binaryData, binaryDataLength, valueStr, numParams, paramData, paramLength, paramFormat );
	
		query = "UPDATE clanMembers SET ";
		query+=valueStr;
		query+=" WHERE ";
		query+=FormatString("clanMembers.userId_fk = %i AND clanMembers.clanId_fk = %i",userId.databaseRowId, clanId.databaseRowId);
		if (updateMEStatus1 && hasRequiredLastStatus)
			query+=FormatString(" AND clanMembers.mEStatus1=%i", requiredLastMEStatus);

	}

	query+=";";
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
		failureMessage = lobbyServer->GetLastError();
	PQclear(result);
}
void UpdateClanMember_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->UpdateClanMember_CB(this, wasCancelled, context);
	Deref();
}
UpdateClanMember_PostgreSQLImpl* UpdateClanMember_PostgreSQLImpl::Alloc(void) {return new UpdateClanMember_PostgreSQLImpl;}
void UpdateClanMember_PostgreSQLImpl::Free(UpdateClanMember_PostgreSQLImpl *s) {delete s;}

void UpdateClan_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	dbQuerySuccess=true;

	if (clanId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetClanIDFromHandle(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan %s", clanId.handle.C_String());
			return;
		}

		clanId.hasDatabaseRowId=true;
	}

	if (updateInts[0])
		PostgreSQLInterface::EncodeQueryUpdate("int_1", integers[0], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateInts[1])
		PostgreSQLInterface::EncodeQueryUpdate("int_2", integers[1], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateInts[2])
		PostgreSQLInterface::EncodeQueryUpdate("int_3", integers[2], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateInts[3])
		PostgreSQLInterface::EncodeQueryUpdate("int_4", integers[3], valueStr, numParams, paramData, paramLength, paramFormat );

	if (updateFloats[0])
		PostgreSQLInterface::EncodeQueryUpdate("float_1", floats[0], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateFloats[1])
		PostgreSQLInterface::EncodeQueryUpdate("float_2", floats[1], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateFloats[2])
		PostgreSQLInterface::EncodeQueryUpdate("float_3", floats[2], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateFloats[3])
		PostgreSQLInterface::EncodeQueryUpdate("float_4", floats[3], valueStr, numParams, paramData, paramLength, paramFormat );

	if (updateDescriptions[0])
		PostgreSQLInterface::EncodeQueryUpdate("description_1", descriptions[0], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateDescriptions[1])
		PostgreSQLInterface::EncodeQueryUpdate("description_2", descriptions[1], valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateRequiresInvitationsToJoin)
		PostgreSQLInterface::EncodeQueryUpdate("requiresInvitationsToJoin", requiresInvitationsToJoin, valueStr, numParams, paramData, paramLength, paramFormat );
	if (updateBinaryData)
		PostgreSQLInterface::EncodeQueryUpdate("binaryData", binaryData, binaryDataLength, valueStr, numParams, paramData, paramLength, paramFormat );

	query = "UPDATE clans SET ";
	query+=valueStr;
	query+=RakNet::RakString(" WHERE clans.clanId_pk=%i;", clanId.databaseRowId);
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void UpdateClan_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->UpdateClan_CB(this, wasCancelled, context);
	Deref();
}
UpdateClan_PostgreSQLImpl* UpdateClan_PostgreSQLImpl::Alloc(void) {return new UpdateClan_PostgreSQLImpl;}
void UpdateClan_PostgreSQLImpl::Free(UpdateClan_PostgreSQLImpl *s) {delete s;}

void CreateClan_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	dbQuerySuccess=true;

	if (initialClanData.handle.IsEmpty())
	{
		initialClanData.failureMessage = "Handle cannot be an empty string.";
		return;
	}

	if (leaderData.userId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(leaderData.userId.handle, leaderData.userId.databaseRowId)==false)
		{
			leaderData.failureMessage=RakNet::RakString("Unknown user %s", leaderData.userId.handle.C_String());
			return;
		}
		leaderData.userId.hasDatabaseRowId=true;
	}

	// Clan data
	PostgreSQLInterface::EncodeQueryInput("handle", initialClanData.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("int_1", initialClanData.integers[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_2", initialClanData.integers[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_3", initialClanData.integers[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_4", initialClanData.integers[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_1", initialClanData.floats[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_2", initialClanData.floats[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_3", initialClanData.floats[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_4", initialClanData.floats[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("requiresInvitationsToJoin", initialClanData.requiresInvitationsToJoin, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("description_1", initialClanData.descriptions[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("description_2", initialClanData.descriptions[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("binaryData", initialClanData.binaryData, initialClanData.binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

	// Leader data
	PostgreSQLInterface::EncodeQueryInput("userId_fk", leaderData.userId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("mEStatus1", CLAN_MEMBER_STATUS_LEADER, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_1", leaderData.integers[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_2", leaderData.integers[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_3", leaderData.integers[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_4", leaderData.integers[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_1", leaderData.floats[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_2", leaderData.floats[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_3", leaderData.floats[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_4", leaderData.floats[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("description_1", leaderData.descriptions[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("description_2", leaderData.descriptions[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
	PostgreSQLInterface::EncodeQueryInput("binaryData", leaderData.binaryData, leaderData.binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

	query = "select * FROM CreateClanOnValidHandle(";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}
	char *getValueResult = PQgetvalue(result, 0, 0);
	if (getValueResult[0]!='O' || getValueResult[1]!='K')
	{
		initialClanData.failureMessage = getValueResult;
		PQclear(result);
		return;
	}
	// New clan row ID follows 'OK' in string format
	leaderData.clanId.databaseRowId=atoi(getValueResult+2);
	leaderData.clanId.hasDatabaseRowId=true;
	initialClanData.clanId=leaderData.clanId;
	PQclear(result);
}
void CreateClan_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->CreateClan_CB(this, wasCancelled, context);
	Deref();
}
CreateClan_PostgreSQLImpl* CreateClan_PostgreSQLImpl::Alloc(void) {return new CreateClan_PostgreSQLImpl;}
void CreateClan_PostgreSQLImpl::Free(CreateClan_PostgreSQLImpl *s) {delete s;}

void ChangeClanHandle_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (newHandle.IsEmpty())
	{
		dbQuerySuccess=true;
		failureMessage = "New handle cannot be an empty string.";
		return;
	}
	
	if (clanId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetClanIDFromHandle(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan %s", clanId.handle.C_String());
			return;
		}
	}

	PostgreSQLInterface::EncodeQueryInput("clanId_fk", clanId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("handle", newHandle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

	query = "select * FROM ChangeClanHandle(";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess)
	{
		char *valueStr = PQgetvalue(result, 0, 0);
		if (valueStr[0]!='O' || valueStr[1]!='K')
			failureMessage=valueStr;
	}
	else
	{
		failureMessage = lobbyServer->GetLastError();
	}
	PQclear(result);
}
void ChangeClanHandle_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->ChangeClanHandle_CB(this, wasCancelled, context);
	Deref();
}
ChangeClanHandle_PostgreSQLImpl* ChangeClanHandle_PostgreSQLImpl::Alloc(void) {return new ChangeClanHandle_PostgreSQLImpl;}
void ChangeClanHandle_PostgreSQLImpl::Free(ChangeClanHandle_PostgreSQLImpl *s) {delete s;}

void DeleteClan_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "DELETE FROM clans where clanId_pk=";
	if (clanId.hasDatabaseRowId)
		query+=FormatString("%i", clanId.databaseRowId);
	else
	{
		query+="(SELECT handleID_pk FROM clanHandles WHERE (lower(handle)) = (lower(";
		PostgreSQLInterface::EncodeQueryInput("handle", clanId.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
	}
	query+=";";
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void DeleteClan_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->DeleteClan_CB(this, wasCancelled, context);
	Deref();
}
DeleteClan_PostgreSQLImpl* DeleteClan_PostgreSQLImpl::Alloc(void) {return new DeleteClan_PostgreSQLImpl;}
void DeleteClan_PostgreSQLImpl::Free(DeleteClan_PostgreSQLImpl *s) {delete s;}

void GetClans_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	const char *sortType;

	if (userId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(userId.handle, userId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown user %s", userId.handle.C_String());
			return;
		}
		userId.hasDatabaseRowId=true;
	}

	if (ascendingSort)
		sortType="ASC";
	else
		sortType="DESC";

	query = 
		RakNet::RakString(
		"SELECT\n"
		"clans.clanId_pk as cl_clanId_pk,clans.int_1 as cl_int_1,clans.int_2 as cl_int_2,clans.int_3 as cl_int_3,clans.int_4 as cl_int_4,\n"
		"clans.float_1 as cl_float_1,clans.float_2 as cl_float_2,clans.float_3 as cl_float_3,clans.float_4 as cl_float_4,\n"
		"clans.requiresInvitationsToJoin as cl_requiresInvitationsToJoin,clans.description_1 as cl_description_1,clans.description_2 as cl_description_2,clans.creationTime as cl_creationTime,clans.binaryData as cl_binaryData,\n"
		"clanHandles.handle as cl_handle,\n"
		"\n"
		"clanMembers.clanId_fk as cm_clanId_fk,clanMembers.userId_fk as cm_userId_fk,clanMembers.mEStatus1 as cm_mEStatus1,clanMembers.int_1 as cm_int_1,clanMembers.int_2 as cm_int_2,clanMembers.int_3 as cm_int_3,clanMembers.int_4 as cm_int_4,\n"
		"clanMembers.float_1 as cm_float_1,clanMembers.float_2 as cm_float_2,clanMembers.float_3 as cm_float_3,clanMembers.float_4 as cm_float_4,\n"
		"clanMembers.description_1 as cm_description_1,clanMembers.description_2 as cm_description_2,clanMembers.lastUpdate as cm_lastUpdate,clanMembers.binaryData as cm_binaryData,\n"
		"handles.handle as cm_handle\n"
		"\n"
		"FROM clans, clanHandles, clanMembers, handles WHERE\n"
		"clans.clanId_pk=clanMembers.clanId_fk AND\n"
		"clanMembers.userId_fk=%i AND\n"
		"clans.handleID_fk=clanHandles.handleID_pk AND\n"
		"handles.userId_fk=%i\n"
		"ORDER BY clans.clanId_pk %s, clanMembers.clanMemberId_pk %s;\n;", userId.databaseRowId, userId.databaseRowId, sortType, sortType);

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}
	int numRowsReturned = PQntuples(result);
	LobbyDBSpec::UpdateClan_Data *clan;
	LobbyDBSpec::UpdateClanMember_Data *member;
	DatabaseKey clanRowId, lastClanRowId;
	int i;
	int status;
	
	lastClanRowId=0;

	for (i=0; i < numRowsReturned; i++)
	{
		PostgreSQLInterface::PQGetValueFromBinary(&clanRowId, result, i, "cl_clanId_pk");
		if (lastClanRowId!=clanRowId)
		{
			clan = new LobbyDBSpec::UpdateClan_Data;
			clan->clanId.databaseRowId=clanRowId;
			PostgreSQLInterface::PQGetValueFromBinary(&clan->clanId.handle, result, i, "cl_handle");
			clan->handle=clan->clanId.handle;
			PostgreSQLInterface::PQGetValueFromBinary(&clan->integers[0], result, i, "cl_int_1");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->integers[1], result, i, "cl_int_2");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->integers[2], result, i, "cl_int_3");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->integers[3], result, i, "cl_int_4");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->floats[0], result, i, "cl_float_1");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->floats[1], result, i, "cl_float_2");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->floats[2], result, i, "cl_float_3");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->floats[3], result, i, "cl_float_4");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->requiresInvitationsToJoin, result, i, "cl_requiresInvitationsToJoin");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->descriptions[0], result, i, "cl_description_1");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->descriptions[1], result, i, "cl_description_2");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->creationTime, result, i, "cl_creationTime");
			PostgreSQLInterface::PQGetValueFromBinary(&clan->binaryData, &clan->binaryDataLength, result, i, "cl_binaryData");
			clans.Insert(clan);
			lastClanRowId=clanRowId;
		}
		
		member = new LobbyDBSpec::UpdateClanMember_Data;
		PostgreSQLInterface::PQGetValueFromBinary(&member->userId.databaseRowId, result, i, "cm_userId_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&member->userId.handle, result, i, "cm_handle");
		PostgreSQLInterface::PQGetValueFromBinary(&status, result, i, "cm_mEStatus1");
		member->mEStatus1=(LobbyDBSpec::ClanMemberStatus)status;
		PostgreSQLInterface::PQGetValueFromBinary(&member->integers[0], result, i, "cm_int_1");
		PostgreSQLInterface::PQGetValueFromBinary(&member->integers[1], result, i, "cm_int_2");
		PostgreSQLInterface::PQGetValueFromBinary(&member->integers[2], result, i, "cm_int_3");
		PostgreSQLInterface::PQGetValueFromBinary(&member->integers[3], result, i, "cm_int_4");
		PostgreSQLInterface::PQGetValueFromBinary(&member->floats[0], result, i, "cm_float_1");
		PostgreSQLInterface::PQGetValueFromBinary(&member->floats[1], result, i, "cm_float_2");
		PostgreSQLInterface::PQGetValueFromBinary(&member->floats[2], result, i, "cm_float_3");
		PostgreSQLInterface::PQGetValueFromBinary(&member->floats[3], result, i, "cm_float_4");
		PostgreSQLInterface::PQGetValueFromBinary(&member->descriptions[0], result, i, "cm_description_1");
		PostgreSQLInterface::PQGetValueFromBinary(&member->descriptions[1], result, i, "cm_description_2");
		PostgreSQLInterface::PQGetValueFromBinary(&member->lastUpdate, result, i, "cm_lastUpdate");
		PostgreSQLInterface::PQGetValueFromBinary(&member->binaryData, &clan->binaryDataLength, result, i, "cm_binaryData");

		clan->members.Insert(member);
	}

	PQclear(result);

}
void GetClans_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetClans_CB(this, wasCancelled, context);
	Deref();
}
GetClans_PostgreSQLImpl* GetClans_PostgreSQLImpl::Alloc(void) {return new GetClans_PostgreSQLImpl;}
void GetClans_PostgreSQLImpl::Free(GetClans_PostgreSQLImpl *s) {delete s;}

void RemoveFromClan_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = "DELETE FROM clanMembers where clanId_fk=";
	if (clanId.hasDatabaseRowId)
		query+=FormatString("%i", clanId.databaseRowId);
	else
	{
		query+="(SELECT clanID_fk FROM clanHandles WHERE (lower(handle)) = (lower(";
		PostgreSQLInterface::EncodeQueryInput("handle", clanId.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
		valueStr.Clear();
		paramTypeStr.Clear();
	}
	query+=" AND userId_fk=";
	if (userId.hasDatabaseRowId)
		query+=FormatString("%i", userId.databaseRowId);
	else
	{
		query+="(SELECT userID_fk FROM handles WHERE (lower(handle)) = (lower(";
		PostgreSQLInterface::EncodeQueryInput("handle", userId.handle, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );
		query+=valueStr;
		query+=")) )";
	}
	if (hasRequiredLastStatus)
		query+=RakNet::RakString(" AND mEStatus1=%i",requiredLastMEStatus);
	query+=";";
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void RemoveFromClan_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->RemoveFromClan_CB(this, wasCancelled, context);
	Deref();
}
RemoveFromClan_PostgreSQLImpl* RemoveFromClan_PostgreSQLImpl::Alloc(void) {return new RemoveFromClan_PostgreSQLImpl;}
void RemoveFromClan_PostgreSQLImpl::Free(RemoveFromClan_PostgreSQLImpl *s) {delete s;}

void AddToClanBoard_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (userId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetUserIDFromHandle(userId.handle, userId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown user %s", userId.handle.C_String());
			return;
		}
		userId.hasDatabaseRowId=true;
	}

	if (clanId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetClanIDFromHandle(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan %s", clanId.handle.C_String());
			return;
		}
		clanId.hasDatabaseRowId=true;
	}

	query = "INSERT INTO clanBoardPosts (";

	PostgreSQLInterface::EncodeQueryInput("clanId_fk", clanId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("userId_fk", userId.databaseRowId, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("subject", subject, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("body", body, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );
	PostgreSQLInterface::EncodeQueryInput("int_1", integers[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_2", integers[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_3", integers[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("int_4", integers[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_1", floats[0], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_2", floats[1], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_3", floats[2], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("float_4", floats[3], paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat );
	PostgreSQLInterface::EncodeQueryInput("binaryData", binaryData, binaryDataLength, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, true );

	query+=paramTypeStr;
	query+=") VALUES (";
	query+=valueStr;
	query+=");";

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void AddToClanBoard_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->AddToClanBoard_CB(this, wasCancelled, context);
	Deref();
}
AddToClanBoard_PostgreSQLImpl* AddToClanBoard_PostgreSQLImpl::Alloc(void) {return new AddToClanBoard_PostgreSQLImpl;}
void AddToClanBoard_PostgreSQLImpl::Free(AddToClanBoard_PostgreSQLImpl *s) {delete s;}

void RemoveFromClanBoard_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	query = RakNet::RakString("DELETE FROM clanBoardPosts WHERE postId_pk=%i;", postId);
	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void RemoveFromClanBoard_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->RemoveFromClanBoard_CB(this, wasCancelled, context);
	Deref();
}
RemoveFromClanBoard_PostgreSQLImpl* RemoveFromClanBoard_PostgreSQLImpl::Alloc(void) {return new RemoveFromClanBoard_PostgreSQLImpl;}
void RemoveFromClanBoard_PostgreSQLImpl::Free(RemoveFromClanBoard_PostgreSQLImpl *s) {delete s;}

void GetClanBoard_PostgreSQLImpl::Process(void *context)
{
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	if (clanId.hasDatabaseRowId==false)
	{
		if (lobbyServer->GetClanIDFromHandle(clanId.handle, clanId.databaseRowId)==false)
		{
			failureMessage=RakNet::RakString("Unknown clan %s", clanId.handle.C_String());
			return;
		}
		clanId.hasDatabaseRowId=true;
	}

	const char *sortType;
	if (ascendingSort)
		sortType="ASC";
	else
		sortType="DESC";

	query = 
		RakNet::RakString(
		"SELECT\n"
		"clanBoardPosts.postId_pk,clanBoardPosts.userId_fk,clanBoardPosts.subject,clanBoardPosts.body,\n"
		"clanBoardPosts.int_1,clanBoardPosts.int_2,clanBoardPosts.int_3,clanBoardPosts.int_4,\n"
		"clanBoardPosts.float_1,clanBoardPosts.float_2,clanBoardPosts.float_3,clanBoardPosts.float_4,clanBoardPosts.creationTime,clanBoardPosts.binaryData,\n"
		"clanHandles.handle as cl_handle,\n"
		"handles.handle as cm_handle\n"
		"FROM clanBoardPosts, clanHandles, handles WHERE\n"
		"clanBoardPosts.clanId_fk=%i AND\n"
		"clanBoardPosts.clanId_fk=clanHandles.clanId_fk AND\n"
		"clanBoardPosts.userId_fk=handles.userId_fk\n"
		"ORDER BY creationTime %s;", clanId.databaseRowId, sortType);

	result = PQexecParams(lobbyServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=lobbyServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}
	int numRowsReturned = PQntuples(result);
	LobbyDBSpec::AddToClanBoard_Data *post;
	int i;

	for (i=0; i < numRowsReturned; i++)
	{
		post = new LobbyDBSpec::AddToClanBoard_Data;
		PostgreSQLInterface::PQGetValueFromBinary(&post->postId, result, i, "postId_pk");
		PostgreSQLInterface::PQGetValueFromBinary(&post->userId.databaseRowId, result, i, "userId_fk");
		PostgreSQLInterface::PQGetValueFromBinary(&post->userId.handle, result, i, "cm_handle");
		PostgreSQLInterface::PQGetValueFromBinary(&post->clanId.handle, result, i, "cl_handle");
		post->clanId.databaseRowId=clanId.databaseRowId;
		clanId.handle=post->clanId.handle;
		PostgreSQLInterface::PQGetValueFromBinary(&post->subject, result, i, "subject");
		PostgreSQLInterface::PQGetValueFromBinary(&post->body, result, i, "body");
		PostgreSQLInterface::PQGetValueFromBinary(&post->integers[0], result, i, "int_1");
		PostgreSQLInterface::PQGetValueFromBinary(&post->integers[1], result, i, "int_2");
		PostgreSQLInterface::PQGetValueFromBinary(&post->integers[2], result, i, "int_3");
		PostgreSQLInterface::PQGetValueFromBinary(&post->integers[3], result, i, "int_4");
		PostgreSQLInterface::PQGetValueFromBinary(&post->floats[0], result, i, "float_1");
		PostgreSQLInterface::PQGetValueFromBinary(&post->floats[1], result, i, "float_2");
		PostgreSQLInterface::PQGetValueFromBinary(&post->floats[2], result, i, "float_3");
		PostgreSQLInterface::PQGetValueFromBinary(&post->floats[3], result, i, "float_4");
		PostgreSQLInterface::PQGetValueFromBinary(&post->creationTime, result, i, "cl_creationTime");
		PostgreSQLInterface::PQGetValueFromBinary(&post->binaryData, &post->binaryDataLength, result, i, "cl_binaryData");
		board.Insert(post);
	}

	PQclear(result);
}
void GetClanBoard_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetClanBoard_CB(this, wasCancelled, context);
	Deref();
}
GetClanBoard_PostgreSQLImpl* GetClanBoard_PostgreSQLImpl::Alloc(void) {return new GetClanBoard_PostgreSQLImpl;}
void GetClanBoard_PostgreSQLImpl::Free(GetClanBoard_PostgreSQLImpl *s) {delete s;}
