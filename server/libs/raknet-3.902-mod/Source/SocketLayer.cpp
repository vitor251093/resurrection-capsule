/// \file
/// \brief SocketLayer class implementation
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "SocketLayer.h"
#include "RakAssert.h"
#include "RakNetTypes.h"
#include "GetTime.h"

#if USE_SLIDING_WINDOW_CONGESTION_CONTROL!=1
#include "CCRakNetUDT.h"
#else
#include "CCRakNetSlidingWindow.h"
#endif


SocketLayerOverride *SocketLayer::slo=0;

#ifdef _WIN32
#else
#include <string.h> // memcpy
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>  // error numbers
#include <stdio.h> // RAKNET_DEBUG_PRINTF
#if !defined(ANDROID)
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#endif










#if   defined(_WIN32)
#include "WSAStartupSingleton.h"
#include <ws2tcpip.h> // 'IP_DONTFRAGMENT' 'IP_TTL'
#elif defined(_PS3) || defined(__PS3__) || defined(SN_TARGET_PS3)
#define closesocket socketclose
#else
#define closesocket close
#include <unistd.h>
#endif

#include "RakSleep.h"
#include <stdio.h>

#include "ExtendedOverlappedPool.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

extern void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetSmartPtr<RakNetSocket> rakNetSocket, RakNetTimeUS timeRead );
extern void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetTimeUS timeRead );
extern void ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );

#ifdef _DEBUG
#include <stdio.h>
#endif

// Frogwares: Define this
// #define DEBUG_SENDTO_SPIKES


SOCKET SocketLayer::Connect( SOCKET writeSocket, unsigned int binaryAddress, unsigned short port )
{
	RakAssert( writeSocket != (SOCKET) -1 );
	sockaddr_in connectSocketAddress;
	memset(&connectSocketAddress,0,sizeof(sockaddr_in));

	connectSocketAddress.sin_family = AF_INET;
	connectSocketAddress.sin_port = htons( port );
	connectSocketAddress.sin_addr.s_addr = binaryAddress;

	if ( connect( writeSocket, ( struct sockaddr * ) & connectSocketAddress, sizeof( struct sockaddr ) ) != 0 )
	{
#if defined(_WIN32) &&  defined(_DEBUG) 
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) &messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "WSAConnect failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	return writeSocket;
}
bool SocketLayer::IsPortInUse(unsigned short port, const char *hostAddress)
{
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	memset(&listenerSocketAddress,0,sizeof(sockaddr_in));
	// Listen on our designated Port#
	listenerSocketAddress.sin_port = htons( port );
	listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( listenSocket == (SOCKET) -1 )
		return true;
	// bind our name to the socket
	// Fill in the rest of the address structure
	listenerSocketAddress.sin_family = AF_INET;
	if ( hostAddress && hostAddress[0] )
		listenerSocketAddress.sin_addr.s_addr = inet_addr( hostAddress );
	else
		listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;
	int ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( listenerSocketAddress ) );
	closesocket(listenSocket);




	#if defined(_DEBUG)
	if (ret == -1)
		perror("Failed to bind to address:");
	#endif
	return ret <= -1;

}
void SocketLayer::SetDoNotFragment( SOCKET listenSocket, int opt )
{

#if defined(IP_DONTFRAGMENT )

#if defined(_WIN32) &&  defined(_DEBUG) 
	// If this assert hit you improperly linked against WSock32.h
	RakAssert(IP_DONTFRAGMENT==14);
#endif

	if ( setsockopt( listenSocket, IPPROTO_IP, IP_DONTFRAGMENT, ( char * ) & opt, sizeof ( opt ) ) == -1 )
	{
#if defined(_WIN32) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		RAKNET_DEBUG_PRINTF( "setsockopt(IP_DONTFRAGMENT) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		LocalFree( messageBuffer );
#endif
	}
#endif

}

void SocketLayer::SetNonBlocking( SOCKET listenSocket)
{
#ifdef _WIN32
	unsigned long nonBlocking = 1;
	ioctlsocket( listenSocket, FIONBIO, &nonBlocking );



#else
	int flags = fcntl(listenSocket, F_GETFL, 0);
	fcntl(listenSocket, F_SETFL, flags | O_NONBLOCK);
#endif
}

