/// \file
/// \brief Not code so much as a specification for the structures and classes that an implementation of the lobby server can use.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __LOBBY_SERVER_DATABASE_INTERFACE_H
#define __LOBBY_SERVER_DATABASE_INTERFACE_H

#include "Export.h"
#include "DS_List.h"
#include "RakString.h"
#include "RefCountedObj.h"

namespace RakNet
{
	class BitStream;
}

// See LobbyDB_PostgreSQL::CreateLobbyServerTables in LobbyDB_PostgreSQL.cpp for CREATE TABLE table specification

namespace LobbyDBSpec
{
	// --------------------- TYPES ---------------------------
	/// Type of the primary keys in the database
	typedef unsigned DatabaseKey;

	struct PairedKeyDbId
	{
		DatabaseKey primaryKey, secondaryKey;
	};


	struct RowIdOrHandle
	{
		RowIdOrHandle() {hasDatabaseRowId=false; databaseRowId=0;}
		~RowIdOrHandle() {}

		/// [in] Use the variable userIdFrom? Faster if true. Otherwise it will lookup the ID using userHandleFrom.
		bool hasDatabaseRowId;
		/// [in] The database primary key of the user.
		DatabaseKey databaseRowId;
		/// [in] If you don't know the user primary key, look it up by this handle.
		RakNet::RakString handle;
	};

	// --------------------- (IMPLEMENT THESE FOR A PARTICULAR DB) ---------------------------

	/// Adds a user to the users table.
	class CreateUser_Data : public RefCountedObj
	{
	public:
		CreateUser_Data();
		virtual ~CreateUser_Data();
		void Serialize(RakNet::BitStream *bs); // Does NOT serialize UserIdOrHandle
		bool Deserialize(RakNet::BitStream *bs);
		void SerializeCCInfo(RakNet::BitStream *bs);
		void SerializeBinaryData(RakNet::BitStream *bs);
		void SerializePersonalInfo(RakNet::BitStream *bs);
		void SerializeEmailAddr(RakNet::BitStream *bs);
		void SerializePassword(RakNet::BitStream *bs);
		void SerializeCaptions(RakNet::BitStream *bs);
		void SerializeOtherInfo(RakNet::BitStream *bs);
		void SerializePermissions(RakNet::BitStream *bs);
		void SerializeAccountNumber(RakNet::BitStream *bs);
		void SerializeAdminLevel(RakNet::BitStream *bs);
		void SerializeAccountBalance(RakNet::BitStream *bs);
		bool DeserializeCCInfo(RakNet::BitStream *bs);
		bool DeserializeBinaryData(RakNet::BitStream *bs);
		bool DeserializePersonalInfo(RakNet::BitStream *bs);
		bool DeserializeEmailAddr(RakNet::BitStream *bs);
		bool DeserializePassword(RakNet::BitStream *bs);
		bool DeserializeCaptions(RakNet::BitStream *bs);
		bool DeserializeOtherInfo(RakNet::BitStream *bs);
		bool DeserializePermissions(RakNet::BitStream *bs);
		bool DeserializeAccountNumber(RakNet::BitStream *bs);
		bool DeserializeAdminLevel(RakNet::BitStream *bs);
		bool DeserializeAccountBalance(RakNet::BitStream *bs);

