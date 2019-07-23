/// \file
/// \brief An implementation of the RankingServerPostgreRepository to use PostgreSQL to store ranking info for users.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RANKING_SERVER_POSTGRESQL_REPOSITORY_H
#define __RANKING_SERVER_POSTGRESQL_REPOSITORY_H

#include "RankingServerDBSpec.h"
#include "PostgreSQLInterface.h"
#include "FunctionThread.h"

struct pg_conn;
typedef struct pg_conn PGconn;

struct pg_result;
typedef struct pg_result PGresult;

class RankingServer_PostgreSQL;
class RankingServerCBInterface;

class RankingServerFunctor : public RakNet::Functor
{
public:
	RankingServerFunctor() {rankingServer=0; callback=0; dbQuerySuccess=false;}
	static char* AllocBytes(int numBytes);
	static void FreeBytes(char *data);

	/// [in] Set this to a pointer to your ranking server, so that the Process() function can access the database classes
	RankingServer_PostgreSQL *rankingServer;
	/// [in] Set this to a pointer to a callback if you wish to get notification calls. Otherwise you will have to override HandleResult()
	RankingServerCBInterface *callback;
	/// [out] Whether or not the database query succeeded
	bool dbQuerySuccess;
};

/// Submit ratings, scores, for a particular game for two players
/// Use CalculateNewEloRating in Elo.h to determine new ratings for the two players involved in the match.
/// If you don't know the ratings for the two players involved beforehand to pass to CalculateNewEloRating, call GetRatingForParticipant() to look it up.
class SubmitMatch_PostgreSQLImpl : public RankingServerDBSpec::SubmitMatch_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static SubmitMatch_PostgreSQLImpl* Alloc(void);
	static void Free(SubmitMatch_PostgreSQLImpl *s);
};

/// Modify the list of IPs allowed to submit matches
class ModifyTrustedIPList_PostgreSQLImpl : public RankingServerDBSpec::ModifyTrustedIPList_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static ModifyTrustedIPList_PostgreSQLImpl* Alloc(void);
	static void Free(ModifyTrustedIPList_PostgreSQLImpl *s);
};

class GetTrustedIPList_PostgreSQLImpl : public RankingServerDBSpec::GetTrustedIPList_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetTrustedIPList_PostgreSQLImpl* Alloc(void);
	static void Free(GetTrustedIPList_PostgreSQLImpl *s);
};

/// Given a particular player and game, get that player's rating from the database
class GetRatingForParticipant_PostgreSQLImpl : public RankingServerDBSpec::GetRatingForParticipant_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetRatingForParticipant_PostgreSQLImpl* Alloc(void);
	static void Free(GetRatingForParticipant_PostgreSQLImpl *s);
};

/// Given a particular game, get ratings for all players
class GetRatingForParticipants_PostgreSQLImpl : public RankingServerDBSpec::GetRatingForParticipants_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetRatingForParticipants_PostgreSQLImpl* Alloc(void);
	static void Free(GetRatingForParticipants_PostgreSQLImpl *s);
};

/// Given a particular player and game, get a match history
class GetHistoryForParticipant_PostgreSQLImpl : public RankingServerDBSpec::GetHistoryForParticipant_Data, public RankingServerFunctor
{
public:
	/// Does SQL Processing
	virtual void Process(void *context);

	/// Calls the callback specified by RankingServerFunctor::callback (which you set)
	/// or RankingServer_PostgreSQL::AssignCallback() (which sets it for you)
	/// Default implementation also deallocates the structure.
	virtual void HandleResult(bool wasCancelled, void *context);

	/// Use these for allocation/deallocation if building to a DLL, so allocation and deallocation happen in the same DLL
	static GetHistoryForParticipant_PostgreSQLImpl* Alloc(void);
	static void Free(GetHistoryForParticipant_PostgreSQLImpl *s);
};


/// If you want to forward Functor::HandleResult() the output to a callback (which is what it does by default), you can use this
class RAK_DLL_EXPORT RankingServerCBInterface
{
public:
	virtual void SubmitMatch_CB(SubmitMatch_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void ModifyTrustedIPList_CB(ModifyTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetTrustedIPList_CB(GetTrustedIPList_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetRatingForParticipant_CB(GetRatingForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetRatingForParticipants_CB(GetRatingForParticipants_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
	virtual void GetHistoryForParticipant_CB(GetHistoryForParticipant_PostgreSQLImpl *callResult, bool wasCancelled, void *context){}
};

/// Provides a central class for all RankingServer functionality using the PostgreSQL interface
/// If you are using more than one class that uses functionThreads, you should maintain this separately and call AssignFunctionThread() to avoid unnecessary threads.
class RAK_DLL_EXPORT RankingServer_PostgreSQL :
	public PostgreSQLInterface, // Gives the class C++ functions to access PostgreSQL
	public RakNet::FunctionThreadDependentClass // Adds a function thread member.
{
public:
	RankingServer_PostgreSQL();
	~RankingServer_PostgreSQL();

	/// Create the tables used by the ranking server, for all applications.  Call this first.
	/// I recommend using UTF8 for the database encoding within PostgreSQL if you are going to store binary data
	/// \return True on success, false on failure.
	bool CreateRankingServerTables(void);

	/// Destroy the tables used by the ranking server.  Don't call this unless you don't want to use the ranking server anymore, or are testing.
	/// \return True on success, false on failure.
	bool DestroyRankingServerTables(void);

	/// Push one of the above *_PostgreSQLImpl functors to run.
	/// \param functor A structure allocated on the HEAP (using new) with the input parameters filled in.
	virtual void PushFunctor(RankingServerFunctor *functor, void *context=0);

	/// Assigns a callback to get the results of processing.
	/// \param[in] A structure allocated on the HEAP (using new) with the input parameters filled in.
	void AssignCallback(RankingServerCBInterface *cb);

protected:
	friend class SubmitMatch_PostgreSQLImpl;
	friend class ModifyTrustedIPList_PostgreSQLImpl;
	friend class GetTrustedIPList_PostgreSQLImpl;
	friend class GetRatingForParticipant_PostgreSQLImpl;
	friend class GetRatingForParticipants_PostgreSQLImpl;
	friend class GetHistoryForParticipant_PostgreSQLImpl;

	RankingServerCBInterface *rankingServerCallback;
};

#endif