void SocketLayer::SetSocketOptions( SOCKET listenSocket)
{
	int sock_opt = 1;
	// // On Vista, can get WSAEACCESS (10013)
	/*
	if ( setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( char * ) & sock_opt, sizeof ( sock_opt ) ) == -1 )
	{
	#if defined(_WIN32) && !defined(_XBOX) && defined(_DEBUG) && !defined(X360)
	DWORD dwIOError = GetLastError();
	LPVOID messageBuffer;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
	( LPTSTR ) & messageBuffer, 0, NULL );
	// something has gone wrong here...
	RAKNET_DEBUG_PRINTF( "setsockopt(SO_REUSEADDR) failed:Error code - %d\n%s", dwIOError, messageBuffer );
	//Free the buffer.
	LocalFree( messageBuffer );
	#endif
	}
	*/

	// This doubles the max throughput rate
	sock_opt=1024*256;
	setsockopt(listenSocket, SOL_SOCKET, SO_RCVBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );

	// Immediate hard close. Don't linger the socket, or recreating the socket quickly on Vista fails.
	sock_opt=0;
	setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, ( char * ) & sock_opt, sizeof ( sock_opt ) );


	// This doesn't make much difference: 10% maybe
	// Not supported on console 2
	sock_opt=1024*16;
	setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );


	/*
	#ifdef _WIN32
		unsigned long nonblocking = 1;
		ioctlsocket( listenSocket, FIONBIO, &nonblocking );
	#elif defined(_PS3) || defined(__PS3__) || defined(SN_TARGET_PS3)
		sock_opt=1;
		setsockopt(listenSocket, SOL_SOCKET, SO_NBIO, ( char * ) & sock_opt, sizeof ( sock_opt ) );
	#else
		fcntl( listenSocket, F_SETFL, O_NONBLOCK );
	#endif
	*/

	// Set broadcast capable
	sock_opt=1;
	if ( setsockopt( listenSocket, SOL_SOCKET, SO_BROADCAST, ( char * ) & sock_opt, sizeof( sock_opt ) ) == -1 )
		{
#if defined(_WIN32) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		// On Vista, can get WSAEACCESS (10013)
		// See http://support.microsoft.com/kb/819124
		// http://blogs.msdn.com/wndp/archive/2007/03/19/winsock-so-exclusiveaddruse-on-vista.aspx
		// http://msdn.microsoft.com/en-us/library/ms740621(VS.85).aspx

		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "setsockopt(SO_BROADCAST) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );

#endif

		}
}
SOCKET SocketLayer::CreateBoundSocket_PS3Lobby( unsigned short port, bool blockingSocket, const char *forceHostAddress )
{
	(void) port;
	(void) blockingSocket;
	(void) forceHostAddress;





































































	return 0;

}
SOCKET SocketLayer::CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress, unsigned int sleepOn10048 )
{
	(void) blockingSocket;

	int ret;
	SOCKET listenSocket;
	sockaddr_in listenerSocketAddress;
	memset(&listenerSocketAddress,0,sizeof(sockaddr_in));
	// Listen on our designated Port#
	listenerSocketAddress.sin_port = htons( port );



	listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );


	if ( listenSocket == (SOCKET) -1 )
	{
#if defined(_WIN32) &&  defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "socket(...) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif

		return (SOCKET) -1;
	}

	SetSocketOptions(listenSocket);

	// Fill in the rest of the address structure
	listenerSocketAddress.sin_family = AF_INET;

	if (forceHostAddress && forceHostAddress[0])
	{
//		printf("Force binding %s:%i\n", forceHostAddress, port);
		listenerSocketAddress.sin_addr.s_addr = inet_addr( forceHostAddress );
	}
	else
	{
//		printf("Binding any on port %i\n", port);
		listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;
	}

	// bind our name to the socket
	ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( listenerSocketAddress ) );

	if ( ret <= -1 )
	{
#if defined(_WIN32) 
		DWORD dwIOError = GetLastError();
		if (dwIOError==10048)
		{
			if (sleepOn10048==0)
				return (SOCKET) -1;
			// Vista has a bug where it returns WSAEADDRINUSE (10048) if you create, shutdown, then rebind the socket port unless you wait a while first.
			// Wait, then rebind
			RakSleep(100);

			closesocket(listenSocket);
			listenerSocketAddress.sin_port = htons( port );
			listenSocket = socket( AF_INET, SOCK_DGRAM, 0 );
			if ( listenSocket == (SOCKET) -1 )
				return false;
			SetSocketOptions(listenSocket);

			// Fill in the rest of the address structure
			listenerSocketAddress.sin_family = AF_INET;
			if (forceHostAddress && forceHostAddress[0])
				listenerSocketAddress.sin_addr.s_addr = inet_addr( forceHostAddress );
			else
				listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;

			// bind our name to the socket
			ret = bind( listenSocket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( listenerSocketAddress ) );

			if ( ret >= 0 )
				return listenSocket;
		}
		dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "bind(...) failed:Error code - %d\n%s", (unsigned int) dwIOError, (char*) messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#elif (defined(__GNUC__)  || defined(__GCCXML__)  ) && !defined(__WIN32)
		switch (ret)
		{
		case EBADF:
			RAKNET_DEBUG_PRINTF("bind(): sockfd is not a valid descriptor.\n"); break;

		case ENOTSOCK:
			RAKNET_DEBUG_PRINTF("bind(): Argument is a descriptor for a file, not a socket.\n"); break;

		case EINVAL:
			RAKNET_DEBUG_PRINTF("bind(): The addrlen is wrong, or the socket was not in the AF_UNIX family.\n"); break;
		case EROFS:
			RAKNET_DEBUG_PRINTF("bind(): The socket inode would reside on a read-only file system.\n"); break;
		case EFAULT:
			RAKNET_DEBUG_PRINTF("bind(): my_addr points outside the user's accessible address space.\n"); break;
		case ENAMETOOLONG:
			RAKNET_DEBUG_PRINTF("bind(): my_addr is too long.\n"); break;
		case ENOENT:
			RAKNET_DEBUG_PRINTF("bind(): The file does not exist.\n"); break;
		case ENOMEM:
			RAKNET_DEBUG_PRINTF("bind(): Insufficient kernel memory was available.\n"); break;
		case ENOTDIR:
			RAKNET_DEBUG_PRINTF("bind(): A component of the path prefix is not a directory.\n"); break;
		case EACCES:
			RAKNET_DEBUG_PRINTF("bind(): Search permission is denied on a component of the path prefix.\n"); break;

		case ELOOP:
			RAKNET_DEBUG_PRINTF("bind(): Too many symbolic links were encountered in resolving my_addr.\n"); break;

		default:
			RAKNET_DEBUG_PRINTF("Unknown bind() error %i.\n", ret); break;
		}
#endif

		return (SOCKET) -1;
	}

	return listenSocket;
}