		/// [in] Identifier for the user, cannot exist in the table disallowedHandles
		RakNet::RakString handle;
		/// [in] Self-apparent
		RakNet::RakString firstName;
		/// [in] Self-apparent
		RakNet::RakString middleName;
		/// [in] Self-apparent
		RakNet::RakString lastName;
		/// [in] Self-apparent
		RakNet::RakString race;
		/// [in] Self-apparent
		RakNet::RakString sex;
		/// [in] Self-apparent
		RakNet::RakString homeAddress1;
		/// [in] Self-apparent
		RakNet::RakString homeAddress2;
		/// [in] Self-apparent
		RakNet::RakString homeCity;
		/// [in] Self-apparent
		RakNet::RakString homeState;
		/// [in] Self-apparent
		RakNet::RakString homeProvince;
		/// [in] Self-apparent
		RakNet::RakString homeCountry;
		/// [in] Self-apparent
		RakNet::RakString homeZipCode;
		/// [in] Self-apparent
		RakNet::RakString billingAddress1;
		/// [in] Self-apparent
		RakNet::RakString billingAddress2;
		/// [in] Self-apparent
		RakNet::RakString billingCity;
		/// [in] Self-apparent
		RakNet::RakString billingState;
		/// [in] Self-apparent
		RakNet::RakString billingProvince;
		/// [in] Self-apparent
		RakNet::RakString billingCountry;
		/// [in] Self-apparent
		RakNet::RakString billingZipCode;
		/// [in] Self-apparent
		RakNet::RakString emailAddress;
		/// [in] Self-apparent
		RakNet::RakString password;
		/// [in] If the user needs to retrieve their password; you could ask them this question.
		RakNet::RakString passwordRecoveryQuestion;
		/// [in] If the user needs to retrieve their password; you could use this for the answer.
		RakNet::RakString passwordRecoveryAnswer;
		/// [in] Lobbies often allow users to set a text description of their user in some fashion.
		RakNet::RakString caption1;
		/// [in] Lobbies often allow users to set a text description of their user in some fashion.
		RakNet::RakString caption2;
		/// [in] Lobbies often allow users to set a text description of their user in some fashion.
		RakNet::RakString caption3;
		/// [in] Self-apparent
		RakNet::RakString dateOfBirth;
		/// [in] If you want to have some kind of unique account number; it can be stored here.
		DatabaseKey accountNumber;
		/// [in] In case you take credit cards
		RakNet::RakString creditCardNumber;
		/// [in] In case you take credit cards
		RakNet::RakString creditCardExpiration;
		/// [in] In case you take credit cards
		RakNet::RakString creditCardCVV;
		/// [in] If your lobby supports moderators; you can indicate what moderator level they have here
		RakNet::RakString adminLevel;
		/// [in] Special account permissions; such as being able to chat; join adult games; etc. Encode using text however you wish.
		/// TODO - Per-column read/write/update permissions for self or others.
		RakNet::RakString permissions;
		/// [in] If users can associate money with their account; the current balance can be stored here.
		float accountBalance;
		/// [in] In case you want to log IPs
		RakNet::RakString sourceIPAddress;
		/// [in] If your lobby supports binary data (such as user images) it can be stored with this field.
		char *binaryData;
		/// [in] If your lobby supports binary data (such as user images) the length can be in this field.
		unsigned int binaryDataLength;
	};

	/// Get a user from the users table, using userId or userHandle for the lookup
	class GetUser_Data : public RefCountedObj
	{
	public:
		/// [in] Get credit card info? (including billing info)
		bool getCCInfo;

		/// [in] Get binary data?
		bool getBinaryData;

		/// [in] Get personal information (Name, address)
		bool getPersonalInfo;

		/// [in] Get email address?
		bool getEmailAddr;

		/// [in] Get password, recovery question, and recovery answer?
		bool getPassword;

		/// [in / out] Which user. If hasUserId==false userId is filled in on return.
		RowIdOrHandle id;

		/// [out] Does this user exist?
		bool userFound;

		/// [out] All [in] parameters are now [out] parameters. Data not downloaded is not changed.
		CreateUser_Data output;

		/// [out] Tag if this user is banned or not. Set directly in the DB (or I can add a function later).
		bool isBanned;

		/// [out] Tag if this user is suspended or not. Suspensions are temporary
		bool isSuspended;

		/// [out] When the suspension expires. Parse with _localtime64()
		long long suspensionExpiration;

		/// [out] Why the user was banned or suspended.
		RakNet::RakString banOrSuspensionReason;

		/// [out] When this row was added to the database. Parse with _localtime64()
		long long creationTime;
	};

