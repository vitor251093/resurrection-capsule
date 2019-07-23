#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPProxyServer==1

#include "UDPProxyServer.h"
#include "BitStream.h"
#include "UDPProxyCommon.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"

using namespace RakNet;

UDPProxyServer::UDPProxyServer()
{
	resultHandler=0;
}
UDPProxyServer::~UDPProxyServer()
{

}
void UDPProxyServer::SetResultHandler(UDPProxyServerResultHandler *rh)
{
	resultHandler=rh;
}
bool UDPProxyServer::LoginToCoordinator(RakNet::RakString password, SystemAddress coordinatorAddress)
{
	DataStructures::DefaultIndexType insertionIndex;
	insertionIndex = loggingInCoordinators.GetInsertionIndex(coordinatorAddress);
	if (insertionIndex==(DataStructures::DefaultIndexType)-1)
		return false;
	if (loggedInCoordinators.GetInsertionIndex(coordinatorAddress)==(DataStructures::DefaultIndexType)-1)
		return false;
	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_LOGIN_REQUEST_FROM_SERVER_TO_COORDINATOR);
	outgoingBs.Write(password);
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, coordinatorAddress, false);
	loggingInCoordinators.InsertAtIndex(coordinatorAddress, insertionIndex, __FILE__, __LINE__ );
	return true;
}
void UDPProxyServer::Update(void)
{
	udpForwarder.Update();
}
PluginReceiveResult UDPProxyServer::OnReceive(Packet *packet)
{
	// Make sure incoming messages from from UDPProxyCoordinator

	if (packet->data[0]==ID_UDP_PROXY_GENERAL && packet->length>1)
	{
		switch (packet->data[1])
		{
		case ID_UDP_PROXY_FORWARDING_REQUEST_FROM_COORDINATOR_TO_SERVER:
			if (loggedInCoordinators.GetIndexOf(packet->systemAddress)!=(DataStructures::DefaultIndexType)-1)
			{
				OnForwardingRequestFromCoordinatorToServer(packet);
				return RR_STOP_PROCESSING_AND_DEALLOCATE;
			}
			break;
		case ID_UDP_PROXY_NO_PASSWORD_SET_FROM_COORDINATOR_TO_SERVER:
		case ID_UDP_PROXY_WRONG_PASSWORD_FROM_COORDINATOR_TO_SERVER:
		case ID_UDP_PROXY_ALREADY_LOGGED_IN_FROM_COORDINATOR_TO_SERVER:
		case ID_UDP_PROXY_LOGIN_SUCCESS_FROM_COORDINATOR_TO_SERVER:
			{
				DataStructures::DefaultIndexType removalIndex = loggingInCoordinators.GetIndexOf(packet->systemAddress);
				if (removalIndex!=(DataStructures::DefaultIndexType)-1)
				{
					loggingInCoordinators.RemoveAtKey(packet->systemAddress, false, __FILE__, __LINE__ );

					RakNet::BitStream incomingBs(packet->data, packet->length, false);
					incomingBs.IgnoreBytes(2);
					RakNet::RakString password;
					incomingBs.Read(password);
					switch (packet->data[1])
					{
					case ID_UDP_PROXY_NO_PASSWORD_SET_FROM_COORDINATOR_TO_SERVER:
						if (resultHandler)
							resultHandler->OnNoPasswordSet(password, this);
						break;
					case ID_UDP_PROXY_WRONG_PASSWORD_FROM_COORDINATOR_TO_SERVER:
						if (resultHandler)
							resultHandler->OnWrongPassword(password, this);
						break;
					case ID_UDP_PROXY_ALREADY_LOGGED_IN_FROM_COORDINATOR_TO_SERVER:
						if (resultHandler)
							resultHandler->OnAlreadyLoggedIn(password, this);
						break;
					case ID_UDP_PROXY_LOGIN_SUCCESS_FROM_COORDINATOR_TO_SERVER:
						RakAssert(loggedInCoordinators.GetIndexOf(packet->systemAddress)==(unsigned int)-1);
						loggedInCoordinators.Push(packet->systemAddress, __FILE__, __LINE__);
						if (resultHandler)
							resultHandler->OnLoginSuccess(password, this);
						break;
					}
				}


				return RR_STOP_PROCESSING_AND_DEALLOCATE;
			}
		}
	}
	return RR_CONTINUE_PROCESSING;
}
void UDPProxyServer::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) rakNetGUID;

	loggingInCoordinators.RemoveAtKey(systemAddress,false, __FILE__, __LINE__ );
	loggedInCoordinators.RemoveAtKey(systemAddress,false, __FILE__, __LINE__ );
}
void UDPProxyServer::OnRakPeerStartup(void)
{
	udpForwarder.Startup();
}
void UDPProxyServer::OnRakPeerShutdown(void)
{
	udpForwarder.Shutdown();
	loggingInCoordinators.Clear(true,__FILE__,__LINE__);
	loggedInCoordinators.Clear(true,__FILE__,__LINE__);
}
void UDPProxyServer::OnAttach(void)
{
	if (rakPeerInterface->IsActive())
		OnRakPeerStartup();
}
void UDPProxyServer::OnDetach(void)
{
	OnRakPeerShutdown();
}
void UDPProxyServer::OnForwardingRequestFromCoordinatorToServer(Packet *packet)
{
	SystemAddress sourceAddress, targetAddress;
	RakNet::BitStream incomingBs(packet->data, packet->length, false);
	incomingBs.IgnoreBytes(2);
	incomingBs.Read(sourceAddress);
	incomingBs.Read(targetAddress);
	RakNetTimeMS timeoutOnNoDataMS;
	incomingBs.Read(timeoutOnNoDataMS);
	RakAssert(timeoutOnNoDataMS > 0 && timeoutOnNoDataMS <= UDP_FORWARDER_MAXIMUM_TIMEOUT);

	unsigned short forwardingPort;
	UDPForwarderResult success = udpForwarder.StartForwarding(sourceAddress, targetAddress, timeoutOnNoDataMS, 0, &forwardingPort, 0);
	RakNet::BitStream outgoingBs;
	outgoingBs.Write((MessageID)ID_UDP_PROXY_GENERAL);
	outgoingBs.Write((MessageID)ID_UDP_PROXY_FORWARDING_REPLY_FROM_SERVER_TO_COORDINATOR);
	outgoingBs.Write(sourceAddress);
	outgoingBs.Write(targetAddress);
	outgoingBs.Write((unsigned char) success);
	if (success==UDPFORWARDER_SUCCESS)
	{
		outgoingBs.Write(forwardingPort);
	}
	rakPeerInterface->Send(&outgoingBs, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
}

#endif // _RAKNET_SUPPORT_*
