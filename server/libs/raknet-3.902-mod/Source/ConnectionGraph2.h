/// \file ConnectionGraph2.h
/// \brief Connection graph plugin, version 2. Tells new systems about existing and new connections
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_ConnectionGraph2==1

#ifndef __CONNECTION_GRAPH_2_H
#define __CONNECTION_GRAPH_2_H

class RakPeerInterface;
#include "RakMemoryOverride.h"
#include "RakNetTypes.h"
#include "PluginInterface2.h"
#include "DS_List.h"
#include "DS_WeightedGraph.h"
#include "GetTime.h"
#include "Export.h"

/// \brief A one hop connection graph.
/// \details Sends ID_CONNECTION_GRAPH_DISCONNECTION_NOTIFICATION, ID_CONNECTION_GRAPH_CONNECTION_LOST, ID_CONNECTION_GRAPH_NEW_CONNECTION<BR>
/// All identifiers are followed by SystemAddress, then RakNetGUID
/// Also stores the list for you, which you can access with GetConnectionListForRemoteSystem 
/// \ingroup CONNECTION_GRAPH_GROUP
class RAK_DLL_EXPORT ConnectionGraph2 : public PluginInterface2
{
public:

	/// \brief Given a remote system identified by RakNetGUID, return the list of SystemAddresses and RakNetGUIDs they are connected to 
	/// \param[in] remoteSystemGuid Which system we are referring to. This only works for remote systems, not ourselves.
	/// \param[out] saOut A preallocated array to hold the output list of SystemAddress. Can be 0 if you don't care.
	/// \param[out] guidOut A preallocated array to hold the output list of RakNetGUID. Can be 0 if you don't care.
	/// \param[in,out] outLength On input, the size of \a saOut and \a guidOut. On output, modified to reflect the number of elements actually written
	/// \return True if \a remoteSystemGuid was found. Otherwise false, and \a saOut, \a guidOut remain unchanged. \a outLength will be set to 0.
	bool GetConnectionListForRemoteSystem(RakNetGUID remoteSystemGuid, SystemAddress *saOut, RakNetGUID *guidOut, unsigned int *outLength);

	/// Returns if g1 is connected to g2
	bool ConnectionExists(RakNetGUID g1, RakNetGUID g2);

	/// \internal
	struct SystemAddressAndGuid
	{
		SystemAddress systemAddress;
		RakNetGUID guid;
	};
	/// \internal
	static int SystemAddressAndGuidComp( const SystemAddressAndGuid &key, const SystemAddressAndGuid &data );

	/// \internal
	struct RemoteSystem
	{
		DataStructures::OrderedList<SystemAddressAndGuid,SystemAddressAndGuid,ConnectionGraph2::SystemAddressAndGuidComp> remoteConnections;
		RakNetGUID guid;
	};
	/// \internal
	static int RemoteSystemComp( const RakNetGUID &key, RemoteSystem * const &data );
	
protected:
	/// \internal
	virtual void OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason );
	/// \internal
	virtual void OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming);
	/// \internal
	virtual PluginReceiveResult OnReceive(Packet *packet);

	// List of systems I am connected to, which in turn stores which systems they are connected to
	DataStructures::OrderedList<RakNetGUID, RemoteSystem*, ConnectionGraph2::RemoteSystemComp> remoteSystems;

};

#endif

#endif // _RAKNET_SUPPORT_*