	/// Overwrites an existing user in the users table
	class UpdateUser_Data : public RefCountedObj
	{
	public:
		void Serialize(RakNet::BitStream *bs);  // Does NOT serialize UserIdOrHandle
		bool Deserialize(RakNet::BitStream *bs); // =&input
		bool Deserialize(RakNet::BitStream *bs, CreateUser_Data *output, bool deserializeBooleans);
		void SerializeBooleans(RakNet::BitStream *bs);
		bool DeserializeBooleans(RakNet::BitStream *bs);

		/// [in] Get credit card info?
		bool updateCCInfo;

		/// [in] Get binary data?
		bool updateBinaryData;

		/// [in] Get personal information (Name, address)
		bool updatePersonalInfo;

		/// [in] Get email address?
		bool updateEmailAddr;

		/// [in] Get password, recovery question, and recovery answer?
		bool updatePassword;

		// [in]
		bool updateCaptions;

		// [in]
		bool updateOtherInfo;

		// [in]
		bool updatePermissions;

		// [in]
		bool updateAccountNumber;

		// [in]
		bool updateAdminLevel;

		// [in]
		bool updateAccountBalance;

		/// [in] Which user
		RowIdOrHandle id;

		/// [in] Will overwrite what is there already, based on the update booleans in this structure.
		/// Will fail if the new handle is different and already in use.
		CreateUser_Data input;
	};

	class DeleteUser_Data : public RefCountedObj
	{
	public:
		/// [in] Which user
		RowIdOrHandle id;
	};

	class ChangeUserHandle_Data : public RefCountedObj
	{
	public:
		/// [in] Which user
		RowIdOrHandle id;

		/// [in] New handle to change to
		RakNet::RakString newHandle;

		/// [out] If the query succeeded
		bool success;
		/// [out] Description of the query result.
		RakNet::RakString queryResult;
	};

	/// Adds an entry to the accountNotes table for an existing user
	class AddAccountNote_Data : public RefCountedObj
	{
	public:
		/// [in] Which user
		RowIdOrHandle id;

		/// [in] Write the variable moderatorId?
		bool writeModeratorId;
		/// [in] internal ID of the moderator (does not have to be in the users table).
		DatabaseKey moderatorId;
		/// [in] internal username of the moderator (does not have to be in the users table)
		RakNet::RakString moderatorUsername;
		/// [in] Type of account note (optional)
		RakNet::RakString type;
		/// [in] Subject of the account note (optional)
		RakNet::RakString subject;
		/// [in] Body of the account note (optional)
		RakNet::RakString body;
		/// [out] When this row was added to the database. Used by GetAccountNotes_Data only.
		long long time;
		/// [out] If the user was found in the database
		bool userFound;
	};

	/// Gets all account notes for an existing user
	class GetAccountNotes_Data : public RefCountedObj
	{
	public:

		virtual ~GetAccountNotes_Data();

		/// [in / out] Which user.
		RowIdOrHandle id;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;
		// All account notes for this user. Sorted by creationTime. AddAccountNote_Data [in] parameters are now [out] parameters. AddAccountNote_Data::userFound and writeModeratorId is not used.
		DataStructures::List<AddAccountNote_Data*> accountNotes;
	};

	/// Adds a userId / friendId to the friends table
	class AddFriend_Data : public RefCountedObj
	{
	public:

		/// [in] Which user
		RowIdOrHandle id;

		/// [in] Which friend
		RowIdOrHandle friendId;

		/// [out] When this row was added to the database. Used by GetFriends_Data only.
		long long creationTime;

		/// [out] If the user was found in the database
		bool userFound;
		/// [out] If the friend was found in the database
		bool friendFound;
		/// [out] If the query succeeded
		bool success;
		/// [out] Description of the query result.
		RakNet::RakString queryResult;
	};

	/// Removes a userId / friendId from the friends table
	class RemoveFriend_Data : public RefCountedObj
	{
	public:
		/// [in] Same fields as AddFriend_Data, but the operation performed is different.
		/// \note Bad input, such as removing someone that is not your friend, simply does nothing
		AddFriend_Data removeFriendInput;
	};

	/// Gets the friends table for a userId or userHandle
	class GetFriends_Data : public RefCountedObj
	{
	public:

		virtual ~GetFriends_Data();

		/// [in / out] Which user. If hasUserId==false userId is filled in.
		RowIdOrHandle id;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;
		// All friends for this user. AddFriend_Data [in] parameters are now [out] parameters.
		DataStructures::List<AddFriend_Data*> friends;
	};

	/// Adds to the ignoreList table for a particular userId / userHandle
	class AddToIgnoreList_Data : public RefCountedObj
	{
	public:
		/// [in] Which user
		RowIdOrHandle id;

		// Which user to ignore
		RowIdOrHandle ignoredUser;

		/// [in] If you want to specify particular actions to ignore, you can encode them into this string.
		/// [in] If this user is already in the ignore list, it will update actions
		RakNet::RakString actions;

		/// [out] If the query succeeded
		bool success;

		/// [out] Description of the query result.
		RakNet::RakString queryResult;

		/// [out] When this row was added to the database. Used by GetIgnoreList_Data only.
		long long creationTime;
	};

	/// Removes from the ignoreList table for a particular userId / userHandle
	class RemoveFromIgnoreList_Data : public RefCountedObj
	{
	public:
		/// [in] Which user
		RowIdOrHandle id;

		// Which user to no longer ignore
		RowIdOrHandle ignoredUser;
	};

	/// Gets the ignoreList table for a particular userId / userHandle
	class GetIgnoreList_Data : public RefCountedObj
	{
	public:

		virtual ~GetIgnoreList_Data();

		/// [in / out] Which user. If hasUserId==false userId is filled in.
		RowIdOrHandle id;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;
		// All ignored (enemies) for this user. AddToIgnoreList_Data [in] parameters are now [out] parameters.
		DataStructures::List<AddToIgnoreList_Data*> ignoredUsers;
	};

	/// Adds an email to the emails table. Added once for the user with inbox=false, n times for n recipients with inbox=true
	class SendEmail_Data : public RefCountedObj
	{
	public:
		SendEmail_Data();
		virtual ~SendEmail_Data();

		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// [in] When sending, our own system
		/// [out] When calling GetEmails, the other system (the one that sent us the email, or the one we sent to)
		RowIdOrHandle id;

		/// [in] recipients, used for SendEmail
		DataStructures::List<LobbyDBSpec::RowIdOrHandle> to;

		/// [in] Email subject
		RakNet::RakString subject;
		/// [in] Email body
		RakNet::RakString body;
		/// [in] Binary data of your choosing. Set to 0, or attachmentLength to 0 to not use.
		char *attachment;
		/// [in] Length of the binary data
		unsigned int attachmentLength;
		/// [in] Use for your own status flags (marked, read, priority, etc). Update with UpdateEmailStatus
		int initialSenderStatus, initialRecipientStatus;
		/// [out] Current status of the email, Used by GetEmails_Data
		int status;
		/// [in/out] For sending emails, initially mark this email as opened?
		/// For receiving mails, can be set by UpdateEmailStatus_Data
		/// Also returned by by GetEmails_Data
		bool wasOpened;

		/// [out] When this row was added to the database. Used by GetEmails_Data.
		long long creationTime;
		/// [out] Email primary key. Used by GetEmails_Data.
		DatabaseKey emailMessageID;

		/// [out] Are the parameters of this email valid?
		bool validParameters;
		/// [out] If validParameters==false, this is why
		RakNet::RakString failureMessage;
		
	};

	/// Gets all emails in the emails table for a particular user
	class GetEmails_Data : public RefCountedObj
	{
	public:

		virtual ~GetEmails_Data();

		/// [in / out] Which user. If hasUserId==false userId is filled in.
		RowIdOrHandle id;

