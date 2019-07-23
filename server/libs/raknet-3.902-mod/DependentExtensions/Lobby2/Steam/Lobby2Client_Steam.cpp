#include "Lobby2Client_Steam.h"
#include "Lobby2Message_Steam.h"
#include <stdlib.h>
#include "NativeTypes.h"
#include "MTUSize.h"
#include <windows.h>

using namespace RakNet;


DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(Lobby2Message,uint64_t,requestId);

int Lobby2Client_Steam::SystemAddressAndRoomMemberComp( const SystemAddress &key, const Lobby2Client_Steam::RoomMember &data )
{
	if (key<data.systemAddress)
		return -1;
	if (key==data.systemAddress)
		return 0;
	return 1;
}

Lobby2Client_Steam::Lobby2Client_Steam() :
m_CallbackLobbyDataUpdated( this, &Lobby2Client_Steam::OnLobbyDataUpdatedCallback ),
m_CallbackPersonaStateChange( this, &Lobby2Client_Steam::OnPersonaStateChange ),
m_CallbackLobbyDataUpdate( this, &Lobby2Client_Steam::OnLobbyDataUpdate ),
m_CallbackChatDataUpdate( this, &Lobby2Client_Steam::OnLobbyChatUpdate ),
m_CallbackChatMessageUpdate( this, &Lobby2Client_Steam::OnLobbyChatMessage ),
m_CallbackP2PSessionRequest( this, &Lobby2Client_Steam::OnP2PSessionRequest ),
m_CallbackP2PSessionConnectFail( this, &Lobby2Client_Steam::OnP2PSessionConnectFail )
{
	// Must recompile RakNet with MAXIMUM_MTU_SIZE set to 1200 in the preprocessor settings	
	RakAssert(MAXIMUM_MTU_SIZE<=1200);
}

Lobby2Client_Steam::Lobby2Client_Steam(const char *gameVersion) :
m_CallbackLobbyDataUpdated( this, &Lobby2Client_Steam::OnLobbyDataUpdatedCallback ),
m_CallbackPersonaStateChange( this, &Lobby2Client_Steam::OnPersonaStateChange ),
m_CallbackLobbyDataUpdate( this, &Lobby2Client_Steam::OnLobbyDataUpdate ),
m_CallbackChatDataUpdate( this, &Lobby2Client_Steam::OnLobbyChatUpdate ),
m_CallbackChatMessageUpdate( this, &Lobby2Client_Steam::OnLobbyChatMessage ),
m_CallbackP2PSessionRequest( this, &Lobby2Client_Steam::OnP2PSessionRequest ),
m_CallbackP2PSessionConnectFail( this, &Lobby2Client_Steam::OnP2PSessionConnectFail )
{
	roomId=0;
	versionString=gameVersion;

}
Lobby2Client_Steam::~Lobby2Client_Steam()
{

}
void Lobby2Client_Steam::SendMsg(Lobby2Message *msg)
{
	if (msg->ClientImpl((Lobby2Client*) this))
	{
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}
	else
	{
		// Won't be deleted by the user's call to Deref.
		msg->resultCode=L2RC_PROCESSING;
		msg->AddRef();
		PushDeferredCallback(msg);
	}
}
void Lobby2Client_Steam::Update(void)
{
	SteamAPI_RunCallbacks();

	/*
	// sending data
	// must be a handle to a connected socket
	// data is all sent via UDP, and thus send sizes are limited to 1200 bytes; after this, many routers will start dropping packets
	// use the reliable flag with caution; although the resend rate is pretty aggressive,
	// it can still cause stalls in receiving data (like TCP)
	virtual bool SendDataOnSocket( SNetSocket_t hSocket, void *pubData, uint32 cubData, bool bReliable ) = 0;

	// receiving data
	// returns false if there is no data remaining
	// fills out *pcubMsgSize with the size of the next message, in bytes
	virtual bool IsDataAvailableOnSocket( SNetSocket_t hSocket, uint32 *pcubMsgSize ) = 0; 

	// fills in pubDest with the contents of the message
	// messages are always complete, of the same size as was sent (i.e. packetized, not streaming)
	// if *pcubMsgSize < cubDest, only partial data is written
	// returns false if no data is available
	virtual bool RetrieveDataFromSocket( SNetSocket_t hSocket, void *pubDest, uint32 cubDest, uint32 *pcubMsgSize ) = 0; 
	*/
}

void Lobby2Client_Steam::PushDeferredCallback(Lobby2Message *msg)
{
	deferredCallbacks.Push(msg, msg->requestId, __FILE__, __LINE__ );
}
void Lobby2Client_Steam::CallCBWithResultCode(Lobby2Message *msg, Lobby2ResultCode rc)
{
	if (msg)
	{
		msg->resultCode=rc;
		for (unsigned long i=0; i < callbacks.Size(); i++)
		{
			if (msg->callbackId==(unsigned char)-1 || msg->callbackId==callbacks[i]->callbackId)
				msg->CallCallback(callbacks[i]);
		}
	}	
}
void Lobby2Client_Steam::OnLobbyMatchListCallback( LobbyMatchList_t *pCallback, bool bIOFailure )
{
	(void) bIOFailure;

	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		// Get any instance of Console_SearchRooms
		if (deferredCallbacks[i]->GetID()==L2MID_Console_SearchRooms)
		{
			Console_SearchRooms_Steam *callbackResult = (Console_SearchRooms_Steam *) deferredCallbacks[i];
			//			iterate the returned lobbies with GetLobbyByIndex(), from values 0 to m_nLobbiesMatching-1
			// lobbies are returned in order of closeness to the user, so add them to the list in that order
			for ( uint32 iLobby = 0; iLobby < pCallback->m_nLobbiesMatching; iLobby++ )
			{
				CSteamID steamId = SteamMatchmaking()->GetLobbyByIndex( iLobby );
				callbackResult->roomIds.Push(steamId, __FILE__, __LINE__ );
				RakNet::RakString s = SteamMatchmaking()->GetLobbyData( steamId, "name" );
				callbackResult->roomNames.Push(s, __FILE__, __LINE__ );
			}

			CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);
			break;
		}
	}
}
void Lobby2Client_Steam::OnLobbyDataUpdatedCallback( LobbyDataUpdate_t *pCallback )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_GetRoomDetails)
		{
			Console_GetRoomDetails_Steam *callbackResult = (Console_GetRoomDetails_Steam *) deferredCallbacks[i];
			if (callbackResult->roomId==pCallback->m_ulSteamIDLobby)
			{
				const char *pchLobbyName = SteamMatchmaking()->GetLobbyData( pCallback->m_ulSteamIDLobby, "name" );
				if ( pchLobbyName[0] )
				{
					callbackResult->roomName=pchLobbyName;
				}
				CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
				msgFactory->Dealloc(callbackResult);
				deferredCallbacks.RemoveAtIndex(i);
				break;
			}
		}
	}

	Console_GetRoomDetails_Steam notification;
	const char *pchLobbyName = SteamMatchmaking()->GetLobbyData( pCallback->m_ulSteamIDLobby, "name" );
	if ( pchLobbyName[0] )
	{
		notification.roomName=pchLobbyName;
	}
	notification.roomId=pCallback->m_ulSteamIDLobby;
	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::OnLobbyCreated( LobbyCreated_t *pCallback, bool bIOFailure )
{
	(void) bIOFailure;

	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_CreateRoom)
		{
			Console_CreateRoom_Steam *callbackResult = (Console_CreateRoom_Steam *) deferredCallbacks[i];
			callbackResult->roomId=pCallback->m_ulSteamIDLobby;
			SteamMatchmaking()->SetLobbyData( callbackResult->roomId, "name", callbackResult->roomName.C_String() );
			roomId=pCallback->m_ulSteamIDLobby;

			printf("\nNumber of Steam Lobby Members:%i in Lobby Name:%s\n", SteamMatchmaking()->GetNumLobbyMembers(roomId), callbackResult->roomName.C_String());
			RoomMember roomMember;
			roomMember.steamIDRemote=SteamMatchmaking()->GetLobbyOwner(roomId).ConvertToUint64();
			roomMember.systemAddress.binaryAddress=nextFreeSystemAddress++;
			roomMember.systemAddress.port=STEAM_UNUSED_PORT;
			roomMembers.Insert(roomMember.systemAddress,roomMember,true,__FILE__,__LINE__); // GetLobbyMemberByIndex( roomId, 0 ).ConvertToUint64());
			
			CallCBWithResultCode(callbackResult, L2RC_SUCCESS);
			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);

			// Commented out: Do not send the notification for yourself
			// CallRoomCallbacks();
			break;
		}
	}
}
void Lobby2Client_Steam::OnLobbyJoined( LobbyEnter_t *pCallback, bool bIOFailure )
{
	(void) bIOFailure;

	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==L2MID_Console_JoinRoom)
		{
			Console_JoinRoom_Steam *callbackResult = (Console_JoinRoom_Steam *) deferredCallbacks[i];

			if (pCallback->m_EChatRoomEnterResponse==k_EChatRoomEnterResponseSuccess)
			{
				roomId=pCallback->m_ulSteamIDLobby;

				CallCBWithResultCode(callbackResult, L2RC_SUCCESS);

				// First push to prevent being notified of ourselves
				RoomMember roomMember;
				roomMember.steamIDRemote=SteamMatchmaking()->GetLobbyOwner(roomId).ConvertToUint64();
				roomMember.systemAddress.binaryAddress=nextFreeSystemAddress++;
				roomMember.systemAddress.port=STEAM_UNUSED_PORT;
				roomMembers.Insert(roomMember.systemAddress,roomMember,true,__FILE__,__LINE__);

				CallRoomCallbacks();

				// In case the asynch lobby update didn't get it fast enough
				DataStructures::DefaultIndexType rmi;
				uint64_t myId64=SteamUser()->GetSteamID().ConvertToUint64();
				for (rmi=0; rmi < roomMembers.Size(); rmi++)
				{
					if (myId64==roomMembers[rmi].steamIDRemote)
						break;
				}

				if (rmi==roomMembers.Size())
				{
					roomMember.steamIDRemote=SteamMatchmaking()->GetLobbyOwner(roomId).ConvertToUint64();
					roomMember.systemAddress.binaryAddress=nextFreeSystemAddress++;
					roomMember.systemAddress.port=STEAM_UNUSED_PORT;
					roomMembers.Insert(roomMember.systemAddress,roomMember,true,__FILE__,__LINE__);
				}

				DataStructures::DefaultIndexType j;
				for (j=0; j < roomMembers.Size(); j++)
				{
					if (roomMembers[j].steamIDRemote==SteamUser()->GetSteamID().ConvertToUint64())
						continue;
				}
			}
			else
			{
				CallCBWithResultCode(callbackResult, L2RC_Console_JoinRoom_NO_SUCH_ROOM);
			}

			msgFactory->Dealloc(callbackResult);
			deferredCallbacks.RemoveAtIndex(i);
			break;
		}
	}
}
bool Lobby2Client_Steam::IsCommandRunning( Lobby2MessageID msgId )
{
	uint32_t i;
	for (i=0; i < deferredCallbacks.GetSize(); i++)
	{
		if (deferredCallbacks[i]->GetID()==msgId)
		{
			return true;
		}
	}
	return false;
}

void Lobby2Client_Steam::OnPersonaStateChange( PersonaStateChange_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every friend who changes state
	// so make sure the user is in the lobby before acting
	if ( !SteamFriends()->IsUserInSource( pCallback->m_ulSteamID, roomId ) )
		return;

	if ((pCallback->m_nChangeFlags & k_EPersonaChangeNameFirstSet) ||
		(pCallback->m_nChangeFlags & k_EPersonaChangeName))
	{
		Notification_Friends_StatusChange_Steam notification;
		notification.friendId=pCallback->m_ulSteamID;
		const char *pchName = SteamFriends()->GetFriendPersonaName( notification.friendId );
		notification.friendNewName=pchName;
		CallCBWithResultCode(&notification, L2RC_SUCCESS);
	}
}
void Lobby2Client_Steam::OnLobbyDataUpdate( LobbyDataUpdate_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	if ( roomId != pCallback->m_ulSteamIDLobby )
		return;

	Notification_Console_UpdateRoomParameters_Steam notification;
	notification.roomId=roomId;
	notification.roomNewName=SteamMatchmaking()->GetLobbyData( roomId, "name" );
	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::OnLobbyChatUpdate( LobbyChatUpdate_t *pCallback )
{
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	if ( roomId != pCallback->m_ulSteamIDLobby )
		return;

	// Purpose: Handles users in the lobby joining or leaving ??????
	CallRoomCallbacks();	
}
void Lobby2Client_Steam::OnLobbyChatMessage( LobbyChatMsg_t *pCallback )
{
	CSteamID speaker;
	EChatEntryType entryType;
	char data[2048];
	int cubData=sizeof(data);
	SteamMatchmaking()->GetLobbyChatEntry( roomId, pCallback->m_iChatID, &speaker, data, cubData, &entryType);
	if (entryType==k_EChatEntryTypeChatMsg)
	{
		Notification_Console_RoomChatMessage_Steam notification;
		notification.message=data;
		CallCBWithResultCode(&notification, L2RC_SUCCESS);
	}

}
void Lobby2Client_Steam::GetRoomMembers(DataStructures::OrderedList<uint64_t, uint64_t> &_roomMembers)
{
	_roomMembers.Clear(true,__FILE__,__LINE__);
	int cLobbyMembers = SteamMatchmaking()->GetNumLobbyMembers( roomId );
	for ( int i = 0; i < cLobbyMembers; i++ )
	{
		CSteamID steamIDLobbyMember = SteamMatchmaking()->GetLobbyMemberByIndex( roomId, i ) ;
		uint64_t memberid=steamIDLobbyMember.ConvertToUint64();
		_roomMembers.Insert(memberid,memberid,true,__FILE__,__LINE__);
	}
}

const char * Lobby2Client_Steam::GetRoomMemberName(uint64_t memberId)
{
	return SteamFriends()->GetFriendPersonaName( memberId );
}

bool Lobby2Client_Steam::IsRoomOwner(CSteamID roomid)
{
	if(SteamUser()->GetSteamID() == SteamMatchmaking()->GetLobbyOwner(roomid))
		return true;

	return false;
}

bool Lobby2Client_Steam::IsInRoom(void) const
{
	return roomMembers.Size() > 0;
}

void Lobby2Client_Steam::CallRoomCallbacks()
{
	DataStructures::OrderedList<uint64_t,uint64_t> currentMembers;
	GetRoomMembers(currentMembers);
	DataStructures::OrderedList<SystemAddress, RoomMember, SystemAddressAndRoomMemberComp> updatedRoomMembers;

	DataStructures::DefaultIndexType currentMemberIndex=0, oldMemberIndex=0;
	while (currentMemberIndex < currentMembers.Size() && oldMemberIndex < roomMembers.Size())
	{
		if (currentMembers[currentMemberIndex]<roomMembers[oldMemberIndex].steamIDRemote)
		{
			RoomMember roomMember;
			roomMember.steamIDRemote=currentMembers[currentMemberIndex];
			roomMember.systemAddress.binaryAddress=nextFreeSystemAddress++;
			roomMember.systemAddress.port=STEAM_UNUSED_PORT;
			updatedRoomMembers.Insert(roomMember.systemAddress,roomMember,true,__FILE__,__LINE__);

			// new member
			NotifyNewMember(currentMembers[currentMemberIndex]);
			currentMemberIndex++;
		}
		else if (currentMembers[currentMemberIndex]>roomMembers[oldMemberIndex].steamIDRemote)
		{
			// dropped member
			NotifyDroppedMember(roomMembers[oldMemberIndex].steamIDRemote);
			oldMemberIndex++;
		}
		else
		{
			updatedRoomMembers.Insert(roomMembers[oldMemberIndex].systemAddress,roomMembers[oldMemberIndex],true,__FILE__,__LINE__);

			currentMemberIndex++;
			oldMemberIndex++;
		}
	}

	while (oldMemberIndex < roomMembers.Size())
	{
		// dropped member
		NotifyDroppedMember(roomMembers[oldMemberIndex].steamIDRemote);

		oldMemberIndex++;
	}
	while (currentMemberIndex < currentMembers.Size())
	{
		RoomMember roomMember;
		roomMember.steamIDRemote=currentMembers[currentMemberIndex];
		roomMember.systemAddress.binaryAddress=nextFreeSystemAddress++;
		roomMember.systemAddress.port=STEAM_UNUSED_PORT;
		updatedRoomMembers.Insert(roomMember.systemAddress,roomMember,true,__FILE__,__LINE__);

		// new member
		NotifyNewMember(currentMembers[currentMemberIndex]);

		currentMemberIndex++;
	}

	roomMembers=updatedRoomMembers;
}
void Lobby2Client_Steam::NotifyNewMember(uint64_t memberId)
{
	// const char *pchName = SteamFriends()->GetFriendPersonaName( memberId );

	Notification_Console_MemberJoinedRoom_Steam notification;
	notification.roomId=roomId;
	notification.srcMemberId=memberId;
	notification.memberName=SteamFriends()->GetFriendPersonaName( memberId );

	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::NotifyDroppedMember(uint64_t memberId)
{
	/// const char *pchName = SteamFriends()->GetFriendPersonaName( memberId );

	Notification_Console_MemberLeftRoom_Steam notification;
	notification.roomId=roomId;
	notification.srcMemberId=memberId;
	notification.memberName=SteamFriends()->GetFriendPersonaName( memberId );
	CallCBWithResultCode(&notification, L2RC_SUCCESS);
}
void Lobby2Client_Steam::ClearRoom(void)
{
	roomId=0;
	if (SteamNetworking())
	{
		for (DataStructures::DefaultIndexType i=0; i < roomMembers.Size(); i++)
		{
			SteamNetworking()->CloseP2PSessionWithUser( roomMembers[i].steamIDRemote );
		}
	}
	roomMembers.Clear(true,__FILE__,__LINE__);
}
void Lobby2Client_Steam::OnP2PSessionRequest( P2PSessionRequest_t *pCallback )
{
	// we'll accept a connection from anyone
	SteamNetworking()->AcceptP2PSessionWithUser( pCallback->m_steamIDRemote );
}
void Lobby2Client_Steam::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pCallback )
{
	(void) pCallback;

	// we've sent a packet to the user, but it never got through
	// we can just use the normal timeout
}
void Lobby2Client_Steam::NotifyLeaveRoom(void)
{
	ClearRoom();
}

int Lobby2Client_Steam::RakNetSendTo( SOCKET s, const char *data, int length, SystemAddress systemAddress )
{
	bool objectExists;
	DataStructures::DefaultIndexType i = roomMembers.GetIndexFromKey(systemAddress, &objectExists);
	if (objectExists)
	{
		if (SteamNetworking()->SendP2PPacket(roomMembers[i].steamIDRemote, data, length, k_EP2PSendUnreliable))
			return length;
		else
			return 0;
	}
	else if (systemAddress.port!=STEAM_UNUSED_PORT)
	{
		return SocketLayer::SendTo_PC(s,data,length,systemAddress.binaryAddress,systemAddress.port,__FILE__,__LINE__);
	}
	return 0;
}

int Lobby2Client_Steam::RakNetRecvFrom( const SOCKET sIn, RakPeer *rakPeerIn, char dataOut[ MAXIMUM_MTU_SIZE ], SystemAddress *senderOut, bool calledFromMainThread)
{
	(void) calledFromMainThread;
	(void) rakPeerIn;
	(void) sIn;

	uint32 pcubMsgSize;
	if (SteamNetworking()->IsP2PPacketAvailable(&pcubMsgSize))
	{
		CSteamID psteamIDRemote;
		if (SteamNetworking()->ReadP2PPacket(dataOut, MAXIMUM_MTU_SIZE, &pcubMsgSize, &psteamIDRemote))
		{
			uint64_t steamIDRemote64=psteamIDRemote.ConvertToUint64();
			DataStructures::DefaultIndexType i;
			for (i=0; i < roomMembers.Size(); i++)
			{
				if (roomMembers[i].steamIDRemote==steamIDRemote64)
				{
					*senderOut=roomMembers[i].systemAddress;
					break;
				}
			}
			return pcubMsgSize;
		}
	}
	return 0;
}

void Lobby2Client_Steam::OnRakPeerShutdown(void)
{
	ClearRoom();
}
void Lobby2Client_Steam::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	bool objectExists;
	DataStructures::DefaultIndexType i = roomMembers.GetIndexFromKey(systemAddress, &objectExists);
	if (objectExists)
	{
		SteamNetworking()->CloseP2PSessionWithUser( roomMembers[i].steamIDRemote );
		roomMembers.RemoveAtIndex(i);
	}
}
void Lobby2Client_Steam::OnFailedConnectionAttempt(Packet *packet, PI2_FailedConnectionAttemptReason failedConnectionAttemptReason)
{
	(void) failedConnectionAttemptReason;

	bool objectExists;
	DataStructures::DefaultIndexType i = roomMembers.GetIndexFromKey(packet->systemAddress, &objectExists);
	if (objectExists)
	{
		SteamNetworking()->CloseP2PSessionWithUser( roomMembers[i].steamIDRemote );
		roomMembers.RemoveAtIndex(i);
	}
}
