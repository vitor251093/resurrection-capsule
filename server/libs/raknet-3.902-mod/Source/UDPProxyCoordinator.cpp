#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPProxyCoordinator==1

#include "UDPProxyCoordinator.h"
#include "BitStream.h"
#include "UDPProxyCommon.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "Rand.h"
#include "GetTime.h"
#include "UDPForwarder.h"

// Larger than the client version
static const int DEFAULT_CLIENT_UNRESPONSIVE_PING_TIME=2000;
static const int DEFAULT_UNRESPONSIVE_PING_TIME=DEFAULT_CLIENT_UNRESPONSIVE_PING_TIME+1000;

using namespace RakNet;

bool operator<( const DataStructures::MLKeyRef<unsigned short> &inputKey, const UDPProxyCoordinator::ServerWithPing &cls ) {return inputKey.Get() < cls.ping;}
bool operator>( const DataStructures::MLKeyRef<unsigned short> &inputKey, const UDPProxyCoordinator::ServerWithPing &cls ) {return inputKey.Get() > cls.ping;}
bool operator==( const DataStructures::MLKeyRef<unsigned short> &inputKey, const UDPProxyCoordinator::ServerWithPing &cls ) {return inputKey.Get() == cls.ping;}

bool operator<( const DataStructures::MLKeyRef<UDPProxyCoordinator::SenderAndTargetAddress> &inputKey, const UDPProxyCoordinator::ForwardingRequest *cls )
{
	return inputKey.Get().senderClientAddress < cls->sata.senderClientAddress ||
		(inputKey.Get().senderClientAddress == cls->sata.senderClientAddress && inputKey.Get().targetClientAddress < cls->sata.targetClientAddress);
}
bool operator>( const DataStructures::MLKeyRef<UDPProxyCoordinator::SenderAndTargetAddress> &inputKey, const UDPProxyCoordinator::ForwardingRequest *cls )
{
	return inputKey.Get().senderClientAddress > cls->sata.senderClientAddress ||
		(inputKey.Get().senderClientAddress == cls->sata.senderClientAddress && inputKey.Get().targetClientAddress > cls->sata.targetClientAddress);
}
bool operator==( const DataStructures::MLKeyRef<UDPProxyCoordinator::SenderAndTargetAddress> &inputKey, const UDPProxyCoordinator::ForwardingRequest *cls )
{
	return inputKey.Get().senderClientAddress == cls->sata.senderClientAddress && inputKey.Get().targetClientAddress == cls->sata.targetClientAddress;
}