const char* SocketLayer::DomainNameToIP( const char *domainName )
{
	struct in_addr addr;
































	struct hostent * phe = gethostbyname( domainName );

	if ( phe == 0 || phe->h_addr_list[ 0 ] == 0 )
	{
		//cerr << "Yow! Bad host lookup." << endl;
		return 0;
	}

	if (phe->h_addr_list[ 0 ]==0)
		return 0;

	memcpy( &addr, phe->h_addr_list[ 0 ], sizeof( struct in_addr ) );
	return inet_ntoa( addr );


//	return "";
}


void SocketLayer::Write( const SOCKET writeSocket, const char* data, const int length )
{
#ifdef _DEBUG
	RakAssert( writeSocket != (SOCKET) -1 );
#endif

	send( writeSocket, data, length, 0 );
}
int SocketLayer::RecvFrom( const SOCKET s, RakPeer *rakPeer, int *errorCode, RakNetSmartPtr<RakNetSocket> rakNetSocket, unsigned short remotePortRakNetWasStartedOn_PS3 )
{
	int len=0;




	char data[ MAXIMUM_MTU_SIZE ];


	if (slo)
	{
		SystemAddress sender;
		len = slo->RakNetRecvFrom(s,rakPeer,data,&sender,true);
		if (len>0)
		{
			ProcessNetworkPacket( sender, data, len, rakPeer, rakNetSocket, RakNet::GetTimeUS() );
			return 1;
		}
	}

	if ( s == (SOCKET) -1 )
	{
		*errorCode = -1;
		return -1;
	}

#if defined (_WIN32) || !defined(MSG_DONTWAIT)
	const int flag=0;
#else
	const int flag=MSG_DONTWAIT;
#endif

	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	socklen_t len2;
	unsigned short portnum=0;
	if (remotePortRakNetWasStartedOn_PS3!=0)
	{








	}
	else
	{
		len2 = sizeof( sa );
		sa.sin_family = AF_INET;
		sa.sin_port=0;


























		len = recvfrom( s, data, MAXIMUM_MTU_SIZE, flag, ( sockaddr* ) & sa, ( socklen_t* ) & len2 );


		portnum = ntohs( sa.sin_port );
	}

	if ( len == 0 )
	{
#ifdef _DEBUG
		RAKNET_DEBUG_PRINTF( "Error: recvfrom returned 0 on a connectionless blocking call\non port %i.  This is a bug with Zone Alarm.  Please turn off Zone Alarm.\n", portnum );
		RakAssert( 0 );
#endif

		// 4/13/09 Changed from returning -1 to 0, to prevent attackers from sending 0 byte messages to shutdown the server
		*errorCode = 0;
		return 0;
	}

	if ( len > 0 )
	{
		ProcessNetworkPacket( SystemAddress(sa.sin_addr.s_addr, portnum), data, len, rakPeer, rakNetSocket, RakNet::GetTimeUS() );

		return 1;
	}
	else
	{
		*errorCode = 0;


#if defined(_WIN32) && defined(_DEBUG)

		DWORD dwIOError = WSAGetLastError();

		if ( dwIOError == WSAEWOULDBLOCK )
		{
			return SOCKET_ERROR;
		}
		if ( dwIOError == WSAECONNRESET )
		{
#if defined(_DEBUG)
//			RAKNET_DEBUG_PRINTF( "A previous send operation resulted in an ICMP Port Unreachable message.\n" );
#endif


			unsigned short portnum=0;
			ProcessPortUnreachable(sa.sin_addr.s_addr, portnum, rakPeer);
			// *errorCode = dwIOError;
			return -1;
		}
		else
		{
#if defined(_DEBUG) 
			if ( dwIOError != WSAEINTR && dwIOError != WSAETIMEDOUT)
			{
				LPVOID messageBuffer;
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
					( LPTSTR ) & messageBuffer, 0, NULL );
				// something has gone wrong here...
				RAKNET_DEBUG_PRINTF( "recvfrom failed:Error code - %d\n%s", dwIOError, messageBuffer );

				//Free the buffer.
				LocalFree( messageBuffer );
			}
#endif
		}
#endif
	}

	return 0; // no data
}
void SocketLayer::RecvFromBlocking( const SOCKET s, RakPeer *rakPeer, unsigned short remotePortRakNetWasStartedOn_PS3, char *dataOut, int *bytesReadOut, SystemAddress *systemAddressOut, RakNetTimeUS *timeRead )
{
	(void) rakPeer;
	// Randomly crashes, slo is 0, yet gets inside loop.
	/*
	if (SocketLayer::slo)
	{
		SystemAddress sender;
		*bytesReadOut = SocketLayer::slo->RakNetRecvFrom(s,rakPeer,dataOut,systemAddressOut,false);
		if (*bytesReadOut>0)
		{
			*timeRead=RakNet::GetTimeUS();
			return;
		}
		else if (*bytesReadOut==0)
		{
			return;
		}
		// Negative, process as normal
	}
	*/


	sockaddr* sockAddrPtr;
	socklen_t sockLen;
	socklen_t* socketlenPtr=(socklen_t*) &sockLen;
	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	char *dataOutModified;
	int dataOutSize;
	const int flag=0;

	(void) remotePortRakNetWasStartedOn_PS3;











	{
		sockLen=sizeof(sa);
		sa.sin_family = AF_INET;
		sa.sin_port=0;
		sockAddrPtr=(sockaddr*) &sa;
	}





	dataOutModified=dataOut;
	dataOutSize=MAXIMUM_MTU_SIZE;

	*bytesReadOut = recvfrom( s, dataOutModified, dataOutSize, flag, sockAddrPtr, socketlenPtr );
	if (*bytesReadOut<=0)
	{
		/*
#if defined(_WIN32) && !defined(_XBOX) && !defined(X360) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "sendto failed:Error code - %d\n%s", dwIOError, messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif
		*/
		return;
	}
	*timeRead=RakNet::GetTimeUS();
	








	{
		systemAddressOut->port=ntohs( sa.sin_port );
		systemAddressOut->binaryAddress=sa.sin_addr.s_addr;
	}

// 	if (systemAddressOut->port!=61111)
// 		printf("Got %i bytes from %s\n", *bytesReadOut, systemAddressOut->ToString());

}

