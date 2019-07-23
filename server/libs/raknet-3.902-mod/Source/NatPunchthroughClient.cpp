#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_NatPunchthroughClient==1

#include "NatPunchthroughClient.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "GetTime.h"
#include "PacketLogger.h"

void NatPunchthroughDebugInterface_Printf::OnClientMessage(const char *msg)
{
	printf("%s\n", msg);
}
#if _RAKNET_SUPPORT_PacketLogger==1
void NatPunchthroughDebugInterface_PacketLogger::OnClientMessage(const char *msg)
{
	if (pl)
	{
		pl->WriteMiscellaneous("Nat", msg);
	}
}
#endif

NatPunchthroughClient::NatPunchthroughClient()
{
	natPunchthroughDebugInterface=0;
	mostRecentNewExternalPort=0;
	sp.nextActionTime=0;
}
NatPunchthroughClient::~NatPunchthroughClient()
{
	rakPeerInterface=0;
	Clear();
}
bool NatPunchthroughClient::OpenNAT(RakNetGUID destination, SystemAddress facilitator)
{
	if (rakPeerInterface->IsConnected(facilitator)==false)
		return false;
	// Already connected
	SystemAddress sa = rakPeerInterface->GetSystemAddressFromGuid(destination);
	if (sa!=UNASSIGNED_SYSTEM_ADDRESS && rakPeerInterface->IsConnected(sa,true,true) )
		return false;

	SendPunchthrough(destination, facilitator);
	return true;
}
void NatPunchthroughClient::SetDebugInterface(NatPunchthroughDebugInterface *i)
{
	natPunchthroughDebugInterface=i;
}
unsigned short NatPunchthroughClient::GetUPNPExternalPort(void) const
{
	return mostRecentNewExternalPort;
}
unsigned short NatPunchthroughClient::GetUPNPInternalPort(void) const
{
	return rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS).port;
}
RakNet::RakString NatPunchthroughClient::GetUPNPInternalAddress(void) const
{
	char dest[64];
	rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS).ToString(false, dest);
	RakNet::RakString rs = dest;
	return rs;
}
void NatPunchthroughClient::Update(void)
{
	RakNetTimeMS time = RakNet::GetTimeMS();
	if (sp.nextActionTime && sp.nextActionTime < time)
	{
		RakNetTimeMS delta = time - sp.nextActionTime;
		if (sp.testMode==SendPing::TESTING_INTERNAL_IPS)
		{
			SendOutOfBand(sp.internalIds[sp.attemptCount],ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			if (++sp.retryCount>=pc.UDP_SENDS_PER_PORT_INTERNAL)
			{
				++sp.attemptCount;
				sp.retryCount=0;
			}

			if (sp.attemptCount>=pc.MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK)
			{
				sp.testMode=SendPing::WAITING_FOR_INTERNAL_IPS_RESPONSE;
				if (pc.INTERNAL_IP_WAIT_AFTER_ATTEMPTS>0)
				{
					sp.nextActionTime=time+pc.INTERNAL_IP_WAIT_AFTER_ATTEMPTS-delta;
				}
				else
				{
					sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT;
					sp.attemptCount=0;
				}
			}
			else
			{
				sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL-delta;
			}
		}
		else if (sp.testMode==SendPing::WAITING_FOR_INTERNAL_IPS_RESPONSE)
		{
			sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT;
			sp.attemptCount=0;
		}

		if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT)
		{
			SystemAddress sa;
			sa=sp.targetAddress;
			int port = sa.port+sp.attemptCount;
			sa.port=(unsigned short) port;
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			if (++sp.retryCount>=pc.UDP_SENDS_PER_PORT_EXTERNAL)
			{
				++sp.attemptCount;
				sp.retryCount=0;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_BETWEEN_PORTS-delta;
			}
			else
			{
				sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL-delta;
			}

			if (sp.attemptCount>=pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				// From 1024 disabled, never helps as I've seen, but slows down the process by half
				//sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FROM_1024;
				//sp.attemptCount=0;
				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;
			}
		}
		else if (sp.testMode==SendPing::TESTING_EXTERNAL_IPS_FROM_1024)
		{
			SystemAddress sa;
			sa=sp.targetAddress;
			int port = 1024+sp.attemptCount;
			sa.port=(unsigned short) port;
			SendOutOfBand(sa,ID_NAT_ESTABLISH_UNIDIRECTIONAL);

			if (++sp.retryCount>=pc.UDP_SENDS_PER_PORT_EXTERNAL)
			{
				++sp.attemptCount;
				sp.retryCount=0;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_BETWEEN_PORTS-delta;
			}
			else
			{
				sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL-delta;
			}

			if (sp.attemptCount>=pc.MAX_PREDICTIVE_PORT_RANGE)
			{
				if (natPunchthroughDebugInterface)
				{
					char ipAddressString[32];
					sp.targetAddress.ToString(true, ipAddressString);
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Likely bidirectional punchthrough failure to guid %s, system address %s.", guidString, ipAddressString));
				}

				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;
			}
		}
		else if (sp.testMode==SendPing::WAITING_AFTER_ALL_ATTEMPTS)
		{
			// Failed. Tell the user
			OnPunchthroughFailure();
		}

		if (sp.testMode==SendPing::PUNCHING_FIXED_PORT)
		{
//			RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);
			SendOutOfBand(sp.targetAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);
			if (++sp.retryCount>=sp.punchingFixedPortAttempts)
			{
				if (natPunchthroughDebugInterface)
				{
					char ipAddressString[32];
					sp.targetAddress.ToString(true, ipAddressString);
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Likely unidirectional punchthrough failure to guid %s, system address %s.", guidString, ipAddressString));
				}

				sp.testMode=SendPing::WAITING_AFTER_ALL_ATTEMPTS;
				sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS-delta;
			}
			else
			{
				if ((sp.retryCount%pc.UDP_SENDS_PER_PORT_EXTERNAL)==0)
					sp.nextActionTime=time+pc.EXTERNAL_IP_WAIT_BETWEEN_PORTS-delta;
				else
					sp.nextActionTime=time+pc.TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL-delta;
			}
		}
	}
}
void NatPunchthroughClient::PushFailure(void)
{
	Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID)+sizeof(unsigned char));
	p->data[0]=ID_NAT_PUNCHTHROUGH_FAILED;
	p->systemAddress=sp.targetAddress;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=sp.targetGuid;
	if (sp.weAreSender)
		p->data[1]=1;
	else
		p->data[1]=0;
	p->bypassPlugins=true;
	rakPeerInterface->PushBackPacket(p, true);
}
void NatPunchthroughClient::OnPunchthroughFailure(void)
{
	if (pc.retryOnFailure==false)
	{
		if (natPunchthroughDebugInterface)
		{
			char ipAddressString[32];
			sp.targetAddress.ToString(true, ipAddressString);
			char guidString[128];
			sp.targetGuid.ToString(guidString);
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Failed punchthrough once. Returning failure to guid %s, system address %s to user.", guidString, ipAddressString));
		}

		PushFailure();
		OnReadyForNextPunchthrough();
		return;
	}

	unsigned int i;
	for (i=0; i < failedAttemptList.Size(); i++)
	{
		if (failedAttemptList[i].guid==sp.targetGuid)
		{
			if (natPunchthroughDebugInterface)
			{
				char ipAddressString[32];
				sp.targetAddress.ToString(true, ipAddressString);
				char guidString[128];
				sp.targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Failed punchthrough twice. Returning failure to guid %s, system address %s to user.", guidString, ipAddressString));
			}

			// Failed a second time, so return failed to user
			PushFailure();

			OnReadyForNextPunchthrough();

			failedAttemptList.RemoveAtIndexFast(i);
			return;
		}
	}

	if (rakPeerInterface->IsConnected(sp.facilitator)==false)
	{
		if (natPunchthroughDebugInterface)
		{
			char ipAddressString[32];
			sp.targetAddress.ToString(true, ipAddressString);
			char guidString[128];
			sp.targetGuid.ToString(guidString);
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Not connected to facilitator, so cannot retry punchthrough after first failure. Returning failure onj guid %s, system address %s to user.", guidString, ipAddressString));
		}

		// Failed, and can't try again because no facilitator
		PushFailure();
		return;
	}

	if (natPunchthroughDebugInterface)
	{
		char ipAddressString[32];
		sp.targetAddress.ToString(true, ipAddressString);
		char guidString[128];
		sp.targetGuid.ToString(guidString);
		natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("First punchthrough failure on guid %s, system address %s. Reattempting.", guidString, ipAddressString));
	}

	// Failed the first time. Add to the failure queue and try again
	AddrAndGuid aag;
	aag.addr=sp.targetAddress;
	aag.guid=sp.targetGuid;
	failedAttemptList.Push(aag, __FILE__, __LINE__);

	// Tell the server we are ready
	OnReadyForNextPunchthrough();

	// If we are the sender, try again, immediately if possible, else added to the queue on the faciltiator
	if (sp.weAreSender)
		SendPunchthrough(sp.targetGuid, sp.facilitator);
}
PluginReceiveResult NatPunchthroughClient::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_NAT_GET_MOST_RECENT_PORT:
		{
			OnGetMostRecentPort(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
	case ID_NAT_PUNCHTHROUGH_FAILED:
	case ID_NAT_PUNCHTHROUGH_SUCCEEDED:
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_OUT_OF_BAND_INTERNAL:
		if (packet->length>=2 &&
			(packet->data[1]==ID_NAT_ESTABLISH_UNIDIRECTIONAL || packet->data[1]==ID_NAT_ESTABLISH_BIDIRECTIONAL) &&
			sp.nextActionTime!=0)
		{
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(2);
			uint16_t sessionId;
			bs.Read(sessionId);
//			RakAssert(sessionId<100);
			if (sessionId!=sp.sessionId)
				break;

			char ipAddressString[32];
			packet->systemAddress.ToString(true,ipAddressString);
			// sp.targetGuid==packet->guid is because the internal IP addresses reported may include loopbacks not reported by RakPeer::IsLocalIP()
			if (packet->data[1]==ID_NAT_ESTABLISH_UNIDIRECTIONAL && sp.targetGuid==packet->guid)
			{
				if (natPunchthroughDebugInterface)
				{
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Received ID_NAT_ESTABLISH_UNIDIRECTIONAL from guid %s, system address %s.", guidString, ipAddressString));
				}
				if (sp.testMode!=SendPing::PUNCHING_FIXED_PORT)
				{
					sp.testMode=SendPing::PUNCHING_FIXED_PORT;
					sp.retryCount+=sp.attemptCount*pc.UDP_SENDS_PER_PORT_EXTERNAL;
					sp.targetAddress=packet->systemAddress;
//					RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);
					// Keeps trying until the other side gives up too, in case it is unidirectional
					sp.punchingFixedPortAttempts=pc.UDP_SENDS_PER_PORT_EXTERNAL*pc.MAX_PREDICTIVE_PORT_RANGE;
				}

				SendOutOfBand(sp.targetAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);
			}
			else if (packet->data[1]==ID_NAT_ESTABLISH_BIDIRECTIONAL &&
				sp.targetGuid==packet->guid)
			{
				// They send back our port
				bs.Read(mostRecentNewExternalPort);

				SendOutOfBand(packet->systemAddress,ID_NAT_ESTABLISH_BIDIRECTIONAL);

				// Tell the user about the success
				sp.targetAddress=packet->systemAddress;
				PushSuccess();
				OnReadyForNextPunchthrough();
//				RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);
				bool removedFromFailureQueue=RemoveFromFailureQueue();

				if (natPunchthroughDebugInterface)
				{
					char guidString[128];
					sp.targetGuid.ToString(guidString);
					if (removedFromFailureQueue)
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough to guid %s, system address %s succeeded on 2nd attempt.", guidString, ipAddressString));
					else
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough to guid %s, system address %s succeeded on 1st attempt.", guidString, ipAddressString));
				}
			}

	//		mostRecentNewExternalPort=packet->systemAddress.port;
		}
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	case ID_NAT_ALREADY_IN_PROGRESS:
		{
			RakNet::BitStream incomingBs(packet->data, packet->length, false);
			incomingBs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID targetGuid;
			incomingBs.Read(targetGuid);
			if (natPunchthroughDebugInterface)
			{
				char guidString[128];
				targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough retry to guid %s failed due to ID_NAT_ALREADY_IN_PROGRESS. Returning failure.", guidString));
			}

		}
		break;
	case ID_NAT_TARGET_NOT_CONNECTED:
	case ID_NAT_CONNECTION_TO_TARGET_LOST:
	case ID_NAT_TARGET_UNRESPONSIVE:
		{
			const char *reason;
			if (packet->data[0]==ID_NAT_TARGET_NOT_CONNECTED)
				reason="ID_NAT_TARGET_NOT_CONNECTED";
			else if (packet->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST)
				reason="ID_NAT_CONNECTION_TO_TARGET_LOST";
			else
				reason="ID_NAT_TARGET_UNRESPONSIVE";

			RakNet::BitStream incomingBs(packet->data, packet->length, false);
			incomingBs.IgnoreBytes(sizeof(MessageID));

			RakNetGUID targetGuid;
			incomingBs.Read(targetGuid);
			if (packet->data[0]==ID_NAT_CONNECTION_TO_TARGET_LOST ||
				packet->data[0]==ID_NAT_TARGET_UNRESPONSIVE)
			{
				uint16_t sessionId;
				incomingBs.Read(sessionId);
				if (sessionId!=sp.sessionId)
					break;
			}

			unsigned int i;
			for (i=0; i < failedAttemptList.Size(); i++)
			{
				if (failedAttemptList[i].guid==targetGuid)
				{
					if (natPunchthroughDebugInterface)
					{
						char guidString[128];
						targetGuid.ToString(guidString);
						natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough retry to guid %s failed due to %s.", guidString, reason));

					}

					// If the retry target is not connected, or loses connection, or is not responsive, then previous failures cannot be retried.

					// Don't need to return failed, the other messages indicate failure anyway
					/*
					Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID));
					p->data[0]=ID_NAT_PUNCHTHROUGH_FAILED;
					p->systemAddress=failedAttemptList[i].addr;
					p->systemAddress.systemIndex=(SystemIndex)-1;
					p->guid=failedAttemptList[i].guid;
					rakPeerInterface->PushBackPacket(p, false);
					*/

					failedAttemptList.RemoveAtIndexFast(i);
					break;
				}
			}

			if (natPunchthroughDebugInterface)
			{
				char guidString[128];
				targetGuid.ToString(guidString);
				natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Punchthrough attempt to guid %s failed due to %s.", guidString, reason));
			}

			// Stop trying punchthrough
			sp.nextActionTime=0;

			/*
			RakNet::BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBytes(sizeof(MessageID));
			RakNetGUID failedSystem;
			bs.Read(failedSystem);
			bool deletedFirst=false;
			unsigned int i=0;
			while (i < pendingOpenNAT.Size())
			{
				if (pendingOpenNAT[i].destination==failedSystem)
				{
					if (i==0)
						deletedFirst=true;
					pendingOpenNAT.RemoveAtIndex(i);
				}
				else
					i++;
			}
			// Failed while in progress. Go to next in attempt queue
			if (deletedFirst && pendingOpenNAT.Size())
			{
				SendPunchthrough(pendingOpenNAT[0].destination, pendingOpenNAT[0].facilitator);
				sp.nextActionTime=0;
			}
			*/
		}
		break;
	case ID_TIMESTAMP:
		if (packet->data[sizeof(MessageID)+sizeof(RakNetTime)]==ID_NAT_CONNECT_AT_TIME)
		{
			OnConnectAtTime(packet);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}
		break;
	}
	return RR_CONTINUE_PROCESSING;
}
/*
void NatPunchthroughClient::ProcessNextPunchthroughQueue(void)
{
	// Go to the next attempt
	if (pendingOpenNAT.Size())
		pendingOpenNAT.RemoveAtIndex(0);

	// Do next punchthrough attempt
	if (pendingOpenNAT.Size())
		SendPunchthrough(pendingOpenNAT[0].destination, pendingOpenNAT[0].facilitator);

	sp.nextActionTime=0;
}
*/
void NatPunchthroughClient::OnConnectAtTime(Packet *packet)
{
//	RakAssert(sp.nextActionTime==0);

	RakNet::BitStream bs(packet->data, packet->length, false);
	bs.IgnoreBytes(sizeof(MessageID));
	bs.Read(sp.nextActionTime);
	bs.IgnoreBytes(sizeof(MessageID));
	bs.Read(sp.sessionId);
	bs.Read(sp.targetAddress);
	//RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);
	int j,k;
	k=0;
	for (j=0; j < MAXIMUM_NUMBER_OF_INTERNAL_IDS; j++)
	{
		SystemAddress id;
		bs.Read(id);
		char str[32];
		id.ToString(false,str);
		if (rakPeerInterface->IsLocalIP(str)==false)
			sp.internalIds[k++]=id;
	}
	sp.attemptCount=0;
	sp.retryCount=0;
	if (pc.MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK>0)
	{
		sp.testMode=SendPing::TESTING_INTERNAL_IPS;
	}
	else
	{
		sp.testMode=SendPing::TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT;
	}
	bs.Read(sp.targetGuid);
	bs.Read(sp.weAreSender);

	//RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);
}
void NatPunchthroughClient::SendTTL(SystemAddress sa)
{
	if (sa==UNASSIGNED_SYSTEM_ADDRESS)
		return;
	if (sa.port==0)
		return;

	char ipAddressString[32];
	sa.ToString(false, ipAddressString);
	rakPeerInterface->SendTTL(ipAddressString,sa.port, 3);
}
void NatPunchthroughClient::SendOutOfBand(SystemAddress sa, MessageID oobId)
{
	if (sa==UNASSIGNED_SYSTEM_ADDRESS)
		return;
	if (sa.port==0)
		return;

	//RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);

	RakNet::BitStream oob;
	oob.Write(oobId);
	oob.Write(sp.sessionId);
//	RakAssert(sp.sessionId<100);
	if (oobId==ID_NAT_ESTABLISH_BIDIRECTIONAL)
		oob.Write(sa.port);
	char ipAddressString[32];
	sa.ToString(false, ipAddressString);
	rakPeerInterface->SendOutOfBand((const char*) ipAddressString,sa.port,(const char*) oob.GetData(),oob.GetNumberOfBytesUsed());

	if (natPunchthroughDebugInterface)
	{
		sa.ToString(true,ipAddressString);
		char guidString[128];
		sp.targetGuid.ToString(guidString);

		if (oobId==ID_NAT_ESTABLISH_UNIDIRECTIONAL)
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Sent OOB ID_NAT_ESTABLISH_UNIDIRECTIONAL to guid %s, system address %s.", guidString, ipAddressString));
		else
			natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Sent OOB ID_NAT_ESTABLISH_BIDIRECTIONAL to guid %s, system address %s.", guidString, ipAddressString));
	}
}
void NatPunchthroughClient::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) rakNetGUID;
	(void) isIncoming;

	// Try to track new port mappings on the router. Not reliable, but better than nothing.
	SystemAddress ourExternalId = rakPeerInterface->GetExternalID(systemAddress);
	if (ourExternalId!=UNASSIGNED_SYSTEM_ADDRESS)
		mostRecentNewExternalPort=ourExternalId.port;
}

