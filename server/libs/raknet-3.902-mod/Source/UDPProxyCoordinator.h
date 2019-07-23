/// \file
/// \brief Essentially maintains a list of servers running UDPProxyServer, and some state management for UDPProxyClient to find a free server to forward datagrams
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_UDPProxyCoordinator==1

#ifndef __UDP_PROXY_COORDINATOR_H
#define __UDP_PROXY_COORDINATOR_H

#include "Export.h"
#include "DS_Multilist.h"
#include "RakNetTypes.h"
#include "PluginInterface2.h"
#include "RakString.h"
#include "BitStream.h"

namespace RakNet
{
	/// When NAT Punchthrough fails, it is possible to use a non-NAT system to forward messages from us to the recipient, and vice-versa
	/// The class to forward messages is UDPForwarder, and it is triggered over the network via the UDPProxyServer plugin.
	/// The UDPProxyClient connects to UDPProxyCoordinator to get a list of servers running UDPProxyServer, and the coordinator will relay our forwarding request
	/// \brief Middleman between UDPProxyServer and UDPProxyClient, maintaining a list of UDPProxyServer, and managing state for clients to find an available forwarding server.
	/// \ingroup NAT_PUNCHTHROUGH_GROUP
	class RAK_DLL_EXPORT UDPProxyCoordinator : public PluginInterface2
	{
	public:
		UDPProxyCoordinator();
		virtual ~UDPProxyCoordinator();

		/// For UDPProxyServers logging in remotely, they must pass a password to UDPProxyServer::LoginToCoordinator(). It must match the password set here.
		/// If no password is set, they cannot login remotely.
		/// By default, no password is set
		void SetRemoteLoginPassword(RakNet::RakString password);

		/// \internal
		virtual void Update(void);
		virtual PluginReceiveResult OnReceive(Packet *packet);
		virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );

		struct SenderAndTargetAddress
		{
			SystemAddress senderClientAddress;
			SystemAddress targetClientAddress;
		};

		struct ServerWithPing
		{
			unsigned short ping;
			SystemAddress serverAddress;
		};

		struct ForwardingRequest
		{
			RakNetTimeMS timeoutOnNoDataMS;
			RakNetTimeMS timeoutAfterSuccess;
			SenderAndTargetAddress sata;
			SystemAddress requestingAddress; // Which system originally sent the network message to start forwarding
			SystemAddress currentlyAttemptedServerAddress;
			DataStructures::Multilist<ML_QUEUE, SystemAddress> remainingServersToTry;
			RakNet::BitStream serverSelectionBitstream;

			DataStructures::Multilist<ML_STACK, ServerWithPing, unsigned short> sourceServerPings, targetServerPings;
			RakNetTime timeRequestedPings;
			// Order based on sourceServerPings and targetServerPings
			void OrderRemainingServersToTry(void);
		
		};

	protected:
		void OnForwardingRequestFromClientToCoordinator(Packet *packet);
		void OnLoginRequestFromServerToCoordinator(Packet *packet);
		void OnForwardingReplyFromServerToCoordinator(Packet *packet);
		void OnPingServersReplyFromClientToCoordinator(Packet *packet);
		void TryNextServer(SenderAndTargetAddress sata, ForwardingRequest *fw);
		void SendAllBusy(SystemAddress senderClientAddress, SystemAddress targetClientAddress, SystemAddress requestingAddress);
		void Clear(void);

		void SendForwardingRequest(SystemAddress sourceAddress, SystemAddress targetAddress, SystemAddress serverAddress, RakNetTimeMS timeoutOnNoDataMS);

		// Logged in servers
		DataStructures::Multilist<ML_UNORDERED_LIST, SystemAddress> serverList;

		// Forwarding requests in progress
		DataStructures::Multilist<ML_ORDERED_LIST, ForwardingRequest*, SenderAndTargetAddress> forwardingRequestList;

		RakNet::RakString remoteLoginPassword;

	};

} // End namespace

#endif

#endif // _RAKNET_SUPPORT_*
