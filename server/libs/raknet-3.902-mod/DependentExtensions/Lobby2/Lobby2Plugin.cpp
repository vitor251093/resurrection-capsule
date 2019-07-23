#include "Lobby2Plugin.h"

using namespace RakNet;

Lobby2Plugin::Lobby2Plugin()
{
	orderingChannel=0;
	packetPriority=HIGH_PRIORITY;
}
Lobby2Plugin::~Lobby2Plugin()
{

}
void Lobby2Plugin::SetOrderingChannel(char oc)
{
	orderingChannel=oc;
}
void Lobby2Plugin::SetSendPriority(PacketPriority pp)
{
	packetPriority=pp;
}
void Lobby2Plugin::SetMessageFactory(Lobby2MessageFactory *f)
{
	msgFactory=f;
}
Lobby2MessageFactory* Lobby2Plugin::GetMessageFactory(void) const
{
	return msgFactory;
}
void Lobby2Plugin::SetCallbackInterface(Lobby2Callbacks *cb)
{
	ClearCallbackInterfaces();
	callbacks.Insert(cb, __FILE__, __LINE__ );
}
void Lobby2Plugin::AddCallbackInterface(Lobby2Callbacks *cb)
{
	RemoveCallbackInterface(cb);
	callbacks.Insert(cb, __FILE__, __LINE__ );
}
void Lobby2Plugin::RemoveCallbackInterface(Lobby2Callbacks *cb)
{
	unsigned long index = callbacks.GetIndexOf(cb);
	if (index!=MAX_UNSIGNED_LONG)
		callbacks.RemoveAtIndex(index);
}
void Lobby2Plugin::ClearCallbackInterfaces()
{
	callbacks.Clear(false, __FILE__, __LINE__);
}