void NatPunchthroughClient::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) systemAddress;
	(void) rakNetGUID;
	(void) lostConnectionReason;

	if (sp.facilitator==systemAddress)
	{
		// If we lose the connection to the facilitator, all previous failures not currently in progress are returned as such
		unsigned int i=0;
		while (i < failedAttemptList.Size())
		{
			if (sp.nextActionTime!=0 && sp.targetGuid==failedAttemptList[i].guid)
			{
				i++;
				continue;
			}

			PushFailure();

			failedAttemptList.RemoveAtIndexFast(i);
		}
	}

	/*
	(void) lostConnectionReason;

	bool deletedFirst=false;
	unsigned int i=0;
	while (i < pendingOpenNAT.Size())
	{
		if (pendingOpenNAT[i].facilitator==systemAddress)
		{
			if (natPunchthroughDebugInterface)
			{
				if (lostConnectionReason==LCR_CLOSED_BY_USER)
					natPunchthroughDebugInterface->OnClientMessage("Nat server connection lost. Reason=LCR_CLOSED_BY_USER\n");
				else if (lostConnectionReason==LCR_DISCONNECTION_NOTIFICATION)
					natPunchthroughDebugInterface->OnClientMessage("Nat server connection lost. Reason=LCR_CLOSED_BY_USER\n");
				else if (lostConnectionReason==LCR_CONNECTION_LOST)
					natPunchthroughDebugInterface->OnClientMessage("Nat server connection lost. Reason=LCR_CONNECTION_LOST\n");
			}

			// Request failed because connection to server lost before remote system ping attempt occurred
			Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID));
			p->data[0]=ID_NAT_CONNECTION_TO_TARGET_LOST;
			p->systemAddress=systemAddress;
			p->systemAddress.systemIndex=(SystemIndex)-1;
			p->guid=rakNetGUID;
			rakPeerInterface->PushBackPacket(p, false);
			if (i==0)
				deletedFirst;

			pendingOpenNAT.RemoveAtIndex(i);
		}
		else
			i++;
	}

	// Lost connection to facilitator while an attempt was in progress. Give up on that attempt, and try the next in the queue.
	if (deletedFirst && pendingOpenNAT.Size())
	{
		SendPunchthrough(pendingOpenNAT[0].destination, pendingOpenNAT[0].facilitator);
	}
	*/
}
void NatPunchthroughClient::OnGetMostRecentPort(Packet *packet)
{
	RakNet::BitStream incomingBs(packet->data,packet->length,false);
	incomingBs.IgnoreBytes(sizeof(MessageID));
	uint16_t sessionId;
	incomingBs.Read(sessionId);

	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_GET_MOST_RECENT_PORT);
	outgoingBs.Write(sessionId);
	if (mostRecentNewExternalPort==0)
		mostRecentNewExternalPort=rakPeerInterface->GetExternalID(packet->systemAddress).port;
	outgoingBs.Write(mostRecentNewExternalPort);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
	sp.facilitator=packet->systemAddress;
}
/*
unsigned int NatPunchthroughClient::GetPendingOpenNATIndex(RakNetGUID destination, SystemAddress facilitator)
{
	unsigned int i;
	for (i=0; i < pendingOpenNAT.Size(); i++)
	{
		if (pendingOpenNAT[i].destination==destination && pendingOpenNAT[i].facilitator==facilitator)
			return i;
	}
	return (unsigned int) -1;
}
*/
void NatPunchthroughClient::SendPunchthrough(RakNetGUID destination, SystemAddress facilitator)
{
	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_PUNCHTHROUGH_REQUEST);
	outgoingBs.Write(destination);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,facilitator,false);