UDPProxyCoordinator::UDPProxyCoordinator()
{

}
UDPProxyCoordinator::~UDPProxyCoordinator()
{
	Clear();
}
void UDPProxyCoordinator::SetRemoteLoginPassword(RakNet::RakString password)
{
	remoteLoginPassword=password;
}
void UDPProxyCoordinator::Update(void)
{
	DataStructures::DefaultIndexType idx;
	RakNetTime curTime = RakNet::GetTime();
	ForwardingRequest *fw;
	idx=0;
	while (idx < forwardingRequestList.GetSize())
	{
		fw=forwardingRequestList[idx];
		if (fw->timeRequestedPings!=0 &&
			curTime > fw->timeRequestedPings + DEFAULT_UNRESPONSIVE_PING_TIME)
		{
			fw->OrderRemainingServersToTry();
			fw->timeRequestedPings=0;
			TryNextServer(fw->sata, fw);
			idx++;
		}
		else if (fw->timeoutAfterSuccess!=0 &&
			curTime > fw->timeoutAfterSuccess)
		{
			// Forwarding request succeeded, we waited a bit to prevent duplicates. Can forget about the entry now.
			RakNet::OP_DELETE(fw,__FILE__,__LINE__);
			forwardingRequestList.RemoveAtIndex(idx,__FILE__,__LINE__);
		}
		else
			idx++;
	}
}
PluginReceiveResult UDPProxyCoordinator::OnReceive(Packet *packet)
{
	if (packet->data[0]==ID_UDP_PROXY_GENERAL && packet->length>1)
	{
		switch (packet->data[1])
		{
		case ID_UDP_PROXY_FORWARDING_REQUEST_FROM_CLIENT_TO_COORDINATOR:
			OnForwardingRequestFromClientToCoordinator(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		case ID_UDP_PROXY_LOGIN_REQUEST_FROM_SERVER_TO_COORDINATOR:
			OnLoginRequestFromServerToCoordinator(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		case ID_UDP_PROXY_FORWARDING_REPLY_FROM_SERVER_TO_COORDINATOR:
			OnForwardingReplyFromServerToCoordinator(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		case ID_UDP_PROXY_PING_SERVERS_REPLY_FROM_CLIENT_TO_COORDINATOR:
			OnPingServersReplyFromClientToCoordinator(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	}
	return RR_CONTINUE_PROCESSING;
}
void UDPProxyCoordinator::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	DataStructures::DefaultIndexType idx, idx2;

	idx=0;
	while (idx < forwardingRequestList.GetSize())
	{
		if (forwardingRequestList[idx]->requestingAddress==systemAddress)
		{
			// Guy disconnected before the attempt completed
			RakNet::OP_DELETE(forwardingRequestList[idx], __FILE__, __LINE__);
			forwardingRequestList.RemoveAtIndex(idx, __FILE__, __LINE__ );
		}
		else
			idx++;
	}

	idx = serverList.GetIndexOf(systemAddress);
	if (idx!=(DataStructures::DefaultIndexType)-1)
	{
		ForwardingRequest *fw;
		// For each pending client for this server, choose from remaining servers.
		for (idx2=0; idx2 < forwardingRequestList.GetSize(); idx2++)
		{
			fw = forwardingRequestList[idx2];
			if (fw->currentlyAttemptedServerAddress==systemAddress)
			{
				// Try the next server
				TryNextServer(fw->sata, fw);
			}
		}

		// Remove dead server
		serverList.RemoveAtIndex(idx, __FILE__, __LINE__ );
	}
}
void UDPProxyCoordinator::OnForwardingRequestFromClientToCoordinator(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);
	SystemAddress sourceAddress;
	incomingBs.Read(sourceAddress);
	if (sourceAddress==UNASSIGNED_SYSTEM_ADDRESS)
		sourceAddress=packet->systemAddress;
	SystemAddress targetAddress;
	RakNetGUID targetGuid;
	bool usesAddress;
	incomingBs.Read(usesAddress);
	if (usesAddress)
	{
		incomingBs.Read(targetAddress);
	}
	else
	{
		incomingBs.Read(targetGuid);
		targetAddress=rakPeerInterface->GetSystemAddressFromGuid(targetGuid);
	}
	ForwardingRequest *fw = RakNet::OP_NEW<ForwardingRequest>(__FILE__,__LINE__);
	fw->timeoutAfterSuccess=0;
	incomingBs.Read(fw->timeoutOnNoDataMS);
	bool hasServerSelectionBitstream;
	incomingBs.Read(hasServerSelectionBitstream);
	if (hasServerSelectionBitstream)
		incomingBs.Read(&(fw->serverSelectionBitstream));

	RakNet::BitStream outgoingBs;
	SenderAndTargetAddress sata;
	sata.senderClientAddress=sourceAddress;
	sata.targetClientAddress=targetAddress;
	SenderAndTargetAddress sataReversed;
	sataReversed.senderClientAddress=targetAddress;
	sataReversed.targetClientAddress=sourceAddress;
	DataStructures::DefaultIndexType insertionIndex;
	insertionIndex = forwardingRequestList.GetInsertionIndex(sata);
	if (insertionIndex==(DataStructures::DefaultIndexType)-1 ||
		forwardingRequestList.GetInsertionIndex(sataReversed)==(DataStructures::DefaultIndexType)-1)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_IN_PROGRESS);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(targetAddress);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		RakNet::OP_DELETE(fw, __FILE__, __LINE__);
		return;
	}

	if (serverList.GetSize()==0)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_NO_SERVERS_ONLINE);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(targetAddress);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		RakNet::OP_DELETE(fw, __FILE__, __LINE__);
		return;
	}

	if (rakPeerInterface->IsConnected(targetAddress)==false && usesAddress==false)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_RECIPIENT_GUID_NOT_CONNECTED_TO_COORDINATOR);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(targetAddress);
		outgoingBs.Write(targetGuid);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		RakNet::OP_DELETE(fw, __FILE__, __LINE__);
		return;
	}

	fw->sata=sata;
	fw->requestingAddress=packet->systemAddress;

	if (serverList.GetSize()>1)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_PING_SERVERS_FROM_COORDINATOR_TO_CLIENT);
		outgoingBs.Write(sourceAddress);
		outgoingBs.Write(targetAddress);
		unsigned short serverListSize = (unsigned short) serverList.GetSize();
		outgoingBs.Write(serverListSize);
		DataStructures::DefaultIndexType idx;
		for (idx=0; idx < serverList.GetSize(); idx++)
			outgoingBs.Write(serverList[idx]);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, sourceAddress, false);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, targetAddress, false);
		fw->timeRequestedPings=RakNet::GetTime();
		DataStructures::DefaultIndexType copyIndex;
		for (copyIndex=0; copyIndex < serverList.GetSize(); copyIndex++)
			fw->remainingServersToTry.Push(serverList[copyIndex], __FILE__, __LINE__ );
		forwardingRequestList.InsertAtIndex(fw, insertionIndex, __FILE__, __LINE__ );
	}
	else
	{
		fw->timeRequestedPings=0;
		fw->currentlyAttemptedServerAddress=serverList[0];
		forwardingRequestList.InsertAtIndex(fw, insertionIndex, __FILE__, __LINE__ );
		SendForwardingRequest(sourceAddress, targetAddress, fw->currentlyAttemptedServerAddress, fw->timeoutOnNoDataMS);
	}
}