void SocketLayer::RawRecvFromNonBlocking( const SOCKET s, unsigned short remotePortRakNetWasStartedOn_PS3, char *dataOut, int *bytesReadOut, SystemAddress *systemAddressOut, RakNetTimeUS *timeRead )
{
	
	sockaddr* sockAddrPtr;
	socklen_t sockLen;
	socklen_t* socketlenPtr=(socklen_t*) &sockLen;
	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	char *dataOutModified;
	int dataOutSize;
	const int flag=0;

	(void) remotePortRakNetWasStartedOn_PS3;

// This is the wrong place for this - call on the socket before calling the function
// 	#if defined(_WIN32)
// 	u_long val = 1;
// 	ioctlsocket (s,FIONBIO,&val);//non block
// 	#else
// 	int flags = fcntl(s, F_GETFL, 0);
// 	fcntl(s, F_SETFL, flags | O_NONBLOCK);
// 	#endif











	{
		sockLen=sizeof(sa);
		sa.sin_family = AF_INET;
		sa.sin_port=0;
		sockAddrPtr=(sockaddr*) &sa;
	}





	dataOutModified=dataOut;
	dataOutSize=MAXIMUM_MTU_SIZE;


	*bytesReadOut = recvfrom( s, dataOutModified, dataOutSize, flag, sockAddrPtr, socketlenPtr );
	if (*bytesReadOut<=0)
	{
		return;
	}
	*timeRead=RakNet::GetTimeUS();
	








	{
		systemAddressOut->port=ntohs( sa.sin_port );
		systemAddressOut->binaryAddress=sa.sin_addr.s_addr;
	}
}

