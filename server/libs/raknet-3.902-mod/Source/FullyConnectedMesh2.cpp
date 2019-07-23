/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_FullyConnectedMesh2==1

#include "FullyConnectedMesh2.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakAssert.h"
#include "GetTime.h"
#include "Rand.h"

// #define DEBUG_FCM2

FullyConnectedMesh2::FullyConnectedMesh2()
{
	startupTime=0;
	totalConnectionCount=0;
	ourFCMGuid=0;
	autoParticipateConnections=true;
	connectOnNewRemoteConnections=true;
}
FullyConnectedMesh2::~FullyConnectedMesh2()
{
	Clear();
}
RakNetGUID FullyConnectedMesh2::GetConnectedHost(void) const
{
	if (ourFCMGuid==0)
		return UNASSIGNED_RAKNET_GUID;
	return hostRakNetGuid;
}
SystemAddress FullyConnectedMesh2::GetConnectedHostAddr(void) const
{
	if (ourFCMGuid==0)
		return UNASSIGNED_SYSTEM_ADDRESS;
	return rakPeerInterface->GetSystemAddressFromGuid(hostRakNetGuid);
}
RakNetGUID FullyConnectedMesh2::GetHostSystem(void) const
{
	if (ourFCMGuid==0)
		return rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);

	return hostRakNetGuid;
}
bool FullyConnectedMesh2::IsHostSystem(void) const
{
	return GetHostSystem()==rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
}
bool FullyConnectedMesh2::IsConnectedHost(void) const
{
	return GetConnectedHost()==rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
}
void FullyConnectedMesh2::SetAutoparticipateConnections(bool b)
{
	autoParticipateConnections=b;
}
void FullyConnectedMesh2::ResetHostCalculation(void)
{
	startupTime=RakNet::GetTimeUS();
	totalConnectionCount=0;
	ourFCMGuid=0;
	for (unsigned int i=0; i < participantList.Size(); i++)
		SendFCMGuidRequest(participantList[i].rakNetGuid);
}
bool FullyConnectedMesh2::AddParticipantInternal( RakNetGUID rakNetGuid, FCM2Guid theirFCMGuid )
{
	for (unsigned int i=0; i < participantList.Size(); i++)
	{
		if (participantList[i].rakNetGuid==rakNetGuid)
		{
			if (theirFCMGuid!=0)
				participantList[i].fcm2Guid=theirFCMGuid;
			return false;
		}
	}

	FCM2Participant participant;
	participant.rakNetGuid=rakNetGuid;
	participant.fcm2Guid=theirFCMGuid;
	participantList.Push(participant,__FILE__,__LINE__);

	SendFCMGuidRequest(rakNetGuid);

	return true;
}
void FullyConnectedMesh2::AddParticipant( RakNetGUID rakNetGuid )
{
	if (rakPeerInterface->IsConnected(rakPeerInterface->GetSystemAddressFromGuid(rakNetGuid),false,false)==false)
	{
#ifdef DEBUG_FCM2
		printf("AddParticipant to %s failed (not connected)\n", rakNetGuid.ToString());
#endif
		return;
	}

	AddParticipantInternal(rakNetGuid,0);
}
PluginReceiveResult FullyConnectedMesh2::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		{
			if (connectOnNewRemoteConnections)
			{
				unsigned int count;
				RakNet::BitStream bsIn(packet->data, packet->length, false);
				bsIn.IgnoreBytes(sizeof(MessageID));
				bsIn.Read(count);
				SystemAddress remoteAddress;
				RakNetGUID remoteGuid;
				char str[64];
				for (unsigned int i=0; i < count; i++)
				{
					bsIn.Read(remoteAddress);
					bsIn.Read(remoteGuid);
					remoteAddress.ToString(false,str);
					rakPeerInterface->Connect(str,remoteAddress.port,connectionPassword.C_String(),(int) connectionPassword.GetLength());
				}
			}
		}
		break;
	case ID_FCM2_REQUEST_FCMGUID:
		OnRequestFCMGuid(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FCM2_RESPOND_CONNECTION_COUNT:
		OnRespondConnectionCount(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_FCM2_INFORM_FCMGUID:
		OnInformFCMGuid(packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	return RR_CONTINUE_PROCESSING;
}
void FullyConnectedMesh2::OnRakPeerStartup(void)
{
	Clear();
	startupTime=RakNet::GetTimeUS();
}
void FullyConnectedMesh2::OnAttach(void)
{
	Clear();
	// In case Startup() was called first
	if (rakPeerInterface->IsActive())
		startupTime=RakNet::GetTimeUS();
}
void FullyConnectedMesh2::OnRakPeerShutdown(void)
{
	Clear();
	startupTime=0;
}
void FullyConnectedMesh2::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) systemAddress;
	(void) rakNetGUID;

	unsigned int idx;
	for (idx=0; idx < participantList.Size(); idx++)
	{
		if (participantList[idx].rakNetGuid==rakNetGUID)
		{
			participantList[idx]=participantList[participantList.Size()-1];
#ifdef DEBUG_FCM2
			printf("Popping participant %s\n", participantList[participantList.Size()-1].rakNetGuid.ToString());
#endif

			participantList.Pop();
			if (rakNetGUID==hostRakNetGuid && ourFCMGuid!=0)
			{	
				if (participantList.Size()==0)
				{
					hostRakNetGuid=UNASSIGNED_RAKNET_GUID;
					hostFCM2Guid=ourFCMGuid;
				}
				else
				{
					CalculateHost(&hostRakNetGuid, &hostFCM2Guid);
				}
				PushNewHost(hostRakNetGuid);
			}
			return;
		}
	}

}
RakNetTimeUS FullyConnectedMesh2::GetElapsedRuntime(void)
{
	RakNetTimeUS curTime=RakNet::GetTimeUS();
	if (curTime>startupTime)
		return curTime-startupTime;
	else
		return 0;
}
void FullyConnectedMesh2::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) isIncoming;
	(void) rakNetGUID;
	(void) systemAddress;

	if (autoParticipateConnections)
		AddParticipant(rakNetGUID);
}
void FullyConnectedMesh2::Clear(void)
{
	participantList.Clear(false, __FILE__,__LINE__);

	totalConnectionCount=0;
	ourFCMGuid=0;
	lastPushedHost=UNASSIGNED_RAKNET_GUID;
}
void FullyConnectedMesh2::PushNewHost(const RakNetGUID &guid)
{
	Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID));
	p->data[0]=ID_FCM2_NEW_HOST;
	p->systemAddress=rakPeerInterface->GetSystemAddressFromGuid(guid);
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=guid;
	p->bypassPlugins=true;
	rakPeerInterface->PushBackPacket(p, true);

	lastPushedHost=guid;
}
void FullyConnectedMesh2::SendFCMGuidRequest(RakNetGUID rakNetGuid)
{
	if (rakNetGuid==rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return;

	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_FCM2_REQUEST_FCMGUID);
	if (ourFCMGuid==0)
	{
		bsOut.Write(false);
		bsOut.Write(GetElapsedRuntime());
	}
	else
	{
		bsOut.Write(true);
		bsOut.Write(totalConnectionCount);
		bsOut.Write(ourFCMGuid);
	}
 	rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,rakNetGuid,false);
}
void FullyConnectedMesh2::SendOurFCMGuid(SystemAddress addr)
{
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_FCM2_INFORM_FCMGUID);
	RakAssert(ourFCMGuid!=0); // Can't inform others of our FCM2Guid if it's unset!
	bsOut.Write(ourFCMGuid);
	bsOut.Write(totalConnectionCount);
	rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,addr,false);
}
void FullyConnectedMesh2::SendConnectionCountResponse(SystemAddress addr, unsigned int responseTotalConnectionCount)
{
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_FCM2_RESPOND_CONNECTION_COUNT);
	bsOut.Write(responseTotalConnectionCount);
	rakPeerInterface->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,addr,false);
}
void FullyConnectedMesh2::AssignOurFCMGuid(void)
{
	// Only assigned once ever
	RakAssert(ourFCMGuid==0);
	unsigned int randomNumber = randomMT();
	randomNumber ^= (unsigned int) (RakNet::GetTimeUS() & 0xFFFFFFFF);
	randomNumber ^= (unsigned int) (rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).g & 0xFFFFFFFF);
	ourFCMGuid |= randomNumber;
	uint64_t reponse64 = totalConnectionCount;
	ourFCMGuid |= reponse64<<32;
}
void FullyConnectedMesh2::CalculateHost(RakNetGUID *rakNetGuid, FCM2Guid *fcm2Guid)
{
	// Can't calculate host without knowing our own
	RakAssert(ourFCMGuid!=0);

	// Can't calculate host without being connected to anyone else
	RakAssert(participantList.Size()>0);

	// Return the lowest value of all FCM2Guid
	FCM2Guid lowestFCMGuid=ourFCMGuid;
//	SystemAddress associatedSystemAddress=UNASSIGNED_SYSTEM_ADDRESS;
	RakNetGUID associatedRakNetGuid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);

	DataStructures::DefaultIndexType idx;
	for (idx=0; idx < participantList.Size(); idx++)
	{
		if (participantList[idx].fcm2Guid!=0 && participantList[idx].fcm2Guid<lowestFCMGuid)
		{
			lowestFCMGuid=participantList[idx].fcm2Guid;
			associatedRakNetGuid=participantList[idx].rakNetGuid;
		}
	}

	*rakNetGuid=associatedRakNetGuid;
	*fcm2Guid=lowestFCMGuid;
}
void FullyConnectedMesh2::OnRequestFCMGuid(Packet *packet)
{
	RakNet::BitStream bsIn(packet->data,packet->length,false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	bool hasRemoteFCMGuid;
	bsIn.Read(hasRemoteFCMGuid);
	RakNetTimeUS senderElapsedRuntime=0;
	unsigned int remoteTotalConnectionCount=0;
	FCM2Guid theirFCMGuid=0;
	if (hasRemoteFCMGuid)
	{
		bsIn.Read(remoteTotalConnectionCount);
		bsIn.Read(theirFCMGuid);
	}
	else
	{
		bsIn.Read(senderElapsedRuntime);
	}
	AddParticipantInternal(packet->guid,theirFCMGuid);
	if (ourFCMGuid==0)
	{
		if (hasRemoteFCMGuid==false)
		{
			// Nobody has a fcmGuid

			RakNetTimeUS ourElapsedRuntime = GetElapsedRuntime();
			if (ourElapsedRuntime>senderElapsedRuntime)
			{
				// We are probably host
				SendConnectionCountResponse(packet->systemAddress, 2);
			}
			else
			{
				// They are probably host
				SendConnectionCountResponse(packet->systemAddress, 1);
			}
		}
		else
		{
			// They have a fcmGuid, we do not

			IncrementTotalConnectionCount(remoteTotalConnectionCount+1);

			AssignOurFCMGuid();
			DataStructures::DefaultIndexType idx;
			for (idx=0; idx < participantList.Size(); idx++)
				SendOurFCMGuid(rakPeerInterface->GetSystemAddressFromGuid(participantList[idx].rakNetGuid));
		}
	}
	else
	{
		if (hasRemoteFCMGuid==false)
		{
			// We have a fcmGuid they do not

			SendConnectionCountResponse(packet->systemAddress, totalConnectionCount+1);

		}
		else
		{
			// We both have fcmGuids

			IncrementTotalConnectionCount(remoteTotalConnectionCount);

			SendOurFCMGuid(packet->systemAddress);
		}
	}
	CalculateAndPushHost();
}
void FullyConnectedMesh2::OnRespondConnectionCount(Packet *packet)
{
	RakNet::BitStream bsIn(packet->data,packet->length,false);
	bsIn.IgnoreBytes(sizeof(MessageID));
	unsigned int responseTotalConnectionCount;
	bsIn.Read(responseTotalConnectionCount);
	IncrementTotalConnectionCount(responseTotalConnectionCount);
	bool wasAssigned;
	if (ourFCMGuid==0)
	{
		wasAssigned=true;
		AssignOurFCMGuid();
	}
	else
		wasAssigned=false;

	// 1 is returned to give us lower priority, but the actual minimum is 2
	IncrementTotalConnectionCount(2);

	if (wasAssigned==true)
	{
		DataStructures::DefaultIndexType idx;
		for (idx=0; idx < participantList.Size(); idx++)
			SendOurFCMGuid(rakPeerInterface->GetSystemAddressFromGuid(participantList[idx].rakNetGuid));
		CalculateAndPushHost();
	}
}
void FullyConnectedMesh2::OnInformFCMGuid(Packet *packet)
{
	RakNet::BitStream bsIn(packet->data,packet->length,false);
	bsIn.IgnoreBytes(sizeof(MessageID));

	FCM2Guid theirFCMGuid;
	unsigned int theirTotalConnectionCount;
	bsIn.Read(theirFCMGuid);
	bsIn.Read(theirTotalConnectionCount);
	IncrementTotalConnectionCount(theirTotalConnectionCount);
	AddParticipantInternal(packet->guid,theirFCMGuid);

	CalculateAndPushHost();
}
void FullyConnectedMesh2::GetParticipantCount(DataStructures::DefaultIndexType *participantListSize) const
{
	*participantListSize=participantList.Size();
}

unsigned int FullyConnectedMesh2::GetParticipantCount(void) const
{
	return participantList.Size();
}
void FullyConnectedMesh2::CalculateAndPushHost(void)
{
	RakNetGUID newHostGuid;
	FCM2Guid newFcmGuid;
	if (ParticipantListComplete())
	{
		CalculateHost(&newHostGuid, &newFcmGuid);
		if (newHostGuid!=lastPushedHost)
		{
			hostRakNetGuid=newHostGuid;
			hostFCM2Guid=newFcmGuid;
			PushNewHost(hostRakNetGuid);
		}
	}
}
bool FullyConnectedMesh2::ParticipantListComplete(void)
{
	for (unsigned int i=0; i < participantList.Size(); i++)
	{
		if (participantList[i].fcm2Guid==0)
			return false;
	}
	return true;
}
void FullyConnectedMesh2::IncrementTotalConnectionCount(unsigned int i)
{
	if (i>totalConnectionCount)
	{
		totalConnectionCount=i;
	//	printf("totalConnectionCount=%i\n",i);
	}
}
void FullyConnectedMesh2::SetConnectOnNewRemoteConnection(bool attemptConnection, RakNet::RakString pw)
{
	connectOnNewRemoteConnections=attemptConnection;
	connectionPassword=pw;
}

#endif // _RAKNET_SUPPORT_*
