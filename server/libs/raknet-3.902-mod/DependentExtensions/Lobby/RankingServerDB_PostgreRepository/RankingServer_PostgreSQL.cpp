/// \file
/// \brief An implementation of the RankingRepositoryInterface to use PostgreSQL to store the relevant data
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "RankingServer_PostgreSQL.h"
// libpq-fe.h is part of PostgreSQL which must be installed on this computer to use the PostgreRepository
#include "libpq-fe.h"
#include <RakAssert.h>
#include "FormatString.h"

#define PQEXECPARAM_FORMAT_TEXT		0
#define PQEXECPARAM_FORMAT_BINARY	1

using namespace RankingServerDBSpec;

RankingServer_PostgreSQL::RankingServer_PostgreSQL()
{
	rankingServerCallback=0;
}
RankingServer_PostgreSQL::~RankingServer_PostgreSQL()
{
}
bool RankingServer_PostgreSQL::CreateRankingServerTables(void)
{
	if (isConnected==false)
		return false;

	const char *rankingServerDBCreateTablesStr =
		"CREATE TABLE rankingServer ("
		"rowId_pk serial UNIQUE,"
		"participantADbId_primaryKey integer NOT NULL,"
		"participantADbId_secondaryKey integer NOT NULL,"
		"participantBDbId_primaryKey integer NOT NULL,"
		"participantBDbId_secondaryKey integer NOT NULL,"
		"participantAScore real,"
		"participantBScore integer,"
		"participantAOldRating real,"
		"participantANewRating real NOT NULL,"
		"participantBOldRating real,"
		"participantBNewRating real NOT NULL,"
		"gameDbId_primaryKey integer NOT NULL,"
		"gameDbId_secondaryKey integer NOT NULL,"
		"matchTime bigint NOT NULL DEFAULT EXTRACT(EPOCH FROM current_timestamp),"
		"matchNotes text,"
		"matchBinaryData bytea);"
		"CREATE TABLE trustedIPs ("
		"rowId_pk serial UNIQUE,"
		"ip text NOT NULL,"
		"gameDbId_primaryKey_fk integer NOT NULL,"
		"gameDbId_secondaryKey_fk integer NOT NULL);";

	PGresult *result;
	bool res = ExecuteBlockingCommand(rankingServerDBCreateTablesStr, &result, false);
	PQclear(result);
	return res;
}
bool RankingServer_PostgreSQL::DestroyRankingServerTables(void)
{
	if (isConnected==false)
		return false;

	const char *rankingServerDBDropTablesStr =
		"DROP TABLE rankingServer;";

	PGresult *result;
	bool res = ExecuteBlockingCommand(rankingServerDBDropTablesStr, &result, false);
	PQclear(result);
	return res;
}
void RankingServer_PostgreSQL::PushFunctor(RankingServerFunctor *functor, void *context)
{
	functor->rankingServer=this;
	functor->callback=rankingServerCallback;
	FunctionThreadDependentClass::PushFunctor(functor,context);
}
void RankingServer_PostgreSQL::AssignCallback(RankingServerCBInterface *cb)
{
	rankingServerCallback=cb;
}
void SubmitMatch_PostgreSQLImpl::Process(void *context)
{
	RakAssert(rankingServer);
	PGresult *result;

	RakNet::RakString query;
	query = "INSERT INTO rankingServer ("
		"participantADbId_primaryKey, participantADbId_secondaryKey, participantBDbId_primaryKey, participantBDbId_secondaryKey"
		",participantAScore, participantBScore, participantAOldRating, participantANewRating, participantBOldRating, participantBNewRating"
		",gameDbId_primaryKey,gameDbId_secondaryKey";
	if (matchNotes.IsEmpty()==false)
		query+=",matchNotes";
	if (matchBinaryDataLength>0 && matchBinaryData)
		query+=",matchBinaryData";
	query+=") VALUES (";
	query+=FormatString("%i", participantADbId.primaryKey);
	query+=",";
	query+=FormatString("%i", participantADbId.secondaryKey);
	query+=",";
	query+=FormatString("%i", participantBDbId.primaryKey);
	query+=",";
	query+=FormatString("%i", participantBDbId.secondaryKey);
	query+=",";
	query+=FormatString("%f", participantAScore);
	query+=",";
	query+=FormatString("%f", participantBScore);
	query+=",";
	query+=FormatString("%f", participantAOldRating);
	query+=",";
	query+=FormatString("%f", participantANewRating);
	query+=",";
	query+=FormatString("%f", participantBOldRating);
	query+=",";
	query+=FormatString("%f", participantBNewRating);
	query+=",";
	query+=FormatString("%i", gameDbId.primaryKey);
	query+=",";
	query+=FormatString("%i", gameDbId.secondaryKey);

	const char *binaryData[2];
	int binaryLengths[2];
	int binaryFormats[2];
	int numParams;

	if (matchNotes.IsEmpty()==false)
	{
		query+=",$1::text";

		binaryData[0]=matchNotes.C_String();
		binaryLengths[0]=(int) matchNotes.GetLength();
		binaryFormats[0]=PQEXECPARAM_FORMAT_TEXT;

		if (matchBinaryDataLength>0 && matchBinaryData)
		{
			query+=",$2::bytea";

			binaryData[1]=matchBinaryData;
			binaryLengths[1]=matchBinaryDataLength;
			binaryFormats[1]=PQEXECPARAM_FORMAT_BINARY;
			numParams=2;
		}
		else
			numParams=1;
	}
	else if (matchBinaryDataLength>0 && matchBinaryData)
	{
		query+=",$1::bytea";

		binaryData[0]=matchBinaryData;
		binaryLengths[0]=matchBinaryDataLength;
		binaryFormats[0]=PQEXECPARAM_FORMAT_BINARY;
		numParams=1;
	}
	else
		numParams=0;
	query+=");";

	result = PQexecParams(rankingServer->pgConn, query.C_String(),numParams,0,binaryData,binaryLengths,binaryFormats,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=rankingServer->IsResultSuccessful(result, false);
	PQclear(result);
}
void ModifyTrustedIPList_PostgreSQLImpl::Process(void *context)
{
	RakAssert(rankingServer);
	PGresult *result;
	RakNet::RakString query;
	RakNet::RakString paramTypeStr;
	RakNet::RakString valueStr;
	int numParams=0;
	char *paramData[512];
	int paramLength[512];
	int paramFormat[512];

	PostgreSQLInterface::EncodeQueryInput("ip", ip, paramTypeStr, valueStr, numParams, paramData, paramLength, paramFormat, false );

	if (addToList)
	{
		query = "INSERT INTO trustedIPs (ip, gameDbId_primaryKey_fk, gameDbId_secondaryKey_fk) VALUES ($1::text, ";
		query+=FormatString("%i", gameDbId.primaryKey);
		query+=",";
		query+=FormatString("%i", gameDbId.secondaryKey);
		query+=");";

	}
	else
	{
		query = "DELETE FROM trustedIPs where (ip=$1::text);";
	}

	result = PQexecParams(rankingServer->pgConn, query.C_String(),numParams,0,paramData,paramLength,paramFormat,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess=rankingServer->IsResultSuccessful(result, true);
}
void GetTrustedIPList_PostgreSQLImpl::Process(void *context)
{
	"CREATE TABLE trustedIPs ("
		"c text NOT NULL,"
		"gameDbId_primaryKey_fk integer NOT NULL,"
		"gameDbId_secondaryKey_fk integer NOT NULL);";

	PGresult *result;
	dbQuerySuccess = rankingServer->ExecuteBlockingCommand("SELECT ip, gameDbId_primaryKey_fk, gameDbId_secondaryKey_fk FROM trustedIPs;", &result, false);
	if (dbQuerySuccess)
	{
		int numRowsReturned = PQntuples(result);
		int i;
		ModifyTrustedIPList_Data *dat;
		for (i=0; i < numRowsReturned; i++)
		{
			dat = new ModifyTrustedIPList_Data;
			char *outputStr=PQgetvalue(result, i, PQfnumber(result, "ip"));
			dat->ip=outputStr;
			dat->systemAddress.SetBinaryAddress(dat->ip.C_String());
			dat->systemAddress.port=65535;
			dat->gameDbId.primaryKey=atoi(PQgetvalue(result, i, PQfnumber(result, "gameDbId_primaryKey_fk")));
			dat->gameDbId.secondaryKey=atoi(PQgetvalue(result, i, PQfnumber(result, "gameDbId_secondaryKey_fk")));
			ipList.Insert(dat);
		}	
	}
	
	PQclear(result);
}
void GetRatingForParticipant_PostgreSQLImpl::Process(void *context)
{
	RakAssert(rankingServer);
	PGresult *result;

	// Get the most recent rating for a particular player
	dbQuerySuccess=rankingServer->ExecuteBlockingCommand(
		FormatString(
		"SELECT newRating FROM ( "
			"SELECT "
				"participantANewRating as newRating, "
				"matchTime "
			"FROM rankingServer "
			"WHERE "
				"participantADbId_primaryKey=%i AND "
				"participantADbId_secondaryKey=%i AND "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			"UNION "
			"SELECT "
				"participantBNewRating as newRating, "
				"matchTime "
			"FROM rankingServer "
			"WHERE "
				"participantBDbId_primaryKey=%i AND "
				"participantBDbId_secondaryKey=%i AND "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			") as unionTable "
			"ORDER BY matchTime DESC LIMIT 1"
	//	"WHERE "
	//		"matchTime=( "
	//		"SELECT max(matchTime) FROM rankingServer "
	//		"WHERE "
	//			"((participantADbId_primaryKey=%i AND participantADbId_secondaryKey=%i) OR "
	//			"(participantBDbId_primaryKey=%i AND participantBDbId_secondaryKey=%i)) AND "
	//			"gameDbId_primaryKey=%i AND "
	//			"gameDbId_secondaryKey=%i "
		";",
		participantDbId.primaryKey,
		participantDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey,
		participantDbId.primaryKey,
		participantDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey
//		participantDbId.primaryKey,
//		participantDbId.secondaryKey,
//		participantDbId.primaryKey,
//		participantDbId.secondaryKey,
//		gameDbId.primaryKey,
//		gameDbId.secondaryKey
	), &result, false);
	if (dbQuerySuccess)
	{
		int numRowsReturned = PQntuples(result);
		if (numRowsReturned>0)
		{
			char *valueStr=PQgetvalue(result, 0, 0);
			if (valueStr)
			{
				participantFound=true;
				rating=(float) atof(valueStr);
			}
			else
				participantFound=false;			
		}
		else
		{
			participantFound=false;
		}
	}
	PQclear(result);
}
void GetRatingForParticipants_PostgreSQLImpl::Process(void *context)
{
	RakAssert(rankingServer);
	PGresult *result;

	// Get the most recent rating for each player
	result = PQexecParams(rankingServer->pgConn,
		FormatString(
		"SELECT newRating, participantDbId_primaryKey, participantDbId_secondaryKey FROM "
			"(SELECT "
				"participantANewRating as newRating, "
				"participantADbId_primaryKey as participantDbId_primaryKey, "
				"participantADbId_secondaryKey as participantDbId_secondaryKey, "
				"matchTime "
			"FROM rankingServer "
			"WHERE "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			"UNION "
			"SELECT "
				"participantBNewRating as newRating, "
				"participantBDbId_primaryKey as participantDbId_primaryKey, "
				"participantBDbId_secondaryKey as participantDbId_secondaryKey, "
				"matchTime "
			"FROM rankingServer "
			"WHERE "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			") as tbl1 "
		"INNER JOIN "
			"(SELECT participantDbId_primaryKey,participantDbId_secondaryKey,max(matchTime) as matchTime FROM "
				"(SELECT "
					"participantADbId_primaryKey as participantDbId_primaryKey, "
					"participantADbId_secondaryKey as participantDbId_secondaryKey, "
					"matchTime "
				"FROM rankingServer "
				"WHERE "
					"gameDbId_primaryKey=%i AND "
					"gameDbId_secondaryKey=%i "
				"UNION "
				"SELECT "
					"participantBDbId_primaryKey as participantDbId_primaryKey, "
					"participantBDbId_secondaryKey as participantDbId_secondaryKey, "
					"matchTime "
				"FROM rankingServer "
				"WHERE "
					"gameDbId_primaryKey=%i AND "
					"gameDbId_secondaryKey=%i "
				") as tbl2 "
			"GROUP BY participantDbId_primaryKey,participantDbId_secondaryKey "
			") as tbl3 "
		"USING (matchTime,participantDbId_primaryKey,participantDbId_secondaryKey) "
		";",
		gameDbId.primaryKey,
		gameDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey
		),
		0,0,0,0,0,PQEXECPARAM_FORMAT_TEXT);
	dbQuerySuccess = rankingServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
	{
		dbQuerySuccess=false;
		PQclear(result);
		return;
	}

	int newRating_index = PQfnumber(result, "newRating");
	int participantDbId_primaryKey_index = PQfnumber(result, "participantDbId_primaryKey");
	int participantDbId_secondaryKey_index = PQfnumber(result, "participantDbId_secondaryKey");
	int numRowsReturned = PQntuples(result);
	int rowIndex;
	RatedEntity ratedEntity;
	char *valueStr;
	for (rowIndex=0; rowIndex < numRowsReturned; rowIndex++)
	{
		valueStr = PQgetvalue(result, rowIndex, newRating_index);
		if (valueStr && valueStr[0])
		{
			ratedEntity.newRating = (float) atof(valueStr);
			valueStr = PQgetvalue(result, rowIndex, participantDbId_primaryKey_index);
			ratedEntity.participantDbId.primaryKey = atoi(valueStr);
			valueStr = PQgetvalue(result, rowIndex, participantDbId_secondaryKey_index);
			if (valueStr && valueStr[0])
				ratedEntity.participantDbId.secondaryKey = atoi(valueStr);
			outputList.Insert(ratedEntity);
		}
	}

	PQclear(result);
}
void GetHistoryForParticipant_PostgreSQLImpl::Process(void *context)
{
	RakAssert(rankingServer);
	PGresult *result;

	// For one particular player, get his history

	result = PQexecParams(rankingServer->pgConn,
		FormatString(
		"SELECT * FROM "
			"(SELECT "
				"participantAScore as participantScore, "
				"participantAOldRating as participantOldRating, "
				"participantANewRating as participantNewRating, "
				"participantBScore as opponentScore, "
				"participantBOldRating as opponentOldRating, "
				"participantBNewRating as opponentNewRating, "
				"participantBDbId_primaryKey as opponentDbId_primaryKey, "
				"participantBDbId_secondaryKey as opponentDbId_secondaryKey, "
				"matchTime, "
				"matchNotes, "
				"matchBinaryData "
			"FROM rankingServer "
			"WHERE "
				"participantADbId_primaryKey=%i AND "
				"participantADbId_secondaryKey=%i AND "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			"UNION "
			"SELECT "
				"participantBScore as participantScore, "
				"participantBOldRating as participantOldRating, "
				"participantBNewRating as participantNewRating, "
				"participantAScore as opponentScore, "
				"participantAOldRating as opponentOldRating, "
				"participantANewRating as opponentNewRating, "
				"participantADbId_primaryKey as opponentDbId_primaryKey, "
				"participantADbId_secondaryKey as opponentDbId_secondaryKey, "
				"matchTime, "
				"matchNotes, "
				"matchBinaryData "
			"FROM rankingServer "
			"WHERE "
				"participantBDbId_primaryKey=%i AND "
				"participantBDbId_secondaryKey=%i AND "
				"gameDbId_primaryKey=%i AND "
				"gameDbId_secondaryKey=%i "
			") as tbl1 ORDER BY matchTime ASC;",
		participantDbId.primaryKey,
		participantDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey,
		participantDbId.primaryKey,
		participantDbId.secondaryKey,
		gameDbId.primaryKey,
		gameDbId.secondaryKey
		),
		0,0,0,0,0,PQEXECPARAM_FORMAT_BINARY);
	dbQuerySuccess = rankingServer->IsResultSuccessful(result, false);
	if (dbQuerySuccess==false)
	{
		PQclear(result);
		return;
	}


	int numRowsReturned = PQntuples(result);
	int rowIndex;
	SubmitMatch_Data* ratingData;
	for (rowIndex=0; rowIndex < numRowsReturned; rowIndex++)
	{
		ratingData = new SubmitMatch_Data;
		ratingData->participantADbId=participantDbId;
		ratingData->gameDbId=gameDbId;
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantAScore, result, rowIndex, "participantScore");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantAOldRating, result, rowIndex, "participantOldRating");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantANewRating, result, rowIndex, "participantNewRating");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantBScore, result, rowIndex, "opponentScore");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantBOldRating, result, rowIndex, "opponentOldRating");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantBNewRating, result, rowIndex, "opponentNewRating");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantBDbId.primaryKey, result, rowIndex, "opponentDbId_primaryKey");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->participantBDbId.secondaryKey, result, rowIndex, "opponentDbId_secondaryKey");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->matchTime, result, rowIndex, "matchTime");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->matchNotes, result, rowIndex, "matchNotes");
		PostgreSQLInterface::PQGetValueFromBinary(&ratingData->matchBinaryData, &ratingData->matchBinaryDataLength, result, rowIndex, "matchBinaryData");
		matchHistoryList.Insert(ratingData);
	}

	PQclear(result);
}
void SubmitMatch_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->SubmitMatch_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
SubmitMatch_PostgreSQLImpl* SubmitMatch_PostgreSQLImpl::Alloc(void) {return new SubmitMatch_PostgreSQLImpl;}
void SubmitMatch_PostgreSQLImpl::Free(SubmitMatch_PostgreSQLImpl *s) {delete s;}