int SocketLayer::SendTo_PS3Lobby( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3 )
{
	(void) s;
	(void) data;
	(void) length;
	(void) binaryAddress;
	(void) port;
	(void) remotePortRakNetWasStartedOn_PS3;

	int len=0;














	return len;
}
int SocketLayer::SendTo_360( SOCKET s, const char *data, int length, const char *voiceData, int voiceLength, unsigned int binaryAddress, unsigned short port )
{
	(void) s;
	(void) data;
	(void) length;
	(void) voiceData;
	(void) voiceLength;
	(void) binaryAddress;
	(void) port;

	int len=0;


























	return len;
}
int SocketLayer::SendTo_PC( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, const char *file, const long line )
{
// 	if (port!=61111)
// 	{
// 		SystemAddress testa(binaryAddress,port);
// 		printf("Sending %i bytes to %s\n", length, testa.ToString());
// 	}


	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	sa.sin_port = htons( port ); // User port
	sa.sin_addr.s_addr = binaryAddress;
	sa.sin_family = AF_INET;
	int len=0;
	do
	{
#ifdef DEBUG_SENDTO_SPIKES
		RakNetTime start = RakNet::GetTime();
#else
		(void) file;
		(void) line;
#endif
		len = sendto( s, data, length, 0, ( const sockaddr* ) & sa, sizeof( sa ) );
#ifdef DEBUG_SENDTO_SPIKES
		RakNetTime end = RakNet::GetTime();
		static unsigned int callCount=1;
		printf("%i. SendTo_PC, time=%"PRINTF_64_BIT_MODIFIER"u, elapsed=%"PRINTF_64_BIT_MODIFIER"u, length=%i, returned=%i, binaryAddress=%i, port=%i, file=%s, line=%i\n", callCount++, end, end-start, length, len, binaryAddress, port, file, line);
#endif
		if (len<0)
		{

#if defined(_WIN32) 
			DWORD dwIOError = GetLastError();
			if (dwIOError!= 10040 && dwIOError != WSAEADDRNOTAVAIL)
			{
	#if defined(_DEBUG)
				LPVOID messageBuffer;
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
					( LPTSTR ) &messageBuffer, 0, NULL );
				// something has gone wrong here...
				RAKNET_DEBUG_PRINTF( "SendTo_PC failed:Error code - %d\n%s", dwIOError, messageBuffer );
				//Free the buffer.
				LocalFree( messageBuffer );
	#endif
			}
			else
			{
				// buffer size exceeded
				return -10040;
			}
#endif

			printf("sendto failed with code %i for char %i and length %i.\n", len, data[0], length);
		}
	}
	while ( len == 0 );
	return len;
}

