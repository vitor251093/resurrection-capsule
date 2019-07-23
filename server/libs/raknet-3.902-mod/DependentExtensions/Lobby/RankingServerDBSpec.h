/// \file
/// \brief Not code so much as a specification for the structures and classes that an implementation of the ranking server can use.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RANKING_SERVER_DATABASE_INTERFACE_H
#define __RANKING_SERVER_DATABASE_INTERFACE_H

#include "Export.h"
#include "DS_List.h"
#include "RakNetTypes.h"
#include "RakString.h"
#include "RefCountedObj.h"

// See RankingServer_PostgreSQL::CreateRankingServerTables in RankingServer_PostgreSQL.cpp for CREATE TABLE table specification


namespace RakNet
{
	class BitStream;
}

namespace RankingServerDBSpec
{
// --------------------- TYPES ---------------------------
/// Type of the primary keys in the database
typedef unsigned DatabaseKey;

/// Entities are represented in the database as integer pairs
/// For example, primaryKey = 1234, secondaryKey = player
/// Another example: primaryKey = Asteroids, secondaryKey = Cooperative mode
struct PairedKeyDbId
{
	DatabaseKey primaryKey, secondaryKey;
};

struct RatedEntity
{
	/// An integer ID used as the primary key for the player in the database.
	/// This number is entirely user defined, and should be implicitly added if it does not already exist.
	PairedKeyDbId participantDbId;

	/// Elo (or other) rating for this entity.
	float newRating;
};

// --------------------- (IMPLEMENT THESE FOR A PARTICULAR DB) ---------------------------

/// Submit ratings, scores, for a particular game for two players
/// Use CalculateNewEloRating in Elo.h to determine new ratings for the two players involved in the match.
/// If you don't know the ratings for the two players involved beforehand to pass to CalculateNewEloRating, call GetRatingForParticipant() to look it up.
/// No data is returned, but the callback is called so you know it completed
class SubmitMatch_Data : public RefCountedObj
{
public:
	SubmitMatch_Data();
	virtual ~SubmitMatch_Data();
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	/// [in] For playerId, an integer ID used as the primary key for the first player in the database. Pick whatever unique number you want. If this ID does not currently exist in the database it will be added. For rating, the new rating that the first player should get, based on the results of this match. Use CalculateNewEloRating with the score, the old ratings, and your kFactor to determine this
	PairedKeyDbId participantADbId;

	/// [in] For playerId, an integer ID used as the primary key for the second player in the database. Pick whatever unique number you want. If this ID does not currently exist in the database it will be added. For rating, the new rating that the second player should get, based on the results of this match. Use CalculateNewEloRating with the score, the old ratings, and your kFactor to determine this
	PairedKeyDbId participantBDbId;

	/// [in] Whatever score the first player got. This is not used, but is written to the database for your own record keeping.
	float participantAScore;

	/// [in] Whatever score the second player got. This is not used, but is written to the database for your own record keeping.
	float participantBScore;

	/// [in] Old rating for participant A. Not used, but good for record keeping.
	float participantAOldRating;

	/// [in] New rating for participant A.
	float participantANewRating;

	/// [in] Old rating for participant B. Not used, but good for record keeping.
	float participantBOldRating;

	/// [in] New rating for participant B.
	float participantBNewRating;

	/// [in] Identifies in the game in the database with a primary and secondary integer key. User defined.
	PairedKeyDbId gameDbId;

	/// [in] Any notes for this match (optional)
	RakNet::RakString matchNotes;

	/// [in] Binary data, if you want to upload it (optional, set to 0 to not use)
	char *matchBinaryData;

	/// [in] Length of the binary data to upload (optional, set to 0 to not use)
	unsigned int matchBinaryDataLength;

	/// [out] Used by GetHistoryForParticipant_Data::matchHistoryList. Parse with _localtime64()
	long long matchTime;

};

/// Add or drop an allowed IP to submit matches
/// This is not enforced in the database, but can be enforced in code
class ModifyTrustedIPList_Data : public RefCountedObj
{
public:
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	/// [in] IP of the system to allow or disallow submissions
	RakNet::RakString ip;

	/// [out] System Address corresponding to this IP. Returned in GetTrustedIPList_Data
	SystemAddress systemAddress;

	/// [in] True to add to the list, false to remove from the list
	bool addToList;

	/// [in] Identifies in the game in the database with a primary and secondary integer key. User defined.
	PairedKeyDbId gameDbId;
};

/// Add or drop an allowed IP to submit matches
/// This is not enforced in the database, but can be enforced in code
class GetTrustedIPList_Data : public RefCountedObj
{
public:
	GetTrustedIPList_Data();
	~GetTrustedIPList_Data();
	/// [out] List of IPs. [in] parameters in ModifyTrustedIPList_Data are now [out] parameters
	DataStructures::List<ModifyTrustedIPList_Data*> ipList;
};

/// Given a particular player and game, get that player's rating from the database
class GetRatingForParticipant_Data : public RefCountedObj
{
public:
	void Serialize(RakNet::BitStream *bs);
	bool Deserialize(RakNet::BitStream *bs);

	/// [in] An integer ID used as the primary key for the player in the database. Should have been previously been added either externally, or through SubmitRatings
	PairedKeyDbId participantDbId;

	/// [in] Identifies in the game in the database with a primary and secondary integer key. User defined.
	PairedKeyDbId gameDbId;

	/// [out] Was this participant found?
	bool participantFound;

	/// [out] Pointer to whatever you want. Returned in GetRatingForParticipantCB().
	float rating;
};

/// Given a particular game, get ratings for all players
class GetRatingForParticipants_Data : public RefCountedObj
{
public:
	/// [in] Identifies in the game in the database with a primary and secondary integer key. User defined.
	PairedKeyDbId gameDbId;

	/// [out] List of players and rankings for a given gameId
	DataStructures::List<RatedEntity> outputList;
};

/// Given a particular player and game, get a match history
class GetHistoryForParticipant_Data : public RefCountedObj
{
public:
	virtual ~GetHistoryForParticipant_Data();

	/// [in] An integer ID used as the primary key for the player in the database. Should have been previously been added either externally, or through SubmitRatings
	PairedKeyDbId participantDbId;

	/// [in] Identifies in the game in the database with a primary and secondary integer key. User defined.
	PairedKeyDbId gameDbId;

	/// [out] Stores a list of pointers to allocated structures, each structure holding what was previously passed to SubmitMatch.
	/// Deallocated in the destructor.
	/// If no such participant found, matchHistoryList.Size() will be 0.
	/// \note All [in] parameters in SubmitMatch_Data are treated as [out] parameters.
	/// \note participantA is the player we are referring to.
	/// \note participantB is his/her opponent for that match.
	DataStructures::List<SubmitMatch_Data*> matchHistoryList;
};

}


#endif
