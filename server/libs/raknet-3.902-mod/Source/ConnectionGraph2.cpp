#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_ConnectionGraph2==1

#include "ConnectionGraph2.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"

int ConnectionGraph2::RemoteSystemComp( const RakNetGUID &key, RemoteSystem * const &data )
{
	if (key < data->guid)
		return -1;
	if (key > data->guid)
		return 1;
	return 0;
}

int ConnectionGraph2::SystemAddressAndGuidComp( const SystemAddressAndGuid &key, const SystemAddressAndGuid &data )
{
	if (key.guid<data.guid)
		return -1;
	if (key.guid>data.guid)
		return 1;
	return 0;
}

bool ConnectionGraph2::GetConnectionListForRemoteSystem(RakNetGUID remoteSystemGuid, SystemAddress *saOut, RakNetGUID *guidOut, unsigned int *outLength)
{
	if ((saOut==0 && guidOut==0) || outLength==0 || *outLength==0 || remoteSystemGuid==UNASSIGNED_RAKNET_GUID)
	{
		*outLength=0;
		return false;
	}

	bool objectExists;
	unsigned int idx = remoteSystems.GetIndexFromKey(remoteSystemGuid, &objectExists);
	if (objectExists==false)
	{
		*outLength=0;
		return false;
	}

	unsigned int idx2;
	if (remoteSystems[idx]->remoteConnections.Size() < *outLength)
		*outLength=remoteSystems[idx]->remoteConnections.Size();
	for (idx2=0; idx2 < *outLength; idx2++)
	{
		if (guidOut)
			guidOut[idx2]=remoteSystems[idx]->remoteConnections[idx2].guid;
		if (saOut)
			saOut[idx2]=remoteSystems[idx]->remoteConnections[idx2].systemAddress;
	}
	return true;
}
bool ConnectionGraph2::ConnectionExists(RakNetGUID g1, RakNetGUID g2)
{
	if (g1==g2)
		return false;

	bool objectExists;
	unsigned int idx = remoteSystems.GetIndexFromKey(g1, &objectExists);
	if (objectExists==false)
	{
		return false;
	}
	SystemAddressAndGuid sag;
	sag.guid=g2;
	return remoteSystems[idx]->remoteConnections.HasData(sag);
}
void ConnectionGraph2::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	// Send notice to all existing connections
	RakNet::BitStream bs;
	if (lostConnectionReason==LCR_CONNECTION_LOST)
		bs.Write((MessageID)ID_REMOTE_CONNECTION_LOST);
	else
		bs.Write((MessageID)ID_REMOTE_DISCONNECTION_NOTIFICATION);
	bs.Write(systemAddress);
	bs.Write(rakNetGUID);
	SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,true);

	bool objectExists;
	unsigned int idx = remoteSystems.GetIndexFromKey(rakNetGUID, &objectExists);
	if (objectExists)
	{
		RakNet::OP_DELETE(remoteSystems[idx],__FILE__,__LINE__);
		remoteSystems.RemoveAtIndex(idx);
	}
}
void ConnectionGraph2::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) isIncoming;

	// Send all existing systems to new connection
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_REMOTE_NEW_INCOMING_CONNECTION);
	bs.Write((uint32_t)1);
	bs.Write(systemAddress);
	bs.Write(rakNetGUID);
	SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,true);

	// Send everyone to the new guy
	DataStructures::List<SystemAddress> addresses;
	DataStructures::List<RakNetGUID> guids;
	rakPeerInterface->GetSystemList(addresses, guids);
	bs.Reset();
	bs.Write((MessageID)ID_REMOTE_NEW_INCOMING_CONNECTION);
	BitSize_t writeOffset = bs.GetWriteOffset();
	bs.Write((uint32_t) addresses.Size());

	unsigned int i;
	uint32_t count=0;
	for (i=0; i < addresses.Size(); i++)
	{
		if (addresses[i]==systemAddress)
			continue;

		bs.Write(addresses[i]);
		bs.Write(guids[i]);
		count++;
	}

	if (count>0)
	{
		BitSize_t writeOffset2 = bs.GetWriteOffset();
		bs.SetWriteOffset(writeOffset);
		bs.Write(count);
		bs.SetWriteOffset(writeOffset2);
		SendUnified(&bs,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,false);
	}

	bool objectExists;
	unsigned int ii = remoteSystems.GetIndexFromKey(rakNetGUID, &objectExists);
	if (objectExists==false)
	{
		RemoteSystem* remoteSystem = RakNet::OP_NEW<RemoteSystem>(__FILE__,__LINE__);
		remoteSystem->guid=rakNetGUID;
		remoteSystems.InsertAtIndex(remoteSystem,ii,__FILE__,__LINE__);
	}
}
PluginReceiveResult ConnectionGraph2::OnReceive(Packet *packet)
{
	if (packet->data[0]==ID_REMOTE_CONNECTION_LOST || packet->data[0]==ID_REMOTE_DISCONNECTION_NOTIFICATION)
	{
		bool objectExists;
		unsigned idx = remoteSystems.GetIndexFromKey(packet->guid, &objectExists);
		if (objectExists)
		{
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(1);
			SystemAddressAndGuid saag;
			bs.Read(saag.systemAddress);
			bs.Read(saag.guid);
			remoteSystems[idx]->remoteConnections.Remove(saag);
		}
	}
	else if (packet->data[0]==ID_REMOTE_NEW_INCOMING_CONNECTION)
	{
		bool objectExists;
		unsigned idx = remoteSystems.GetIndexFromKey(packet->guid, &objectExists);
		if (objectExists)
		{
			uint32_t numAddresses;
			RakNet::BitStream bs(packet->data,packet->length,false);
			bs.IgnoreBytes(1);
			bs.Read(numAddresses);
			for (unsigned int idx2=0; idx2 < numAddresses; idx2++)
			{
				SystemAddressAndGuid saag;
				bs.Read(saag.systemAddress);
				bs.Read(saag.guid);
				bool objectExists;
				unsigned int ii = remoteSystems[idx]->remoteConnections.GetIndexFromKey(saag, &objectExists);
				if (objectExists==false)
					remoteSystems[idx]->remoteConnections.InsertAtIndex(saag,ii,__FILE__,__LINE__);
			}
		}		
	}
	
	return RR_CONTINUE_PROCESSING;
}

#endif // _RAKNET_SUPPORT_*