void ModifyTrustedIPList_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->ModifyTrustedIPList_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
ModifyTrustedIPList_PostgreSQLImpl* ModifyTrustedIPList_PostgreSQLImpl::Alloc(void) {return new ModifyTrustedIPList_PostgreSQLImpl;}
void ModifyTrustedIPList_PostgreSQLImpl::Free(ModifyTrustedIPList_PostgreSQLImpl *s) {delete s;}

void GetTrustedIPList_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetTrustedIPList_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
GetTrustedIPList_PostgreSQLImpl* GetTrustedIPList_PostgreSQLImpl::Alloc(void) {return new GetTrustedIPList_PostgreSQLImpl;}
void GetTrustedIPList_PostgreSQLImpl::Free(GetTrustedIPList_PostgreSQLImpl *s) {delete s;}

char* RankingServerFunctor::AllocBytes(int numBytes) {return new char[numBytes];}
void RankingServerFunctor::FreeBytes(char *data) {delete [] data;}

void GetRatingForParticipant_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetRatingForParticipant_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
GetRatingForParticipant_PostgreSQLImpl* GetRatingForParticipant_PostgreSQLImpl::Alloc(void) {return new GetRatingForParticipant_PostgreSQLImpl;}
void GetRatingForParticipant_PostgreSQLImpl::Free(GetRatingForParticipant_PostgreSQLImpl *s) {delete s;}

void GetRatingForParticipants_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetRatingForParticipants_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
GetRatingForParticipants_PostgreSQLImpl* GetRatingForParticipants_PostgreSQLImpl::Alloc(void) {return new GetRatingForParticipants_PostgreSQLImpl;}
void GetRatingForParticipants_PostgreSQLImpl::Free(GetRatingForParticipants_PostgreSQLImpl *s) {delete s;}

void GetHistoryForParticipant_PostgreSQLImpl::HandleResult(bool wasCancelled, void *context)
{
	if (callback) callback->GetHistoryForParticipant_CB(this, wasCancelled, context); // Call the callback to another class
	Deref(); // Deallocate, since a pointer to this class must have been allocated on the heap
}
GetHistoryForParticipant_PostgreSQLImpl* GetHistoryForParticipant_PostgreSQLImpl::Alloc(void) {return new GetHistoryForParticipant_PostgreSQLImpl;}
void GetHistoryForParticipant_PostgreSQLImpl::Free(GetHistoryForParticipant_PostgreSQLImpl *s) {delete s;}