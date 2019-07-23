#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_ReplicaManager3==1

#include "ReplicaManager3.h"
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "NetworkIDManager.h"

using namespace RakNet;

DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS(LastSerializationResult,Replica3*,replica);

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool PRO::operator==( const PRO& right ) const
{
	return priority == right.priority && reliability == right.reliability && orderingChannel == right.orderingChannel && sendReceipt == right.sendReceipt;
}

bool PRO::operator!=( const PRO& right ) const
{
	return priority != right.priority || reliability != right.reliability || orderingChannel != right.orderingChannel || sendReceipt != right.sendReceipt;
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

LastSerializationResult::LastSerializationResult()
{
	replica=0;
	lastSerializationResultBS=0;
}
LastSerializationResult::~LastSerializationResult()
{
	if (lastSerializationResultBS)
		RakNet::OP_DELETE(lastSerializationResultBS,__FILE__,__LINE__);
}
void LastSerializationResult::AllocBS(void)
{
	if (lastSerializationResultBS==0)
	{
		lastSerializationResultBS=RakNet::OP_NEW<LastSerializationResultBS>(__FILE__,__LINE__);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ReplicaManager3::ReplicaManager3()
{
	defaultSendParameters.orderingChannel=0;
	defaultSendParameters.priority=HIGH_PRIORITY;
	defaultSendParameters.reliability=RELIABLE_ORDERED;
	defaultSendParameters.sendReceipt=0;
	autoSerializeInterval=30;
	lastAutoSerializeOccurance=0;
	worldId=0;
	autoCreateConnections=true;
	autoDestroyConnections=true;
	networkIDManager=0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ReplicaManager3::~ReplicaManager3()
{
	if (autoDestroyConnections)
	{
		// Clear() calls DeallocConnection(), which is pure virtual and cannot be called from the destructor
		RakAssert(connectionList.GetSize()==0);
	}
	Clear();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetAutoManageConnections(bool autoCreate, bool autoDestroy)
{
	autoCreateConnections=autoCreate;
	autoDestroyConnections=autoDestroy;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool ReplicaManager3::PushConnection(RakNet::Connection_RM3 *newConnection)
{
	if (newConnection==0)
		return false;
	if (GetConnectionByGUID(newConnection->GetRakNetGUID()))
		return false;
	DataStructures::DefaultIndexType index = connectionList.GetInsertionIndex(newConnection);
	if (index!=(DataStructures::DefaultIndexType)-1)
	{
		connectionList.InsertAtIndex(newConnection,index,__FILE__,__LINE__);

		// Send message to validate the connection
		newConnection->SendValidation(rakPeerInterface, worldId);

		Connection_RM3::ConstructionMode constructionMode = newConnection->QueryConstructionMode();
		if (constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
		{
			DataStructures::DefaultIndexType pushIdx;
			for (pushIdx=0; pushIdx < userReplicaList.GetSize(); pushIdx++)
				newConnection->OnLocalReference(userReplicaList[pushIdx], this);
		}		
	}
	return true;
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNet::Connection_RM3 * ReplicaManager3::PopConnection(DataStructures::DefaultIndexType index)
{
	DataStructures::Multilist<ML_STACK, Replica3*> replicaList;
	DataStructures::Multilist<ML_STACK, Replica3*> destructionList;
	DataStructures::Multilist<ML_STACK, Replica3*> broadcastList;
	RakNet::Connection_RM3 *connection;
	DataStructures::DefaultIndexType index2;
	RM3ActionOnPopConnection action;

	connection=connectionList[index];

	// Clear out downloadGroup
	connection->ClearDownloadGroup(rakPeerInterface);

	RakNetGUID guid = connection->GetRakNetGUID();
	GetReplicasCreatedByGuid(guid, replicaList);

	for (index2=0; index2 < replicaList.GetSize(); index2++)
	{
		action = replicaList[index2]->QueryActionOnPopConnection(connection);
		replicaList[index2]->OnPoppedConnection(connection);
		if (action==RM3AOPC_DELETE_REPLICA)
		{
			destructionList.Push( replicaList[index2], __FILE__, __LINE__  );
		}
		else if (action==RM3AOPC_DELETE_REPLICA_AND_BROADCAST_DESTRUCTION)
		{
			destructionList.Push( replicaList[index2], __FILE__, __LINE__  );

			broadcastList.Push( replicaList[index2], __FILE__, __LINE__  );
		}
	}

	BroadcastDestructionList(broadcastList, connection->GetSystemAddress());
	for (index2=0; index2 < destructionList.GetSize(); index2++)
	{
		destructionList[index2]->PreDestruction(connection);
		destructionList[index2]->DeallocReplica(connection);
	}

	connectionList.RemoveAtIndex(index,__FILE__,__LINE__);
	return connection;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNet::Connection_RM3 * ReplicaManager3::PopConnection(RakNetGUID guid)
{
	DataStructures::DefaultIndexType index;

	for (index=0; index < connectionList.GetSize(); index++)
	{
		if (connectionList[index]->GetRakNetGUID()==guid)
		{
			return PopConnection(index);
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Reference(RakNet::Replica3 *replica3)
{
	DataStructures::DefaultIndexType index = ReferenceInternal(replica3);

	if (index!=(DataStructures::DefaultIndexType)-1)
	{
		DataStructures::DefaultIndexType pushIdx;
		for (pushIdx=0; pushIdx < connectionList.GetSize(); pushIdx++)
		{
			Connection_RM3::ConstructionMode constructionMode = connectionList[pushIdx]->QueryConstructionMode();
			if (constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==Connection_RM3::QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
			{
				connectionList[pushIdx]->OnLocalReference(replica3, this);
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DataStructures::DefaultIndexType ReplicaManager3::ReferenceInternal(RakNet::Replica3 *replica3)
{
	DataStructures::DefaultIndexType index;
	index = userReplicaList.GetInsertionIndex(replica3);
	if (index!=(DataStructures::DefaultIndexType)-1)
	{
		if (networkIDManager==0)
			networkIDManager=rakPeerInterface->GetNetworkIDManager();
		RakAssert(networkIDManager);
		replica3->SetNetworkIDManager(networkIDManager);
		if (replica3->creatingSystemGUID==UNASSIGNED_RAKNET_GUID)
			replica3->creatingSystemGUID=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
		replica3->replicaManager=this;
		userReplicaList.InsertAtIndex(replica3,index,__FILE__,__LINE__);
	}
	return index;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Dereference(RakNet::Replica3 *replica3)
{
	DataStructures::DefaultIndexType index, index2;
	for (index=0; index < userReplicaList.GetSize(); index++)
	{
		if (userReplicaList[index]==replica3)
		{
			userReplicaList.RemoveAtIndex(index,__FILE__,__LINE__);
			break;
		}
	}

	// Remove from all connections
	for (index2=0; index2 < connectionList.GetSize(); index2++)
	{
		connectionList[index2]->OnDereference(replica3, this);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::DereferenceList(DataStructures::Multilist<ML_STACK, Replica3*> &replicaListIn)
{
	DataStructures::DefaultIndexType index;
	for (index=0; index < replicaListIn.GetSize(); index++)
		Dereference(replicaListIn[index]);
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReplicasCreatedByMe(DataStructures::Multilist<ML_STACK, Replica3*> &replicaListOut)
{
	RakNetGUID myGuid = rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	GetReplicasCreatedByGuid(rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS), replicaListOut);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReferencedReplicaList(DataStructures::Multilist<ML_STACK, Replica3*> &replicaListOut)
{
	replicaListOut=userReplicaList;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetReplicasCreatedByGuid(RakNetGUID guid, DataStructures::Multilist<ML_STACK, Replica3*> &replicaListOut)
{
	replicaListOut.Clear(false,__FILE__,__LINE__);
	DataStructures::DefaultIndexType index;
	for (index=0; index < userReplicaList.GetSize(); index++)
	{
		if (userReplicaList[index]->creatingSystemGUID==guid)
			replicaListOut.Push(userReplicaList[index],__FILE__,__LINE__);
	}
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned ReplicaManager3::GetReplicaCount(void) const
{
	return userReplicaList.GetSize();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3 *ReplicaManager3::GetReplicaAtIndex(unsigned index)
{
	return userReplicaList[index];
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

DataStructures::DefaultIndexType ReplicaManager3::GetConnectionCount(void) const
{
	return connectionList.GetSize();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionAtIndex(unsigned index) const
{
	return connectionList[index];
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionBySystemAddress(SystemAddress sa) const
{
	DataStructures::DefaultIndexType index;
	for (index=0; index < connectionList.GetSize(); index++)
	{
		if (connectionList[index]->GetSystemAddress()==sa)
		{
			return connectionList[index];
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3* ReplicaManager3::GetConnectionByGUID(RakNetGUID guid) const
{
	DataStructures::DefaultIndexType index;
	for (index=0; index < connectionList.GetSize(); index++)
	{
		if (connectionList[index]->GetRakNetGUID()==guid)
		{
			return connectionList[index];
		}
	}
	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultOrderingChannel(char def)
{
	defaultSendParameters.orderingChannel=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultPacketPriority(PacketPriority def)
{
	defaultSendParameters.priority=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetDefaultPacketReliability(PacketReliability def)
{
	defaultSendParameters.reliability=def;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetAutoSerializeInterval(RakNetTime intervalMS)
{
	autoSerializeInterval=intervalMS;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::GetConnectionsThatHaveReplicaConstructed(Replica3 *replica, DataStructures::Multilist<ML_STACK, Connection_RM3*> &connectionsThatHaveConstructedThisReplica)
{
	connectionsThatHaveConstructedThisReplica.Clear(false,__FILE__,__LINE__);
	DataStructures::DefaultIndexType index;
	for (index=0; index < connectionList.GetSize(); index++)
	{
		if (connectionList[index]->HasReplicaConstructed(replica))
			connectionsThatHaveConstructedThisReplica.Push(connectionList[index],__FILE__,__LINE__);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::Clear(void)
{
	if (autoDestroyConnections)
	{
		for (DataStructures::DefaultIndexType i=0; i < connectionList.GetSize(); i++)
			DeallocConnection(connectionList[i]);
	}
	else
	{
		// Clear out downloadGroup even if not auto destroying the connection, since the packets need to go back to RakPeer
		for (DataStructures::DefaultIndexType i=0; i < connectionList.GetSize(); i++)
			connectionList[i]->ClearDownloadGroup(rakPeerInterface);
	}



	connectionList.Clear(true,__FILE__,__LINE__);
	userReplicaList.Clear(true,__FILE__,__LINE__);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PRO ReplicaManager3::GetDefaultSendParameters(void) const
{
	return defaultSendParameters;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetWorldID(unsigned char id)
{
	worldId=id;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned char ReplicaManager3::GetWorldID(void) const
{
	return worldId;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

NetworkIDManager *ReplicaManager3::GetNetworkIDManager(void) const
{
	return networkIDManager;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::SetNetworkIDManager(NetworkIDManager *_networkIDManager)
{
	networkIDManager=_networkIDManager;
	if (networkIDManager)
		networkIDManager->SetGuid(rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnReceive(Packet *packet)
{
	if (packet->length<2)
		return RR_CONTINUE_PROCESSING;

	unsigned char incomingWorldId;

	RakNetTime timestamp=0;
	unsigned char packetIdentifier, packetDataOffset;
	if ( ( unsigned char ) packet->data[ 0 ] == ID_TIMESTAMP )
	{
		if ( packet->length > sizeof( unsigned char ) + sizeof( RakNetTime ) )
		{
			packetIdentifier = ( unsigned char ) packet->data[ sizeof( unsigned char ) + sizeof( RakNetTime ) ];
			// Required for proper endian swapping
			RakNet::BitStream tsBs(packet->data+sizeof(MessageID),packet->length-1,false);
			tsBs.Read(timestamp);
			incomingWorldId=packet->data[sizeof( unsigned char )*2 + sizeof( RakNetTime )];
			packetDataOffset=sizeof( unsigned char )*3 + sizeof( RakNetTime );
		}
		else
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	else
	{
		packetIdentifier = ( unsigned char ) packet->data[ 0 ];
		incomingWorldId=packet->data[sizeof( unsigned char )];
		packetDataOffset=sizeof( unsigned char )*2;
	}

	switch (packetIdentifier)
	{
	case ID_REPLICA_MANAGER_CONSTRUCTION:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnConstruction(packet, packet->data, packet->length, packet->guid, packetDataOffset);
	case ID_REPLICA_MANAGER_SERIALIZE:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnSerialize(packet, packet->data, packet->length, packet->guid, timestamp, packetDataOffset);
	case ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_REJECTED:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		OnLocalConstructionRejected(packet->data, packet->length, packet->guid, packetDataOffset);
		break;
	case ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_ACCEPTED:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		OnLocalConstructionAccepted(packet->data, packet->length, packet->guid, packetDataOffset);
		break;
	case ID_REPLICA_MANAGER_DOWNLOAD_STARTED:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnDownloadStarted(packet, packet->data, packet->length, packet->guid, packetDataOffset);
	case ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		return OnDownloadComplete(packet, packet->data, packet->length, packet->guid, packetDataOffset);
	case ID_REPLICA_MANAGER_3_SERIALIZE_CONSTRUCTION_EXISTING:
		if (incomingWorldId!=worldId)
			return RR_CONTINUE_PROCESSING;
		OnConstructionExisting(packet->data, packet->length, packet->guid, packetDataOffset);
		break;
	case ID_REPLICA_MANAGER_SCOPE_CHANGE:
		{
			if (incomingWorldId!=worldId)
				return RR_CONTINUE_PROCESSING;

			Connection_RM3 *connection = GetConnectionByGUID(packet->guid);
			if (connection && connection->isValidated==false)
			{
				// This connection is now confirmed bidirectional
				connection->isValidated=true;
				// Reply back on validation
				connection->SendValidation(rakPeerInterface,worldId);
			}
		}
	}

	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::AutoConstructByQuery(ReplicaManager3 *replicaManager3)
{
	ValidateLists(replicaManager3);

	ConstructionMode constructionMode = QueryConstructionMode();

	DataStructures::DefaultIndexType index;
	RM3ConstructionState constructionState;
	LastSerializationResult *lsr;
	index=0;

	constructedReplicasCulled.Clear(false,__FILE__,__LINE__);
	destroyedReplicasCulled.Clear(false,__FILE__,__LINE__);

	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		while (index < queryToConstructReplicaList.GetSize())
		{
			lsr=queryToConstructReplicaList[index];
			constructionState=lsr->replica->QueryConstruction(this, replicaManager3);
			if (constructionState==RM3CS_ALREADY_EXISTS_REMOTELY)
			{
				OnReplicaAlreadyExists(index, replicaManager3);

				// Serialize construction data to this connection
				RakNet::BitStream bsOut;
				bsOut.Write((MessageID)ID_REPLICA_MANAGER_3_SERIALIZE_CONSTRUCTION_EXISTING);
				bsOut.Write(replicaManager3->GetWorldID());
				NetworkID networkId;
				networkId=lsr->replica->GetNetworkID();
				bsOut.Write(networkId);
				BitSize_t bitsWritten = bsOut.GetNumberOfBitsUsed();
				lsr->replica->SerializeConstructionExisting(&bsOut, this);
				if (bsOut.GetNumberOfBitsUsed()!=bitsWritten)
					replicaManager3->SendUnified(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,GetSystemAddress(), false);

				// Serialize first serialization to this connection.
				// This is done here, as it isn't done in PushConstruction
				SerializeParameters sp;
				RakNet::BitStream emptyBs;
				for (index=0; index < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; index++)
				{
					sp.lastSentBitstream[index]=&emptyBs;
					sp.pro[index]=replicaManager3->GetDefaultSendParameters();
				}
				sp.bitsWrittenSoFar=0;
				sp.destinationConnection=this;
				sp.messageTimestamp=0;
				sp.whenLastSerialized=0;

				RakNet::Replica3 *replica = lsr->replica;

				RM3SerializationResult res = replica->Serialize(&sp);
				if (res!=RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION &&
					res!=RM3SR_DO_NOT_SERIALIZE &&
					res!=RM3SR_SERIALIZED_UNIQUELY)
				{
					bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
					for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
					{
						sp.bitsWrittenSoFar+=sp.outputBitstream[z].GetNumberOfBitsUsed();
						allIndices[z]=true;
					}
					if (SendSerialize(replica, allIndices, sp.outputBitstream, sp.messageTimestamp, sp.pro, replicaManager3->GetRakPeerInterface(), replicaManager3->GetWorldID())==SSICR_SENT_DATA)
						lsr->replica->whenLastSerialized=RakNet::GetTime();
				}
			}
			else if (constructionState==RM3CS_SEND_CONSTRUCTION)
			{
				OnConstructToThisConnection(index, replicaManager3);
				constructedReplicasCulled.Push(lsr->replica,lsr->replica,__FILE__,__LINE__);
			}
			else if (constructionState==RM3CS_NEVER_CONSTRUCT)
			{
				OnNeverConstruct(index, replicaManager3);
			}
			else//  if (constructionState==RM3CS_NO_ACTION)
			{
				// Do nothing
				index++;
			}
		}

		if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
		{
			RM3DestructionState destructionState;
			index=0;
			while (index < queryToDestructReplicaList.GetSize())
			{
				lsr=queryToDestructReplicaList[index];
				destructionState=lsr->replica->QueryDestruction(this, replicaManager3);
				if (destructionState==RM3DS_SEND_DESTRUCTION)
				{
					OnSendDestructionFromQuery(index, replicaManager3);
					destroyedReplicasCulled.Push(lsr->replica,lsr->replica,__FILE__,__LINE__);
				}
				else if (destructionState==RM3DS_DO_NOT_QUERY_DESTRUCTION)
				{
					OnDoNotQueryDestruction(index, replicaManager3);
				}
				else//  if (destructionState==RM3CS_NO_ACTION)
				{
					// Do nothing
					index++;
				}
			}
		}
	}
	else if (constructionMode==QUERY_CONNECTION_FOR_REPLICA_LIST)
	{
		QueryReplicaList(constructedReplicasCulled,destroyedReplicasCulled);

		DataStructures::DefaultIndexType idx1, idx2;
#ifdef _DEBUG
		// The user should not construct a replica that already exists
		for (idx2=0; idx2 < constructedReplicasCulled.GetSize(); idx2++)
		{
			RakAssert(constructedReplicaList.GetIndexOf(constructedReplicasCulled[idx2])==(DataStructures::DefaultIndexType)-1);
		}

#endif

		// Create new
		for (idx2=0; idx2 < constructedReplicasCulled.GetSize(); idx2++)
			OnConstructToThisConnection(constructedReplicasCulled[idx2], replicaManager3);

		bool exists;
		for (idx2=0; idx2 < destroyedReplicasCulled.GetSize(); idx2++)
		{
			exists=false;
			idx1=constructedReplicaList.GetIndexOf(destroyedReplicasCulled[idx2]);
			RakAssert(idx1!=(DataStructures::DefaultIndexType)-1);
			if (idx1!=(DataStructures::DefaultIndexType)-1)
			{
				OnSendDestructionFromQuery(idx1,replicaManager3);
			}

			// If this assert hits, the user tried to destroy a replica that doesn't exist on the remote system
			RakAssert(exists);
		}
	}

	SendConstruction(constructedReplicasCulled,destroyedReplicasCulled,replicaManager3->defaultSendParameters,replicaManager3->rakPeerInterface,replicaManager3->worldId);

}
void ReplicaManager3::Update(void)
{
	DataStructures::DefaultIndexType index,index2;

	for (index=0; index < connectionList.GetSize(); index++)
	{
		if (connectionList[index]->isValidated==false)
			continue;
		connectionList[index]->AutoConstructByQuery(this);
	}

	if (autoSerializeInterval>0)
	{
		RakNetTime time = RakNet::GetTime();

		if (time - lastAutoSerializeOccurance > autoSerializeInterval)
		{
			for (index=0; index < userReplicaList.GetSize(); index++)
			{
				userReplicaList[index]->forceSendUntilNextUpdate=false;
				userReplicaList[index]->OnUserReplicaPreSerializeTick();
			}


			DataStructures::DefaultIndexType index;
			SerializeParameters sp;
			sp.curTime=time;
			Connection_RM3 *connection;
			SendSerializeIfChangedResult ssicr;
			sp.messageTimestamp=0;
			for (int i=0; i < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; i++)
				sp.pro[i]=defaultSendParameters;
			index2=0;
			for (index=0; index < connectionList.GetSize(); index++)
			{
				connection = connectionList[index];
				sp.bitsWrittenSoFar=0;
				index2=0;
				while (index2 < connection->queryToSerializeReplicaList.GetSize())
				{
					sp.destinationConnection=connection;
					sp.whenLastSerialized=connection->queryToSerializeReplicaList[index2]->replica->whenLastSerialized;
					ssicr=connection->SendSerializeIfChanged(index2, &sp, GetRakPeerInterface(), GetWorldID(), this);
					if (ssicr==SSICR_SENT_DATA)
					{
						connection->queryToSerializeReplicaList[index2]->replica->whenLastSerialized=time;
						index2++;
					}
					else if (ssicr==SSICR_NEVER_SERIALIZE)
					{
						// Removed from the middle of the list
					}
					else
						index2++;
				}
			}

			lastAutoSerializeOccurance=time;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) lostConnectionReason;
	(void) systemAddress;
	if (autoDestroyConnections)
	{
		Connection_RM3 *connection = PopConnection(rakNetGUID);
		if (connection)
			DeallocConnection(connection);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) isIncoming;
	if (autoCreateConnections)
	{
		Connection_RM3 *connection = AllocConnection(systemAddress, rakNetGUID);
		if (connection)
			PushConnection(connection);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnRakPeerShutdown(void)
{
	if (autoDestroyConnections)
	{
		while (connectionList.GetSize())
		{
			Connection_RM3 *connection = PopConnection(connectionList.GetSize()-1);
			if (connection)
				DeallocConnection(connection);
		}
	}

	Clear();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnDetach(void)
{
	OnRakPeerShutdown();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnConstructionExisting(unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
	{
		// Almost certainly a bug
		RakAssert("Got OnConstruction but no connection yet" && 0);
		return;
	}

	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);

	if (networkIDManager==0)
		networkIDManager=rakPeerInterface->GetNetworkIDManager();
	RakAssert(networkIDManager);

	NetworkID networkId;
	bsIn.Read(networkId);
	Replica3* existingReplica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
	if (existingReplica)
	{
		existingReplica->DeserializeConstructionExisting(&bsIn, connection);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnConstruction(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
	{
		// Almost certainly a bug
		RakAssert("Got OnConstruction but no connection yet" && 0);
		return RR_CONTINUE_PROCESSING;
	}
	if (connection->groupConstructionAndSerialize)
	{
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}

	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	DataStructures::DefaultIndexType objectListSize, index, index2;
	BitSize_t bitOffset;
	Replica3 *replica;
	uint32_t allocationNumber=0;
	NetworkID networkId;
	RakNetGUID creatingSystemGuid;

	if (networkIDManager==0)
		networkIDManager=rakPeerInterface->GetNetworkIDManager();
	RakAssert(networkIDManager);

	bsIn.Read(objectListSize);
	for (index=0; index < objectListSize; index++)
	{
		bsIn.Read(bitOffset);
		bsIn.Read(networkId);
		Replica3* existingReplica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
		if (existingReplica)
		{
			existingReplica->replicaManager=this;

			// Network ID already in use
			connection->OnDownloadExisting(existingReplica, this);

			bsIn.SetReadOffset(bitOffset);
			continue;
		}

		replica = connection->AllocReplica(&bsIn, this);
		if (replica==0)
		{
			bsIn.SetReadOffset(bitOffset);
			continue;
		}

		replica->SetNetworkIDManager(networkIDManager);

		if (networkId==UNASSIGNED_NETWORK_ID)
		{
			if (networkIDManager->IsNetworkIDAuthority()==false)
			{
				// Can't assign network ID
				replica->replicaManager=0;
				replica->DeallocReplica(connection);
				bsIn.SetReadOffset(bitOffset);
				continue;
			}

			bsIn.Read(allocationNumber);
		}
		else
		{

			replica->SetNetworkID(networkId);
		}

		replica->replicaManager=this;
		bsIn.Read(creatingSystemGuid);
		replica->creatingSystemGUID=creatingSystemGuid;

		if (!replica->QueryRemoteConstruction(connection) ||
			!replica->DeserializeConstruction(&bsIn, connection))
		{
			// Overtake this message to mean construction rejected
			if (networkId==UNASSIGNED_NETWORK_ID)
			{
				RakNet::BitStream bsOut;
				bsOut.Write((MessageID)ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_REJECTED);
				bsOut.Write(worldId);
				bsOut.Write(allocationNumber);
				replica->SerializeConstructionRequestRejected(&bsOut,connection);
				rakPeerInterface->Send(&bsOut, defaultSendParameters.priority, defaultSendParameters.reliability, defaultSendParameters.orderingChannel, connection->GetSystemAddress(), false, defaultSendParameters.sendReceipt);
			}

			replica->replicaManager=0;
			replica->DeallocReplica(connection);
			bsIn.SetReadOffset(bitOffset);
			continue;
		}

		bsIn.SetReadOffset(bitOffset);
		replica->PostDeserializeConstruction(connection);

		if (networkId==UNASSIGNED_NETWORK_ID)
		{
			// Overtake this message to mean construction accepted
			RakNet::BitStream bsOut;
			bsOut.Write((MessageID)ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_ACCEPTED);
			bsOut.Write(worldId);
			bsOut.Write(allocationNumber);
			bsOut.Write(replica->GetNetworkID());
			replica->SerializeConstructionRequestAccepted(&bsOut,connection);
			rakPeerInterface->Send(&bsOut, defaultSendParameters.priority, defaultSendParameters.reliability, defaultSendParameters.orderingChannel, connection->GetSystemAddress(), false, defaultSendParameters.sendReceipt);
		}
		bsIn.AlignReadToByteBoundary();

		// Register the replica
		ReferenceInternal(replica);

		// Tell the connection(s) that this object exists since they just sent it to us
		connection->OnDownloadFromThisSystem(replica, this);

		for (index2=0; index2 < connectionList.GetSize(); index2++)
		{
			if (connectionList[index2]!=connection)
				connectionList[index2]->OnDownloadFromOtherSystem(replica, this);
		}
	}

	// Destructions
	bool b = bsIn.Read(objectListSize);
	RakAssert(b);
	for (index=0; index < objectListSize; index++)
	{
		bsIn.Read(networkId);
		bsIn.Read(bitOffset);
		replica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
		if (replica==0)
		{
			// Unknown object
			bsIn.SetReadOffset(bitOffset);
			continue;
		}
		bsIn.Read(replica->deletingSystemGUID);
		if (replica->DeserializeDestruction(&bsIn,connection))
		{
			// Make sure it wasn't deleted in DeserializeDestruction
			if (networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId))
			{
				replica->PreDestruction(connection);

				// Forward deletion by remote system
				BroadcastDestruction(replica,connection->GetSystemAddress());
				Dereference(replica);
				replica->replicaManager=0; // Prevent BroadcastDestruction from being called again
				replica->DeallocReplica(connection);
			}
		}
		else
		{
			replica->PreDestruction(connection);
			connection->OnDereference(replica, this);
		}

		bsIn.AlignReadToByteBoundary();
	}
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnSerialize(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, RakNetTime timestamp, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;
	if (connection->groupConstructionAndSerialize)
	{
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}
	if (networkIDManager==0)
		networkIDManager=rakPeerInterface->GetNetworkIDManager();
	RakAssert(networkIDManager);
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);

	struct DeserializeParameters ds;
	ds.timeStamp=timestamp;
	ds.sourceConnection=connection;

	Replica3 *replica;
	NetworkID networkId;
	BitSize_t bitsUsed;
	bsIn.Read(networkId);
	replica = networkIDManager->GET_OBJECT_FROM_ID<Replica3*>(networkId);
	if (replica)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			bsIn.Read(ds.bitstreamWrittenTo[z]);
			if (ds.bitstreamWrittenTo[z])
			{
				bsIn.ReadCompressed(bitsUsed);
				bsIn.AlignReadToByteBoundary();
				bsIn.Read(ds.serializationBitstream[z], bitsUsed);
			}
		}
		replica->Deserialize(&ds);
	}
	return RR_CONTINUE_PROCESSING;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnDownloadStarted(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;
	if (connection->QueryGroupDownloadMessages() &&
		// ID_DOWNLOAD_STARTED will be processed twice, being processed the second time once ID_DOWNLOAD_COMPLETE arrives.
		// However, the second time groupConstructionAndSerialize will be set to true so it won't be processed a third time
		connection->groupConstructionAndSerialize==false
		)
	{
		// These messages will be held by the plugin and returned when the download is complete
		connection->groupConstructionAndSerialize=true;
		RakAssert(connection->downloadGroup.GetSize()==0);
		connection->downloadGroup.Push(packet, __FILE__, __LINE__);
		return RR_STOP_PROCESSING;
	}

	connection->groupConstructionAndSerialize=false;
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	connection->DeserializeOnDownloadStarted(&bsIn);
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

PluginReceiveResult ReplicaManager3::OnDownloadComplete(Packet *packet, unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return RR_CONTINUE_PROCESSING;

	if (connection->groupConstructionAndSerialize==true && connection->downloadGroup.GetSize()>0)
	{
		// Push back buffered packets in front of this one
		DataStructures::DefaultIndexType i;
		for (i=0; i < connection->downloadGroup.GetSize(); i++)
			rakPeerInterface->PushBackPacket(connection->downloadGroup[i],false);

		// Push this one to be last too. It will be processed again, but the second time 
		// groupConstructionAndSerialize will be false and downloadGroup will be empty, so it will go past this block
		connection->downloadGroup.Clear(false,__FILE__,__LINE__);
		rakPeerInterface->PushBackPacket(packet,false);

		return RR_STOP_PROCESSING;
	}

	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	connection->DeserializeOnDownloadComplete(&bsIn);
	return RR_CONTINUE_PROCESSING;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnLocalConstructionRejected(unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return;
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	uint32_t allocationNumber;
	bsIn.Read(allocationNumber);
	DataStructures::DefaultIndexType index;
	for (index=0; index < userReplicaList.GetSize(); index++)
	{
		if (userReplicaList[index]->GetAllocationNumber()==allocationNumber)
		{
			userReplicaList[index]->DeserializeConstructionRequestRejected(&bsIn,connection);
			break;
		}
	}

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReplicaManager3::OnLocalConstructionAccepted(unsigned char *packetData, int packetDataLength, RakNetGUID senderGuid, unsigned char packetDataOffset)
{
	Connection_RM3 *connection = GetConnectionByGUID(senderGuid);
	if (connection==0)
		return;
	RakNet::BitStream bsIn(packetData,packetDataLength,false);
	bsIn.IgnoreBytes(packetDataOffset);
	uint32_t allocationNumber;
	bsIn.Read(allocationNumber);
	NetworkID assignedNetworkId;
	bsIn.Read(assignedNetworkId);
	DataStructures::DefaultIndexType index;
	DataStructures::DefaultIndexType index2;
	Replica3 *replica;
	SerializeParameters sp;
	sp.whenLastSerialized=0;
	RakNet::BitStream emptyBs;
	for (index=0; index < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; index++)
		sp.lastSentBitstream[index]=&emptyBs;
	RM3SerializationResult res;
	for (index=0; index < userReplicaList.GetSize(); index++)
	{
		if (userReplicaList[index]->GetAllocationNumber()==allocationNumber)
		{
			replica=userReplicaList[index];
			index2=connection->constructedReplicaList.GetIndexOf(replica);
			if (index2!=(DataStructures::DefaultIndexType)-1)
			{
				LastSerializationResult *lsr = connection->constructedReplicaList[index2];

				replica->SetNetworkID(assignedNetworkId);
				replica->DeserializeConstructionRequestAccepted(&bsIn,connection);
				sp.destinationConnection=connection;

				// Immediately serialize
				res = replica->Serialize(&sp);
				if (res!=RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION &&
					res!=RM3SR_DO_NOT_SERIALIZE &&
					res!=RM3SR_SERIALIZED_UNIQUELY)
				{
					sp.destinationConnection=connection;
					sp.messageTimestamp=0;
					for (int i=0; i < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; i++)
						sp.pro[i]=defaultSendParameters;
					bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
					for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
					{
						allIndices[z]=true;
					}
					if (connection->SendSerialize(replica, allIndices, sp.outputBitstream, sp.messageTimestamp, sp.pro, rakPeerInterface, worldId)==SSICR_SENT_DATA)
						lsr->replica->whenLastSerialized=RakNet::GetTime();
				}

				// Start serialization queries
				connection->queryToSerializeReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);

				return;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3* ReplicaManager3::GetReplicaByNetworkID(NetworkID networkId)
{
	DataStructures::DefaultIndexType i;
	for (i=0; i < userReplicaList.GetSize(); i++)
	{
		if (userReplicaList[i]->GetNetworkID()==networkId)
			return userReplicaList[i];
	}
	return 0;
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void ReplicaManager3::BroadcastDestructionList(DataStructures::Multilist<ML_STACK, Replica3*> &replicaList, SystemAddress exclusionAddress)
{
	RakNet::BitStream bsOut;
	DataStructures::DefaultIndexType i,j;

	for (i=0; i < replicaList.GetSize(); i++)
	{

		if (replicaList[i]->deletingSystemGUID==UNASSIGNED_RAKNET_GUID)
			replicaList[i]->deletingSystemGUID=GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
	}

	for (j=0; j < connectionList.GetSize(); j++)
	{
		if (connectionList[j]->GetSystemAddress()==exclusionAddress)
			continue;

		bsOut.Reset();
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_CONSTRUCTION);
		bsOut.Write(worldId);
		DataStructures::DefaultIndexType cnt=0;
		bsOut.Write(cnt); // No construction
		cnt=replicaList.GetSize();
		bsOut.Write(cnt);

		for (i=0; i < replicaList.GetSize(); i++)
		{
			if (connectionList[j]->HasReplicaConstructed(replicaList[i])==false)
				continue;

			NetworkID networkId;
			networkId=replicaList[i]->GetNetworkID();
			bsOut.Write(networkId);
			BitSize_t offsetStart, offsetEnd;
			offsetStart=bsOut.GetWriteOffset();
			bsOut.Write(offsetStart);
			bsOut.Write(replicaList[i]->deletingSystemGUID);
			replicaList[i]->SerializeDestruction(&bsOut, connectionList[j]);
			bsOut.AlignWriteToByteBoundary();
			offsetEnd=bsOut.GetWriteOffset();
			bsOut.SetWriteOffset(offsetStart);
			bsOut.Write(offsetEnd);
			bsOut.SetWriteOffset(offsetEnd);
		}

		rakPeerInterface->Send(&bsOut,defaultSendParameters.priority,defaultSendParameters.reliability,defaultSendParameters.orderingChannel,connectionList[j]->GetSystemAddress(),false, defaultSendParameters.sendReceipt);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void ReplicaManager3::BroadcastDestruction(Replica3 *replica, SystemAddress exclusionAddress)
{
	DataStructures::Multilist<ML_STACK, Replica3*> replicaList;
	replicaList.Push(replica, __FILE__, __LINE__ );
	BroadcastDestructionList(replicaList,exclusionAddress);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3::Connection_RM3(SystemAddress _systemAddress, RakNetGUID _guid)
: systemAddress(_systemAddress), guid(_guid)
{
	isValidated=false;
	isFirstConstruction=true;
	groupConstructionAndSerialize=false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Connection_RM3::~Connection_RM3()
{
	constructedReplicaList.ClearPointers(true,__FILE__,__LINE__);
	queryToConstructReplicaList.ClearPointers(true,__FILE__,__LINE__);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::GetConstructedReplicas(DataStructures::Multilist<ML_STACK, Replica3*> &objectsTheyDoHave)
{
	objectsTheyDoHave.Clear(true,__FILE__,__LINE__);
	for (DataStructures::DefaultIndexType idx=0; idx < constructedReplicaList.GetSize(); idx++)
	{
		objectsTheyDoHave.Push(constructedReplicaList[idx]->replica, __FILE__, __LINE__ );
	}
	objectsTheyDoHave.TagSorted();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Connection_RM3::HasReplicaConstructed(RakNet::Replica3 *replica)
{
	return constructedReplicaList.GetIndexOf(replica)!=(DataStructures::DefaultIndexType)-1;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::SendSerializeHeader(RakNet::Replica3 *replica, RakNetTime timestamp, RakNet::BitStream *bs, unsigned char worldId)
{
	bs->Reset();

	if (timestamp!=0)
	{
		bs->Write((MessageID)ID_TIMESTAMP);
		bs->Write(timestamp);
	}
	bs->Write((MessageID)ID_REPLICA_MANAGER_SERIALIZE);
	bs->Write(worldId);
	bs->Write(replica->GetNetworkID());
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::ClearDownloadGroup(RakPeerInterface *rakPeerInterface)
{
	DataStructures::DefaultIndexType i;
	for (i=0; i < downloadGroup.GetSize(); i++)
		rakPeerInterface->DeallocatePacket(downloadGroup[i]);
	downloadGroup.Clear(false,__FILE__,__LINE__);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SendSerializeIfChangedResult Connection_RM3::SendSerialize(RakNet::Replica3 *replica, bool indicesToSend[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNet::BitStream serializationData[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakNetTime timestamp, PRO sendParameters[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS], RakPeerInterface *rakPeer, unsigned char worldId)
{
	bool channelHasData;
	BitSize_t sum=0;
	for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
	{
		if (indicesToSend[z])
			sum+=serializationData[z].GetNumberOfBitsUsed();
	}
	if (sum==0)
		return SSICR_DID_NOT_SEND_DATA;

	RakAssert(replica->GetNetworkID()!=UNASSIGNED_NETWORK_ID);

	RakNet::BitStream out;
	BitSize_t bitsUsed;

	int channelIndex;
	PRO lastPro=sendParameters[0];

	for (channelIndex=0; channelIndex < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex++)
	{
		if (channelIndex==0)
		{
			SendSerializeHeader(replica, timestamp, &out, worldId);
		}
		else if (lastPro!=sendParameters[channelIndex])
		{
			// Write out remainder
			for (int channelIndex2=channelIndex; channelIndex2 < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex2++)
				out.Write(false);

			// Send remainder
			replica->OnSerializeTransmission(&out, systemAddress);
			rakPeer->Send(&out,lastPro.priority,lastPro.reliability,lastPro.orderingChannel,systemAddress,false,lastPro.sendReceipt);

			// If no data left to send, quit out
			bool anyData=false;
			for (int channelIndex2=channelIndex; channelIndex2 < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; channelIndex2++)
			{
				if (serializationData[channelIndex2].GetNumberOfBitsUsed()>0)
				{
					anyData=true;
					break;
				}
			}
			if (anyData==false)
				return SSICR_SENT_DATA;

			// Restart stream
			SendSerializeHeader(replica, timestamp, &out, worldId);

			for (int channelIndex2=0; channelIndex2 < channelIndex; channelIndex2++)
				out.Write(false);
			lastPro=sendParameters[channelIndex];
		}

		bitsUsed=serializationData[channelIndex].GetNumberOfBitsUsed();
		channelHasData = indicesToSend[channelIndex]==true && bitsUsed>0;
		out.Write(channelHasData);
		if (channelHasData)
		{
			out.WriteCompressed(bitsUsed);
			out.AlignWriteToByteBoundary();
			out.Write(serializationData[channelIndex]);
			// Crap, forgot this line, was a huge bug in that I'd only send to the first 3 systems
			serializationData[channelIndex].ResetReadPointer();
		}
	}
	replica->OnSerializeTransmission(&out, systemAddress);
	rakPeer->Send(&out,lastPro.priority,lastPro.reliability,lastPro.orderingChannel,systemAddress,false,lastPro.sendReceipt);
	return SSICR_SENT_DATA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

SendSerializeIfChangedResult Connection_RM3::SendSerializeIfChanged(DataStructures::DefaultIndexType queryToSerializeIndex, SerializeParameters *sp, RakPeerInterface *rakPeer, unsigned char worldId, ReplicaManager3 *replicaManager)
{
	RakNet::Replica3 *replica = queryToSerializeReplicaList[queryToSerializeIndex]->replica;

	if (replica->GetNetworkID()==UNASSIGNED_NETWORK_ID)
		return SSICR_DID_NOT_SEND_DATA;

	RM3QuerySerializationResult rm3qsr = replica->QuerySerialization(this);
	if (rm3qsr==RM3QSR_NEVER_CALL_SERIALIZE)
	{
		// Never again for this connection and replica pair
		OnNeverSerialize(queryToSerializeIndex, replicaManager);
		return SSICR_NEVER_SERIALIZE;
	}

	if (rm3qsr==RM3QSR_DO_NOT_CALL_SERIALIZE)
		return SSICR_DID_NOT_SEND_DATA;

	if (replica->forceSendUntilNextUpdate)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (replica->lastSentSerialization.indicesToSend[z])
				sp->bitsWrittenSoFar+=replica->lastSentSerialization.bitStream[z].GetNumberOfBitsUsed();
		}
		return SendSerialize(replica, replica->lastSentSerialization.indicesToSend, replica->lastSentSerialization.bitStream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	for (int i=0; i < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; i++)
	{
		sp->outputBitstream[i].Reset();
		if (queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS)
			sp->lastSentBitstream[i]=&queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[i];
		else
			sp->lastSentBitstream[i]=&replica->lastSentSerialization.bitStream[i];
	}

	RM3SerializationResult serializationResult = replica->Serialize(sp);

	if (serializationResult==RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION)
	{
		// Never again for this connection and replica pair
		OnNeverSerialize(queryToSerializeIndex, replicaManager);
		return SSICR_NEVER_SERIALIZE;
	}

	if (serializationResult==RM3SR_DO_NOT_SERIALIZE)
	{
		// Don't serialize this tick only
		return SSICR_DID_NOT_SEND_DATA;
	}

	// This is necessary in case the user in the Serialize() function for some reason read the bitstream they also wrote
	// WIthout this code, the Write calls to another bitstream would not write the entire bitstream
	BitSize_t sum=0;
	for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
	{
		sp->outputBitstream[z].ResetReadPointer();
		sum+=sp->outputBitstream[z].GetNumberOfBitsUsed();
	}
	
	if (sum==0)
	{
		// Don't serialize this tick only
		return SSICR_DID_NOT_SEND_DATA;
	}

	if (serializationResult==RM3SR_SERIALIZED_ALWAYS)
	{
		bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
			allIndices[z]=true;

			queryToSerializeReplicaList[queryToSerializeIndex]->AllocBS();
			queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Reset();
			queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Write(&sp->outputBitstream[z]);
			sp->outputBitstream[z].ResetReadPointer();
		}
		return SendSerialize(replica, allIndices, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	if (serializationResult==RM3SR_SERIALIZED_ALWAYS_IDENTICALLY)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			replica->lastSentSerialization.indicesToSend[z]=sp->outputBitstream[z].GetNumberOfBitsUsed()>0;
			sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
			replica->lastSentSerialization.bitStream[z].Reset();
			replica->lastSentSerialization.bitStream[z].Write(&sp->outputBitstream[z]);
			sp->outputBitstream[z].ResetReadPointer();
			replica->forceSendUntilNextUpdate=true;
		}
		return SendSerialize(replica, replica->lastSentSerialization.indicesToSend, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
	}

	bool indicesToSend[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
	if (serializationResult==RM3SR_BROADCAST_IDENTICALLY || serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION)
	{
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (sp->outputBitstream[z].GetNumberOfBitsUsed() > 0 &&
				(serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION ||
				((sp->outputBitstream[z].GetNumberOfBitsUsed()!=replica->lastSentSerialization.bitStream[z].GetNumberOfBitsUsed() ||
				memcmp(sp->outputBitstream[z].GetData(), replica->lastSentSerialization.bitStream[z].GetData(), sp->outputBitstream[z].GetNumberOfBytesUsed())!=0))))
			{
				indicesToSend[z]=true;
				replica->lastSentSerialization.indicesToSend[z]=true;
				sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
				replica->lastSentSerialization.bitStream[z].Reset();
				replica->lastSentSerialization.bitStream[z].Write(&sp->outputBitstream[z]);
				sp->outputBitstream[z].ResetReadPointer();
				replica->forceSendUntilNextUpdate=true;
			}
			else
			{
				indicesToSend[z]=false;
				replica->lastSentSerialization.indicesToSend[z]=false;
			}
		}
	}
	else
	{
		queryToSerializeReplicaList[queryToSerializeIndex]->AllocBS();

		// RM3SR_SERIALIZED_UNIQUELY
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			if (sp->outputBitstream[z].GetNumberOfBitsUsed() > 0 &&
				(sp->outputBitstream[z].GetNumberOfBitsUsed()!=queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].GetNumberOfBitsUsed() ||
				memcmp(sp->outputBitstream[z].GetData(), queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].GetData(), sp->outputBitstream[z].GetNumberOfBytesUsed())!=0)
				)
			{
				indicesToSend[z]=true;
				sp->bitsWrittenSoFar+=sp->outputBitstream[z].GetNumberOfBitsUsed();
				queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Reset();
				queryToSerializeReplicaList[queryToSerializeIndex]->lastSerializationResultBS->bitStream[z].Write(&sp->outputBitstream[z]);
				sp->outputBitstream[z].ResetReadPointer();
			}
			else
			{
				indicesToSend[z]=false;
			}
		}
	}


	if (serializationResult==RM3SR_BROADCAST_IDENTICALLY || serializationResult==RM3SR_BROADCAST_IDENTICALLY_FORCE_SERIALIZATION)
		replica->forceSendUntilNextUpdate=true;

	// Send out the data
	return SendSerialize(replica, indicesToSend, sp->outputBitstream, sp->messageTimestamp, sp->pro, rakPeer, worldId);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Connection_RM3::OnLocalReference(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	(void) replicaManager;

	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(__FILE__,__LINE__);
	lsr->replica=replica3;
	queryToConstructReplicaList.Push(lsr,replica3,__FILE__,__LINE__);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDereference(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);

	LastSerializationResult* lsr=0;
	DataStructures::DefaultIndexType idx;
	idx=constructedReplicaList.GetIndexOf(replica3);
	if (idx!=(DataStructures::DefaultIndexType)-1)
	{
		lsr=constructedReplicaList[idx];
		constructedReplicaList.RemoveAtIndex(idx,__FILE__,__LINE__);
	}
	
	for (idx=0; idx < queryToConstructReplicaList.GetSize(); idx++)
	{
		if (queryToConstructReplicaList[idx]->replica==replica3)
		{
			lsr=queryToConstructReplicaList[idx];
			queryToConstructReplicaList.RemoveAtIndex(idx,__FILE__,__LINE__);
			break;
		}
	}

	for (idx=0; idx < queryToSerializeReplicaList.GetSize(); idx++)
	{
		if (queryToSerializeReplicaList[idx]->replica==replica3)
		{
			lsr=queryToSerializeReplicaList[idx];
			queryToSerializeReplicaList.RemoveAtIndex(idx,__FILE__,__LINE__);
			break;
		}
	}

	for (idx=0; idx < queryToDestructReplicaList.GetSize(); idx++)
	{
		if (queryToDestructReplicaList[idx]->replica==replica3)
		{
			lsr=queryToDestructReplicaList[idx];
			queryToDestructReplicaList.RemoveAtIndex(idx,__FILE__,__LINE__);
			break;
		}
	}

	ValidateLists(replicaManager);

	if (lsr)
		RakNet::OP_DELETE(lsr,__FILE__,__LINE__);

	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadFromThisSystem(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(__FILE__,__LINE__);
	lsr->replica=replica3;

	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		queryToConstructReplicaList.RemoveAtKey(replica3,false,__FILE__,__LINE__);
		queryToDestructReplicaList.Push(lsr,replica3,__FILE__,__LINE__);
	}

	constructedReplicaList.Push(lsr,replica3,__FILE__,__LINE__);
	//assert(queryToSerializeReplicaList.GetIndexOf(replica3)==(DataStructures::DefaultIndexType)-1);
	queryToSerializeReplicaList.Push(lsr,replica3,__FILE__,__LINE__);

	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadFromOtherSystem(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		if (queryToConstructReplicaList.GetIndexOf(replica3)==(DataStructures::DefaultIndexType)-1)
			OnLocalReference(replica3, replicaManager);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnNeverConstruct(DataStructures::DefaultIndexType queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx,__FILE__,__LINE__);
	RakNet::OP_DELETE(lsr,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnConstructToThisConnection(DataStructures::DefaultIndexType queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx,__FILE__,__LINE__);
	//assert(constructedReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	constructedReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	//assert(queryToDestructReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	queryToDestructReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	//assert(queryToSerializeReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	if (lsr->replica->GetNetworkID()!=UNASSIGNED_NETWORK_ID)
		queryToSerializeReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnConstructToThisConnection(Replica3 *replica, ReplicaManager3 *replicaManager)
{
	RakAssert(QueryConstructionMode()==QUERY_CONNECTION_FOR_REPLICA_LIST);
	(void) replicaManager;

	LastSerializationResult* lsr=RakNet::OP_NEW<LastSerializationResult>(__FILE__,__LINE__);
	lsr->replica=replica;
	constructedReplicaList.Push(lsr,replica,__FILE__,__LINE__);
	if (lsr->replica->GetNetworkID()!=UNASSIGNED_NETWORK_ID)
		queryToSerializeReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnNeverSerialize(DataStructures::DefaultIndexType queryToSerializeIndex, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	queryToSerializeReplicaList.RemoveAtIndex(queryToSerializeIndex,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnReplicaAlreadyExists(DataStructures::DefaultIndexType queryToConstructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToConstructReplicaList[queryToConstructIdx];
	queryToConstructReplicaList.RemoveAtIndex(queryToConstructIdx,__FILE__,__LINE__);
	//assert(constructedReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	constructedReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	//assert(queryToDestructReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	queryToDestructReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	//assert(queryToSerializeReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	queryToSerializeReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDownloadExisting(Replica3* replica3, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);

	ConstructionMode constructionMode = QueryConstructionMode();
	if (constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)
	{
		DataStructures::DefaultIndexType idx;
		for (idx=0; idx < queryToConstructReplicaList.GetSize(); idx++)
		{
			if (queryToConstructReplicaList[idx]->replica==replica3)
			{
				OnConstructToThisConnection(idx, replicaManager);
				return;
			}
		}
	}	
	else
	{
		OnConstructToThisConnection(replica3, replicaManager);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnSendDestructionFromQuery(DataStructures::DefaultIndexType queryToDestructIdx, ReplicaManager3 *replicaManager)
{
	ConstructionMode constructionMode = QueryConstructionMode();
	RakAssert(constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION || constructionMode==QUERY_REPLICA_FOR_CONSTRUCTION_AND_DESTRUCTION)

	ValidateLists(replicaManager);
	LastSerializationResult* lsr = queryToDestructReplicaList[queryToDestructIdx];
	queryToDestructReplicaList.RemoveAtIndex(queryToDestructIdx,__FILE__,__LINE__);
	queryToSerializeReplicaList.RemoveAtKey(lsr->replica,false,__FILE__,__LINE__);
	constructedReplicaList.RemoveAtKey(lsr->replica,true,__FILE__,__LINE__);
	//assert(queryToConstructReplicaList.GetIndexOf(lsr->replica)==(DataStructures::DefaultIndexType)-1);
	queryToConstructReplicaList.Push(lsr,lsr->replica,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::OnDoNotQueryDestruction(DataStructures::DefaultIndexType queryToDestructIdx, ReplicaManager3 *replicaManager)
{
	ValidateLists(replicaManager);
	queryToDestructReplicaList.RemoveAtIndex(queryToDestructIdx,__FILE__,__LINE__);
	ValidateLists(replicaManager);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::ValidateLists(ReplicaManager3 *replicaManager) const
{
	(void) replicaManager;
	/*
#ifdef _DEBUG
	// Each object should exist only once in either constructedReplicaList or queryToConstructReplicaList
	// replicaPointer from LastSerializationResult should be same among all lists
	DataStructures::DefaultIndexType idx, idx2;
	for (idx=0; idx < constructedReplicaList.GetSize(); idx++)
	{
		idx2=queryToConstructReplicaList.GetIndexOf(constructedReplicaList[idx]->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	for (idx=0; idx < queryToConstructReplicaList.GetSize(); idx++)
	{
		idx2=constructedReplicaList.GetIndexOf(queryToConstructReplicaList[idx]->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	LastSerializationResult *lsr, *lsr2;
	for (idx=0; idx < constructedReplicaList.GetSize(); idx++)
	{
		lsr=constructedReplicaList[idx];

		idx2=queryToSerializeReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			lsr2=queryToSerializeReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}

		idx2=queryToDestructReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			lsr2=queryToDestructReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}
	}
	for (idx=0; idx < queryToConstructReplicaList.GetSize(); idx++)
	{
		lsr=queryToConstructReplicaList[idx];

		idx2=queryToSerializeReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			lsr2=queryToSerializeReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}

		idx2=queryToDestructReplicaList.GetIndexOf(lsr->replica);
		if (idx2!=(DataStructures::DefaultIndexType)-1)
		{
			lsr2=queryToDestructReplicaList[idx2];
			if (lsr2!=lsr)
			{
				int a=5;
				assert(a==0);
				int *b=0;
				*b=5;
			}
		}
	}

	// Verify pointer integrity
	for (idx=0; idx < constructedReplicaList.GetSize(); idx++)
	{
		if (constructedReplicaList[idx]->replica->replicaManager!=replicaManager)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}

	// Verify pointer integrity
	for (idx=0; idx < queryToConstructReplicaList.GetSize(); idx++)
	{
		if (queryToConstructReplicaList[idx]->replica->replicaManager!=replicaManager)
		{
			int a=5;
			assert(a==0);
			int *b=0;
			*b=5;
		}
	}
#endif
	*/
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::SendConstruction(DataStructures::Multilist<ML_STACK, Replica3*, Replica3*> &newObjects, DataStructures::Multilist<ML_STACK, Replica3*, Replica3*> &deletedObjects, PRO sendParameters, RakPeerInterface *rakPeer, unsigned char worldId)
{
	if (newObjects.GetSize()==0 && deletedObjects.GetSize()==0)
		return;

	// All construction and destruction takes place in the same network message
	// Otherwise, if objects rely on each other being created the same tick to be valid, this won't always be true
	//	DataStructures::Multilist<ML_STACK, LastSerializationResult* > serializedObjects;
	BitSize_t offsetStart, offsetEnd;
	DataStructures::DefaultIndexType newListIndex, oldListIndex;
	RakNet::BitStream bsOut;
	NetworkID networkId;
	if (isFirstConstruction)
	{
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_DOWNLOAD_STARTED);
		bsOut.Write(worldId);
		SerializeOnDownloadStarted(&bsOut);
		rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);
	}
	

	//	LastSerializationResult* lsr;
	bsOut.Reset();
	bsOut.Write((MessageID)ID_REPLICA_MANAGER_CONSTRUCTION);
	bsOut.Write(worldId);
	bsOut.Write(newObjects.GetSize());
	// Construction
	for (newListIndex=0; newListIndex < newObjects.GetSize(); newListIndex++)
	{
		offsetStart=bsOut.GetWriteOffset();
		bsOut.Write(offsetStart);
		networkId=newObjects[newListIndex]->GetNetworkID();
		bsOut.Write(networkId);
		newObjects[newListIndex]->WriteAllocationID(&bsOut);
		if (networkId==UNASSIGNED_NETWORK_ID)
			bsOut.Write(newObjects[newListIndex]->GetAllocationNumber());
		bsOut.Write(newObjects[newListIndex]->creatingSystemGUID);
		newObjects[newListIndex]->SerializeConstruction(&bsOut, this);
		bsOut.AlignWriteToByteBoundary();
		offsetEnd=bsOut.GetWriteOffset();
		bsOut.SetWriteOffset(offsetStart);
		bsOut.Write(offsetEnd);
		bsOut.SetWriteOffset(offsetEnd);
		//		lsr = Reference(newObjects[newListIndex],false);
		//		serializedObjects.Push(newObjects[newListIndex]);
	}

	// Destruction
	DataStructures::DefaultIndexType listSize=deletedObjects.GetSize();
	bsOut.Write(listSize);
	for (oldListIndex=0; oldListIndex < deletedObjects.GetSize(); oldListIndex++)
	{
		networkId=deletedObjects[oldListIndex]->GetNetworkID();
		bsOut.Write(networkId);
		offsetStart=bsOut.GetWriteOffset();
		bsOut.Write(offsetStart);
		deletedObjects[oldListIndex]->deletingSystemGUID=rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
		bsOut.Write(deletedObjects[oldListIndex]->deletingSystemGUID);
		deletedObjects[oldListIndex]->SerializeDestruction(&bsOut, this);
		bsOut.AlignWriteToByteBoundary();
		offsetEnd=bsOut.GetWriteOffset();
		bsOut.SetWriteOffset(offsetStart);
		bsOut.Write(offsetEnd);
		bsOut.SetWriteOffset(offsetEnd);
	}
	rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);

	// Initial Download serialize to a new system
	// Immediately send serialize after construction if the replica object already has saved data
	// If the object was serialized identically, and does not change later on, then the new connection never gets the data
	SerializeParameters sp;
	sp.whenLastSerialized=0;
	RakNet::BitStream emptyBs;
	for (int index=0; index < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; index++)
	{
		sp.lastSentBitstream[index]=&emptyBs;
		sp.pro[index]=sendParameters;
		sp.pro[index].reliability=RELIABLE_ORDERED;
	}

	sp.bitsWrittenSoFar=0;
	RakNetTime t = RakNet::GetTime();
	for (newListIndex=0; newListIndex < newObjects.GetSize(); newListIndex++)
	{
		sp.destinationConnection=this;
		sp.messageTimestamp=0;
		RakNet::Replica3 *replica = newObjects[newListIndex];
		// 8/22/09 Forgot ResetWritePointer
		for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
		{
			sp.outputBitstream[z].ResetWritePointer();
		}

		RM3SerializationResult res = replica->Serialize(&sp);
		if (replica->GetNetworkID()!=UNASSIGNED_NETWORK_ID)
		{
			if (res!=RM3SR_NEVER_SERIALIZE_FOR_THIS_CONNECTION &&
				res!=RM3SR_DO_NOT_SERIALIZE &&
				res!=RM3SR_SERIALIZED_UNIQUELY)
			{
				bool allIndices[RM3_NUM_OUTPUT_BITSTREAM_CHANNELS];
				for (int z=0; z < RM3_NUM_OUTPUT_BITSTREAM_CHANNELS; z++)
				{
					sp.bitsWrittenSoFar+=sp.outputBitstream[z].GetNumberOfBitsUsed();
					allIndices[z]=true;
				}
				SendSerialize(replica, allIndices, sp.outputBitstream, sp.messageTimestamp, sp.pro, rakPeer, worldId);
				newObjects[newListIndex]->whenLastSerialized=t;

			}
		}
		// else wait for construction request accepted before serializing
	}

	if (isFirstConstruction)
	{
		bsOut.Reset();
		bsOut.Write((MessageID)ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE);
		bsOut.Write(worldId);
		SerializeOnDownloadComplete(&bsOut);
		rakPeer->Send(&bsOut,sendParameters.priority,RELIABLE_ORDERED,sendParameters.orderingChannel,systemAddress,false,sendParameters.sendReceipt);
	}
	
	isFirstConstruction=false;

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Connection_RM3::SendValidation(RakPeerInterface *rakPeer, unsigned char worldId)
{
	// Hijack to mean sendValidation
	RakNet::BitStream bsOut;
	bsOut.Write((MessageID)ID_REPLICA_MANAGER_SCOPE_CHANGE);
	bsOut.Write(worldId);
	rakPeer->Send(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,systemAddress,false);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3::Replica3()
{
	creatingSystemGUID=UNASSIGNED_RAKNET_GUID;
	deletingSystemGUID=UNASSIGNED_RAKNET_GUID;
	replicaManager=0;
	forceSendUntilNextUpdate=false;
	whenLastSerialized=0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Replica3::~Replica3()
{
	if (replicaManager)
	{
		replicaManager->Dereference(this);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void Replica3::BroadcastDestruction(void)
{
	if (replicaManager)
		replicaManager->BroadcastDestruction(this,UNASSIGNED_SYSTEM_ADDRESS);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNetGUID Replica3::GetCreatingSystemGUID(void) const
{
	return creatingSystemGUID;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_ClientConstruction(RakNet::Connection_RM3 *destinationConnection)
{
	(void) destinationConnection;
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3CS_SEND_CONSTRUCTION;
	// Send back to the owner client too, because they couldn't assign the network ID
	if (networkIDManager->IsNetworkIDAuthority())
		return RM3CS_SEND_CONSTRUCTION;
	return RM3CS_NEVER_CONSTRUCT;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_ClientConstruction(RakNet::Connection_RM3 *sourceConnection)
{
	(void) sourceConnection;

	// OK to create
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_ServerConstruction(RakNet::Connection_RM3 *destinationConnection)
{
	(void) destinationConnection;

	if (networkIDManager->IsNetworkIDAuthority())
		return RM3CS_SEND_CONSTRUCTION;
	return RM3CS_NEVER_CONSTRUCT;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_ServerConstruction(RakNet::Connection_RM3 *sourceConnection)
{
	(void) sourceConnection;
	if (networkIDManager->IsNetworkIDAuthority())
		return false;
	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ConstructionState Replica3::QueryConstruction_PeerToPeer(RakNet::Connection_RM3 *destinationConnection)
{
	(void) destinationConnection;

	// We send to all, others do nothing
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3CS_SEND_CONSTRUCTION;
	return RM3CS_NEVER_CONSTRUCT;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool Replica3::QueryRemoteConstruction_PeerToPeer(RakNet::Connection_RM3 *sourceConnection)
{
	(void) sourceConnection;

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_ClientSerializable(RakNet::Connection_RM3 *destinationConnection)
{
	// Owner client sends to all
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3QSR_CALL_SERIALIZE;
	// Server sends to all but owner client
	if (networkIDManager->IsNetworkIDAuthority() && destinationConnection->GetRakNetGUID()!=creatingSystemGUID)
		return RM3QSR_CALL_SERIALIZE;
	// Remote clients do not send
	return RM3QSR_NEVER_CALL_SERIALIZE;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_ServerSerializable(RakNet::Connection_RM3 *destinationConnection)
{
	(void) destinationConnection;
	// Server sends to all
	if (networkIDManager->IsNetworkIDAuthority())
		return RM3QSR_CALL_SERIALIZE;

	// Clients do not send
	return RM3QSR_NEVER_CALL_SERIALIZE;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3QuerySerializationResult Replica3::QuerySerialization_PeerToPeer(RakNet::Connection_RM3 *destinationConnection)
{
	(void) destinationConnection;

	// Owner peer sends to all
	if (creatingSystemGUID==replicaManager->GetRakPeerInterface()->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS))
		return RM3QSR_CALL_SERIALIZE;

	// Remote peers do not send
	return RM3QSR_NEVER_CALL_SERIALIZE;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_Client(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_Server(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA_AND_BROADCAST_DESTRUCTION;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RM3ActionOnPopConnection Replica3::QueryActionOnPopConnection_PeerToPeer(RakNet::Connection_RM3 *droppedConnection) const
{
	(void) droppedConnection;
	return RM3AOPC_DELETE_REPLICA;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif // _RAKNET_SUPPORT_*
