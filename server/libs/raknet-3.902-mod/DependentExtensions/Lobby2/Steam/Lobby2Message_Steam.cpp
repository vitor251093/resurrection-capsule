#include "Lobby2Message_Steam.h"
#include "steam_api.h"

using namespace RakNet;

bool Client_Login_Steam::ClientImpl( Lobby2Client *client)
{
	if ( !SteamAPI_Init() )
		resultCode=L2RC_GENERAL_ERROR;
	else
		resultCode=L2RC_SUCCESS;
	return true; // Done immediately
}
bool Client_Logoff_Steam::ClientImpl( Lobby2Client *client)
{
	Lobby2Client_Steam *steam = (Lobby2Client_Steam *)client;
	steam->NotifyLeaveRoom();

	resultCode=L2RC_SUCCESS;
	SteamAPI_Shutdown();

	return true; // Done immediately
}
bool Console_SearchRooms_Steam::ClientImpl( Lobby2Client *client)
{
	requestId = SteamMatchmaking()->RequestLobbyList();
	m_SteamCallResultLobbyMatchList.Set( requestId, (RakNet::Lobby2Client_Steam*) client, &Lobby2Client_Steam::OnLobbyMatchListCallback );
	return false; // Asynch
}
bool Console_GetRoomDetails_Steam::ClientImpl( Lobby2Client *client)
{
	SteamMatchmaking()->RequestLobbyData( roomId );

	return false; // Asynch
}
bool Console_CreateRoom_Steam::ClientImpl( Lobby2Client *client)
{
	if (roomIsPublic)
		requestId = SteamMatchmaking()->CreateLobby( k_ELobbyTypePublic, 10  );
	else
		requestId = SteamMatchmaking()->CreateLobby( k_ELobbyTypeFriendsOnly, 10  );

	// set the function to call when this completes
	m_SteamCallResultLobbyCreated.Set( requestId, (RakNet::Lobby2Client_Steam*) client, &Lobby2Client_Steam::OnLobbyCreated );

	return false; // Asynch
}
bool Console_JoinRoom_Steam::ClientImpl( Lobby2Client *client)
{
	requestId = SteamMatchmaking()->JoinLobby( roomId  );

	// set the function to call when this completes
	m_SteamCallResultLobbyEntered.Set( requestId, (RakNet::Lobby2Client_Steam*) client, &Lobby2Client_Steam::OnLobbyJoined );

	return false; // Asynch
}
bool Console_LeaveRoom_Steam::ClientImpl( Lobby2Client *client)
{
	SteamMatchmaking()->LeaveLobby( roomId );

	Lobby2Client_Steam *steam = (Lobby2Client_Steam *)client;
	steam->NotifyLeaveRoom();

	resultCode=L2RC_SUCCESS;
	return true; // Synchronous
}
bool Console_SendRoomChatMessage_Steam::ClientImpl( Lobby2Client *client)
{
	SteamMatchmaking()->SendLobbyChatMsg(roomId, message.C_String(), (int) message.GetLength()+1);

	// ISteamMatchmaking.h
	/*
	// Broadcasts a chat message to the all the users in the lobby
	// users in the lobby (including the local user) will receive a LobbyChatMsg_t callback
	// returns true if the message is successfully sent
	// pvMsgBody can be binary or text data, up to 4k
	// if pvMsgBody is text, cubMsgBody should be strlen( text ) + 1, to include the null terminator
	virtual bool SendLobbyChatMsg( CSteamID steamIDLobby, const void *pvMsgBody, int cubMsgBody ) = 0;
	*/

	resultCode=L2RC_SUCCESS;
	return true; // Synchronous
}