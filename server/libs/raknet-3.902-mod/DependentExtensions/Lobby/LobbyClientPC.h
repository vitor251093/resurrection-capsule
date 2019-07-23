/// \file
/// \brief Interface to the lobby client for the PC
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __LOBBY_CLIENT_PC_H
#define __LOBBY_CLIENT_PC_H

#include "PluginInterface.h"
#include "LobbyClientInterface.h"
#include "PacketPriority.h"
#include "LobbyPCCommon.h"

namespace RakNet
{

struct LobbyClientUser
{
	LobbyClientUser();
	~LobbyClientUser();
	LobbyClientUserId userId;
	SystemAddress systemAddress;
	LobbyDBSpec::CreateUser_Data *userData;
	bool readyToPlay;
};

/// \ingroup PLUGINS_GROUP
/// LobbyClient plugin for the PC
/// \brief PC specific client side code to the lobby system
/// Attach class as a plugin to RakNet as usual. Connect to the lobby server (as usual). Then use provided functions for specific functionality.
/// \note Most function calls are asynchronous so use a callback to get the result. This callback is set with LobbyClientPC::SetCallbackInterface()
/// \sa LobbyServer
/// \sa LobbyServerPostgreSQL
class RAK_DLL_EXPORT LobbyClientPC : public PluginInterface, public LobbyClientInterface
{
public:
	LobbyClientPC();
	virtual ~LobbyClientPC();

	virtual void Update(void) { };

	// ------------ NETWORKING --------------

	/// Logs into the lobby server with a given handle and password. All operations other than registering an account or
	/// Retrieving passwords require that you first login.
	/// \param[in] userHandle The handle used to register an account
	/// \param[in] userPassword The password used when registering the account
	/// \param[in] lobbyServerAddr SystemAddress of the lobby server. Must be currently connected.
	/// \pre Must be connected to the lobby server, and have previously created an account (or had one directly created on the server or in the database)
	/// \sa RegisterAccount()
	/// \sa LobbyClientInterfaceCB::Login_Result()
	virtual void Login(const char *userHandle, const char *userPassword, SystemAddress lobbyServerAddr);

	/// Indicates to the system what title (game) you are playing.
	/// Games are identified by a unique key, specified in binary via \a _titleLoginPW and \a _titleLoginPWLength
	/// Games must have been already created on the server, or directly in the database.
	/// One way to add games is through the functor class AddTitle_PostgreSQLImpl . An example of this can be found in the LobbyServerTest project
	/// \note When the title was added to the server, if  requireUserKeyToLogon==true, then you must first call ValidateUserKey() with the name of the title. Failure to do so will return the error LC_RC_USER_KEY_NOT_SET
	/// \param[in] _titleIdStr Actual name of the game. Not used at present, but may be used in the future.
	/// \param[in] _titleLoginPW Secret binary identifer assigned with the game. This should be kept secret so that people cannot submit scores for the wrong game, etc.
	/// \param[in] _titleLoginPWLength Length of \a _titleLoginPW
	/// \sa LobbyClientInterfaceCB::SetTitleLoginId_Result()
	virtual void SetTitleLoginId(const char *_titleIdStr, const char *_titleLoginPW, int _titleLoginPWLength);

	/// Logs off the lobby system.
	/// \sa LobbyClientInterfaceCB::Logoff_Result()
	virtual void Logoff(void);

	/// Are we connected to the Lobby server?
	/// \return true if connected, false otherwise.
	/// \sa Login()
	virtual bool IsConnected(void) const;

	// ------------ YOURSELF --------------

	/// Creates an account on the lobby server. Unlike most functions you do not have to be logged in to do this.
	/// LobbyDBSpec::CreateUser_Data::handle is required and must be unique in the database. It also cannot be in the disallowedHandles table
	/// LobbyDBSpec::CreateUser_Data::password is also required
	/// \param[in] input Data describing the new account. All fields optional but handle and password
	/// \param[in] lobbyServerAddr System address of the lobby server
	/// \pre SetTitleLoginId() must have been successfully called. This is because titles can disallow account creation.
	/// \sa LobbyClientInterfaceCB::RegisterAccount_Result()
	virtual void RegisterAccount(LobbyDBSpec::CreateUser_Data *input, SystemAddress lobbyServerAddr);

	/// Updates details of a previously created account.
	/// \param[in] input Details on the account. See LobbyDBSpec::UpdateUser_Data for more details.
	/// \pre Must be logged in
	/// \sa RegisterAccount();
	/// \sa LobbyClientInterfaceCB::UpdateAccount_Result()
	virtual void UpdateAccount(LobbyDBSpec::UpdateUser_Data *input);

	/// Change the handle associated with your user.
	/// Handle must be unique and not in the disallowedHandles table
	/// \note This may be disabled in the database TitleValidationDBSpec::AddTitle_Data::defaultAllowUpdateHandle
	/// \pre Must be logged in
	/// \sa LobbyClientInterfaceCB::ChangeHandle_Result()
	virtual void ChangeHandle(const char *newHandle);

	/// Returns the current action you are performing, if any.
	/// \return Your action
	virtual LobbyUserStatus GetLobbyUserStatus(void) const;

	/// Gets the password recovery question entered when the account was last created or updated.
	/// The question should not ask what the password is, but should ask some specific question that only the user would know
	/// \param[in] userHandle Current user's handle, specified when creating the account via RegisterAccount() or ChangeHandle()
	/// \param[in] lobbyServerAddr System address of the lobby server
	/// \sa LobbyClientInterfaceCB::RetrievePasswordRecoveryQuestion_Result()
	virtual void RetrievePasswordRecoveryQuestion(const char *userHandle, SystemAddress lobbyServerAddr);