#ifdef _MSC_VER
#pragma warning( disable : 4702 ) // warning C4702: unreachable code
#endif
int SocketLayer::SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3, const char *file, const long line )
{
	RakAssert(length<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
	RakAssert(port!=0);
	if (slo)
	{
		SystemAddress sa(binaryAddress,port);
		return slo->RakNetSendTo(s,data,length,sa);
	}

	if ( s == (SOCKET) -1 )
	{
		return -1;
	}

	int len=0;

	if (remotePortRakNetWasStartedOn_PS3!=0)
	{
		len = SendTo_PS3Lobby(s,data,length,binaryAddress,port, remotePortRakNetWasStartedOn_PS3);
	}
	else
	{




		len = SendTo_PC(s,data,length,binaryAddress,port,file,line);

	}

	if ( len != -1 )
		return 0;

#if defined(_WIN32) && !defined(_WIN32_WCE)

	DWORD dwIOError = WSAGetLastError();

	if ( dwIOError == WSAECONNRESET )
	{
#if defined(_DEBUG)
		RAKNET_DEBUG_PRINTF( "A previous send operation resulted in an ICMP Port Unreachable message.\n" );
#endif

	}
	else if ( dwIOError != WSAEWOULDBLOCK && dwIOError != WSAEADDRNOTAVAIL)
	{
#if defined(_WIN32) &&  defined(_DEBUG)
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "sendto failed:Error code - %d\n%s", dwIOError, messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif

	}

	return dwIOError;
#endif

	return 1; // error
}
int SocketLayer::SendTo( SOCKET s, const char *data, int length, const char ip[ 16 ], unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3, const char *file, const long line )
{
	unsigned int binaryAddress;
	binaryAddress = inet_addr( ip );
	return SendTo( s, data, length, binaryAddress, port,remotePortRakNetWasStartedOn_PS3, file, line );
}
int SocketLayer::SendToTTL( SOCKET s, const char *data, int length, const char ip[ 16 ], unsigned short port, int ttl )
{
	unsigned int binaryAddress;
	binaryAddress = inet_addr( ip );
	SystemAddress sa(binaryAddress,port);

	if (slo)
		return slo->RakNetSendTo(s,data,length,sa);


	int oldTTL;
	socklen_t opLen=sizeof(oldTTL);
	// Get the current TTL
	if (getsockopt(s, IPPROTO_IP, IP_TTL, ( char * ) & oldTTL, &opLen ) == -1)
	{
#if defined(_WIN32) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "getsockopt(IPPROTO_IP,IP_TTL) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	// Set to TTL
	int newTTL=ttl;
	if (setsockopt(s, IPPROTO_IP, IP_TTL, ( char * ) & newTTL, sizeof ( newTTL ) ) == -1)
	{

#if defined(_WIN32) && defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "setsockopt(IPPROTO_IP,IP_TTL) failed:Error code - %d\n%s", dwIOError, messageBuffer );
		//Free the buffer.
		LocalFree( messageBuffer );
#endif
	}

	// Send
	int res = SendTo(s,data,length,ip,port,false, __FILE__, __LINE__ );

	// Restore the old TTL
	setsockopt(s, IPPROTO_IP, IP_TTL, ( char * ) & oldTTL, opLen );

	return res;



}


RakNet::RakString SocketLayer::GetSubNetForSocketAndIp(SOCKET inSock, RakNet::RakString inIpString)
{
	RakNet::RakString netMaskString;
	RakNet::RakString ipString;





#if   defined(_WIN32)
	INTERFACE_INFO InterfaceList[20];
	unsigned long nBytesReturned;
	if (WSAIoctl(inSock, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
		sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) {
			return "";
	}

	int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);

	for (int i = 0; i < nNumInterfaces; ++i)
	{
		sockaddr_in *pAddress;
		pAddress = (sockaddr_in *) & (InterfaceList[i].iiAddress);
		ipString=inet_ntoa(pAddress->sin_addr);

		if (inIpString==ipString)
		{
			pAddress = (sockaddr_in *) & (InterfaceList[i].iiNetmask);
			netMaskString=inet_ntoa(pAddress->sin_addr);
			return netMaskString;
		}
	}
	return "";
#else

	int fd,fd2;
	fd2 = socket(AF_INET, SOCK_DGRAM, 0);

	if(fd2 < 0)
	{
		return "";
	}

	struct ifconf ifc;
	char          buf[1999];
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if(ioctl(fd2, SIOCGIFCONF, &ifc) < 0)
	{
		return "";
	}

	struct ifreq *ifr;
	ifr         = ifc.ifc_req;
	int intNum = ifc.ifc_len / sizeof(struct ifreq);
	for(int i = 0; i < intNum; i++)
	{
		ipString=inet_ntoa(((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr);

		if (inIpString==ipString)
		{
			struct ifreq ifr2;
			fd = socket(AF_INET, SOCK_DGRAM, 0);
			if(fd < 0)
			{
				return "";
			}
			ifr2.ifr_addr.sa_family = AF_INET;

			strncpy(ifr2.ifr_name, ifr[i].ifr_name, IFNAMSIZ-1);

			ioctl(fd, SIOCGIFNETMASK, &ifr2);

			close(fd);
			close(fd2);
			netMaskString=inet_ntoa(((struct sockaddr_in *)&ifr2.ifr_addr)->sin_addr);

			return netMaskString;
		}
	}

	close(fd2);
	return "";

#endif

}










#if   defined(_WIN32)
void GetMyIP_Win32( char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] )
{
	char ac[ 80 ];
	if ( gethostname( ac, sizeof( ac ) ) == -1 )
	{
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "gethostname failed:Error code - %d\n%s", dwIOError, reinterpret_cast<char*>(messageBuffer) );
		//Free the buffer.
		LocalFree( messageBuffer );
		return ;
	}

	struct hostent *phe = gethostbyname( ac );

	if ( phe == 0 )
	{
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "gethostbyname failed:Error code - %d\n%s", dwIOError, reinterpret_cast<char*>(messageBuffer));

		//Free the buffer.
		LocalFree( messageBuffer );
		return ;
	}

	struct in_addr addr[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ];
	int idx;
	for ( idx = 0; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
	{
		if (phe->h_addr_list[ idx ] == 0)
			break;

		memcpy( &addr[idx], phe->h_addr_list[ idx ], sizeof( struct in_addr ) );
		binaryAddresses[idx]=addr[idx].S_un.S_addr;
		strcpy( ipList[ idx ], inet_ntoa( addr[idx] ) );

	}

	for ( ; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
	{
		ipList[idx][0]=0;
	}
}
#elif defined(ANDROID)
void GetMyIP_Linux( char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] )
{
	struct ifreq ifreqs[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
	struct ifconf ifconf;
	struct in_addr bin_addr;

	memset(&ifconf,0,sizeof(ifconf));
	ifconf.ifc_buf = (char*) (ifreqs);
	ifconf.ifc_len = sizeof(ifreqs);

	{ // get list of interfaces
		int sock = socket(AF_INET,SOCK_STREAM,0);
		if (sock < 0)
		{
			perror("Error creating socket");
			return;
		}
		if ((ioctl(sock, SIOCGIFCONF, (char*) &ifconf  )) < 0 )
		{
			perror("Error returned from ioctl(SIOGIFCONF)");
			ifconf.ifc_len = 0;
		}
		close(sock);
	}

	int idx = 0;
	int iface_count =  ifconf.ifc_len/sizeof(struct ifreq);
	printf("Interfaces (%d):\n", iface_count);
	for( ; idx < iface_count; idx++)
	{
		char ip_addr[ 16/*INET_ADDRSTRLEN */];
		struct sockaddr_in *b = (struct sockaddr_in *) &(ifreqs[idx].ifr_addr);
		const char* host = inet_ntop(AF_INET, & b->sin_addr, ip_addr, sizeof ip_addr);
		strcpy( ipList[idx], host );
		if (inet_aton(host, &bin_addr) == 0)
		{
			perror("inet_aton error");
			continue;
		}
		else
		{
			binaryAddresses[idx] = bin_addr.s_addr;
		}
		printf("\t%-10s\t%s (%08x)\n", ifreqs[idx].ifr_name, ipList[idx], binaryAddresses[idx]);
	}
	for ( ; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
	{
		ipList[idx][0]=0;
	}
}
#else
void GetMyIP_Linux( char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] )
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s;
	char host[NI_MAXHOST];
	struct in_addr linux_in_addr;

	if (getifaddrs(&ifaddr) == -1) {
		printf( "Error getting interface list\n");
	}

	int idx = 0;
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) continue;
		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET) {
			s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				printf ("getnameinfo() failed: %s\n", gai_strerror(s));
			}
			//printf ("IP address: %s\n", host);
			if (strcmp(host,"127.0.0.1")!=0)
			{
				strcpy( ipList[ idx ], host );
				if (inet_aton(host, &linux_in_addr) == 0) {
					perror("inet_aton");
				}
				else {
					binaryAddresses[idx]=linux_in_addr.s_addr;
				}

				idx++;
			}
		}
	}

	for ( ; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
	{
		ipList[idx][0]=0;
	}

	freeifaddrs(ifaddr);
}
#endif


