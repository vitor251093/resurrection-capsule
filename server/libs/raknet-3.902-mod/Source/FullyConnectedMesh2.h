/// \file FullyConnectedMesh2.h
/// \brief Fully connected mesh plugin, revision 2.  
/// \details This will connect RakPeer to all connecting peers, and all peers the connecting peer knows about.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_FullyConnectedMesh2==1

#ifndef __FULLY_CONNECTED_MESH_2_H
#define __FULLY_CONNECTED_MESH_2_H

class RakPeerInterface;
#include "PluginInterface2.h"
#include "RakMemoryOverride.h"
#include "DS_Multilist.h"
#include "NativeTypes.h"
#include "DS_List.h"
#include "RakString.h"

typedef int64_t FCM2Guid;

/// \brief Fully connected mesh plugin, revision 2
/// \details This will connect RakPeer to all connecting peers, and all peers the connecting peer knows about.<BR>
/// It will also calculate which system has been running longest, to find out who should be host, if you need one system to act as a host
/// \pre You must also install the ConnectionGraph2 plugin
/// \ingroup FULLY_CONNECTED_MESH_GROUP
class RAK_DLL_EXPORT FullyConnectedMesh2 : public PluginInterface2
{
public:
	FullyConnectedMesh2();
	virtual ~FullyConnectedMesh2();

	/// When the message ID_REMOTE_NEW_INCOMING_CONNECTION arrives, we try to connect to that system
	/// \param[in] attemptConnection If true, we try to connect to any systems we are notified about with ID_REMOTE_NEW_INCOMING_CONNECTION, which comes from the ConnectionGraph2 plugin. Defaults to true.
	/// \param[in] pw The password to use to connect with. Only used if \a attemptConnection is true
	void SetConnectOnNewRemoteConnection(bool attemptConnection, RakNet::RakString pw);

	/// \brief The connected host is whichever system we are connected to that has been running the longest.
	/// \details Will return UNASSIGNED_RAKNET_GUID if we are not connected to anyone, or if we are connected and are calculating the host
	/// If includeCalculating is true, will return the estimated calculated host as long as the calculation is nearly complete
	/// includeCalculating should be true if you are taking action based on another system becoming host, because not all host calculations may compelte at the exact same time
	/// \return System address of whichever system is host. 
	RakNetGUID GetConnectedHost(void) const;
	SystemAddress GetConnectedHostAddr(void) const;

	/// \return System address of whichever system is host. Always returns something, even though it may be our own system.
	RakNetGUID GetHostSystem(void) const;

	/// \return If our system is host
	bool IsHostSystem(void) const;

	/// \param[in] includeCalculating If true, and we are currently calculating a new host, return the new host if the calculation is nearly complete
	/// \return If our system is host
	bool IsConnectedHost(void) const;

	/// \brief Automatically add new connections to the fully connected mesh.
	/// \details Defaults to true.
	/// \param[in] b As stated
	void SetAutoparticipateConnections(bool b);

	/// Clear our own host order, and recalculate as if we had just reconnected
	void ResetHostCalculation(void);

	/// \brief if SetAutoparticipateConnections() is called with false, then you need to use AddParticipant before these systems will be added to the mesh 
	/// \param[in] participant The new participant
	void AddParticipant(RakNetGUID rakNetGuid);

	unsigned int GetParticipantCount(void) const;
	void GetParticipantCount(DataStructures::DefaultIndexType *participantListSize) const;
	/// \internal
	RakNetTimeUS GetElapsedRuntime(void);

	/// \internal
	virtual PluginReceiveResult OnReceive(Packet *packet);
	/// \internal
	virtual void OnRakPeerStartup(void);
	/// \internal
	virtual void OnAttach(void);
	/// \internal
	virtual void OnRakPeerShutdown(void);
	/// \internal
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );
	/// \internal
	virtual void OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming);

	/// \internal
	struct FCM2Participant
	{
		FCM2Participant() {}
		FCM2Participant(const FCM2Guid &_fcm2Guid, const RakNetGUID &_rakNetGuid) : fcm2Guid(_fcm2Guid), rakNetGuid(_rakNetGuid) {}

		// Low half is a random number.
		// High half is the order we connected in (totalConnectionCount)
		FCM2Guid fcm2Guid;
		RakNetGUID rakNetGuid;
	};

