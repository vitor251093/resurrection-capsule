/// \file
/// \brief Not code so much as a specification for the structures and classes that an implementation of a title validation system (CD Keys and game server logins)
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __TITLE_VALIDATION_DATABASE_INTERFACE_H
#define __TITLE_VALIDATION_DATABASE_INTERFACE_H

#include "Export.h"
#include "DS_List.h"
#include "RefCountedObj.h"
#include "RakString.h"

namespace TitleValidationDBSpec
{
// --------------------- TYPES ---------------------------
/// Type of the primary keys in the database
typedef unsigned DatabaseKey;

// --------------------- (IMPLEMENT THESE FOR A PARTICULAR DB) ---------------------------

/// Adds a title to the database of titles. This can be used to look up titleIds given a title password.
class AddTitle_Data : public RefCountedObj
{
public:
	AddTitle_Data();
	virtual ~AddTitle_Data();
	
	/// [out] Database id of the title.
	DatabaseKey titleId;
	
	/// [in] Name of the title
	RakNet::RakString titleName;
		
	/// [in] Password to use with the title (game servers should be provided this).  Binary
	char *titlePassword;

	/// [in] Length of the binary data titlePassword
	int titlePasswordLength;

	/// [in] Global permission (not per user): allow users to create accounts? Handled in the server code. If false, user accounts must be created on the server.
	bool allowClientAccountCreation;

	/// [in] Global permission (not per user): If allowLobbyChat==true, this flag controls if users can view/chat with users of any other game that also has this flag set.
	bool lobbyIsGameIndependent;

	/// Should the server require that the user specify a cd key to login?
	bool requireUserKeyToLogon;

	// Server code should set defaults lobby chat
	// Permissions are assumed to be false if unknown (no game set yet).
	// TODO - deal with per user permissions later. 
	// Simplified permissions:
	bool defaultAllowUpdateHandle;
	bool defaultAllowUpdateCCInfo;
	bool defaultAllowUpdateAccountNumber;
	bool defaultAllowUpdateAdminLevel;
	bool defaultAllowUpdateAccountBalance;
	bool defaultAllowClientsUploadActionHistory;
	RakNet::RakString defaultPermissionsStr;
	
	/// [out] Added automatically by the database on row entry. Parse with _localtime64()
	long long creationDate;
};

/// Download all titles added with AddTitle_Data
class GetTitles_Data : public RefCountedObj
{
public:
	GetTitles_Data();
	virtual ~GetTitles_Data();
	
	/// [out] Titles previously added with AddTitle_Data
	DataStructures::List<AddTitle_Data*> titles;
};

/// User keys are a more general form of CD Keys
/// This database can maintain records of what keys have been used, and additional data per-key if required
class UpdateUserKey_Data : public RefCountedObj
{
	public:
	UpdateUserKey_Data();
	virtual ~UpdateUserKey_Data();
		
	/// [in/out] integer identifier of this key. If this is a new key and you do not yet have a userKeyId, set userKeyIsSet to false. Otherwise, this will overwrite the existing key.
	DatabaseKey userKeyId;
	/// [in] True to overwrite an existing row specified by userKeyId. False to add a new row, and fill out userKeyId
	bool userKeyIsSet;
	
	/// [in] Which title this key should be associated with
	DatabaseKey titleId;
	
	/// [in] Password to the key. Binary data.
	char *keyPassword;
	
	/// [in] length of keyPassword
	int keyPasswordLength;
	
	/// [in] Which user this key has been assigned to. You do not necessarily have to use this if your keys are not assigned to users.
	DatabaseKey userId;
	
	/// [ip] IP address of the system this key was assigned to. (Optional)
	RakNet::RakString userIP;
		
	/// [in] If you want to track the number of times a key has been registered, you can store it here. Useful to see if a key has been pirated
	int numRegistrations;
	
	/// [in] If you want to mark a key as invalid, you can store it here.
	bool invalidKey;
	
	/// [in] Reason why a key was invalid.
	RakNet::RakString invalidKeyReason;
		
	/// [in] Any other data you wish to store.
	char *binaryData;
	
	/// [in] Length of binaryDataLength
	int binaryDataLength;
			
	/// [out] Added automatically by the database on row entry. Parse with _localtime64()
	long long creationDate;
};

// Gets all AddTitle_Data structures previously added to the repository
class GetUserKeys_Data : public RefCountedObj
{
	public:
		GetUserKeys_Data();
		virtual ~GetUserKeys_Data();
	
	/// List of keys added with UpdateUserKey_Data. [in] parameters are now [out] parameters, except for userKeyIsSet which is not used.
	DataStructures::List<UpdateUserKey_Data*> keys;	
};

class ValidateUserKey_Data : public RefCountedObj
{
public:
	ValidateUserKey_Data();
	virtual ~ValidateUserKey_Data();
	/// [in] Which title this key should be associated with
	DatabaseKey titleId;

	/// [in] Password to the key. Binary data.
	char *keyPassword;

	/// [in] length of keyPassword
	int keyPasswordLength;

	/// [out] Binary data stored when the key was uploaded
	char *binaryData;

	/// [out] Length of binaryDataLength
	int binaryDataLength;

	/// [out] Validation result. 1 = success. -1 = no keys matched. -2 = key marked as invalid (invalidKeyReason will be filled in), -3 = undefined
	int successful;

	/// [out] If successful !=0, invalidKeyReason will be filled in.
	RakNet::RakString invalidKeyReason;
};

}


#endif