	/// Recovers the user's password given the handle and password recovery answer
	/// \param[in] userHandle Current user's handle, specified when creating the account via RegisterAccount() or ChangeHandle()
	/// \param[in] passwordRecoveryAnswer Specified when creating or last updating the account
	/// \param[in] lobbyServerAddr System address of the lobby server
	/// \note Note the answer is currently not case sensitive, but is spacing sensitive (stricmp())
	/// \sa LobbyClientInterfaceCB::RetrievePassword_Result()
	virtual void RetrievePassword(const char *userHandle, const char *passwordRecoveryAnswer, SystemAddress lobbyServerAddr);

	// ------------ FRIENDS --------------
	
	/// Downloads your friends from the database, and stores the list internally
	/// Results are contained in the LC_Friend structure.
	/// Friends do not have to be online, and you will get LobbyClientInterfaceCB::FriendStatus_Notify() indicating when your friends go online and offline.
	/// \note You will not get any friends information or notifications until calling DownloadFriends()
	/// \param[in] ascendingSortByDate Sort result by date, ascending or descending
	/// \pre Must be logged in.
	/// \sa GetFriendsList();
	/// \sa LobbyClientInterfaceCB::DownloadFriends_Result()
	virtual void DownloadFriends(bool ascendingSortByDate);

	/// Invites someone to be your friend. They must be online at the time of the invite
	/// They will get the callback FriendInvitation_Notify indicating who the request is from, and can accept or deny it (or ignore it)
	/// If accepted, you will get notifications via FriendStatus_Notify of when they go on and offline
	/// \param[in] friendHandle Handle of the potential friend. If you know \a friendId leave this at 0.
	/// \param[in] friendId ID of the potential friend. Lookup is faster using ID. If you don't know it, specify \a friendHandle instead.
	/// \param[in] messageBody Optional message to send with the friend request.
	/// \param[in] language Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \pre Must be logged in.
	/// \sa StringCompressor
	/// \sa LobbyClientInterfaceCB::SendAddFriendInvite_Result()
	/// \sa LobbyClientInterfaceCB::FriendInvitation_Notify()
	/// \sa LobbyClientInterfaceCB::FriendStatus_Notify
	virtual void SendAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language);

