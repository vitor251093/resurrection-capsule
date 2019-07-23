#ifndef __RAKNET_SOCKET_H
#define __RAKNET_SOCKET_H

#include "RakNetTypes.h"
#include "RakNetDefines.h"
#include "Export.h"
// #include "SocketIncludes.h"

struct RAK_DLL_EXPORT RakNetSocket
{
	RakNetSocket();
	~RakNetSocket();
	// SocketIncludes.h includes Windows.h, which messes up a lot of compiles
	// SOCKET s;
	unsigned int s;
	unsigned int userConnectionSocketIndex;
	SystemAddress boundAddress;
#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	void* recvEvent;
#endif
	// Only need to set for the PS3, when using signaling.
	// Connect with the port returned by signaling. Set this to whatever port RakNet was actually started on
	unsigned short remotePortRakNetWasStartedOn_PS3;

};

#endif