		/// [in] Check the inbox or sent emails?
		bool inbox;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;

		/// [out] Are the parameters of this email valid?
		bool validParameters;

		/// [out] If validParameters==false, this is why
		RakNet::RakString failureMessage;

		// [out] List of emails. creationTime and emailMessageID will be filled in for each email.
		DataStructures::List<SendEmail_Data*> emails;
	};


	/// Deletes an email from either the incomingEmails or sentEmails table
	class DeleteEmail_Data : public RefCountedObj
	{
	public:

		/// [in] Refers to what email to delete. Get from GetEmails_Data structure, member emails::emailMessageID
		DatabaseKey emailMessageID;
	};

	/// Updates the mark flags on an email
	class UpdateEmailStatus_Data : public RefCountedObj
	{
	public:
		/// [in] Status flags, define as you wish
		int status;

		/// [in] Update the opened status flag on the email
		bool wasOpened;

		/// [in] Refers to what email to mark as read.
		DatabaseKey emailMessageID;
	};

	class GetHandleFromUserId_Data : public RefCountedObj
	{
	public:

		/// [in] The database primary key of the user.
		DatabaseKey userId;
		/// [out] handle of this user
		RakNet::RakString handle;
		/// [out] success
		bool success; // Was the lookup successful?
	};

	class GetUserIdFromHandle_Data : public RefCountedObj
	{
	public:

		/// [in] handle of this user
		RakNet::RakString handle;
		/// [out] The database primary key of the user.
		DatabaseKey userId;
		/// [out] success
		bool success; // Was the lookup successful?
	};

	class AddDisallowedHandle_Data : public RefCountedObj
	{
	public:

		/// [in] Handle to add to the table disallowedHandles
		RakNet::RakString handle;
	};

	class RemoveDisallowedHandle_Data : public RefCountedObj
	{
	public:

		/// [in] Handle to remove from the table disallowedHandles
		RakNet::RakString handle;
	};

	class IsDisallowedHandle_Data : public RefCountedObj
	{
	public:

		/// [in] Handle to check
		RakNet::RakString handle;
		/// [out] Is this handle in the table disallowedHandles?
		bool exists;
	};

	class AddToActionHistory_Data : public RefCountedObj
	{
	public:
		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// [in] Which user
		RowIdOrHandle id;
		/// [in] When this action occurred (if you want something in addition to creationTime)
		RakNet::RakString actionTime;
		/// [in] What action was taken
		RakNet::RakString actionTaken;
		/// [in] Description of the action
		RakNet::RakString description;
		/// [in] IP Address this action occurred from
		RakNet::RakString sourceIpAddress;
		/// [out] When this row was added to the database. Used by GetUserHistory_Data and filled out when inserted.
		long long creationTime;
	};

	class GetActionHistory_Data : public RefCountedObj
	{
	public:

		void Serialize(RakNet::BitStream *bs); // Does NOT serialize UserIdOrHandle
		bool Deserialize(RakNet::BitStream *bs);

		virtual ~GetActionHistory_Data();

		/// [in / out] Which user. If hasUserId==false userId is filled in.
		RowIdOrHandle id;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;
		// [out] History for this user. Sorted by creationTime::AddToUserHistory_Data
		DataStructures::List<AddToActionHistory_Data*> history;
	};

// -------------------------------------------------------------------------------------
// CLANS
// -------------------------------------------------------------------------------------
	enum ClanMemberStatus {
		CLAN_MEMBER_STATUS_LEADER,
		CLAN_MEMBER_STATUS_SUBLEADER,
		CLAN_MEMBER_STATUS_MEMBER,
		// Non-members should start at 10000 so the DB can correctly count how many members there are. See the function deleteClanOnNoMembers in LobbyDB_PostgreSQL.cpp
		// If there are no members then the clan is deleted in that triggered function
		CLAN_MEMBER_STATUS_REQUESTED_TO_JOIN=10000,
		CLAN_MEMBER_STATUS_INVITED_TO_JOIN,
		// Blacklisted means no longer a member, join requests ignored.
		CLAN_MEMBER_STATUS_BLACKLISTED=20000,
	};