	/// Accept a friend invite that you were notified of with LobbyClientInterfaceCB::FriendInvitation_Notify()
	/// \param[in] friendHandle Handle of potential friend, use if you don't have the Id, but less efficient
	/// \param[in] friendId ID of the potential friend, found at FriendInvitation_Notification::invitor
	/// \param[in] messageBody Optional message to reply with.
	/// \param[in] language Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \pre Must be logged in.
	/// \sa StringCompressor
	/// \sa LobbyClientInterfaceCB::AcceptAddFriendInvite_Result()
	/// \sa LobbyClientInterfaceCB::FriendInvitation_Notify()
	virtual void AcceptAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language);

	/// Decline a friend invite that you were notified of with LobbyClientInterfaceCB::FriendInvitation_Notify()
	/// \param[in] friendHandle Handle of the guy asking to be a friend, use if you don't have the Id, but less efficient
	/// \param[in] friendId ID of the guy asking to be a friend, found at FriendInvitation_Notification::invitor
	/// \param[in] messageBody Optional message to reply with.
	/// \param[in] language Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \pre Must be logged in.
	/// \sa StringCompressor
	/// \sa LobbyClientInterfaceCB::DeclineAddFriendInvite_Result()
	/// \sa LobbyClientInterfaceCB::FriendInvitation_Notify()
	virtual void DeclineAddFriendInvite(const char *friendHandle, LobbyClientUserId *friendId, const char *messageBody, unsigned char language);

	/// Get the downloaded friends list.
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \pre Must be logged in.
	/// \pre Successful call to DownloadFriends
	/// \return List of structures containing information about each friend. Cleared on logoff.
	/// \sa LC_Friend
	virtual const DataStructures::List<LC_Friend *>& GetFriendsList(void) const;

	/// Is this user a friend?
	/// \pre Must have downloaded the friends list, or will always be false
	/// \param[in] friendHandle Handle of the friend
	/// \return True if this user is a friend, false if not, or unknown
	virtual bool IsFriend(const char *friendHandle);

	/// Is this user a friend?
	/// \pre Must have downloaded the friends list, or will always be false
	/// \param[in] friendId ID of the friend
	/// \return True if this user is a friend, false if not, or unknown
	virtual bool IsFriend(LobbyClientUserId *friendId);

	// ------------ IGNORE LIST --------------

	/// Adds a user to the ignore list, stored in the database as well
	/// Will block room chat, room invites, and instant messages
	/// Does not currently block user from joining your room, but this may change depending on user requests
	/// \param[in] ascendingSort Which direction to sort the ignore list, sorted on date
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::DownloadIgnoreList_Result()
	/// \sa GetIgnoreList()
	virtual void DownloadIgnoreList(bool ascendingSort);

	/// Add / update a user on the ignore list. If this user is not currently on the ignore list, they will be added.
	/// If this user IS currently on the ignore list, LC_Ignored::actionString will be updated.
	/// \param[in] userHandle Handle of the user on which to perform this action. If you know \a userId leave this at 0.
	/// \param[in] userId ID of the user on which to perform this action. Lookup is faster using ID. If you don't know it, specify \a userHandle instead.
	/// \param[in] actionsStr Arbitrary, user-defined string to store any extra data you wish (such as reason for the ignore)
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::AddToOrUpdateIgnoreList_Result()
	virtual void AddToOrUpdateIgnoreList(const char *userHandle, LobbyClientUserId *userId, const char *actionsStr);

	/// Remove a user from the ignore list.
	/// \param[in] userId ID of the user on which to perform this action. You can get this through GetIgnoreList()
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::RemoveFromIgnoreList_Result()
	/// \sa GetIgnoreList()
	virtual void RemoveFromIgnoreList(LobbyClientUserId *userId);

	/// Returns the internally stored list of ignored users.
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \return List of structures containing information about each ignored users. Cleared on logoff.
	virtual const DataStructures::List<LC_Ignored *>& GetIgnoreList(void) const;

	// ------------ EMAILS --------------

	/// Downloads emails on the server.
	/// Currently (this may change) emails are purely intra-database, so the name is a bit misleading. A better name would be peristent database message.
	/// \param[in] ascendingSort Which direction to sort the resulting downloaded list, by creation date
	/// \param[in] inbox true to download mails sent to you. false to download mails that you have sent.
	/// \param[in] language Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::DownloadEmails_Result()
	/// \sa GetInboxEmailList()
	/// \sa GetSentEmailList()
	virtual void DownloadEmails(bool ascendingSort, bool inbox, unsigned char language);

	/// Sends an email to one or more users
	/// For each recipient, two copies of the email are stored in the database. One is in your sent mailbox, one is in the recipient incoming mailbox.
	/// \param[in] input Data corresponding to the email. See LobbyDBSpec.h for all parameters.
	/// \param[in] language Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::SendEmail_Result()
	virtual void SendEmail(LobbyDBSpec::SendEmail_Data *input, unsigned char language);

	/// Deletes an email, from either your inbox or sent emails list.
	/// \param[in] emailId Found at LobbyDBSpec::SendEmail_Data::emailMessageID
	/// \pre Must be logged in.
	/// \sa GetInboxEmailList()
	/// \sa GetSentEmailList()
	/// \sa LobbyClientInterfaceCB::DeleteEmail_Result()
	virtual void DeleteEmail(EmailId *emailId);

	/// Update the status flags associated with an email.
	/// \param[in] emailId Found at LobbyDBSpec::SendEmail_Data::emailMessageID
	/// \param[in] newStatus arbitrary integer stored alongside each email. Use to indicate user-defined fields (priority, etc).
	/// \param[in] wasOpened boolean field. Use to indicate if an email was opened. Sent emails take the field LobbyDBSpec::SendEmail_Data::wasOpened . Inbox has this always true until set off.
	/// \pre Must be logged in.
	/// \sa GetInboxEmailList()
	/// \sa GetSentEmailList()
	/// \sa LobbyClientInterfaceCB::UpdateEmailStatus_Result()
	virtual void UpdateEmailStatus(EmailId *emailId, int newStatus, bool wasOpened);

	/// Gets emails downloaded with DownloadEmails(), for the inbox.
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \note It takes two calls to get all emails, one call to DownloadEmails only gets the inbox or sent list.
	/// \pre Must be logged in.
	/// \return The email
	virtual const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & GetInboxEmailList( void ) const;

	/// Gets emails downloaded with DownloadEmails(), for sent emails.
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \note It takes two calls to get all emails, one call to DownloadEmails only gets the inbox or sent list.
	/// \pre Must be logged in.
	/// \return The email
	virtual const DataStructures::List<LobbyDBSpec::SendEmail_Data *> & GetSentEmailList( void ) const;

	// ------------ INSTANT MESSAGING --------------

	/// Sends a message to another user. Unlike emails, this is not stored in the database.
	/// The callback will indicate failure if the user is not online.
	/// If the user has ignored us, the callback will indicate this as well.
	/// \note It is valid to send an empty message with only binary data, or vice-versa
	/// \param[in] userHandle Handle of the user on which to perform this action. If you know \a userId leave this at 0.
	/// \param[in] userId ID of the user on which to perform this action. Lookup is faster using ID. If you don't know it, specify \a userHandle instead.
	/// \param[in] chatMessage Body of the message to send
	/// \param[in] languageId Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \param[in] chatBinaryData Any binary data you wish to send with the message (custom encoding, images, etc)
	/// \param[in] chatBinaryLength Length in bytes of \a chatBinaryData
	/// \pre Must be logged in.
	/// \sa LobbyClientInterfaceCB::SendIM_Result()
	virtual void SendIM(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength);

	// ------------ ROOMS --------------
	// ***  Anyone anytime  ***

	/// Returns if we are in a room or not.
	/// \return True if we are in a room. False for any other state.
	bool IsInRoom(void) const;

	// ***  Anyone Outside of a room  ***

	/// Creates a room
	/// \param[in] roomName Name of the room (required)
	/// \param[in] roomPassword Password for the room (optional)
	/// \param[in] roomIsHidden Sets the DefaultTableColumns::TC_ROOM_HIDDEN column to true or false in the rooms table.
	/// \param[in] publicSlots Number of slots that anyone can join by calling DownloadRooms() to view your room, followed by JoinRoom() to join it.
	/// \param[in] privateSlots Number of slots that users can only join if they previously were invited by the current moderator with InviteToRoom(). Those users will join a public slot if all private slots are full when they join the room.
	/// \param[in] allowSpectators True to allow spectators. Spectators are normal room members, but there is (currently) no limit on the number of spectators.
	/// \param[in] customProperties A table with named columns and one row of properties to add to the server rooms table. Column names must not already be in defaultTableColumns in LobbyTypes.cpp
	/// \pre You are in the lobby (the normal state after logging on)
	/// \pre At least one prior successful call to SetTitleLoginId() to indicate what game to start. SetTitleLoginId() can be called multiple times to change games if desired.
	/// \sa LobbyClientInterfaceCB::CreateRoom_Result()
	/// \sa LobbyClientInterfaceCB::RoomDestroyed_Notify
	/// \sa LobbyClientInterfaceCB::RoomMemberDrop_Notify
	/// \sa LobbyClientInterfaceCB::RoomMemberJoin_Notify
	virtual void CreateRoom(const char *roomName, const char *roomPassword, bool roomIsHidden, int publicSlots, int privateSlots, bool allowSpectators,
		DataStructures::Table *customProperties);

	/// Downloads rooms, possibly based on filters
	/// \param[in] inclusionFilters Filters to apply to the search query. Use 0 to get all rooms.
	/// \param[in] numInclusionFilters Number of structures in the \a inclusionFilters array
	/// \pre Must be logged in.
	/// \note In order to honor hidden rooms, you should set the DefaultTableColumns::TC_ROOM_HIDDEN column.
	/// Use GetAllRooms() to access the downloaded room list.
	///
	/// Example of setting DefaultTableColumns::TC_ROOM_HIDDEN to false, to not find hidden rooms
	///
	/// DataStructures::Table::FilterQuery inclusionFilters[1];
	/// DataStructures::Table::Cell filterCells[1];
	/// filterCells[0].Set(0); // 0 = integer value false
	/// inclusionFilters[0].cellValue=&filterCells[0];
	/// inclusionFilters[0].columnIndex=DefaultTableColumns::TC_ROOM_HIDDEN;
	/// inclusionFilters[0].operation=DataStructures::Table::QF_EQUAL;
	/// lobbyClient.DownloadRooms(inclusionFilters, 1);
	///
	/// \sa LobbyClientInterfaceCB::DownloadRooms_Result()
	virtual void DownloadRooms(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters);
	
	/// Returns a table of all downloaded rooms, since the last time DownloadRooms() was called.
	/// This list is reset to only your own room if you create a room.
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \return Table of downloaded rooms
	virtual DataStructures::Table * GetAllRooms(void);

	/// Are we currently waiting for DownloadRooms() to finish?
	/// \return true if DownloadRooms() is in progress. When complete, we will get the LobbyClientInterfaceCB::DownloadRooms_Result() callback.
	virtual bool IsDownloadingRoomList(void);

	/// Leave our current room. As with other asynchronous calls, this is not immediate, and you should wait for the callback for verification.
	/// \pre Must be currently in a room
	/// \sa LobbyClientInterfaceCB::LeaveRoom_Result()
	virtual void LeaveRoom(void);

	// ***  Anyone inside of a room  ***

	/// Gets a list of pointers to room member identifiers.
	/// Use GetUserDetails() to get more information about each user.
	/// Pointers are valid until next call to this same function
	/// \return All members in the room
	virtual const DataStructures::List<LobbyClientUserId *>& GetAllRoomMembers(void) const;

	/// Gets a list of pointers to room member identifiers.
	/// Use GetUserDetails() to get more information about each user.
	/// Pointers are valid until next call to this same function
	/// \return All members occupying open slots
	virtual const DataStructures::List<LobbyClientUserId *>& GetOpenSlotMembers(void) const;

	/// Gets a list of pointers to room member identifiers.
	/// Use GetUserDetails() to get more information about each user.
	/// Pointers are valid until next call to this same function
	/// \return All members occupying reserved slots
	virtual const DataStructures::List<LobbyClientUserId *>& GetReservedSlotMembers(void) const;

	/// Gets a list of pointers to room member identifiers.
	/// Use GetUserDetails() to get more information about each user.
	/// Pointers are valid until next call to this same function
	/// \return All spectators in the room
	virtual const DataStructures::List<LobbyClientUserId *>& GetSpectators(void) const;

	/// Get identifier of the moderator of this room
	/// The moderator will be one of the members in a public or priate slot
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \return moderator id
	virtual LobbyClientUserId* GetRoomModerator(void);

	/// Get ID of this room, used in some other functions
	/// Pointers are guaranteed to remain valid until the next call to RakPeer::Receive()
	/// \return room id
	virtual LobbyClientRoomId* GetRoomId(void);

	/// Get number of public slots that can be filled.
	/// \note This may be lower than the actual number of members in the room if ChangeNumSlots() is called.
	virtual unsigned int GetPublicSlotMaxCount(void) const;

	/// Get number of private slots that can be filled.
	/// \note This may be lower than the actual number of members in the room if ChangeNumSlots() is called.
	virtual unsigned int GetPrivateSlotMaxCount(void) const;

	/// Get unique identifier of the title (game). On the PC this corresponds to the database primary key.
	/// \return title id
	virtual LobbyClientTitleId* GetTitleId(void) const;

	/// \return The name of the room we are currently in, or 0 if none.
	virtual const char *GetRoomName(void) const;

	/// \return The name of the game we are currently in, or 0 if none. Specified in the titles table as titleName
	virtual const char *GetTitleName(void) const;

	/// \return The value of \a roomIsHidden passed to CreateRoom()
	virtual bool RoomIsHidden(void) const;

	/// Convenience function. Calls GetAllRooms(), returning the row of the room that we are currently in
	/// This is necessary to access \a customProperties passed to CreateRoom()
	virtual DataStructures::Table::Row *GetRoomTableRow(void) const;

	/// Join an existing room
	/// \param[in] roomId Identifier of the room. Get this by calling DownloadRooms(). It is also specified with room invitation requests in the \a roomId member of LobbyClientInterfaceCB::RoomInvite_Notify
	/// \param[in] roomName Name identifier of the room. Can specify this or the roomId
	/// \param[in] roomPassword Password used to enter the room. If the room does not have a password, this field is ignored.
	/// \param[in] joinAsSpectator Spectators do not typically play in the room. There is currently no limit on the number of spectators that can be in a room
	/// \pre Must be currently in a room
	/// \sa LobbyClientInterfaceCB::JoinRoom_Result()
	virtual void JoinRoom(LobbyClientRoomId* roomId, const char *roomName, const char *roomPassword, bool joinAsSpectator);

	/// Joins the first available room based on a search filter.
	/// The room must not have a password and if \a joinAsSpectator==false,  must have an available open slot
	/// \param[in] inclusionFilters Filters to apply to the search query. Use 0 to get all rooms.
	/// \param[in] numInclusionFilters Number of structures in the \a inclusionFilters array
	/// \param[in] joinAsSpectator Spectators do not typically play in the room. There is currently no limit on the number of spectators that can be in a room
	/// \pre Must be logged in and not in a room
	/// \note In order to honor hidden rooms, you should set the DefaultTableColumns::TC_ROOM_HIDDEN column.
	/// \sa DownloadRooms();
	virtual void JoinRoomByFilter(DataStructures::Table::FilterQuery *inclusionFilters, unsigned numInclusionFilters, bool joinAsSpectator);

	/// Chat to all other members in a room
	/// Members which have you on ignore will not get your messages. There is no notification of this.
	/// \param[in] chatMessage Body of the message to send
	/// \param[in] languageId Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \param[in] chatBinaryData Any binary data you wish to send with the message (custom encoding, images, etc)
	/// \param[in] chatBinaryLength Length in bytes of \a chatBinaryData
	/// \pre Must be currently in a room
	/// \sa LobbyClientInterfaceCB::RoomChat_Result()
	virtual void RoomChat(const char *chatMessage, unsigned char languageId, const char *chatBinaryData, int chatBinaryLength);

	/// \return If the specified userId is in a reserved slot in the room.
	virtual bool IsInReservedSlot(LobbyClientUserId *userId);

	/// \return If the specified userId is the moderator of the room
	virtual bool IsRoomModerator(LobbyClientUserId *userId);

	/// \return If the specified userId is in the room, in any capacity
	virtual bool IsUserInRoom(LobbyClientUserId *userId) const;

	/// \return If the room allows spectators to join.
	virtual bool RoomAllowsSpectators(void) const;

	/// Invite someone to join your room
	/// Invited users will join private slots if they exist, public slots if they do not or if the private slots are full
	/// Invitations are linked to the current moderator, so if the moderator leaves or changes via GrantModerator(), those invites are cleared.
	/// \param[in] userHandle Handle of the user on which to perform this action. If you know \a userId leave this at 0.
	/// \param[in] userId ID of the user on which to perform this action. Lookup is faster using ID. If you don't know it, specify \a userHandle instead.
	/// \param[in] chatMessage Body of the message to send
	/// \param[in] languageId Language to use for string encoding for the StringCompressor class. Use 0 if unsure.
	/// \param[in] chatBinaryData Any binary data you wish to send with the message (custom encoding, images, etc)
	/// \param[in] chatBinaryLength Length in bytes of \a chatBinaryData
	/// \param[in] Invite this member to an private slot. (If false, they will be under the same restrictions as normal members attempting to join the room).
	/// \pre Must be currently in a room
	/// \sa LobbyClientInterfaceCB::InviteToRoom_Result()
	virtual void InviteToRoom(const char *userHandle, LobbyClientUserId *userId, const char *chatMessage, unsigned char languageId,
		const char *chatBinaryData, int chatBinaryLength, bool inviteToPrivateSlot);

	/// Rooms cannot start until all members first call SetReadyToPlayStatus(true)
	/// New members to a room have this set to false by default
	/// \pre Must be currently in a room
	/// \sa LobbyClientInterfaceCB::SetReadyToPlayStatus_Result()
	/// \sa LobbyClientInterfaceCB::RoomMemberReadyStateChange_Notify()
	virtual void SetReadyToPlayStatus(bool isReady);

	// ***  Moderators inside of a room  ***

	/// Kick someone out of your room. Unfortunate recipient will get the callback KickedOutOfRoom_Notify()
	/// \pre Must be the moderator of your room.
	/// \sa LobbyClientInterfaceCB::KickRoomMember_Result()
	/// \sa LobbyClientInterfaceCB::KickedOutOfRoom_Notify
	virtual void KickRoomMember(LobbyClientUserId *userId);

	/// Sets the DefaultTableColumns::TC_ROOM_HIDDEN column to true or false in the rooms table.
	/// Other room members will get the callback RoomSetIsHidden_Notify()
	/// \sa LobbyClientInterfaceCB::SetRoomIsHidden_Result()
	/// \sa LobbyClientInterfaceCB::RoomSetIsHidden_Notify()
	virtual void SetRoomIsHidden(bool roomIsHidden);

	/// Sets if the room allows spectators
	/// Other room members will get the callback RoomSetAllowSpectators_Notify()
	/// \sa LobbyClientInterfaceCB::SetRoomAllowSpectators_Result()
	/// \sa LobbyClientInterfaceCB::RoomSetAllowSpectators_Notify()
	virtual void SetRoomAllowSpectators(bool spectatorsOn);

	/// Changes the number of total and private slots for the room
	/// Other room members will get the callback RoomChangeNumSlots_Notify()
	/// \sa LobbyClientInterfaceCB::ChangeNumSlots_Result()
	/// \sa RoomChangeNumSlots_Notify
	virtual void ChangeNumSlots(int totalSlots, int privateSlots);

	/// Grants moderator to another room member.
	/// This room member cannot be a spectator.
	/// Changing moderators clears current private slot invitations
	/// Other room members will get the callback RoomNewModerator_Notify()
	/// \sa LobbyClientInterfaceCB::GrantModerator_Result()
	/// \sa LobbyClientInterfaceCB::RoomNewModerator_Notify
	virtual void GrantModerator(LobbyClientUserId* userId);

	/// Attempts to start the game.
	/// All members in the room must be ready, set by calling SetReadyToPlayStatus(true)
	/// All members, including spectators, will get the callback StartGame_Notify() if successful
	/// This callback contains a list of all members in the room, along with SystemAddresses and other pertinent information
	/// \sa LobbyClientInterfaceCB::StartGame_Result()
	/// \sa LobbyClientInterfaceCB::StartGame_Notify()
	virtual void StartGame(void);

	// ---------- SCORE TRACKING -------------

	/// Submits the results of a match between two users
	/// This is typically a server command, not a user command
	/// For security, your IP must be in the trustedIpList table.
	/// \param[in] input Data corresponding to the match. See RankingServerDBSpec.h for all parameters.
	/// \sa LobbyClientInterfaceCB::SubmitMatch_Result()
	virtual void SubmitMatch(RankingServerDBSpec::SubmitMatch_Data *input);

	/// Downloads the rating for a particular user
	/// \param[in] input Which user you are referring to. See RankingServerDBSpec.h for all parameters.
	/// \sa LobbyClientInterfaceCB::DownloadRating_Result()
	virtual void DownloadRating(RankingServerDBSpec::GetRatingForParticipant_Data *input);

	/// Returns the rating that was most recently downloaded
	/// \return A pointing to the rating structure
	virtual RankingServerDBSpec::GetRatingForParticipant_Data * GetRating(void);

	// ------------ QUICK MATCH --------------

	/// \return True if we called QuickMatch(), the game did not yet start, and QuickMatch::timeoutInSeconds did not yet elapsed
	virtual bool IsWaitingOnQuickMatch(void);

	/// A quick match automatically groups other users that have also called QuickMatch().
	/// The only condition is that they are in the same game, and have at least the value of requiredPlayers
	/// When the game starts all members will get the callback StartGame_Notify()
	/// The moderator is chosen at random.
	/// If the timer elapses with not enough members to start the game, the callback QuickMatchTimeout_Notify() will be called.
	/// \todo This feature is currently hacked in and will start games but does not honor the requiredPlayers. In the near future I will have it working properly
	/// \pre You must be in the lobby to start this function
	/// \sa LobbyClientInterfaceCB::QuickMatch_Result()
	/// \sa LobbyClientInterfaceCB::StartGame_Notify()
	/// \sa LobbyClientInterfaceCB::QuickMatchTimeout_Notify()
	virtual void QuickMatch(int requiredPlayers, int timeoutInSeconds);

	/// Cancel a prior call to QuickMatch() assuming \a timeoutInSeconds did not yet elapse.
	/// \sa LobbyClientInterfaceCB::CancelQuickMatch_Result()
	virtual void CancelQuickMatch(void);

	// ------------ ACTION HISTORY --------------

	/// Downloads the action history for yourself
	/// \param[in] ascendingSort Sort order, by upload date
	/// \sa LobbyClientInterfaceCB::DownloadActionHistory_Result()
	/// \sa GetActionHistoryList()
	virtual void DownloadActionHistory(bool ascendingSort);

	/// Adds to the history of actions for yourself
	/// For security, defaultAllowClientsUploadActionHistory must be true in the titles table OR
	/// Your IP must be in the trustedIpList table
	/// \param[in] input Data corresponding to the action. See LobbyDBSpec.h for all parameters.
	/// \sa LobbyClientInterfaceCB::AddToActionHistory_Result()
	/// \sa GetActionHistoryList()
	virtual void AddToActionHistory(LobbyDBSpec::AddToActionHistory_Data *input);

	/// Returns the results of a previous successful call to AddToActionHistory where AddToActionHistory_Result() returned success
	/// \return The action history for your user
	/// \sa DownloadActionHistory()
	virtual const DataStructures::List<LobbyDBSpec::AddToActionHistory_Data *> & GetActionHistoryList(void) const;

	// ------------ CLANS --------------
	// INCOMPLETE - DO NOT USE YET
	virtual void CreateClan(LobbyDBSpec::CreateClan_Data *clanData, bool failIfAlreadyInClan);
	virtual void ChangeClanHandle(ClanId *clanId, const char *newHandle);
	virtual void LeaveClan(ClanId *clanId, bool dissolveIfClanLeader, bool autoSendEmailIfClanDissolved);
	virtual void DownloadClans(const char *userHandle, LobbyClientUserId* userId, bool ascendingSort);
	virtual void SendClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId);
	virtual void WithdrawClanJoinInvitation(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId);
	virtual void AcceptClanJoinInvitation(LobbyDBSpec::UpdateClanMember_Data *clanMemberData, bool failIfAlreadyInClan);
	virtual void RejectClanJoinInvitation(const char *clanHandle, ClanId *clanId);
	virtual void DownloadByClanMemberStatus(LobbyDBSpec::ClanMemberStatus status);
	virtual void SendClanMemberJoinRequest(LobbyDBSpec::UpdateClanMember_Data *clanMemberData);
	virtual void WithdrawClanMemberJoinRequest(const char *clanHandle, ClanId *clanId);
	virtual void AcceptClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool failIfAlreadyInClan);
	virtual void RejectClanMemberJoinRequest(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId);
	virtual void SetClanMemberRank(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, LobbyDBSpec::ClanMemberStatus newRank );
	virtual void ClanKickAndBlacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId, bool kick, bool blacklist);
	virtual void ClanUnblacklistUser(ClanId *clanId, const char *userHandle, LobbyClientUserId* userId);
	virtual void AddPostToClanBoard(LobbyDBSpec::AddToClanBoard_Data *postData, bool failIfNotLeader, bool failIfNotMember, bool failIfBlacklisted);
	virtual void RemovePostFromClanBoard(ClanId *clanId, ClanBoardPostId *postId, bool allowMemberToRemoveOwnPosts );
	virtual void DownloadMyClanBoards(bool ascendingSort);
	virtual void DownloadClanBoard(const char *clanHandle, ClanId *clanId, bool ascendingSort);
	virtual void DownloadClanMember(const char *clanHandle, ClanId *clanId, const char *userHandle, LobbyClientUserId* userId);

	// ------------ USER LOGIN KEYS --------------
	virtual void ValidateUserKey(const char *titleIdStr, const char *keyPassword, int keyPasswordLength);

	// ------------ MET USERS --------------
	/// Users recently played against
	virtual const DataStructures::List<LobbyUserClientDetails *>& GetRecentlyMetUsersList(void) const;

	// ------------ HELPERS --------------
	
	/// Lookup extra details on a user specified by id
	/// Currently only users in your current room are downloaded.
	/// \note Only "safe" details are downloaded for other users. If you pass your own Id (GetMyId()) all fields are fully filled out.
	/// \return True if the lookup succeeded
	virtual bool GetUserDetails(LobbyClientUserId *userId, LobbyUserClientDetails *details);

	/// Given the name of a user, return the id
	/// \note To get the name of a user given an id, use GetUserDetails()
	virtual LobbyClientUserId *GerUserIdFromName(const char *name);

	/// Get my own ID
	virtual LobbyClientUserId *GetMyId(void);

	/// Are two IDs equal? For the PC this is not necessary to call, as LobbyClientUserId is an unsigned integer
	virtual bool EqualIDs(const LobbyClientUserId* userId1, const LobbyClientUserId* userId2);

	/// Sets the class to get callbacks
	virtual void SetCallbackInterface(LobbyClientInterfaceCB *cbi);

	// ------------ PC SPECIFIC --------------
	/// Ordering channel to use with RakPeer::Send()
	void SetOrderingChannel(char oc);

	/// Send priority to use with RakPeer::Send()
	void SetSendPriority(PacketPriority pp);