void SocketLayer::GetMyIP( char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] )
{


#if   defined(_WIN32)
	GetMyIP_Win32(ipList, binaryAddresses);
#else
	GetMyIP_Linux(ipList, binaryAddresses);
#endif
}


unsigned short SocketLayer::GetLocalPort ( SOCKET s )
{
	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	socklen_t len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len)!=0)
	{
#if defined(_WIN32) &&  defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "getsockname failed:Error code - %d\n%s", dwIOError, messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif
		return 0;
	}
	return ntohs(sa.sin_port);
}

SystemAddress SocketLayer::GetSystemAddress ( SOCKET s )
{
	sockaddr_in sa;
	memset(&sa,0,sizeof(sockaddr_in));
	socklen_t len = sizeof(sa);
	if (getsockname(s, (sockaddr*)&sa, &len)!=0)
	{
#if defined(_WIN32) &&  defined(_DEBUG)
		DWORD dwIOError = GetLastError();
		LPVOID messageBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwIOError, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),  // Default language
			( LPTSTR ) & messageBuffer, 0, NULL );
		// something has gone wrong here...
		RAKNET_DEBUG_PRINTF( "getsockname failed:Error code - %d\n%s", dwIOError, messageBuffer );

		//Free the buffer.
		LocalFree( messageBuffer );
#endif
		return UNASSIGNED_SYSTEM_ADDRESS;
	}

	SystemAddress out;
	out.port=ntohs(sa.sin_port);
	out.binaryAddress=sa.sin_addr.s_addr;
	return out;
}

void SocketLayer::SetSocketLayerOverride(SocketLayerOverride *_slo)
{
	slo=_slo;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif
