/// \file
/// \brief SocketLayer class implementation
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.



#ifndef __SOCKET_LAYER_H
#define __SOCKET_LAYER_H

#include "RakMemoryOverride.h"
#include "SocketIncludes.h"
#include "RakNetTypes.h"
#include "RakNetSmartPtr.h"
#include "RakNetSocket.h"
#include "Export.h"
#include "MTUSize.h"
#include "RakString.h"

//#include "ClientContextStruct.h"

class RakPeer;

class RAK_DLL_EXPORT SocketLayerOverride
{
public:
	SocketLayerOverride() {}
	virtual ~SocketLayerOverride() {}

	/// Called when SendTo would otherwise occur.
	virtual int RakNetSendTo( SOCKET s, const char *data, int length, SystemAddress systemAddress )=0;

	/// Called when RecvFrom would otherwise occur. Return number of bytes read. Write data into dataOut
	// Return -1 to use RakNet's normal recvfrom, 0 to abort RakNet's normal recvfrom, and positive to return data
	virtual int RakNetRecvFrom( const SOCKET sIn, RakPeer *rakPeerIn, char dataOut[ MAXIMUM_MTU_SIZE ], SystemAddress *senderOut, bool calledFromMainThread )=0;
};


// A platform independent implementation of Berkeley sockets, with settings used by RakNet
class RAK_DLL_EXPORT SocketLayer
{

public:
	
	/// Default Constructor
	SocketLayer();
	
	// Destructor	
	~SocketLayer();
	
	// Connect a socket to a remote host.
	/// \param[in] writeSocket The local socket.
	/// \param[in] binaryAddress The address of the remote host.
	/// \param[in] port the remote port.
	/// \return A new socket used for communication.
	static SOCKET Connect( SOCKET writeSocket, unsigned int binaryAddress, unsigned short port );
	
	/// Creates a bound socket to listen for incoming connections on the specified port
	/// \param[in] port the port number 
	/// \param[in] blockingSocket 
	/// \return A new socket used for accepting clients 
	static SOCKET CreateBoundSocket( unsigned short port, bool blockingSocket, const char *forceHostAddress, unsigned int sleepOn10048 );
	static SOCKET CreateBoundSocket_PS3Lobby( unsigned short port, bool blockingSocket, const char *forceHostAddress );

	/// Returns if this specified port is in use, for UDP
	/// \param[in] port the port number 
	/// \return If this port is already in use
	static bool IsPortInUse(unsigned short port, const char *hostAddress=0);

	static const char* DomainNameToIP( const char *domainName );
	
	/// Start an asynchronous read using the specified socket.  The callback will use the specified SystemAddress (associated with this socket) and call either the client or the server callback (one or
	/// the other should be 0)
	/// \note Was used for deprecated IO completion ports.	
	static bool AssociateSocketWithCompletionPortAndRead( SOCKET readSocket, unsigned int binaryAddress, unsigned short port, RakPeer* rakPeer );
	
	/// Write \a data of length \a length to \a writeSocket
	/// \param[in] writeSocket The socket to write to
	/// \param[in] data The data to write
	/// \param[in] length The length of \a data	
	static void Write( const SOCKET writeSocket, const char* data, const int length );
	
	/// Read data from a socket 
	/// \param[in] s the socket 
	/// \param[in] rakPeer The instance of rakPeer containing the recvFrom C callback
	/// \param[in] errorCode An error code if an error occured .
	/// \param[in] connectionSocketIndex Which of the sockets in RakPeer we are using
	/// \return Returns true if you successfully read data, false on error.
	static int RecvFrom( const SOCKET s, RakPeer *rakPeer, int *errorCode, RakNetSmartPtr<RakNetSocket> rakNetSocket, unsigned short remotePortRakNetWasStartedOn_PS3 );
	// Newer version, for reads from a thread
	static void RecvFromBlocking( const SOCKET s, RakPeer *rakPeer, unsigned short remotePortRakNetWasStartedOn_PS3, char *dataOut, int *bytesReadOut, SystemAddress *systemAddressOut, RakNetTimeUS *timeRead );
	
