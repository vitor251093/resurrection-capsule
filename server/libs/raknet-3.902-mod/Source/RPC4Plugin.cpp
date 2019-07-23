#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_RPC4Plugin==1

#include "RPC4Plugin.h"
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "PacketizedTCP.h"

using namespace RakNet;

RPC4Plugin::RPC4Plugin()
{

}
RPC4Plugin::~RPC4Plugin()
{

}
bool RPC4Plugin::RegisterFunction(const char* uniqueID, void ( *functionPointer ) ( RakNet::BitStream *userData, Packet *packet ))
{
	DataStructures::StringKeyedHashIndex skhi = registeredFunctions.GetIndexOf(uniqueID);
	if (skhi.IsInvalid()==false)
		return false;

	registeredFunctions.Push(uniqueID,functionPointer,__FILE__,__LINE__);
	return true;
}
bool RPC4Plugin::UnregisterFunction(const char* uniqueID)
{
	void ( *f ) ( RakNet::BitStream *, Packet * );
	return registeredFunctions.Pop(f,uniqueID,__FILE__,__LINE__);
}
void RPC4Plugin::CallLoopback( const char* uniqueID, RakNet::BitStream * bitStream )
{
	Packet *p=0;

	DataStructures::StringKeyedHashIndex skhi = registeredFunctions.GetIndexOf(uniqueID);

	if (skhi.IsInvalid()==true)
	{
		if (rakPeerInterface) 
			p=rakPeerInterface->AllocatePacket(sizeof(MessageID)+sizeof(unsigned char)+strlen(uniqueID)+1);
#if _RAKNET_SUPPORT_PacketizedTCP==1
		else
			p=packetizedTCP->AllocatePacket(sizeof(MessageID)+sizeof(unsigned char)+strlen(uniqueID)+1);
#endif

		if (rakPeerInterface)
			p->guid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
#if _RAKNET_SUPPORT_PacketizedTCP==1
		else
			p->guid=UNASSIGNED_RAKNET_GUID;
#endif

		p->systemAddress=UNASSIGNED_SYSTEM_ADDRESS;
		p->systemAddress.systemIndex=(SystemIndex)-1;
		p->data[0]=ID_RPC_REMOTE_ERROR;
		p->data[1]=RPC_ERROR_FUNCTION_NOT_REGISTERED;
		strcpy((char*) p->data+2, uniqueID);
		
		PushBackPacketUnified(p,false);

		return;
	}

	RakNet::BitStream out;
	out.Write((MessageID) ID_RPC_4_PLUGIN);
	out.WriteCompressed(uniqueID);
	if (bitStream)
	{
		bitStream->ResetReadPointer();
		out.Write(bitStream);
	}
	if (rakPeerInterface) 
		p=rakPeerInterface->AllocatePacket(out.GetNumberOfBytesUsed());
#if _RAKNET_SUPPORT_PacketizedTCP==1
	else
		p=packetizedTCP->AllocatePacket(out.GetNumberOfBytesUsed());
#endif

	if (rakPeerInterface)
		p->guid=rakPeerInterface->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS);
#if _RAKNET_SUPPORT_PacketizedTCP==1
	else
		p->guid=UNASSIGNED_RAKNET_GUID;
#endif
	p->systemAddress=UNASSIGNED_SYSTEM_ADDRESS;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	memcpy(p->data,out.GetData(),out.GetNumberOfBytesUsed());
	PushBackPacketUnified(p,false);
	return;
}
void RPC4Plugin::Call( const char* uniqueID, RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast )
{
	RakNet::BitStream out;
	out.Write((MessageID) ID_RPC_4_PLUGIN);
	out.WriteCompressed(uniqueID);
	if (bitStream)
	{
		bitStream->ResetReadPointer();
		out.Write(bitStream);
	}
	SendUnified(&out,priority,reliability,orderingChannel,systemIdentifier,broadcast);
}
PluginReceiveResult RPC4Plugin::OnReceive(Packet *packet)
{
	if (packet->data[0]==ID_RPC_4_PLUGIN)
	{
		RakNet::BitStream bsIn(packet->data,packet->length,false);
		bsIn.IgnoreBytes(1);
		RakNet::RakString functionName;
		bsIn.ReadCompressed(functionName);
		DataStructures::StringKeyedHashIndex skhi = registeredFunctions.GetIndexOf(functionName.C_String());
		if (skhi.IsInvalid())
		{
			RakNet::BitStream bsOut;
			bsOut.Write((unsigned char) ID_RPC_REMOTE_ERROR);
			bsOut.Write((unsigned char) RPC_ERROR_FUNCTION_NOT_REGISTERED);
			bsOut.Write(functionName.C_String(),functionName.GetLength()+1);
			SendUnified(&bsOut,HIGH_PRIORITY,RELIABLE_ORDERED,0,packet->systemAddress,false);
			return RR_STOP_PROCESSING_AND_DEALLOCATE;
		}

		void ( *fp ) ( RakNet::BitStream *, Packet * );
		fp = registeredFunctions.ItemAtIndex(skhi);
		fp(&bsIn,packet);
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}

	return RR_CONTINUE_PROCESSING;
}

#endif // _RAKNET_SUPPORT_*
