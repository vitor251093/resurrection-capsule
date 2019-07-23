#include "RakNetTransport2.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "LinuxStrings.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

RakNetTransport2::RakNetTransport2()
{
}
RakNetTransport2::~RakNetTransport2()
{
	Stop();
}
bool RakNetTransport2::Start(unsigned short port, bool serverMode)
{
	(void) port;
	(void) serverMode;
	return true;
}
void RakNetTransport2::Stop(void)
{
	newConnections.Clear(__FILE__, __LINE__);
	lostConnections.Clear(__FILE__, __LINE__);
	for (unsigned int i=0; i < packetQueue.Size(); i++)
	{
		rakFree_Ex(packetQueue[i]->data,__FILE__,__LINE__);
		RakNet::OP_DELETE(packetQueue[i],__FILE__,__LINE__);
	}
	packetQueue.Clear(__FILE__, __LINE__);
}
void RakNetTransport2::Send( SystemAddress systemAddress, const char *data, ... )
{
	if (data==0 || data[0]==0) return;

	char text[REMOTE_MAX_TEXT_INPUT];
	va_list ap;
	va_start(ap, data);
	_vsnprintf(text, REMOTE_MAX_TEXT_INPUT, data, ap);
	va_end(ap);
	text[REMOTE_MAX_TEXT_INPUT-1]=0;

	RakNet::BitStream str;
	str.Write((MessageID)ID_TRANSPORT_STRING);
	str.Write(text, (int) strlen(text));
	str.Write((unsigned char) 0); // Null terminate the string
	rakPeerInterface->Send(&str, MEDIUM_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, (systemAddress==UNASSIGNED_SYSTEM_ADDRESS)!=0);
}
void RakNetTransport2::CloseConnection( SystemAddress systemAddress )
{
	rakPeerInterface->CloseConnection(systemAddress, true, 0);
}
Packet* RakNetTransport2::Receive( void )
{
	if (packetQueue.Size()==0)
		return 0;
	return packetQueue.Pop();
}
SystemAddress RakNetTransport2::HasNewIncomingConnection(void)
{
	if (newConnections.Size())
		return newConnections.Pop();
	return UNASSIGNED_SYSTEM_ADDRESS;
}
SystemAddress RakNetTransport2::HasLostConnection(void)
{
	if (lostConnections.Size())
		return lostConnections.Pop();
	return UNASSIGNED_SYSTEM_ADDRESS;
}
void RakNetTransport2::DeallocatePacket( Packet *packet )
{
	rakFree_Ex(packet->data, __FILE__, __LINE__ );
	RakNet::OP_DELETE(packet, __FILE__, __LINE__ );
}
PluginReceiveResult RakNetTransport2::OnReceive(Packet *packet)
{
	switch (packet->data[0])
	{
	case ID_TRANSPORT_STRING:
		{
			if (packet->length==sizeof(MessageID))
				return RR_STOP_PROCESSING_AND_DEALLOCATE;

			Packet *p = RakNet::OP_NEW<Packet>(__FILE__,__LINE__);
			*p=*packet;
			p->bitSize-=8;
			p->length--;
			p->data=(unsigned char*) rakMalloc_Ex(p->length,__FILE__,__LINE__);
			memcpy(p->data, packet->data+1, p->length);
			packetQueue.Push(p, __FILE__, __LINE__ );

		}
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	}
	return RR_CONTINUE_PROCESSING;
}
void RakNetTransport2::OnClosedConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, PI2_LostConnectionReason lostConnectionReason )
{
	(void) rakNetGUID;
	(void) lostConnectionReason;
	lostConnections.Push(systemAddress, __FILE__, __LINE__ );
}
void RakNetTransport2::OnNewConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID, bool isIncoming)
{
	(void) rakNetGUID;
	(void) isIncoming;
	newConnections.Push(systemAddress, __FILE__, __LINE__ );
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif
