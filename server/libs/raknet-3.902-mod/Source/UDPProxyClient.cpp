#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPProxyClient==1

#include "UDPProxyClient.h"
#include "BitStream.h"
#include "UDPProxyCommon.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "GetTime.h"

using namespace RakNet;
static const int DEFAULT_UNRESPONSIVE_PING_TIME=1000;

bool operator<( const DataStructures::MLKeyRef<UDPProxyClient::ServerWithPing> &inputKey, const UDPProxyClient::ServerWithPing &cls ) {return inputKey.Get().serverAddress < cls.serverAddress;}
bool operator>( const DataStructures::MLKeyRef<UDPProxyClient::ServerWithPing> &inputKey, const UDPProxyClient::ServerWithPing &cls ) {return inputKey.Get().serverAddress > cls.serverAddress;}
bool operator==( const DataStructures::MLKeyRef<UDPProxyClient::ServerWithPing> &inputKey, const UDPProxyClient::ServerWithPing &cls ) {return inputKey.Get().serverAddress == cls.serverAddress;}


UDPProxyClient::UDPProxyClient()
{
	resultHandler=0;
}
UDPProxyClient::~UDPProxyClient()
{
	Clear();
}
void UDPProxyClient::SetResultHandler(UDPProxyClientResultHandler *rh)
{
	resultHandler=rh;
}
bool UDPProxyClient::RequestForwarding(SystemAddress proxyCoordinator, SystemAddress sourceAddress, RakNetGUID targetGuid, RakNetTimeMS timeoutOnNoDataMS, RakNet::BitStream *serverSelectionBitstream)
{
	if (rakPeerInterface->IsConnected(proxyCoordinator,false,false)==false)
		return false;

	// Pretty much a bug not to set the result handler, as otherwise you won't know if the operation succeeed or not
	RakAssert(resultHandler!=0);
	if (resultHandler==0)
		return false;

	BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_REQUEST_FROM_CLIENT_TO_COORDINATOR);
	outgoingBs.Write(sourceAddress);
	outgoingBs.Write(false);
	outgoingBs.Write(targetGuid);
	outgoingBs.Write(timeoutOnNoDataMS);
	if (serverSelectionBitstream && serverSelectionBitstream->GetNumberOfBitsUsed()>0)
	{
		outgoingBs.Write(true);
		outgoingBs.Write(serverSelectionBitstream);
	}
	else
	{
		outgoingBs.Write(false);
	}
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, proxyCoordinator, false);

	return true;
}
bool UDPProxyClient::RequestForwarding(SystemAddress proxyCoordinator, SystemAddress sourceAddress, SystemAddress targetAddressAsSeenFromCoordinator, RakNetTimeMS timeoutOnNoDataMS, RakNet::BitStream *serverSelectionBitstream)
{
	if (rakPeerInterface->IsConnected(proxyCoordinator,false,false)==false)
		return false;

	// Pretty much a bug not to set the result handler, as otherwise you won't know if the operation succeeed or not
	RakAssert(resultHandler!=0);
	if (resultHandler==0)
		return false;

	BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_REQUEST_FROM_CLIENT_TO_COORDINATOR);
	outgoingBs.Write(sourceAddress);
	outgoingBs.Write(true);
		outgoingBs.Write(targetAddressAsSeenFromCoordinator);
	outgoingBs.Write(timeoutOnNoDataMS);
	if (serverSelectionBitstream && serverSelectionBitstream->GetNumberOfBitsUsed()>0)
	{
		outgoingBs.Write(true);
		outgoingBs.Write(serverSelectionBitstream);
	}
	else
	{
		outgoingBs.Write(false);
	}
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, proxyCoordinator, false);

	return true;
}
void UDPProxyClient::Update(void)
{
	DataStructures::DefaultIndexType idx1=0;
	while (idx1 < pingServerGroups.GetSize())
	{
		PingServerGroup *psg = pingServerGroups[idx1];

		if (psg->serversToPing.GetSize() > 0 && 
			RakNet::GetTime() > psg->startPingTime+DEFAULT_UNRESPONSIVE_PING_TIME)
		{
			// If they didn't reply within DEFAULT_UNRESPONSIVE_PING_TIME, just give up on them
			psg->SendPingedServersToCoordinator(rakPeerInterface);

			RakNet::OP_DELETE(psg,__FILE__,__LINE__);
			pingServerGroups.RemoveAtIndex(idx1, __FILE__, __LINE__ );
		}
		else
			idx1++;
	}

}
PluginReceiveResult UDPProxyClient::OnReceive(Packet *packet)
{
	if (packet->data[0]==ID_PONG)
	{
		DataStructures::DefaultIndexType idx1, idx2;
		PingServerGroup *psg;
		for (idx1=0; idx1 < pingServerGroups.GetSize(); idx1++)
		{
			psg = pingServerGroups[idx1];
			for (idx2=0; idx2 < psg->serversToPing.GetSize(); idx2++)
			{
				if (psg->serversToPing[idx2].serverAddress==packet->systemAddress)
				{
					RakNet::BitStream bsIn(packet->data,packet->length,false);
					bsIn.IgnoreBytes(sizeof(MessageID));
					RakNetTime sentTime;
					bsIn.Read(sentTime);
					RakNetTime curTime=RakNet::GetTime();
					int ping;
					if (curTime>sentTime)
						ping=(int) (curTime-sentTime);
					else
						ping=0;
					psg->serversToPing[idx2].ping=(unsigned short) ping;

					// If all servers to ping are now pinged, reply to coordinator
					if (psg->AreAllServersPinged())
					{
						psg->SendPingedServersToCoordinator(rakPeerInterface);
						RakNet::OP_DELETE(psg,__FILE__,__LINE__);
						pingServerGroups.RemoveAtIndex(idx1, __FILE__, __LINE__ );
					}

					return RR_STOP_PROCESSING_AND_DEALLOCATE;
				}
			}

		}
	}
	else if (packet->data[0]==ID_UDP_PROXY_GENERAL && packet->length>1)
	{
		switch (packet->data[1])
		{
		case ID_UDP_PROXY_PING_SERVERS_FROM_COORDINATOR_TO_CLIENT:
			{
				OnPingServers(packet);
			}
		break;
		case ID_UDP_PROXY_FORWARDING_SUCCEEDED:
		case ID_UDP_PROXY_ALL_SERVERS_BUSY:
		case ID_UDP_PROXY_IN_PROGRESS:
		case ID_UDP_PROXY_NO_SERVERS_ONLINE:
		case ID_UDP_PROXY_RECIPIENT_GUID_NOT_CONNECTED_TO_COORDINATOR:
		case ID_UDP_PROXY_FORWARDING_NOTIFICATION:
			{
				SystemAddress senderAddress, targetAddress;
				RakNet::BitStream incomingBs(packet->data, packet->length, false);
				incomingBs.IgnoreBytes(sizeof(MessageID)*2);
				incomingBs.Read(senderAddress);
				incomingBs.Read(targetAddress);

				switch (packet->data[1])
				{
				case ID_UDP_PROXY_FORWARDING_NOTIFICATION:
				case ID_UDP_PROXY_FORWARDING_SUCCEEDED:
					{
						unsigned short forwardingPort;
						RakNet::RakString serverIP;
						incomingBs.Read(serverIP);
						incomingBs.Read(forwardingPort);
						if (packet->data[1]==ID_UDP_PROXY_FORWARDING_SUCCEEDED)
						{
							if (resultHandler)
								resultHandler->OnForwardingSuccess(serverIP.C_String(), forwardingPort, packet->systemAddress, senderAddress, targetAddress, this);
						}
						else
						{
							// Send a datagram to the proxy, so if we are behind a router, that router adds an entry to the routing table.
							// Otherwise the router would block the incoming datagrams from source
							// It doesn't matter if the message actually arrives as long as it goes through the router
							rakPeerInterface->Ping(serverIP.C_String(), forwardingPort, false);

							if (resultHandler)
								resultHandler->OnForwardingNotification(serverIP.C_String(), forwardingPort, packet->systemAddress, senderAddress, targetAddress, this);
						}
					}
					break;
				case ID_UDP_PROXY_ALL_SERVERS_BUSY:
					if (resultHandler)
						resultHandler->OnAllServersBusy(packet->systemAddress, senderAddress, targetAddress, this);
					break;
				case ID_UDP_PROXY_IN_PROGRESS:
					if (resultHandler)
						resultHandler->OnForwardingInProgress(packet->systemAddress, senderAddress, targetAddress, this);
					break;
				case ID_UDP_PROXY_NO_SERVERS_ONLINE:
					if (resultHandler)
						resultHandler->OnNoServersOnline(packet->systemAddress, senderAddress, targetAddress, this);
					break;
				case ID_UDP_PROXY_RECIPIENT_GUID_NOT_CONNECTED_TO_COORDINATOR:
					{
						RakNetGUID targetGuid;
						incomingBs.Read(targetGuid);
						if (resultHandler)
							resultHandler->OnRecipientNotConnected(packet->systemAddress, senderAddress, targetAddress, targetGuid, this);
						break;
					}
				}
			
			}
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}
	return RR_CONTINUE_PROCESSING;
}
void UDPProxyClient::OnRakPeerShutdown(void)
{
	Clear();
}
void UDPProxyClient::OnPingServers(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);

	PingServerGroup *psg = RakNet::OP_NEW<PingServerGroup>(__FILE__,__LINE__);
	
	ServerWithPing swp;
	incomingBs.Read(psg->sata.senderClientAddress);
	incomingBs.Read(psg->sata.targetClientAddress);
	psg->startPingTime=RakNet::GetTime();
	psg->coordinatorAddressForPings=packet->systemAddress;
	unsigned short serverListSize;
	incomingBs.Read(serverListSize);
	SystemAddress serverAddress;
	unsigned short serverListIndex;
	char ipStr[64];
	for (serverListIndex=0; serverListIndex<serverListSize; serverListIndex++)
	{
		incomingBs.Read(swp.serverAddress);
		swp.ping=DEFAULT_UNRESPONSIVE_PING_TIME;
		psg->serversToPing.Push(swp, __FILE__, __LINE__ );
		swp.serverAddress.ToString(false,ipStr);
		rakPeerInterface->Ping(ipStr,swp.serverAddress.port,false,0);
	}
	pingServerGroups.Push(psg,__FILE__,__LINE__);
}