	// Update a member of a clan
	class UpdateClanMember_Data : public RefCountedObj
	{	
	public:
		UpdateClanMember_Data();
		~UpdateClanMember_Data();

		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		// NOTE TO SELF - IF I ADD ANYTHING HERE, UPDATE CreateClan_PostgreSQLImpl::Process

		/// [in/out] Row ID of this clan. Output parameter for CreateClan_Data.
		/// Only used when /a addNew=true
		RowIdOrHandle clanId;

		/// [in / out] Who we are referring to
		RowIdOrHandle userId;

		/// [out] Last time mEStatus1 was updated
		long long lastUpdate;

		/// [in] Update the integers? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateInts[4];

		void SetUpdateInts(bool b) {updateInts[0]=b; updateInts[1]=b; updateInts[2]=b; updateInts[3]=b;}

		/// [in/out] Integer data. Use as you wish
		int integers[4];

		/// [in] Update the floats? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateFloats[4];

		void SetUpdateFloats(bool b) {updateFloats[0]=b; updateFloats[1]=b; updateFloats[2]=b; updateFloats[3]=b;}

		/// [in/out] Floating point data. Use as you wish
		float floats[4];

		/// [in] Update the descriptions? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateDescriptions[2];

		void SetUpdateDescriptions(bool b) {updateDescriptions[0]=b; updateDescriptions[1]=b;}

		/// [in/out] Per-clan per-member data
		RakNet::RakString descriptions[2];

		// Update binary data? If false, stays the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateBinaryData;

		/// [in/out] Per-clan per-member data
		char *binaryData;

		/// [in] Length of \a binaryData
		unsigned int binaryDataLength;

		/// [in] Update status at all?
		/// Only used for updating. For creating, all values are always uploaded
		/// \internal
		bool updateMEStatus1;

		/// [in] Fail update if true, and current status != requiredLastMEStatus
		/// Used to ensure that unbanning a member doesn't set an invited member to a regular member, etc.
		/// \internal
		bool hasRequiredLastStatus;
		ClanMemberStatus requiredLastMEStatus;

		/// Mutually exclusive status 1. Always set to CLAN_MEMBER_STATUS_LEADER for CreateClan_Data
		/// \internal
		ClanMemberStatus mEStatus1;

		/// [out]
		/// Was this clan member added as a new row, or was he just updated in the db?
		/// Will be true if userId is empty, or invalid
		bool isNewRow;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	// Clan data
	class UpdateClan_Data : public RefCountedObj
	{
	public:
		UpdateClan_Data();
		~UpdateClan_Data();

		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		// [in/out] Row ID of this clan. Filled out via CreateClan_Data
		RowIdOrHandle clanId;

		/// [in/out] String name of the clan (unique)
		/// Handle can only be set via CreateClan_Data or ChangeClanHandle_Data. Setting via UpdateClan_Data is ignored
		RakNet::RakString handle;

		/// [in] Update the integers? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateInts[4];

		void SetUpdateInts(bool b) {updateInts[0]=b; updateInts[1]=b; updateInts[2]=b; updateInts[3]=b;}

		/// [in/out] Integer data for the clan. Use as you wish
		int integers[4];

		/// [in] Update the floats? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateFloats[4];

		void SetUpdateFloats(bool b) {updateFloats[0]=b; updateFloats[1]=b; updateFloats[2]=b; updateFloats[3]=b;}

		/// [in/out] Floating point data for the clan. Use as you wish
		float floats[4];

		/// [in] Update the descriptions? If false, they stay the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateDescriptions[2];

		void SetUpdateDescriptions(bool b) {updateDescriptions[0]=b; updateDescriptions[1]=b;}

		/// [in/out] String descriptions of the clan
		RakNet::RakString descriptions[2];

