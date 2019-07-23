#include "Lobby2Presence.h"
#include "BitStream.h"

using namespace RakNet;

Lobby2Presence::Lobby2Presence()
{
	status=UNDEFINED;
	isVisible=true;
}
Lobby2Presence::~Lobby2Presence()
{

}
void Lobby2Presence::Serialize(RakNet::BitStream *bitStream, bool writeToBitstream)
{
	unsigned char gs = (unsigned char) status;
	bitStream->Serialize(writeToBitstream,gs);
	status=(Status) gs;
	bitStream->Serialize(writeToBitstream,isVisible);
	bitStream->Serialize(writeToBitstream,titleName);
	bitStream->Serialize(writeToBitstream,statusString);
}