void UDPProxyCoordinator::SendForwardingRequest(SystemAddress sourceAddress, SystemAddress targetAddress, SystemAddress serverAddress, RakNetTimeMS timeoutOnNoDataMS)
{
	RakNet::BitStream outgoingBs;
	// Send request to desired server
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_REQUEST_FROM_COORDINATOR_TO_SERVER);
	outgoingBs.Write(sourceAddress);
	outgoingBs.Write(targetAddress);
	outgoingBs.Write(timeoutOnNoDataMS);
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, serverAddress, false);
}
void UDPProxyCoordinator::OnLoginRequestFromServerToCoordinator(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);
	RakNet::RakString password;
	incomingBs.Read(password);
	RakNet::BitStream outgoingBs;

	if (remoteLoginPassword.IsEmpty())
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_NO_PASSWORD_SET_FROM_COORDINATOR_TO_SERVER);
		outgoingBs.Write(password);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		return;
	}

	if (remoteLoginPassword!=password)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_WRONG_PASSWORD_FROM_COORDINATOR_TO_SERVER);
		outgoingBs.Write(password);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		return;
	}

	DataStructures::DefaultIndexType insertionIndex;
	insertionIndex=serverList.GetInsertionIndex(packet->systemAddress);
	if (insertionIndex==(DataStructures::DefaultIndexType)-1)
	{
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_ALREADY_LOGGED_IN_FROM_COORDINATOR_TO_SERVER);
		outgoingBs.Write(password);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
		return;
	}
	serverList.InsertAtIndex(packet->systemAddress, insertionIndex, __FILE__, __LINE__ );
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_LOGIN_SUCCESS_FROM_COORDINATOR_TO_SERVER);
	outgoingBs.Write(password);
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
}
void UDPProxyCoordinator::OnForwardingReplyFromServerToCoordinator(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);
	SenderAndTargetAddress sata;
	incomingBs.Read(sata.senderClientAddress);
	incomingBs.Read(sata.targetClientAddress);
	DataStructures::DefaultIndexType index = forwardingRequestList.GetIndexOf(sata);
	if (index==(DataStructures::DefaultIndexType)-1)
	{
		// The guy disconnected before the request finished
		return;
	}
	ForwardingRequest *fw = forwardingRequestList[index];

	UDPForwarderResult success;
	unsigned char c;
	incomingBs.Read(c);
	success=(UDPForwarderResult)c;

	RakNet::BitStream outgoingBs;
	if (success==UDPFORWARDER_SUCCESS)
	{
		char serverIP[64];
		packet->systemAddress.ToString(false,serverIP);
		unsigned short forwardingPort;
		incomingBs.Read(forwardingPort);

		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_SUCCEEDED);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(sata.targetClientAddress);
		outgoingBs.Write(RakNet::RakString(serverIP));
		outgoingBs.Write(forwardingPort);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, fw->requestingAddress, false);

		outgoingBs.Reset();
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_NOTIFICATION);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(sata.targetClientAddress);
		outgoingBs.Write(RakNet::RakString(serverIP));
		outgoingBs.Write(forwardingPort);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, sata.targetClientAddress, false);

		// 05/18/09 Keep the entry around for some time after success, so duplicates are reported if attempting forwarding from the target system before notification of success
		fw->timeoutAfterSuccess=RakNet::GetTime()+fw->timeoutOnNoDataMS;
		// forwardingRequestList.RemoveAtIndex(index);
		// RakNet::OP_DELETE(fw,__FILE__,__LINE__);

		return;
	}
	else if (success==UDPFORWARDER_NO_SOCKETS)
	{
		// Try next server
		TryNextServer(sata, fw);
	}
	else
	{
		RakAssert(success==UDPFORWARDER_FORWARDING_ALREADY_EXISTS);

		// Return in progress
		outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
		outgoingBs.Write((MessageID)ID_UDP_PROXY_IN_PROGRESS);
		outgoingBs.Write(sata.senderClientAddress);
		outgoingBs.Write(sata.targetClientAddress);
		rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, fw->requestingAddress, false);
		forwardingRequestList.RemoveAtIndex(index,__FILE__,__LINE__);
		RakNet::OP_DELETE(fw,__FILE__,__LINE__);
	}
}
void UDPProxyCoordinator::OnPingServersReplyFromClientToCoordinator(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);
	unsigned short serversToPingSize;
	SystemAddress serverAddress;
	SenderAndTargetAddress sata;
	incomingBs.Read(sata.senderClientAddress);
	incomingBs.Read(sata.targetClientAddress);
	DataStructures::DefaultIndexType index = forwardingRequestList.GetIndexOf(sata);
	if (index==(DataStructures::DefaultIndexType)-1)
		return;
	unsigned short idx;
	ServerWithPing swp;
	ForwardingRequest *fw = forwardingRequestList[index];
	if (fw->timeRequestedPings==0)
		return;

	incomingBs.Read(serversToPingSize);
	if (packet->systemAddress==sata.senderClientAddress)
	{
		for (idx=0; idx < serversToPingSize; idx++)
		{
			incomingBs.Read(swp.serverAddress);
			incomingBs.Read(swp.ping);
			fw->sourceServerPings.Push(swp, swp.ping, __FILE__, __LINE__);
		}
	}
	else
	{
		for (idx=0; idx < serversToPingSize; idx++)
		{
			incomingBs.Read(swp.serverAddress);
			incomingBs.Read(swp.ping);
			fw->targetServerPings.Push(swp, swp.ping, __FILE__, __LINE__);
		}
	}

	// Both systems have to give us pings to progress here. Otherwise will timeout in Update()
	if (fw->sourceServerPings.GetSize()>0 &&
		fw->targetServerPings.GetSize()>0)
	{
		fw->OrderRemainingServersToTry();
		fw->timeRequestedPings=0;
		TryNextServer(fw->sata, fw);
	}
}
void UDPProxyCoordinator::TryNextServer(SenderAndTargetAddress sata, ForwardingRequest *fw)
{
	bool pickedGoodServer=false;
	while(fw->remainingServersToTry.GetSize()>0)
	{
		fw->currentlyAttemptedServerAddress=fw->remainingServersToTry.Pop(__FILE__, __LINE__ );
		if (serverList.GetIndexOf(fw->currentlyAttemptedServerAddress)!=(DataStructures::DefaultIndexType)-1)
		{
			pickedGoodServer=true;
			break;
		}
	}

	if (pickedGoodServer==false)
	{
		SendAllBusy(sata.senderClientAddress, sata.targetClientAddress, fw->requestingAddress);
		forwardingRequestList.RemoveAtKey(sata,true,__FILE__,__LINE__);
		RakNet::OP_DELETE(fw,__FILE__,__LINE__);
		return;
	}

	SendForwardingRequest(sata.senderClientAddress, sata.targetClientAddress, fw->currentlyAttemptedServerAddress, fw->timeoutOnNoDataMS);
}
void UDPProxyCoordinator::SendAllBusy(SystemAddress senderClientAddress, SystemAddress targetClientAddress, SystemAddress requestingAddress)
{
	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_ALL_SERVERS_BUSY);
	outgoingBs.Write(senderClientAddress);
	outgoingBs.Write(targetClientAddress);
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, requestingAddress, false);
}
void UDPProxyCoordinator::Clear(void)
{
	serverList.Clear(true, __FILE__, __LINE__);
	forwardingRequestList.ClearPointers(true, __FILE__, __LINE__);
}
void UDPProxyCoordinator::ForwardingRequest::OrderRemainingServersToTry(void)
{
	DataStructures::Multilist<ML_ORDERED_LIST,UDPProxyCoordinator::ServerWithPing,unsigned short> swpList;
	swpList.SetSortOrder(true);
	if (sourceServerPings.GetSize()==0 && targetServerPings.GetSize()==0)
		return;

	DataStructures::DefaultIndexType idx;
	UDPProxyCoordinator::ServerWithPing swp;
	for (idx=0; idx < remainingServersToTry.GetSize(); idx++)
	{
		swp.serverAddress=remainingServersToTry[idx];
		swp.ping=0;
		if (sourceServerPings.GetSize())
			swp.ping+=(unsigned short) sourceServerPings[idx].ping;
		else
			swp.ping+=(unsigned short) DEFAULT_CLIENT_UNRESPONSIVE_PING_TIME;
		if (targetServerPings.GetSize())
			swp.ping+=(unsigned short) targetServerPings[idx].ping;
		else
			swp.ping+=(unsigned short) DEFAULT_CLIENT_UNRESPONSIVE_PING_TIME;
		swpList.Push(swp, swp.ping, __FILE__, __LINE__);
	}
	remainingServersToTry.Clear(true, __FILE__, __LINE__ );
	for (idx=0; idx < swpList.GetSize(); idx++)
	{
		remainingServersToTry.Push(swpList[idx].serverAddress, __FILE__, __LINE__ );
	}
}

#endif // _RAKNET_SUPPORT_*