protected:
	void Clear(void);
	void PushNewHost(const RakNetGUID &guid);
	void SendOurFCMGuid(SystemAddress addr);
	void SendFCMGuidRequest(RakNetGUID rakNetGuid);
	void SendConnectionCountResponse(SystemAddress addr, unsigned int responseTotalConnectionCount);
	void OnRequestFCMGuid(Packet *packet);
	void OnRespondConnectionCount(Packet *packet);
	void OnInformFCMGuid(Packet *packet);
	void AssignOurFCMGuid(void);
	void CalculateHost(RakNetGUID *rakNetGuid, FCM2Guid *fcm2Guid);
	bool AddParticipantInternal( RakNetGUID rakNetGuid, FCM2Guid theirFCMGuid );
	void CalculateAndPushHost(void);
	bool ParticipantListComplete(void);
	void IncrementTotalConnectionCount(unsigned int i);

	// Used to track how long RakNet has been running. This is so we know who has been running longest
	RakNetTimeUS startupTime;

	// Option for SetAutoparticipateConnections
	bool autoParticipateConnections;

	// totalConnectionCount is roughly maintained across all systems, and increments by 1 each time a new system connects to the mesh
	// It is always kept at the highest known value
	// It is used as the high 4 bytes for new FCMGuids. This causes newer values of FCM2Guid to be higher than lower values. The lowest value is the host.
	unsigned int totalConnectionCount;

	// Our own ourFCMGuid. Starts at unassigned (0). Assigned once we send ID_FCM2_REQUEST_FCMGUID and get back ID_FCM2_RESPOND_CONNECTION_COUNT
	FCM2Guid ourFCMGuid;

	/// List of systems we know the FCM2Guid for
	DataStructures::List<FCM2Participant> participantList;

	RakNetGUID lastPushedHost;

	// Optimization: Store last calculated host in these variables.
	RakNetGUID hostRakNetGuid;
	FCM2Guid hostFCM2Guid;

	RakNet::RakString connectionPassword;
	bool connectOnNewRemoteConnections;
};

/*
Startup()
ourFCMGuid=unknown
totalConnectionCount=0
Set startupTime

AddParticipant()
if (sender by guid is a participant)
return;
AddParticipantInternal(guid);
if (ourFCMGuid==unknown)
Send to that system a request for their fcmGuid, totalConnectionCount. Inform startupTime.
else
Send to that system a request for their fcmGuid. Inform total connection count, our fcmGuid

OnRequestGuid()
if (sender by guid is not a participant)
{
	// They added us as a participant, but we didn't add them. This can be caused by lag where both participants are not added at the same time.
	// It doesn't affect the outcome as long as we still process the data
	AddParticipantInternal(guid);
}
if (ourFCMGuid==unknown)
{
	if (includedStartupTime)
	{
		// Nobody has a fcmGuid

		if (their startup time is greater than our startup time)
			ReplyConnectionCount(1);
		else
			ReplyConnectionCount(2);
	}
	else
	{
		// They have a fcmGuid, we do not

		SetMaxTotalConnectionCount(remoteCount);
		AssignTheirGuid()
		GenerateOurGuid();
		SendOurGuid(all);
	}
}
else
{
	if (includedStartupTime)
	{
		// We have a fcmGuid they do not

		ReplyConnectionCount(totalConnectionCount+1);
		SendOurGuid(sender);
	}
	else
	{
		// We both have fcmGuids

		SetMaxTotalConnectionCount(remoteCount);
		AssignTheirGuid();
		SendOurGuid(sender);
	}
}

OnReplyConnectionCount()
SetMaxTotalConnectionCount(remoteCount);
GenerateOurGuid();
SendOurGuid(allParticipants);

OnReceiveTheirGuid()
AssignTheirGuid()
*/

#endif

#endif // _RAKNET_SUPPORT_*
