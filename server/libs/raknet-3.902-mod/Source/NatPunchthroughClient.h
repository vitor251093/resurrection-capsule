
/// \file
/// \brief Contains the NAT-punchthrough plugin for the client.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_NatPunchthroughClient==1

#ifndef __NAT_PUNCHTHROUGH_CLIENT_H
#define __NAT_PUNCHTHROUGH_CLIENT_H

#include "RakNetTypes.h"
#include "Export.h"
#include "PluginInterface2.h"
#include "PacketPriority.h"
#include "SocketIncludes.h"
#include "DS_List.h"
#include "RakString.h"

// Trendnet TEW-632BRP sometimes starts at port 1024 and increments sequentially.
// Zonnet zsr1134we. Replies go out on the net, but are always absorbed by the remote router??
// Dlink ebr2310 to Trendnet ok
// Trendnet TEW-652BRP to Trendnet 632BRP OK
// Trendnet TEW-632BRP to Trendnet 632BRP OK
// Buffalo WHR-HP-G54 OK
// Netgear WGR614 ok

class RakPeerInterface;
struct Packet;
#if _RAKNET_SUPPORT_PacketLogger==1
class PacketLogger;
#endif

/// \ingroup NAT_PUNCHTHROUGH_GROUP
struct RAK_DLL_EXPORT PunchthroughConfiguration
{
	/// internal: (15 ms * 2 tries + 30 wait) * 5 ports * 8 players = 2.4 seconds
	/// external: (50 ms * 8 sends + 100 wait) * 2 port * 8 players = 8 seconds
	/// Total: 8 seconds
	PunchthroughConfiguration() {
		TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL=15;
		TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL=50;
		UDP_SENDS_PER_PORT_INTERNAL=2;
		UDP_SENDS_PER_PORT_EXTERNAL=8;
		INTERNAL_IP_WAIT_AFTER_ATTEMPTS=30;
		MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK=5; /// set to 0 to not do lan connects
		MAX_PREDICTIVE_PORT_RANGE=2;
		EXTERNAL_IP_WAIT_BETWEEN_PORTS=100;
		EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS=EXTERNAL_IP_WAIT_BETWEEN_PORTS;
		retryOnFailure=false;
	}

	/// How much time between each UDP send
	RakNetTimeMS TIME_BETWEEN_PUNCH_ATTEMPTS_INTERNAL;
	RakNetTimeMS TIME_BETWEEN_PUNCH_ATTEMPTS_EXTERNAL;

	/// How many tries for one port before giving up and going to the next port
	int UDP_SENDS_PER_PORT_INTERNAL;
	int UDP_SENDS_PER_PORT_EXTERNAL;

	/// After giving up on one internal port, how long to wait before trying the next port
	int INTERNAL_IP_WAIT_AFTER_ATTEMPTS;

	/// How many external ports to try past the last known starting port
	int MAX_PREDICTIVE_PORT_RANGE;

	/// After giving up on one external  port, how long to wait before trying the next port
	int EXTERNAL_IP_WAIT_BETWEEN_PORTS;

	/// After trying all external ports, how long to wait before returning ID_NAT_PUNCHTHROUGH_FAILED
	int EXTERNAL_IP_WAIT_AFTER_ALL_ATTEMPTS;

	/// Maximum number of internal IP address to try to connect to.
	/// Cannot be greater than MAXIMUM_NUMBER_OF_INTERNAL_IDS
	/// Should be high enough to try all internal IP addresses on the majority of computers
	int MAXIMUM_NUMBER_OF_INTERNAL_IDS_TO_CHECK;

	/// If the first punchthrough attempt fails, try again
	/// This sometimes works because the remote router was looking for an incoming message on a higher numbered port before responding to a lower numbered port from the other system
	bool retryOnFailure;
};

/// \ingroup NAT_PUNCHTHROUGH_GROUP
struct RAK_DLL_EXPORT NatPunchthroughDebugInterface
{
	NatPunchthroughDebugInterface() {}
	virtual ~NatPunchthroughDebugInterface() {}
	virtual void OnClientMessage(const char *msg)=0;
};

/// \ingroup NAT_PUNCHTHROUGH_GROUP
struct RAK_DLL_EXPORT NatPunchthroughDebugInterface_Printf : public NatPunchthroughDebugInterface
{
	virtual void OnClientMessage(const char *msg);
};

#if _RAKNET_SUPPORT_PacketLogger==1
/// \ingroup NAT_PUNCHTHROUGH_GROUP
struct RAK_DLL_EXPORT NatPunchthroughDebugInterface_PacketLogger : public NatPunchthroughDebugInterface
{
	// Set to non-zero to write to the packetlogger!
	PacketLogger *pl;

	NatPunchthroughDebugInterface_PacketLogger() {pl=0;}
	~NatPunchthroughDebugInterface_PacketLogger() {}
	virtual void OnClientMessage(const char *msg);
};
#endif