protected:
	// Plugin interface functions
	void OnAttach(RakPeerInterface *peer);
	PluginReceiveResult OnReceive(RakPeerInterface *peer, Packet *packet);
	void OnCloseConnection(RakPeerInterface *peer, SystemAddress systemAddress);
	void OnShutdown(RakPeerInterface *peer);

	// Incoming packets
	void Login_NetMsg(Packet *packet);
	void SetTitleLoginId_NetMsg(Packet *packet);
	void Logoff_NetMsg(Packet *packet);
	void RegisterAccount_NetMsg(Packet *packet);
	void UpdateAccount_NetMsg(Packet *packet);
	void ChangeHandle_NetMsg(Packet *packet);
	void RetrievePasswordRecoveryQuestion_NetMsg(Packet *packet);
	void RetrievePassword_NetMsg(Packet *packet);
	void DownloadFriends_NetMsg(Packet *packet);
	void SendAddFriendInvite_NetMsg(Packet *packet);
	void AcceptAddFriendInvite_NetMsg(Packet *packet);
	void NotifyFriendStatus_NetMsg(Packet *packet);
	void DeclineAddFriendInvite_NetMsg(Packet *packet);
	void DownloadIgnoreList_NetMsg(Packet *packet);
	void UpdateIgnoreList_NetMsg(Packet *packet);
	void RemoveFromIgnoreList_NetMsg(Packet *packet);
	void DownloadEmails_NetMsg(Packet *packet);
	void SendEmail_NetMsg(Packet *packet);
	void DeleteEmail_NetMsg(Packet *packet);
	void UpdateEmailStatus_NetMsg(Packet *packet);
	void IncomingEmail_NetMsg(Packet *packet);
	void SendIM_NetMsg(Packet *packet);
	void ReceiveIM_NetMsg(Packet *packet);
	void CreateRoom_NetMsg(Packet *packet);
	void DownloadRooms_NetMsg(Packet *packet);
	void LeaveRoom_NetMsg(Packet *packet);
	void JoinRoom_NetMsg(Packet *packet);
	void JoinRoomByFilter_NetMsg(Packet *packet);
	void RoomChat_NetMsg(Packet *packet);
	void InviteToRoom_NetMsg(Packet *packet);
	void KickRoomMember_NetMsg(Packet *packet);
	void SetRoomIsHidden_NetMsg(Packet *packet);
	void SetRoomAllowSpectators_NetMsg(Packet *packet);
	void ChangeNumSlots_NetMsg(Packet *packet);
	void GrantModerator_NetMsg(Packet *packet);
	void StartGame_NetMsg(Packet *packet);
	void SubmitMatch_NetMsg(Packet *packet);
	void DownloadRating_NetMsg(Packet *packet);
	void QuickMatch_NetMsg(Packet *packet);
	void CancelQuickMatch_NetMsg(Packet *packet);
	void DownloadActionHistory_NetMsg(Packet *packet);
	void AddToActionHistory_NetMsg(Packet *packet);
	void CreateClan_NetMsg(Packet *packet);
	void ChangeClanHandle_NetMsg(Packet *packet);
	void LeaveClan_NetMsg(Packet *packet);
	void DownloadClans_NetMsg(Packet *packet);
	void SendClanJoinInvitation_NetMsg(Packet *packet);
	void WithdrawClanJoinInvitation_NetMsg(Packet *packet);
	void AcceptClanJoinInvitation_NetMsg(Packet *packet);
	void RejectClanJoinInvitation_NetMsg(Packet *packet);
	void DownloadByClanMemberStatus_NetMsg(Packet *packet);
	void SendClanMemberJoinRequest_NetMsg(Packet *packet);
	void WithdrawClanMemberJoinRequest_NetMsg(Packet *packet);
	void AcceptClanMemberJoinRequest_NetMsg(Packet *packet);
	void RejectClanMemberJoinRequest_NetMsg(Packet *packet);
	void SetClanMemberRank_NetMsg(Packet *packet);
	void ClanKickAndBlacklistUser_NetMsg(Packet *packet);
	void ClanUnblacklistUser_NetMsg(Packet *packet);
	void AddPostToClanBoard_NetMsg(Packet *packet);
	void RemovePostFromClanBoard_NetMsg(Packet *packet);
	void DownloadMyClanBoards_NetMsg(Packet *packet);
	void DownloadClanBoard_NetMsg(Packet *packet);
	void DownloadClanMember_NetMsg(Packet *packet);
	void ValidateUserKey_NetMsg(Packet *packet);
	void NotifyRoomMemberDrop_NetMsg(Packet *packet);
	void NotifyRoomMemberJoin_NetMsg(Packet *packet);
	void NotifyRoomMemberReadyState_NetMsg(Packet *packet);
	void NotifyRoomAllNewModerator_NetMsg(Packet *packet);
	void NotifyRoomSetroomIsHidden_NetMsg(Packet *packet);
	void NotifyRoomSetAllowSpectators_NetMsg(Packet *packet);
	void NotifyRoomChangeNumSlots_NetMsg(Packet *packet);
	void NotifyRoomChat_NetMsg(Packet *packet);
	void NotifyRoomInvite_NetMsg(Packet *packet);
	void NotifyRoomDestroyed_NetMsg(Packet *packet);
	void NotifyKickedOutOfRoom_NetMsg(Packet *packet);
	void NotifyQuickMatchTimeout_NetMsg(Packet *packet);
	void NotifyStartGame(Packet *packet);
	void NotifyAddFriendResponse_NetMsg(Packet *packet);
	void NotifyClanDissolved_NetMsg(Packet *packet);
	void NotifyLeaveClan_NetMsg(Packet *packet);
	void NotifySendClanJoinInvitation_NetMsg(Packet *packet);
	void NotifyWithdrawClanJoinInvitation_NetMsg(Packet *packet);
	void NotifyAcceptClanJoinInvitation_NetMsg(Packet *packet);
	void NotifyRejectClanJoinInvitation_NetMsg(Packet *packet);
	void NotifySendClanMemberJoinRequest_NetMsg(Packet *packet);
	void NotifyWithdrawClanMemberJoinRequest_NetMsg(Packet *packet);
	void NotifyAcceptClanMemberJoinRequest_NetMsg(Packet *packet);
	void NotifyRejectClanMemberJoinRequest_NetMsg(Packet *packet);
	void NotifySetClanMemberRank_NetMsg(Packet *packet);
	void NotifyClanMemberKicked_NetMsg(Packet *packet);
	void NotifyClanMemberUnblacklisted_NetMsg(Packet *packet);
	
	bool Send( const RakNet::BitStream * bitStream );
	void InitializeVariables(void);

	void SendSetTitleLoginId(void);

	void AddFriend(bool isOnline, const char *friendName, LobbyClientUserId id);
	bool RemoveFriend(LobbyClientUserId id);
	bool IsFriend(LobbyClientUserId id);

	void AddIgnoredUser(LobbyClientUserId id, const char *actions, long long creationTime);
	bool RemoveIgnoredUser(LobbyClientUserId id);
	bool IsIgnoredUser(LobbyClientUserId id);

	void DeserializeAndAddClientSafeUser(RakNet::BitStream *bs);
	void SerializeHandleOrId(const char *userHandle, LobbyClientUserId *userId, RakNet::BitStream *bs);
	void SerializeOptionalId(unsigned int *id, RakNet::BitStream *bs);
	void DeserializeClientSafeCreateUserData(LobbyClientUserId *userId, SystemAddress *systemAddress, LobbyDBSpec::CreateUser_Data *data, bool *readyToPlay, RakNet::BitStream *bs);
	LobbyClientUser * GetLobbyClientUser(LobbyClientUserId id);
	void AddLobbyClientUser(LobbyClientUser *user);
	void ClearLobbyClientUsers(void);
	void ClearLobbyClientUser(LobbyClientUserId id);
	void ClearActionHistory(void);
	void ClearEmails(bool inbox);
	void ClearRecentlyMetUsers(void);
	void AddEmail(LobbyDBSpec::SendEmail_Data *email, bool inbox);
	void UpdateEmail(EmailId id, int status, bool wasOpened);
	void RemoveEmail(EmailId id);
	void LeaveCurrentRoom(bool roomIsDead);

	
	
	RakPeerInterface *rakPeer;
	SystemAddress serverAddr;
	LobbyClientInterfaceCB *cb;
	char orderingChannel;
	PacketPriority packetPriority;

	// Used to store the sort order of GetEmails so we can automatically query the new list after sending an email
	bool hasCalledGetEmails;
	bool lastGetEmailsSortIsAscending;
	unsigned char lastGetEmailsLangauge;

	DataStructures::List<LC_Ignored *> ignoreList;
	DataStructures::List<LC_Friend*> friendsList;
	DataStructures::List<LobbyUserClientDetails*> recentlyMetUsers;
	DataStructures::List<LobbyDBSpec::SendEmail_Data *> inboxEmails, sentEmails;
	DataStructures::List<LobbyDBSpec::AddToActionHistory_Data *> actionHistory;
	DataStructures::List<LobbyClientUser*> lobbyClientUsers;
	DataStructures::Table allRooms;
	LobbyRoom currentRoom;

	LobbyUserStatus lobbyUserStatus;
	bool downloadingRooms;

	char *titleIdStr;
	char *titleLoginPW;
	int titleLoginPWLength;

	LobbyDBSpec::CreateUser_Data myself;
	RankingServerDBSpec::GetRatingForParticipant_Data rating;
	LobbyClientUserId myUserId;
	bool readyToPlay;
};

}

#endif