	/// Read raw unprocessed data from the socket
	/// \param[in] s the socket 
	/// \param[in] remotePortRakNetWasStartedOn_PS3 was started on the PS3?
	/// \param[out] dataOut The data read
	/// \param[out]	bytesReadOut Number of bytes read, -1 if nothing was read
	/// \param[out] systemAddressOut Who is sending the packet
	/// \param[out]	timeRead Time that thre read occured
	static void RawRecvFromNonBlocking( const SOCKET s, unsigned short remotePortRakNetWasStartedOn_PS3, char *dataOut, int *bytesReadOut, SystemAddress *systemAddressOut, RakNetTimeUS *timeRead );


	/// Given a socket and IP, retrieves the subnet mask, on linux the socket is unused
	/// \param[in] inSock the socket 
	/// \param[in] inIpString The ip of the interface you wish to retrieve the subnet mask from
	/// \return Returns the ip dotted subnet mask if successful, otherwise returns empty string ("")
	static RakNet::RakString GetSubNetForSocketAndIp(SOCKET inSock, RakNet::RakString inIpString);


	/// Sets the socket flags to nonblocking 
	/// \param[in] listenSocket the socket to set
	static void SetNonBlocking( SOCKET listenSocket);


	/// Retrieve all local IP address in a string format.
	/// \param[in] s The socket whose port we are referring to
	/// \param[in] ipList An array of ip address in dotted notation.
	static void GetMyIP( char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] );

	
	/// Call sendto (UDP obviously)
	/// \param[in] s the socket
	/// \param[in] data The byte buffer to send 
	/// \param[in] length The length of the \a data in bytes
	/// \param[in] ip The address of the remote host in dotted notation.
	/// \param[in] port The port number to send to.
	/// \return 0 on success, nonzero on failure.
	static int SendTo( SOCKET s, const char *data, int length, const char ip[ 16 ], unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3, const char *file, const long line );

	/// Call sendto (UDP obviously)
	/// It won't reach the recipient, except on a LAN
	/// However, this is good for opening routers / firewalls
	/// \param[in] s the socket
	/// \param[in] data The byte buffer to send 
	/// \param[in] length The length of the \a data in bytes
	/// \param[in] ip The address of the remote host in dotted notation.
	/// \param[in] port The port number to send to.
	/// \param[in] ttl Max hops of datagram
	/// \return 0 on success, nonzero on failure.
	static int SendToTTL( SOCKET s, const char *data, int length, const char ip[ 16 ], unsigned short port, int ttl );

	/// Call sendto (UDP obviously)
	/// \param[in] s the socket
	/// \param[in] data The byte buffer to send 
	/// \param[in] length The length of the \a data in bytes
	/// \param[in] binaryAddress The address of the remote host in binary format.
	/// \param[in] port The port number to send to.
	/// \return 0 on success, nonzero on failure.
	static int SendTo( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3, const char *file, const long line );

	/// Returns the local port, useful when passing 0 as the startup port.
	/// \param[in] s The socket whose port we are referring to
	/// \return The local port
	static unsigned short GetLocalPort ( SOCKET s );

	static SystemAddress GetSystemAddress ( SOCKET s );

	static void SetSocketLayerOverride(SocketLayerOverride *_slo);
	static SocketLayerOverride* GetSocketLayerOverride(void) {return slo;}

	static int SendTo_PS3Lobby( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, unsigned short remotePortRakNetWasStartedOn_PS3 );
	static int SendTo_360( SOCKET s, const char *data, int length, const char *voiceData, int voiceLength, unsigned int binaryAddress, unsigned short port );
	static int SendTo_PC( SOCKET s, const char *data, int length, unsigned int binaryAddress, unsigned short port, const char *file, const long line );


	static void SetDoNotFragment( SOCKET listenSocket, int opt );
private:

	static void SetSocketOptions( SOCKET listenSocket);
	static SocketLayerOverride *slo;
};

#endif