/// \brief Client code for NATPunchthrough
/// \details Maintain connection to NatPunchthroughServer to process incoming connection attempts through NatPunchthroughClient<BR>
/// Client will send datagrams to port to estimate next port<BR>
/// Will simultaneously connect with another client once ports are estimated.
/// \sa NatTypeDetectionClient
/// See also http://www.jenkinssoftware.com/raknet/manual/natpunchthrough.html
/// \ingroup NAT_PUNCHTHROUGH_GROUP
class RAK_DLL_EXPORT NatPunchthroughClient : public PluginInterface2
{
public:
	NatPunchthroughClient();
	~NatPunchthroughClient();

	/// Punchthrough a NAT. Doesn't connect, just tries to setup the routing table
	bool OpenNAT(RakNetGUID destination, SystemAddress facilitator);

	/// Modify the system configuration if desired
	/// Don't modify the variables in the structure while punchthrough is in progress
	PunchthroughConfiguration* GetPunchthroughConfiguration(void);

	/// Sets a callback to be called with debug messages
	/// \param[in] i Pointer to an interface. The pointer is stored, so don't delete it while in progress. Pass 0 to clear.
	void SetDebugInterface(NatPunchthroughDebugInterface *i);

	/// Returns the port on the router that incoming messages will be sent to
	/// UPNP needs to know this (See UPNP project)
	/// \pre Must have connected to the facilitator first
	/// \return Port that incoming messages will be sent to, from other clients. This probably won't be the same port RakNet was started on.
	unsigned short GetUPNPExternalPort(void) const;

	/// Returns our internal port that RakNet was started on
	/// Equivalent to calling rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS).port
	/// \return Port that incoming messages will arrive on, on our actual system.
	unsigned short GetUPNPInternalPort(void) const;

	/// Returns our locally bound system address
	/// Equivalent to calling rakPeer->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS).ToString(false);
	/// \return Internal address that UPNP should forward messages to
	RakNet::RakString GetUPNPInternalAddress(void) const;

	/// \internal For plugin handling
	virtual void Update(void);

	/// \internal For plugin handling
	virtual PluginReceiveResult OnReceive(Packet *packet);

	/// \internal For plugin handling
	virtual void OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming);

	/// \internal For plugin handling
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );

	virtual void OnAttach(void);
	virtual void OnDetach(void);
	virtual void OnRakPeerShutdown(void);
	void Clear(void);

protected:
	unsigned short mostRecentNewExternalPort;
	void OnGetMostRecentPort(Packet *packet);
	void OnConnectAtTime(Packet *packet);
	unsigned int GetPendingOpenNATIndex(RakNetGUID destination, SystemAddress facilitator);
	void SendPunchthrough(RakNetGUID destination, SystemAddress facilitator);
	void SendTTL(SystemAddress sa);
	void SendOutOfBand(SystemAddress sa, MessageID oobId);
	void OnPunchthroughFailure(void);
	void OnReadyForNextPunchthrough(void);
	void PushFailure(void);
	bool RemoveFromFailureQueue(void);
	void PushSuccess(void);
	//void ProcessNextPunchthroughQueue(void);

	/*
	struct PendingOpenNAT
	{
		RakNetGUID destination;
		SystemAddress facilitator;
	};
	DataStructures::List<PendingOpenNAT> pendingOpenNAT;
	*/

	struct SendPing
	{
		RakNetTime nextActionTime;
		SystemAddress targetAddress;
		SystemAddress facilitator;
		SystemAddress internalIds[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
		RakNetGUID targetGuid;
		bool weAreSender;
		int attemptCount;
		int retryCount;
		int punchingFixedPortAttempts; // only used for TestMode::PUNCHING_FIXED_PORT
		uint16_t sessionId;
		// Give priority to internal IP addresses because if we are on a LAN, we don't want to try to connect through the internet
		enum TestMode
		{
			TESTING_INTERNAL_IPS,
			WAITING_FOR_INTERNAL_IPS_RESPONSE,
			TESTING_EXTERNAL_IPS_FROM_FACILITATOR_PORT,
			TESTING_EXTERNAL_IPS_FROM_1024,
			WAITING_AFTER_ALL_ATTEMPTS,

			// The trendnet remaps the remote port to 1024.
			// If you continue punching on a different port for the same IP it bans you and the communication becomes unidirectioal
			PUNCHING_FIXED_PORT,

			// try port 1024-1028
		} testMode;
	} sp;

	PunchthroughConfiguration pc;
	NatPunchthroughDebugInterface *natPunchthroughDebugInterface;

	// The first time we fail a NAT attempt, we add it to failedAttemptList and try again, since sometimes trying again later fixes the problem
	// The second time we fail, we return ID_NAT_PUNCHTHROUGH_FAILED
	struct AddrAndGuid
	{
		SystemAddress addr;
		RakNetGUID guid;
	};
	DataStructures::List<AddrAndGuid> failedAttemptList;
};

#endif

#endif // _RAKNET_SUPPORT_*