//	RakAssert(rakPeerInterface->GetSystemAddressFromGuid(destination)==UNASSIGNED_SYSTEM_ADDRESS);

	if (natPunchthroughDebugInterface)
	{
		char guidString[128];
		destination.ToString(guidString);
		natPunchthroughDebugInterface->OnClientMessage(RakNet::RakString("Starting ID_NAT_PUNCHTHROUGH_REQUEST to guid %s.", guidString));
	}
}
void NatPunchthroughClient::OnAttach(void)
{
	Clear();
}
void NatPunchthroughClient::OnDetach(void)
{
	Clear();
}
void NatPunchthroughClient::OnRakPeerShutdown(void)
{
	Clear();
}
void NatPunchthroughClient::Clear(void)
{
	OnReadyForNextPunchthrough();

	failedAttemptList.Clear(false, __FILE__,__LINE__);
}
PunchthroughConfiguration* NatPunchthroughClient::GetPunchthroughConfiguration(void)
{
	return &pc;
}
void NatPunchthroughClient::OnReadyForNextPunchthrough(void)
{
	if (rakPeerInterface==0)
		return;

	sp.nextActionTime=0;

	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_NAT_CLIENT_READY);
	rakPeerInterface->Send(&outgoingBs,HIGH_PRIORITY,RELIABLE_ORDERED,0,sp.facilitator,false);
}

void NatPunchthroughClient::PushSuccess(void)
{
//	RakAssert(rakPeerInterface->IsConnected(sp.targetAddress,true,true)==false);

	Packet *p = rakPeerInterface->AllocatePacket(sizeof(MessageID)+sizeof(unsigned char));
	p->data[0]=ID_NAT_PUNCHTHROUGH_SUCCEEDED;
	p->systemAddress=sp.targetAddress;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	p->guid=sp.targetGuid;
	if (sp.weAreSender)
		p->data[1]=1;
	else
		p->data[1]=0;
	p->bypassPlugins=true;
	rakPeerInterface->PushBackPacket(p, true);
}

bool NatPunchthroughClient::RemoveFromFailureQueue(void)
{
	unsigned int i;
	for (i=0; i < failedAttemptList.Size(); i++)
	{
		if (failedAttemptList[i].guid==sp.targetGuid)
		{
			// Remove from failure queue
			failedAttemptList.RemoveAtIndexFast(i);
			return true;
		}
	}
	return false;
}

#endif // _RAKNET_SUPPORT_*