bool UDPProxyClient::PingServerGroup::AreAllServersPinged(void) const
{
	DataStructures::DefaultIndexType serversToPingIndex;
	for (serversToPingIndex=0; serversToPingIndex < serversToPing.GetSize(); serversToPingIndex++)
	{
		if (serversToPing[serversToPingIndex].ping==DEFAULT_UNRESPONSIVE_PING_TIME)
			return false;
	}
	return true;
}

void UDPProxyClient::PingServerGroup::SendPingedServersToCoordinator(RakPeerInterface *rakPeerInterface)
{
	BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_PING_SERVERS_REPLY_FROM_CLIENT_TO_COORDINATOR);
	outgoingBs.Write(sata.senderClientAddress);
	outgoingBs.Write(sata.targetClientAddress);
	unsigned short serversToPingSize = (unsigned short) serversToPing.GetSize();
	outgoingBs.Write(serversToPingSize);
	DataStructures::DefaultIndexType serversToPingIndex;
	for (serversToPingIndex=0; serversToPingIndex < serversToPingSize; serversToPingIndex++)
	{
		outgoingBs.Write(serversToPing[serversToPingIndex].serverAddress);
		outgoingBs.Write(serversToPing[serversToPingIndex].ping);
	}
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, coordinatorAddressForPings, false);
}
void UDPProxyClient::Clear(void)
{
	pingServerGroups.ClearPointers(false,__FILE__,__LINE__);
}


#endif // _RAKNET_SUPPORT_*

