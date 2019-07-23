#ifndef __LOBBY_2_CLIENT_STEAM_H
#define __LOBBY_2_CLIENT_STEAM_H

#include "Lobby2Plugin.h"
#include "DS_Multilist.h"
#include "SocketLayer.h"
#include "steam_api.h"
#include "DS_OrderedList.h"

namespace RakNet
{
struct Lobby2Message;

#define STEAM_UNUSED_PORT 0

class RAK_DLL_EXPORT Lobby2Client_Steam : public RakNet::Lobby2Plugin, public SocketLayerOverride
{
public:	
	Lobby2Client_Steam(const char *gameVersion);
	virtual ~Lobby2Client_Steam();

	/// Send a command to the server
	/// \param[in] msg The message that represents the command
	virtual void SendMsg(Lobby2Message *msg);

	// Base class implementation
	virtual void Update(void);

	void OnLobbyMatchListCallback( LobbyMatchList_t *pCallback, bool bIOFailure );
	void OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure );
	void OnLobbyJoined( LobbyEnter_t *pCallback, bool bIOFailure );
	bool IsCommandRunning( Lobby2MessageID msgId );
	void GetRoomMembers(DataStructures::OrderedList<uint64_t, uint64_t> &_roomMembers);
	const char * GetRoomMemberName(uint64_t memberId);
	bool IsRoomOwner(const CSteamID roomid);
	bool IsInRoom(void) const;
	uint64_t GetNumRoomMembers(const uint64_t roomid){return SteamMatchmaking()->GetNumLobbyMembers(roomid);}
	uint64 GetMyUserID(void){return SteamUser()->GetSteamID().ConvertToUint64();}
	const char* GetMyUserPersonalName(void) {return SteamFriends()->GetPersonaName();}
	
	void NotifyLeaveRoom(void);

	// Returns 0 if none
	uint64_t GetRoomID(void) const {return roomId;}

	/// Called when SendTo would otherwise occur.
	virtual int RakNetSendTo( SOCKET s, const char *data, int length, SystemAddress systemAddress );

	/// Called when RecvFrom would otherwise occur. Return number of bytes read. Write data into dataOut
	virtual int RakNetRecvFrom( const SOCKET sIn, RakPeer *rakPeerIn, char dataOut[ MAXIMUM_MTU_SIZE ], SystemAddress *senderOut, bool calledFromMainThread);

	virtual void OnRakPeerShutdown(void);
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );
	virtual void OnFailedConnectionAttempt(Packet *packet, PI2_FailedConnectionAttemptReason failedConnectionAttemptReason);

	struct RoomMember
	{
		SystemAddress systemAddress;
		uint64_t steamIDRemote;
	};
	static int SystemAddressAndRoomMemberComp( const SystemAddress &key, const RoomMember &data );
protected:
	Lobby2Client_Steam();
	void CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc);
	void PushDeferredCallback(Lobby2Message *msg);
	void CallRoomCallbacks(void);
	void NotifyNewMember(uint64_t memberId);
	void NotifyDroppedMember(uint64_t memberId);
	void CallCompletedCallbacks(void);

	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyDataUpdatedCallback, LobbyDataUpdate_t, m_CallbackLobbyDataUpdated );
	STEAM_CALLBACK( Lobby2Client_Steam, OnPersonaStateChange, PersonaStateChange_t, m_CallbackPersonaStateChange );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyChatUpdate, LobbyChatUpdate_t, m_CallbackChatDataUpdate );
	STEAM_CALLBACK( Lobby2Client_Steam, OnLobbyChatMessage, LobbyChatMsg_t, m_CallbackChatMessageUpdate );

	STEAM_CALLBACK( Lobby2Client_Steam, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest );
	STEAM_CALLBACK( Lobby2Client_Steam, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_CallbackP2PSessionConnectFail );

	DataStructures::Multilist<ML_UNORDERED_LIST, Lobby2Message *, uint64_t > deferredCallbacks;

	uint64_t roomId;
	DataStructures::OrderedList<SystemAddress, RoomMember, SystemAddressAndRoomMemberComp> roomMembers;
	DataStructures::Multilist<ML_ORDERED_LIST, uint64_t> rooms;

	void ClearRoom(void);

	RakNet::RakString versionString;

	uint32_t nextFreeSystemAddress;

};

};

#endif