		// Update binary data? If false, stays the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateBinaryData;

		/// [in/out] Binary data. Use as you wish.
		char *binaryData;

		/// [in/out] Length of \a binaryData
		unsigned int binaryDataLength;

		/// [in] Update requiresInvitationsToJoin? If false, it stays the same
		/// Only used for updating. For creating, all values are always uploaded
		bool updateRequiresInvitationsToJoin;

		/// [in/out] If false, anyone can join the clan at any time
		bool requiresInvitationsToJoin;

		/// [out] When this row was added to the database. Used by GetClans_Data and filled out when inserted.
		long long creationTime;

		/// [out] Members of the clan, and also blacklisted members
		DataStructures::List<UpdateClanMember_Data*> members;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	class CreateClan_Data : public RefCountedObj
	{
	public:
		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// Initial data to set for the new clan
		/// Failure message is in UpdateClan_Data::failureMessage
		UpdateClan_Data initialClanData;

		/// [in] Leader of the clan's initial data
		UpdateClanMember_Data leaderData;
	};

	class ChangeClanHandle_Data : public RefCountedObj
	{
	public:
		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// [out] Row ID of this clan. Filled out via CreateClan_Data
		RowIdOrHandle clanId;

		/// New clan handle
		RakNet::RakString newHandle;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};


	class DeleteClan_Data : public RefCountedObj
	{
	public:
		// [in] Which clan to delete
		RowIdOrHandle clanId;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	class GetClans_Data : public RefCountedObj
	{
	public:
		GetClans_Data();
		~GetClans_Data();
		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;

		/// [in] Which user to get clans for
		RowIdOrHandle userId;

		// [out] All clans for this user
		DataStructures::List<UpdateClan_Data*> clans;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	class RemoveFromClan_Data : public RefCountedObj
	{
	public:
		RemoveFromClan_Data();
		~RemoveFromClan_Data();

		/// [in] Which clan
		RowIdOrHandle clanId;

		/// [in] Which user
		RowIdOrHandle userId;

		/// [in] Fail update if true, and current status != requiredLastMEStatus
		/// Used to ensure that unbanning a member doesn't set an invited member to a regular member, etc.
		/// \internal
		bool hasRequiredLastStatus;
		ClanMemberStatus requiredLastMEStatus;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	class AddToClanBoard_Data : public RefCountedObj
	{
	public:
		AddToClanBoard_Data();
		~AddToClanBoard_Data();
		void Serialize(RakNet::BitStream *bs);
		bool Deserialize(RakNet::BitStream *bs);

		/// [in] Which clan
		RowIdOrHandle clanId;

		/// [in] Which user added the post
		RowIdOrHandle userId;

		/// [out] Data row of the post
		DatabaseKey postId;

		/// [in] Self-explanatory
		RakNet::RakString subject;

		/// [in] Self-explanatory
		RakNet::RakString body;

		/// [out] When this row was added to the database.
		long long creationTime;

		/// [in/out] Integer data. Use as you wish
		int integers[4];

		/// [in/out] Floating point data. Use as you wish
		float floats[4];

		/// [in/out] Binary data. Use as you wish.
		char *binaryData;

		/// [in] Length of \a binaryData
		unsigned int binaryDataLength;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};

	class RemoveFromClanBoard_Data : public RefCountedObj
	{
	public:
		/// [out] Data row of the post
		DatabaseKey postId;
	};

	class GetClanBoard_Data : public RefCountedObj
	{
	public:
		GetClanBoard_Data();
		~GetClanBoard_Data();

		/// [in] Which clan
		RowIdOrHandle clanId;

		/// [in] Sort list by creationTime, ascending. False for descending.
		bool ascendingSort;

		// [out]
		DataStructures::List<AddToClanBoard_Data*> board;

		/// [out] If not empty, DB query failed, and this is why
		/// \internal
		RakNet::RakString failureMessage;
	};
}


#endif
