// \file
//
// This file is part of RakNet Copyright 2003 Jenkins Software LLC
//
// Usage of RakNet is subject to the appropriate license agreement.


#include "RakNetDefines.h"
#include "RakPeer.h"
#include "RakNetTypes.h"
#include "WSAStartupSingleton.h"

#ifdef _WIN32


#else
#define closesocket close
#include <unistd.h>
#endif

#if defined(new)
#pragma push_macro("new")
#undef new
#define RMO_NEW_UNDEF_ALLOCATING_QUEUE
#endif


#include <time.h>

#include <ctype.h> // toupper
#include <string.h>
#include "GetTime.h"
#include "MessageIdentifiers.h"
#include "DS_HuffmanEncodingTree.h"
#include "Rand.h"
#include "PluginInterface2.h"
#include "StringCompressor.h"
#include "StringTable.h"
#include "NetworkIDObject.h"
#include "RakNetTypes.h"
#include "SHA1.h"
#include "RakSleep.h"
#include "RouterInterface.h"
#include "RakAssert.h"
#include "RakNetVersion.h"
#include "NetworkIDManager.h"
#include "DataBlockEncryptor.h"
#include "gettimeofday.h"
#include "SignaledEvent.h"
#include "SuperFastHash.h"

RAK_THREAD_DECLARATION(UpdateNetworkLoop);
RAK_THREAD_DECLARATION(RecvFromLoop);
RAK_THREAD_DECLARATION(UDTConnect);

#define REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE 8

#if !defined ( __APPLE__ ) && !defined ( __APPLE_CC__ )
#include <stdlib.h> // malloc
#endif



#if   defined(_WIN32)
//
#else
/*
#include <alloca.h> // Console 2
#include <stdlib.h>
extern bool _extern_Console2LoadModules(void);
extern int _extern_Console2GetConnectionStatus(void);
extern int _extern_Console2GetLobbyStatus(void);
//extern bool Console2StartupFluff(unsigned int *);
extern void Console2ShutdownFluff(void);
//extern unsigned int Console2ActivateConnection(unsigned int, void *);
//extern bool Console2BlockOnEstablished(void);
extern void Console2GetIPAndPort(unsigned int, char *, unsigned short *, unsigned int );
//extern void Console2DeactivateConnection(unsigned int, unsigned int);
*/
#endif


static const int NUM_MTU_SIZES=3;



static const int mtuSizes[NUM_MTU_SIZES]={MAXIMUM_MTU_SIZE, 1200, 576};



#include "RakAlloca.h"

// Note to self - if I change this it might affect RECIPIENT_OFFLINE_MESSAGE_INTERVAL in Natpunchthrough.cpp
//static const int MAX_OPEN_CONNECTION_REQUESTS=8;
//static const int TIME_BETWEEN_OPEN_CONNECTION_REQUESTS=500;

#ifdef _MSC_VER
#pragma warning( push )
#endif

using namespace RakNet;

static RakNetRandom rnr;

struct RakPeerAndIndex
{
	SOCKET s;
	unsigned short remotePortRakNetWasStartedOn_PS3;
	RakPeer *rakPeer;
};

// On a Little-endian machine the RSA key and message are mangled, but we're
// trying to be friendly to the little endians, so we do byte order
// mangling on Big-Endian machines.  Note that this mangling is independent
// of the byte order used on the network (which also defaults to little-end).
#ifdef HOST_ENDIAN_IS_BIG
	void __inline BSWAPCPY(unsigned char *dest, unsigned char *source, int bytesize)
	{
	#ifdef _DEBUG
		RakAssert( (bytesize % 4 == 0)&&(bytesize)&& "Something is wrong with your exponent or modulus size.");
	#endif
		int i;
		for (i=0; i<bytesize; i+=4)
		{
			dest[i] = source[i+3];
			dest[i+1] = source[i+2];
			dest[i+2] = source[i+1];
			dest[i+3] = source[i];
		}
	}
	void __inline BSWAPSELF(unsigned char *source, int bytesize)
	{
	#ifdef _DEBUG
		RakAssert( (bytesize % 4 == 0)&&(bytesize)&& "Something is wrong with your exponent or modulus size.");
	#endif
		int i;
		unsigned char a, b;
		for (i=0; i<bytesize; i+=4)
		{
			a = source[i];
			b = source[i+1];
			source[i] = source[i+3];
			source[i+1] = source[i+2];
			source[i+2] = b;
			source[i+3] = a;
		}
	}
#endif

static const unsigned int SYN_COOKIE_OLD_RANDOM_NUMBER_DURATION = 10000;
static const unsigned int MAX_OFFLINE_DATA_LENGTH=400; // I set this because I limit ID_CONNECTION_REQUEST to 512 bytes, and the password is appended to that packet.

// Used to distinguish between offline messages with data, and messages from the reliability layer
// Should be different than any message that could result from messages from the reliability layer
#pragma warning(disable:4309) // 'initializing' : truncation of constant value
// Make sure highest bit is 0, so isValid in DatagramHeaderFormat is false
static const char OFFLINE_MESSAGE_DATA_ID[16]={0x00,0xFF,0xFF,0x00,0xFE,0xFE,0xFE,0xFE,0xFD,0xFD,0xFD,0xFD,0x12,0x34,0x56,0x78};

//#define _DO_PRINTF

// UPDATE_THREAD_POLL_TIME is how often the update thread will poll to see
// if receive wasn't called within UPDATE_THREAD_UPDATE_TIME.  If it wasn't called within that time,
// the updating thread will activate and take over network communication until Receive is called again.
//static const unsigned int UPDATE_THREAD_UPDATE_TIME=30;
//static const unsigned int UPDATE_THREAD_POLL_TIME=30;

//#define _TEST_AES

struct PacketFollowedByData
{
	Packet p;
	unsigned char data[1];
};

Packet *RakPeer::AllocPacket(unsigned dataSize, const char *file, unsigned int line)
{
	// Crashes when dataSize is 4 bytes - not sure why
// 	unsigned char *data = (unsigned char *) rakMalloc_Ex(sizeof(PacketFollowedByData)+dataSize, file, line);
// 	Packet *p = &((PacketFollowedByData *)data)->p;
// 	p->data=((PacketFollowedByData *)data)->data;
// 	p->length=dataSize;
// 	p->bitSize=BYTES_TO_BITS(dataSize);
// 	p->deleteData=false;
// 	p->guid=UNASSIGNED_RAKNET_GUID;
// 	return p;

	Packet *p;
	packetAllocationPoolMutex.Lock();
	p = packetAllocationPool.Allocate(file,line);
	packetAllocationPoolMutex.Unlock();
	p = new ((void*)p) Packet;
	p->data=(unsigned char*) rakMalloc_Ex(dataSize,file,line);
	p->length=dataSize;
	p->bitSize=BYTES_TO_BITS(dataSize);
	p->deleteData=true;
	p->guid=UNASSIGNED_RAKNET_GUID;
	p->bypassPlugins=false;
	return p;
}

Packet *RakPeer::AllocPacket(unsigned dataSize, unsigned char *data, const char *file, unsigned int line)
{
	// Packet *p = (Packet *)rakMalloc_Ex(sizeof(Packet), file, line);
	Packet *p;
	packetAllocationPoolMutex.Lock();
	p = packetAllocationPool.Allocate(file,line);
	packetAllocationPoolMutex.Unlock();
	p = new ((void*)p) Packet;
	p->data=data;
	p->length=dataSize;
	p->bitSize=BYTES_TO_BITS(dataSize);
	p->deleteData=true;
	p->guid=UNASSIGNED_RAKNET_GUID;
	p->bypassPlugins=false;
	return p;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RakPeer()
{
	StringCompressor::AddReference();
	RakNet::StringTable::AddReference();
	WSAStartupSingleton::AddRef();

#if   !defined(_WIN32_WCE) 
	usingSecurity = false;
#endif
	memset( frequencyTable, 0, sizeof( unsigned int ) * 256 );
	rawBytesSent = rawBytesReceived = compressedBytesSent = compressedBytesReceived = 0;
	outputTree = inputTree = 0;
	defaultMTUSize = mtuSizes[NUM_MTU_SIZES-1];
	trackFrequencyTable = false;
	maximumIncomingConnections = 0;
	maximumNumberOfPeers = 0;
	//remoteSystemListSize=0;
	remoteSystemList = 0;
	remoteSystemLookup=0;
	bytesSentPerSecond = bytesReceivedPerSecond = 0;
	endThreads = true;
	isMainLoopThreadActive = false;
	isRecvFromLoopThreadActive = false;
	// isRecvfromThreadActive=false;
#if defined(GET_TIME_SPIKE_LIMIT) && GET_TIME_SPIKE_LIMIT>0
	occasionalPing = true;
#else
	occasionalPing = false;
#endif
	allowInternalRouting=false;
	for (unsigned int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
		mySystemAddress[i]=UNASSIGNED_SYSTEM_ADDRESS;
	allowConnectionResponseIPMigration = false;
	blockOnRPCReply=false;
	//incomingPasswordLength=outgoingPasswordLength=0;
	incomingPasswordLength=0;
	router=0;
	splitMessageProgressInterval=0;
	//unreliableTimeout=0;
	unreliableTimeout=1000;
	networkIDManager=0;
	maxOutgoingBPS=0;
	firstExternalID=UNASSIGNED_SYSTEM_ADDRESS;
	myGuid=UNASSIGNED_RAKNET_GUID;
	networkIDManager=0;
	userUpdateThreadPtr=0;
	userUpdateThreadData=0;

#ifdef _DEBUG
	// Wait longer to disconnect in debug so I don't get disconnected while tracing
	defaultTimeoutTime=30000;
#else
	defaultTimeoutTime=10000;
#endif

#ifdef _DEBUG
	_packetloss=0.0;
	_minExtraPing=0;
	_extraPingVariance=0;
#endif

#ifdef _RAKNET_THREADSAFE
	bufferedCommands.SetPageSize(sizeof(BufferedCommandStruct)*32);
	socketQueryOutput.SetPageSize(sizeof(SocketQueryOutput)*8);
	bufferedPackets.SetPageSize(sizeof(RecvFromStruct)*32);
#endif

	packetAllocationPoolMutex.Lock();
	packetAllocationPool.SetPageSize(sizeof(DataStructures::MemoryPool<Packet>::MemoryWithPage)*32);
	packetAllocationPoolMutex.Unlock();

	remoteSystemIndexPool.SetPageSize(sizeof(DataStructures::MemoryPool<RemoteSystemIndex>::MemoryWithPage)*32);

	GenerateGUID();

	quitAndDataEvents.InitEvent();
	limitConnectionFrequencyFromTheSameIP=false;
	ResetSendReceipt();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Destructor
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::~RakPeer()
{
//	unsigned i;


	Shutdown( 0, 0 );

	// Free the ban list.
	ClearBanList();

	StringCompressor::RemoveReference();
	RakNet::StringTable::RemoveReference();
	WSAStartupSingleton::Deref();

	quitAndDataEvents.CloseEvent();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// \brief Starts the network threads, opens the listen port.
// You must call this before calling Connect().
// Multiple calls while already active are ignored.  To call this function again with different settings, you must first call Shutdown().
// \note Call SetMaximumIncomingConnections if you want to accept incoming connections
// \note Set _RAKNET_THREADSAFE in RakNetDefines.h if you want to call RakNet functions from multiple threads (not recommended, as it is much slower and RakNet is already asynchronous).
// \param[in] maxConnections The maximum number of connections between this instance of RakPeer and another instance of RakPeer. Required so the network can preallocate and for thread safety. A pure client would set this to 1.  A pure server would set it to the number of allowed clients.- A hybrid would set it to the sum of both types of connections
// \param[in] localPort The port to listen for connections on.
// \param[in] _threadSleepTimer How many ms to Sleep each internal update cycle. With new congestion control, the best results will be obtained by passing 10.
// \param[in] socketDescriptors An array of SocketDescriptor structures to force RakNet to listen on a particular IP address or port (or both).  Each SocketDescriptor will represent one unique socket.  Do not pass redundant structures.  To listen on a specific port, you can pass &socketDescriptor, 1SocketDescriptor(myPort,0); such as for a server.  For a client, it is usually OK to just pass SocketDescriptor();
// \param[in] socketDescriptorCount The size of the \a socketDescriptors array.  Pass 1 if you are not sure what to pass.
// \return False on failure (can't create socket or thread), true on success.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Startup( unsigned short maxConnections, int _threadSleepTimer, SocketDescriptor *socketDescriptors, unsigned socketDescriptorCount, int threadPriority )
{
	if (IsActive())
		return false;

	if (threadPriority==-99999)
	{


#if   defined(_WIN32)
		threadPriority=0;


#else
		threadPriority=1000;
#endif
	}

	// Fill out ipList structure

	memset( ipList, 0, sizeof( char ) * 16 * MAXIMUM_NUMBER_OF_INTERNAL_IDS );
	SocketLayer::GetMyIP( ipList,binaryAddresses );


	unsigned int i;
	if (myGuid==UNASSIGNED_RAKNET_GUID)
	{
		seedMT( GenerateSeedFromGuid() );
	}

	rnr.SeedMT( GenerateSeedFromGuid() );

	RakPeerAndIndex rpai[32];
	RakAssert(socketDescriptorCount<32);

	RakAssert(socketDescriptors && socketDescriptorCount>=1);

	if (socketDescriptors==0 || socketDescriptorCount<1)
		return false;

	//unsigned short localPort;
	//localPort=socketDescriptors[0].port;

	RakAssert( maxConnections > 0 );

	if ( maxConnections <= 0 )
		return false;

	DerefAllSockets();


	// Go through all socket descriptors and precreate sockets on the specified addresses
	for (i=0; i<socketDescriptorCount; i++)
	{
		if (socketDescriptors[i].port!=0 && SocketLayer::IsPortInUse(socketDescriptors[i].port, socketDescriptors[i].hostAddress)==true)
		{
			DerefAllSockets();
			return false;
		}

		RakNetSmartPtr<RakNetSocket> rns(RakNet::OP_NEW<RakNetSocket>(__FILE__,__LINE__));
		if (socketDescriptors[i].remotePortRakNetWasStartedOn_PS3==0)
			rns->s = (unsigned int) SocketLayer::CreateBoundSocket( socketDescriptors[i].port, true, socketDescriptors[i].hostAddress, 100 );
		else // if (socketDescriptors[i].socketType==SocketDescriptor::PS3_LOBBY_UDP)
			rns->s = (unsigned int) SocketLayer::CreateBoundSocket_PS3Lobby( socketDescriptors[i].port, true, socketDescriptors[i].hostAddress );

		if ((SOCKET)rns->s==(SOCKET)-1)
		{
			DerefAllSockets();
			return false;
		}


		rns->boundAddress=SocketLayer::GetSystemAddress( rns->s );
		rns->remotePortRakNetWasStartedOn_PS3=socketDescriptors[i].remotePortRakNetWasStartedOn_PS3;
		rns->userConnectionSocketIndex=i;

		// Test the socket
		int zero=0;
		if (SocketLayer::SendTo((SOCKET)rns->s, (const char*) &zero,4,"127.0.0.1", rns->boundAddress.port, rns->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__ )!=0)
		{
			DerefAllSockets();
			return false;
		}

		socketList.Push(rns, __FILE__, __LINE__ );

		/*
#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
		if (_threadSleepTimer>0)
		{
			rns->recvEvent=CreateEvent(0,FALSE,FALSE,0);
			WSAEventSelect(rns->s,rns->recvEvent,FD_READ);
		}
#endif
		*/
	}

	// 05/05/09 - Updated to dynamically bind sockets on IP addresses, so we always reply on the same address we recieved from
	/*

	for (i=0; i<connectionSocketsLength; i++)
	{
		if (connectionSockets[i].haveRakNetCloseSocket && connectionSockets[i].s != (SOCKET) -1)
			closesocket(connectionSockets[i].s);
	}
	if (connectionSockets)
	{
		RakNet::OP_DELETE_ARRAY(connectionSockets, __FILE__, __LINE__);
	}
	connectionSocketsLength=0;

	connectionSockets=RakNet::OP_NEW_ARRAY<RakNetSocket>(socketDescriptorCount, __FILE__, __LINE__ );
	for (i=0; i<socketDescriptorCount; i++)
	{
		if (socketDescriptors[i].socketType==SocketDescriptor::NONE)
			connectionSockets[i].s = (SOCKET) -1;
		else if (socketDescriptors[i].remotePortRakNetWasStartedOn_PS3==0)
			connectionSockets[i].s = SocketLayer::CreateBoundSocket( socketDescriptors[i].port, true, socketDescriptors[i].hostAddress );
		else if (socketDescriptors[i].socketType==SocketDescriptor::PS3_LOBBY_UDP)
			connectionSockets[i].s = SocketLayer::CreateBoundSocket_PS3Lobby( socketDescriptors[i].port, true, socketDescriptors[i].hostAddress );
		connectionSockets[i].haveRakNetCloseSocket=true;
		connectionSockets[i].socketType=socketDescriptors[i].socketType;

		if (connectionSockets[i].s==(SOCKET)-1 && socketDescriptors[i].socketType!=SocketDescriptor::NONE)
		{
			unsigned int j;
			for (j=0; j < i; j++)
			{
				if (connectionSockets[j].haveRakNetCloseSocket)
					closesocket(connectionSockets[j].s);
			}
			if (connectionSockets)
			{
				RakNet::OP_DELETE_ARRAY(connectionSockets, __FILE__, __LINE__);
			}
			connectionSocketsLength=0;
			connectionSockets=0;
			return false;
		}
	}

	connectionSocketsLength=socketDescriptorCount;

#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
	if (_threadSleepTimer>0)
	{
		recvEvent=CreateEvent(0,FALSE,FALSE,0);
		for (i=0; i<socketDescriptorCount; i++)
		{
			if (socketDescriptors[i].socketType!=SocketDescriptor::NONE)
				WSAEventSelect(connectionSockets[i].s,recvEvent,FD_READ);
		}
	}
	#endif
*/

	if ( maximumNumberOfPeers == 0 )
	{
		// Don't allow more incoming connections than we have peers.
		if ( maximumIncomingConnections > maxConnections )
			maximumIncomingConnections = maxConnections;

		maximumNumberOfPeers = maxConnections;
		// 04/19/2006 - Don't overallocate because I'm no longer allowing connected pings.
		// The disconnects are not consistently processed and the process was sloppy and complicated.
		// Allocate 10% extra to handle new connections from players trying to connect when the server is full
		//remoteSystemListSize = maxConnections;// * 11 / 10 + 1;

		// remoteSystemList in Single thread
		//remoteSystemList = RakNet::OP_NEW<RemoteSystemStruct[ remoteSystemListSize ]>( __FILE__, __LINE__ );
		remoteSystemList = RakNet::OP_NEW_ARRAY<RemoteSystemStruct>(maximumNumberOfPeers, __FILE__, __LINE__ );

		remoteSystemLookup = RakNet::OP_NEW_ARRAY<RemoteSystemIndex*>((unsigned int) maximumNumberOfPeers * REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE, __FILE__, __LINE__ );

		for ( i = 0; i < maximumNumberOfPeers; i++ )
		//for ( i = 0; i < remoteSystemListSize; i++ )
		{
			// remoteSystemList in Single thread
			remoteSystemList[ i ].isActive = false;
			remoteSystemList[ i ].systemAddress = UNASSIGNED_SYSTEM_ADDRESS;
			remoteSystemList[ i ].guid = UNASSIGNED_RAKNET_GUID;
			remoteSystemList[ i ].myExternalSystemAddress = UNASSIGNED_SYSTEM_ADDRESS;
			remoteSystemList[ i ].connectMode=RemoteSystemStruct::NO_ACTION;
			remoteSystemList[ i ].MTUSize = defaultMTUSize;
			#ifdef _DEBUG
			remoteSystemList[ i ].reliabilityLayer.ApplyNetworkSimulator(_packetloss, _minExtraPing, _extraPingVariance);
			#endif
		}

		for (unsigned int i=0; i < (unsigned int) maximumNumberOfPeers*REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE; i++)
		{
			remoteSystemLookup[i]=0;
		}
	}

	// For histogram statistics
	// nextReadBytesTime=0;
	// lastSentBytes=lastReceivedBytes=0;

	if ( endThreads )
	{
	//	lastUserUpdateCycle = 0;

		// Reset the frequency table that we use to save outgoing data
		memset( frequencyTable, 0, sizeof( unsigned int ) * 256 );

		// Reset the statistical data
		rawBytesSent = rawBytesReceived = compressedBytesSent = compressedBytesReceived = 0;

		updateCycleIsRunning = false;
		endThreads = false;
		// Create the threads
		threadSleepTimer = _threadSleepTimer;

		ClearBufferedCommands();
		ClearBufferedPackets();
		ClearSocketQueryOutput();


		for (int ipIndex=0; ipIndex < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ipIndex++)
		{

			if (ipList[ipIndex][0])
			{
				mySystemAddress[ipIndex].port = SocketLayer::GetLocalPort(socketList[0]->s);
//				if (socketDescriptors[0].hostAddress==0 || socketDescriptors[0].hostAddress[0]==0)
				mySystemAddress[ipIndex].binaryAddress = inet_addr( ipList[ ipIndex ] );
				//			else
				//				mySystemAddress[ipIndex].binaryAddress = inet_addr( socketDescriptors[0].hostAddress );
			}
			else
				mySystemAddress[ipIndex]=UNASSIGNED_SYSTEM_ADDRESS;



		}

		if ( isMainLoopThreadActive == false )
		{

			int errorCode = RakNet::RakThread::Create(UpdateNetworkLoop, this, threadPriority);

			if ( errorCode != 0 )
			{
				Shutdown( 0, 0 );
				return false;
			}

			for (i=0; i<socketDescriptorCount; i++)
			{
				rpai[i].remotePortRakNetWasStartedOn_PS3=socketDescriptors[i].remotePortRakNetWasStartedOn_PS3;
				rpai[i].s=socketList[i]->s;
				rpai[i].rakPeer=this;
				isRecvFromLoopThreadActive=false;
				errorCode = RakNet::RakThread::Create(RecvFromLoop, &rpai[i], threadPriority);

				if ( errorCode != 0 )
				{
					Shutdown( 0, 0 );
					return false;
				}

				while (  isRecvFromLoopThreadActive == false )
					RakSleep(10);
			}

		}

		// Wait for the threads to activate.  When they are active they will set these variables to true

		while (  isMainLoopThreadActive == false )
			RakSleep(10);

	}

	for (i=0; i < messageHandlerList.Size(); i++)
	{
		messageHandlerList[i]->OnRakPeerStartup();
	}

#ifdef USE_THREADED_SEND
	SendToThread::AddRef();
#endif

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Must be called while offline
// Secures connections though a combination of SHA1, AES128, SYN Cookies, and RSA to prevent
// connection spoofing, replay attacks, data eavesdropping, packet tampering, and MitM attacks.
// There is a significant amount of processing and a slight amount of bandwidth
// overhead for this feature.
//
// If you accept connections, you must call this or else secure connections will not be enabled
// for incoming connections.
// If you are connecting to another system, you can call this with values for the
// (e and p,q) public keys before connecting to prevent MitM
//
// Parameters:
// pubKeyE, pubKeyN - A pointer to the public keys from the RSACrypt class. See the Encryption sample
// privKeyP, privKeyQ - Private keys generated from the RSACrypt class.  See the Encryption sample
// If the private keys are 0, then a new key will be generated when this function is called
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::InitializeSecurity(const char *pubKeyE, const char *pubKeyN, const char *privKeyP, const char *privKeyQ )
{
#if   !defined(_WIN32_WCE) 
	if ( endThreads == false )
		return ;

	// Setting the client key is e,n,
	// Setting the server key is p,q
	if ( //( privKeyP && privKeyQ && ( pubKeyE || pubKeyN ) ) ||
		//( pubKeyE && pubKeyN && ( privKeyP || privKeyQ ) ) ||
		( privKeyP && privKeyQ == 0 ) ||
		( privKeyQ && privKeyP == 0 ) ||
		( pubKeyE && pubKeyN == 0 ) ||
		( pubKeyN && pubKeyE == 0 ) )
	{
		// Invalid parameters
		RakAssert( 0 );
	}

	GenerateSYNCookieRandomNumber();

	usingSecurity = true;

	if ( privKeyP == 0 && privKeyQ == 0 && pubKeyE == 0 && pubKeyN == 0 )
	{
		keysLocallyGenerated = true;
		//rsacrypt.generateKeys();
		rsacrypt.generatePrivateKey(RAKNET_RSA_FACTOR_LIMBS);
	}

	else
	{
		if ( pubKeyE && pubKeyN )
		{
			// Save public keys
			memcpy( ( char* ) & publicKeyE, pubKeyE, sizeof( publicKeyE ) );
			memcpy( publicKeyN, pubKeyN, sizeof( publicKeyN ) );
		}

		if ( privKeyP && privKeyQ )
		{
			bool b = rsacrypt.setPrivateKey( (const uint32_t*) privKeyP, (const uint32_t*) privKeyQ, RAKNET_RSA_FACTOR_LIMBS/2);
			(void) b;
			RakAssert(b);
		//	BIGHALFSIZE( RSA_BIT_SIZE, p );
		//	BIGHALFSIZE( RSA_BIT_SIZE, q );

//			memcpy( p, privKeyP, sizeof( p ) );
//			memcpy( q, privKeyQ, sizeof( q ) );
			// Save private keys
//			rsacrypt.setPrivateKey( p, q );
		}

		keysLocallyGenerated = false;
	}
#endif
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description
// Must be called while offline
// Disables all security.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DisableSecurity( void )
{
#if   !defined(_WIN32_WCE) 
	if ( endThreads == false )
		return ;

	usingSecurity = false;
#endif
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AddToSecurityExceptionList(const char *ip)
{
	securityExceptionMutex.Lock();
	securityExceptionList.Insert(RakString(ip), __FILE__, __LINE__);
	securityExceptionMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RemoveFromSecurityExceptionList(const char *ip)
{
	if (securityExceptionList.Size()==0)
		return;

	if (ip==0)
	{
		securityExceptionMutex.Lock();
		securityExceptionList.Clear(false, __FILE__, __LINE__);
		securityExceptionMutex.Unlock();
	}
	else
	{
		unsigned i=0;
		securityExceptionMutex.Lock();
		while (i < securityExceptionList.Size())
		{
			if (securityExceptionList[i].IPAddressMatch(ip))
			{
				securityExceptionList[i]=securityExceptionList[securityExceptionList.Size()-1];
				securityExceptionList.RemoveAtIndex(securityExceptionList.Size()-1);
			}
			else
				i++;
		}
		securityExceptionMutex.Unlock();
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsInSecurityExceptionList(const char *ip)
{
	if (securityExceptionList.Size()==0)
		return false;

	unsigned i=0;
	securityExceptionMutex.Lock();
	for (; i < securityExceptionList.Size(); i++)
	{
		if (securityExceptionList[i].IPAddressMatch(ip))
		{
			securityExceptionMutex.Unlock();
			return true;
		}
	}
	securityExceptionMutex.Unlock();
	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sets how many incoming connections are allowed.  If this is less than the number of players currently connected, no
// more players will be allowed to connect.  If this is greater than the maximum number of peers allowed, it will be reduced
// to the maximum number of peers allowed.  Defaults to 0.
//
// Parameters:
// numberAllowed - Maximum number of incoming connections allowed.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetMaximumIncomingConnections( unsigned short numberAllowed )
{
	maximumIncomingConnections = numberAllowed;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the maximum number of incoming connections, which is always <= maxConnections
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetMaximumIncomingConnections( void ) const
{
	return maximumIncomingConnections;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns how many open connections there are at this time
// \return the number of open connections
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::NumberOfConnections(void) const
{
	unsigned short i, count=0;
	for (i=0; i < maximumNumberOfPeers; i++)
		if (remoteSystemList[i].isActive)
			count++;
	return count;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sets the password incoming connections must match in the call to Connect (defaults to none)
// Pass 0 to passwordData to specify no password
//
// Parameters:
// passwordData: A data block that incoming connections must match.  This can be just a password, or can be a stream of data.
// - Specify 0 for no password data
// passwordDataLength: The length in bytes of passwordData
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetIncomingPassword( const char* passwordData, int passwordDataLength )
{
	//if (passwordDataLength > MAX_OFFLINE_DATA_LENGTH)
	//	passwordDataLength=MAX_OFFLINE_DATA_LENGTH;

	if (passwordDataLength > 255)
		passwordDataLength=255;

	if (passwordData==0)
		passwordDataLength=0;

	// Not threadsafe but it's not important enough to lock.  Who is going to change the password a lot during runtime?
	// It won't overflow at least because incomingPasswordLength is an unsigned char
	if (passwordDataLength>0)
		memcpy(incomingPassword, passwordData, passwordDataLength);
	incomingPasswordLength=(unsigned char)passwordDataLength;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GetIncomingPassword( char* passwordData, int *passwordDataLength  )
{
	if (passwordData==0)
	{
		*passwordDataLength=incomingPasswordLength;
		return;
	}

	if (*passwordDataLength > incomingPasswordLength)
		*passwordDataLength=incomingPasswordLength;

	if (*passwordDataLength>0)
		memcpy(passwordData, incomingPassword, *passwordDataLength);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Call this to connect to the specified host (ip or domain name) and server port.
// Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.  Calling both acts as a true peer.
// This is a non-blocking connection.  You know the connection is successful when IsConnected() returns true
// or receive gets a packet with the type identifier ID_CONNECTION_ACCEPTED.  If the connection is not
// successful, such as rejected connection or no response then neither of these things will happen.
// Requires that you first call Initialize
//
// Parameters:
// host: Either a dotted IP address or a domain name
// remotePort: Which port to connect to on the remote machine.
// passwordData: A data block that must match the data block on the server.  This can be just a password, or can be a stream of data
// passwordDataLength: The length in bytes of passwordData
//
// Returns:
// True on successful initiation. False on incorrect parameters, internal error, or too many existing peers
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Connect( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime )
{
	// If endThreads is true here you didn't call Startup() first.
	if ( host == 0 || endThreads || connectionSocketIndex>=socketList.Size() )
		return false;

	connectionSocketIndex=GetRakNetSocketFromUserConnectionSocketIndex(connectionSocketIndex);

	if (passwordDataLength>255)
		passwordDataLength=255;

	if (passwordData==0)
		passwordDataLength=0;

	// Not threadsafe but it's not important enough to lock.  Who is going to change the password a lot during runtime?
	// It won't overflow at least because outgoingPasswordLength is an unsigned char
//	if (passwordDataLength>0)
//		memcpy(outgoingPassword, passwordData, passwordDataLength);
//	outgoingPasswordLength=(unsigned char) passwordDataLength;

	if ( NonNumericHostString( host ) )
	{
		host = ( char* ) SocketLayer::DomainNameToIP( host );

		if (host==0)
			return false;
	}

	// 04/02/09 - Can't remember why I disabled connecting to self, but it seems to work
	// Connecting to ourselves in the same instance of the program?
//	if ( ( strcmp( host, "127.0.0.1" ) == 0 || strcmp( host, "0.0.0.0" ) == 0 ) && remotePort == mySystemAddress[0].port )
//		return false;

	return SendConnectionRequest( host, remotePort, passwordData, passwordDataLength, connectionSocketIndex, 0, sendConnectionAttemptCount, timeBetweenSendConnectionAttemptsMS, timeoutTime);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool RakPeer::ConnectWithSocket(const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, RakNetSmartPtr<RakNetSocket> socket, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime)
{
	if ( host == 0 || endThreads || socket.IsNull() )
		return false;

	if (passwordDataLength>255)
		passwordDataLength=255;

	if (passwordData==0)
		passwordDataLength=0;

		if ( NonNumericHostString( host ) )
	{
		host = ( char* ) SocketLayer::DomainNameToIP( host );

		if (host==0)
			return false;
	}

		return SendConnectionRequest( host, remotePort, passwordData, passwordDataLength, 0, 0, sendConnectionAttemptCount, timeBetweenSendConnectionAttemptsMS, timeoutTime, socket );

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Stops the network threads and close all connections.  Multiple calls are ok.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::Shutdown( unsigned int blockDuration, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority )
{
	unsigned i,j;
	bool anyActive;
	RakNetTime startWaitingTime;
//	SystemAddress systemAddress;
	RakNetTime time;
	//unsigned short systemListSize = remoteSystemListSize; // This is done for threading reasons
	unsigned short systemListSize = maximumNumberOfPeers;

	if ( blockDuration > 0 )
	{
		for ( i = 0; i < systemListSize; i++ )
		{
			// remoteSystemList in user thread
			if (remoteSystemList[i].isActive)
				NotifyAndFlagForShutdown(remoteSystemList[i].systemAddress, false, orderingChannel, disconnectionNotificationPriority);
		}

		time = RakNet::GetTime();
		startWaitingTime = time;
		while ( time - startWaitingTime < blockDuration )
		{
			anyActive=false;
			for (j=0; j < systemListSize; j++)
			{
				// remoteSystemList in user thread
				if (remoteSystemList[j].isActive)
				{
					anyActive=true;
					break;
				}
			}

			// If this system is out of packets to send, then stop waiting
			if ( anyActive==false )
				break;

			// This will probably cause the update thread to run which will probably
			// send the disconnection notification

			RakSleep(15);
			time = RakNet::GetTime();
		}
	}

	for (i=0; i < messageHandlerList.Size(); i++)
	{
		messageHandlerList[i]->OnRakPeerShutdown();
	}

	quitAndDataEvents.SetEvent();

	endThreads = true;
	// Get recvfrom to unblock
	for (i=0; i < socketList.Size(); i++)
	{
		if (SocketLayer::SendTo(socketList[i]->s, (const char*) &i,1,"127.0.0.1", socketList[i]->boundAddress.port, socketList[i]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__ )!=0)
			break;
	}
	while ( isMainLoopThreadActive )
	{
		endThreads = true;
		RakSleep(15);
	}

//	char c=0;
//	unsigned int socketIndex;
	// remoteSystemList in Single thread
	for ( i = 0; i < systemListSize; i++ )
	{
		// Reserve this reliability layer for ourselves
		remoteSystemList[ i ].isActive = false;

		// Remove any remaining packets
		remoteSystemList[ i ].reliabilityLayer.Reset(false, remoteSystemList[ i ].MTUSize);

		remoteSystemList[ i ].rakNetSocket.SetNull();
	}


	// Setting maximumNumberOfPeers to 0 allows remoteSystemList to be reallocated in Initialize.
	// Setting remoteSystemListSize prevents threads from accessing the reliability layer
	maximumNumberOfPeers = 0;
	//remoteSystemListSize = 0;

	// Free any packets the user didn't deallocate
	packetReturnMutex.Lock();
	for (unsigned int i=0; i < packetReturnQueue.Size(); i++)
		DeallocatePacket(packetReturnQueue[i]);
	packetReturnQueue.Clear(__FILE__,__LINE__);
	packetReturnMutex.Unlock();
	packetAllocationPoolMutex.Lock();
	packetAllocationPool.Clear(__FILE__,__LINE__);
	packetAllocationPoolMutex.Unlock();

	blockOnRPCReply=false;

	RakNetTimeMS timeout = RakNet::GetTimeMS()+1000;
	while ( isRecvFromLoopThreadActive && RakNet::GetTimeMS()<timeout )
	{
		// Get recvfrom to unblock
		for (i=0; i < socketList.Size(); i++)
		{
			SocketLayer::SendTo(socketList[i]->s, (const char*) &i,1,"127.0.0.1", socketList[i]->boundAddress.port, socketList[i]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__ );
		}

		RakSleep(30);
	}

	if (isRecvFromLoopThreadActive)
	{
		timeout = RakNet::GetTimeMS()+1000;
		while ( isRecvFromLoopThreadActive && RakNet::GetTimeMS()<timeout )
		{
			RakSleep(30);
		}
	}

	DerefAllSockets();

	ClearBufferedCommands();
	ClearBufferedPackets();
	ClearSocketQueryOutput();
	bytesSentPerSecond = bytesReceivedPerSecond = 0;

	ClearRequestedConnectionList();


	// Clear out the reliability layer list in case we want to reallocate it in a successive call to Init.
	RemoteSystemStruct * temp = remoteSystemList;
	remoteSystemList = 0;
	RakNet::OP_DELETE_ARRAY(temp, __FILE__, __LINE__);

	ClearRemoteSystemLookup();

#ifdef USE_THREADED_SEND
	SendToThread::Deref();
#endif

	ResetSendReceipt();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns true if the network threads are running
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline bool RakPeer::IsActive( void ) const
{
	return endThreads == false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Fills the array remoteSystems with the systemAddress of all the systems we are connected to
//
// Parameters:
// remoteSystems (out): An array of SystemAddress structures to be filled with the SystemAddresss of the systems we are connected to
// - pass 0 to remoteSystems to only get the number of systems we are connected to
// numberOfSystems (int, out): As input, the size of remoteSystems array.  As output, the number of elements put into the array
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GetConnectionList( SystemAddress *remoteSystems, unsigned short *numberOfSystems ) const
{
	int count, index;
	count=0;

	if ( remoteSystemList == 0 || endThreads == true )
	{
		*numberOfSystems = 0;
		return false;
	}

	// This is called a lot so I unrolled the loop
	if ( remoteSystems )
	{
		// remoteSystemList in user thread
		//for ( count = 0, index = 0; index < remoteSystemListSize; ++index )
		for ( count = 0, index = 0; index < maximumNumberOfPeers; ++index )
			if ( remoteSystemList[ index ].isActive && remoteSystemList[ index ].connectMode==RemoteSystemStruct::CONNECTED)
			{
				if ( count < *numberOfSystems )
					remoteSystems[ count ] = remoteSystemList[ index ].systemAddress;

				++count;
			}
	}
	else
	{
		// remoteSystemList in user thread
		//for ( count = 0, index = 0; index < remoteSystemListSize; ++index )
		for ( count = 0, index = 0; index < maximumNumberOfPeers; ++index )
			if ( remoteSystemList[ index ].isActive && remoteSystemList[ index ].connectMode==RemoteSystemStruct::CONNECTED)
				++count;
	}

	*numberOfSystems = ( unsigned short ) count;

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t RakPeer::GetNextSendReceipt(void)
{
	sendReceiptSerialMutex.Lock();
	uint32_t retVal = sendReceiptSerial;
	sendReceiptSerialMutex.Unlock();
	return retVal;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t RakPeer::IncrementNextSendReceipt(void)
{
	sendReceiptSerialMutex.Lock();
	uint32_t returned = sendReceiptSerial;
	if (++sendReceiptSerial==0)
		sendReceiptSerial=1;
	sendReceiptSerialMutex.Unlock();
	return returned;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sends a block of data to the specified system that you are connected to.
// This function only works while the client is connected (Use the Connect function).
// The first byte should be a message identifier starting at ID_USER_PACKET_ENUM
//
// Parameters:
// data: The block of data to send
// length: The size in bytes of the data to send
// bitStream: The bitstream to send
// priority: What priority level to send on.
// reliability: How reliability to send this data
// orderingChannel: When using ordered or sequenced packets, what channel to order these on.
// - Packets are only ordered relative to other packets on the same stream
// systemAddress: Who to send this packet to, or in the case of broadcasting who not to send it to. Use UNASSIGNED_SYSTEM_ADDRESS to specify none
// broadcast: True to send this packet to all connected systems.  If true, then systemAddress specifies who not to send the packet to.
// Returns:
// \return 0 on bad input. Otherwise a number that identifies this message. If \a reliability is a type that returns a receipt, on a later call to Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS with bytes 1-4 inclusive containing this number
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t RakPeer::Send( const char *data, const int length, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt )
{
#ifdef _DEBUG
	RakAssert( data && length > 0 );
#endif
	RakAssert( !( reliability >= NUMBER_OF_RELIABILITIES || reliability < 0 ) );
	RakAssert( !( priority > NUMBER_OF_PRIORITIES || priority < 0 ) );
	RakAssert( !( orderingChannel >= NUMBER_OF_ORDERED_STREAMS ) );

	if ( data == 0 || length < 0 )
		return 0;

	if ( remoteSystemList == 0 || endThreads == true )
		return 0;

	if ( broadcast == false && systemIdentifier.IsUndefined())
		return 0;

	uint32_t usedSendReceipt;
	if (forceReceipt!=0)
		usedSendReceipt=forceReceipt;
	else
		usedSendReceipt=IncrementNextSendReceipt();

	if (broadcast==false && IsLoopbackAddress(systemIdentifier,true))
	{
		SendLoopback(data,length);

		if (reliability>=UNRELIABLE_WITH_ACK_RECEIPT)
		{
			char buff[5];
			buff[0]=ID_SND_RECEIPT_ACKED;
			sendReceiptSerialMutex.Lock();
			memcpy(buff+1, &sendReceiptSerial, 4);
			sendReceiptSerialMutex.Unlock();
			SendLoopback( buff, 5 );
		}

		return usedSendReceipt;
	}

	if (broadcast==false && router && IsConnected(systemIdentifier.systemAddress)==false)
	{
		router->Send(data, BYTES_TO_BITS(length), priority, reliability, orderingChannel, systemIdentifier.systemAddress);
	}
	else
	{
		SendBuffered(data, length*8, priority, reliability, orderingChannel, systemIdentifier, broadcast, RemoteSystemStruct::NO_ACTION, usedSendReceipt);
	}

	return usedSendReceipt;
}

void RakPeer::SendLoopback( const char *data, const int length )
{
	if ( data == 0 || length < 0 )
		return;

	Packet *packet = AllocPacket(length, __FILE__, __LINE__);
	memcpy(packet->data, data, length);
	packet->systemAddress = GetLoopbackAddress();
	packet->guid=myGuid;
	PushBackPacket(packet, false);
}

uint32_t RakPeer::Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt )
{
#ifdef _DEBUG
	RakAssert( bitStream->GetNumberOfBytesUsed() > 0 );
#endif

	RakAssert( !( reliability >= NUMBER_OF_RELIABILITIES || reliability < 0 ) );
	RakAssert( !( priority > NUMBER_OF_PRIORITIES || priority < 0 ) );
	RakAssert( !( orderingChannel >= NUMBER_OF_ORDERED_STREAMS ) );

	if ( bitStream->GetNumberOfBytesUsed() == 0 )
		return 0;

	if ( remoteSystemList == 0 || endThreads == true )
		return 0;

	if ( broadcast == false && systemIdentifier.IsUndefined() )
		return 0;

	uint32_t usedSendReceipt;
	if (forceReceipt!=0)
		usedSendReceipt=forceReceipt;
	else
		usedSendReceipt=IncrementNextSendReceipt();

	if (broadcast==false && IsLoopbackAddress(systemIdentifier,true))
	{
		SendLoopback((const char*) bitStream->GetData(),bitStream->GetNumberOfBytesUsed());
		if (reliability>=UNRELIABLE_WITH_ACK_RECEIPT)
		{
			char buff[5];
			buff[0]=ID_SND_RECEIPT_ACKED;
			sendReceiptSerialMutex.Lock();
			memcpy(buff+1, &sendReceiptSerial,4);
			sendReceiptSerialMutex.Unlock();
			SendLoopback( buff, 5 );
		}
		return usedSendReceipt;
	}

	if (broadcast==false && router && IsConnected(systemIdentifier.systemAddress)==false)
	{
		router->Send((const char*)bitStream->GetData(), bitStream->GetNumberOfBitsUsed(), priority, reliability, orderingChannel, systemIdentifier.systemAddress);
	}
	else
	{
		// Sends need to be buffered and processed in the update thread because the systemAddress associated with the reliability layer can change,
		// from that thread, resulting in a send to the wrong player!  While I could mutex the systemAddress, that is much slower than doing this
		SendBuffered((const char*)bitStream->GetData(), bitStream->GetNumberOfBitsUsed(), priority, reliability, orderingChannel, systemIdentifier, broadcast, RemoteSystemStruct::NO_ACTION, usedSendReceipt);
	}

	return usedSendReceipt;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Sends multiple blocks of data, concatenating them automatically.
//
// This is equivalent to:
// RakNet::BitStream bs;
// bs.WriteAlignedBytes(block1, blockLength1);
// bs.WriteAlignedBytes(block2, blockLength2);
// bs.WriteAlignedBytes(block3, blockLength3);
// Send(&bs, ...)
//
// This function only works while the connected
// \param[in] data An array of pointers to blocks of data
// \param[in] lengths An array of integers indicating the length of each block of data
// \param[in] numParameters Length of the arrays data and lengths
// \param[in] priority What priority level to send on.  See PacketPriority.h
// \param[in] reliability How reliability to send this data.  See PacketPriority.h
// \param[in] orderingChannel When using ordered or sequenced messages, what channel to order these on. Messages are only ordered relative to other messages on the same stream
// \param[in] systemIdentifier Who to send this packet to, or in the case of broadcasting who not to send it to. Pass either a SystemAddress structure or a RakNetGUID structure. Use UNASSIGNED_SYSTEM_ADDRESS or to specify none
// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
// \return False if we are not connected to the specified recipient.  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint32_t RakPeer::SendList( const char **data, const int *lengths, const int numParameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt )
{
#ifdef _DEBUG
	RakAssert( data );
#endif

	if ( data == 0 || lengths == 0 )
		return 0;

	if ( remoteSystemList == 0 || endThreads == true )
		return 0;

	if (numParameters==0)
		return 0;

	if (lengths==0)
		return 0;

	if ( broadcast == false && systemIdentifier.IsUndefined() )
		return 0;

	uint32_t usedSendReceipt;
	if (forceReceipt!=0)
		usedSendReceipt=forceReceipt;
	else
		usedSendReceipt=IncrementNextSendReceipt();

	SendBufferedList(data, lengths, numParameters, priority, reliability, orderingChannel, systemIdentifier, broadcast, RemoteSystemStruct::NO_ACTION, usedSendReceipt);

	return usedSendReceipt;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Gets a packet from the incoming packet queue. Use DeallocatePacket to deallocate the packet after you are done with it.  Packets must be deallocated in the same order they are received.
// Check the Packet struct at the top of CoreNetworkStructures.h for the format of the struct
//
// Returns:
// 0 if no packets are waiting to be handled, otherwise an allocated packet
// If the client is not active this will also return 0, as all waiting packets are flushed when the client is Disconnected
// This also updates all memory blocks associated with synchronized memory and distributed objects
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Packet* RakPeer::Receive( void )
{
	Packet *packet = ReceiveIgnoreRPC();
	while (packet && (packet->data[ 0 ] == ID_RPC || (packet->length>sizeof(unsigned char)+sizeof(RakNetTime) && packet->data[0]==ID_TIMESTAMP && packet->data[sizeof(unsigned char)+sizeof(RakNetTime)]==ID_RPC)))
	{
		// Do RPC calls from the user thread, not the network update thread
		// If we are currently blocking on an RPC reply, send ID_RPC to the blocker to handle rather than handling RPCs automatically
		HandleRPCPacket( ( char* ) packet->data, packet->length, packet->systemAddress );
		DeallocatePacket( packet );

		packet = ReceiveIgnoreRPC();
	}

    return packet;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Internal - Gets a packet without checking for RPCs
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning( disable : 4701 ) // warning C4701: local variable <variable name> may be used without having been initialized
#endif
Packet* RakPeer::ReceiveIgnoreRPC( void )
{
	if ( !( IsActive() ) )
		return 0;

	Packet *packet;
//	Packet **threadPacket;
	PluginReceiveResult pluginResult;

	int offset;
	unsigned int i;

	for (i=0; i < messageHandlerList.Size(); i++)
	{
		messageHandlerList[i]->Update();
	}

	do
	{
		packetReturnMutex.Lock();
		if (packetReturnQueue.IsEmpty())
			packet=0;
		else
			packet = packetReturnQueue.Pop();
		packetReturnMutex.Unlock();
		if (packet==0)
			return 0;

		unsigned char msgId;
		if ( ( packet->length >= sizeof(unsigned char) + sizeof( RakNetTime ) ) &&
			( (unsigned char) packet->data[ 0 ] == ID_TIMESTAMP ) )
		{
			offset = sizeof(unsigned char);
			ShiftIncomingTimestamp( packet->data + offset, packet->systemAddress );
			msgId=packet->data[sizeof(unsigned char) + sizeof( RakNetTime )];
		}
		else
			msgId=packet->data[0];

		if ( (unsigned char) packet->data[ 0 ] == ID_RPC_REPLY )
		{
			HandleRPCReplyPacket( ( char* ) packet->data, packet->length, packet->systemAddress );
			DeallocatePacket( packet );
			packet=0; // Will do the loop again and get another packet
		}
		else
		{
			for (i=0; i < messageHandlerList.Size(); i++)
			{
				switch (msgId)
				{
					case ID_DISCONNECTION_NOTIFICATION:
						messageHandlerList[i]->OnClosedConnection(packet->systemAddress, packet->guid, LCR_DISCONNECTION_NOTIFICATION);
						break;
					case ID_CONNECTION_LOST:
						messageHandlerList[i]->OnClosedConnection(packet->systemAddress, packet->guid, LCR_CONNECTION_LOST);
						break;
					case ID_NEW_INCOMING_CONNECTION:
						messageHandlerList[i]->OnNewConnection(packet->systemAddress, packet->guid, true);
						break;
					case ID_CONNECTION_REQUEST_ACCEPTED:
						messageHandlerList[i]->OnNewConnection(packet->systemAddress, packet->guid, false);
						break;
					case ID_CONNECTION_ATTEMPT_FAILED:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_CONNECTION_ATTEMPT_FAILED);
						break;
					case ID_ALREADY_CONNECTED:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_ALREADY_CONNECTED);
						break;
					case ID_NO_FREE_INCOMING_CONNECTIONS:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_NO_FREE_INCOMING_CONNECTIONS);
						break;
					case ID_RSA_PUBLIC_KEY_MISMATCH:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_RSA_PUBLIC_KEY_MISMATCH);
						break;
					case ID_CONNECTION_BANNED:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_CONNECTION_BANNED);
						break;
					case ID_INVALID_PASSWORD:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_INVALID_PASSWORD);
						break;
					case ID_INCOMPATIBLE_PROTOCOL_VERSION:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_INCOMPATIBLE_PROTOCOL);
						break;
					case ID_IP_RECENTLY_CONNECTED:
						messageHandlerList[i]->OnFailedConnectionAttempt(packet, FCAR_IP_RECENTLY_CONNECTED);
						break;
				}


				pluginResult=messageHandlerList[i]->OnReceive(packet);
				if (pluginResult==RR_STOP_PROCESSING_AND_DEALLOCATE)
				{
					DeallocatePacket( packet );
					packet=0; // Will do the loop again and get another packet
					break; // break out of the enclosing for
				}
				else if (pluginResult==RR_STOP_PROCESSING)
				{
					packet=0;
					break;
				}
			}
		}

	} while(packet==0);

#ifdef _DEBUG
	RakAssert( packet->data );
#endif

	return packet;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Call this to deallocate a packet returned by Receive
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DeallocatePacket( Packet *packet )
{
	if ( packet == 0 )
		return;

	if (packet->deleteData)
	{
		rakFree_Ex(packet->data, __FILE__, __LINE__ );
		packet->~Packet();
		packetAllocationPoolMutex.Lock();
		packetAllocationPool.Release(packet,__FILE__,__LINE__);
		packetAllocationPoolMutex.Unlock();
	}
	else
	{
		rakFree_Ex(packet, __FILE__, __LINE__ );
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the total number of connections we are allowed
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetMaximumNumberOfPeers( void ) const
{
	return maximumNumberOfPeers;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Register a C function as available for calling as a remote procedure call
//
// Parameters:
// uniqueID: A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions
// functionName(...): The name of the C function or C++ singleton to be used as a function pointer
// This can be called whether the client is active or not, and registered functions stay registered unless unregistered with
// UnregisterAsRemoteProcedureCall
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RegisterAsRemoteProcedureCall( const char* uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms ) )
{
	if ( uniqueID == 0 || uniqueID[ 0 ] == 0 || functionPointer == 0 )
		return;

	rpcMap.AddIdentifierWithFunction(uniqueID, (void*)functionPointer, false);

	/*
	char uppercaseUniqueID[ 256 ];

	int counter = 0;

	while ( uniqueID[ counter ] )
	{
		uppercaseUniqueID[ counter ] = ( char ) toupper( uniqueID[ counter ] );
		counter++;
	}

	uppercaseUniqueID[ counter ] = 0;

	// Each id must be unique
//#ifdef _DEBUG
//	RakAssert( rpcTree.IsIn( RPCNode( uppercaseUniqueID, functionName ) ) == false );
//#endif

	if (rpcTree.IsIn( RPCNode( uppercaseUniqueID, functionName ) ))
		return;

	rpcTree.Add( RPCNode( uppercaseUniqueID, functionName ) );
	*/
}

void RakPeer::RegisterClassMemberRPC( const char* uniqueID, void *functionPointer )
{
	if ( uniqueID == 0 || uniqueID[ 0 ] == 0 || functionPointer == 0 )
		return;

	rpcMap.AddIdentifierWithFunction(uniqueID, functionPointer, true);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Unregisters a C function as available for calling as a remote procedure call that was formerly registered
// with RegisterAsRemoteProcedureCall
//
// Parameters:
// uniqueID: A null terminated string to identify this procedure.  Must match the parameter
// passed to RegisterAsRemoteProcedureCall
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::UnregisterAsRemoteProcedureCall( const char* uniqueID )
{
	if ( uniqueID == 0 || uniqueID[ 0 ] == 0 )
		return;

// Don't call this while running because if you remove RPCs and add them they will not match the indices on the other systems anymore
#ifdef _DEBUG
	RakAssert(IsActive()==false);
	//RakAssert( strlen( uniqueID ) < 256 );
#endif

	rpcMap.RemoveNode(uniqueID);

	/*
	char uppercaseUniqueID[ 256 ];

	strcpy( uppercaseUniqueID, uniqueID );

	int counter = 0;

	while ( uniqueID[ counter ] )
	{
		uppercaseUniqueID[ counter ] = ( char ) toupper( uniqueID[ counter ] );
		counter++;
	}

	uppercaseUniqueID[ counter ] = 0;

	// Unique ID must exist
#ifdef _DEBUG
	RakAssert( rpcTree.IsIn( RPCNode( uppercaseUniqueID, 0 ) ) == true );
#endif

	rpcTree.Del( RPCNode( uppercaseUniqueID, 0 ) );
	*/
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetNetworkIDManager( NetworkIDManager *manager )
{
	networkIDManager=manager;
	if (manager)
		manager->SetGuid(myGuid);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
NetworkIDManager *RakPeer::GetNetworkIDManager(void) const
{
	return networkIDManager;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::RPC( const char* uniqueID, const char *data, BitSize_t bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RakNetTime *includedTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget )
{
#ifdef _DEBUG
	RakAssert( uniqueID && uniqueID[ 0 ] );
	RakAssert(orderingChannel >=0 && orderingChannel < 32);
#endif

	if ( uniqueID == 0 )
		return false;

	if ( strlen( uniqueID ) > 256 )
	{
#ifdef _DEBUG
		RakAssert( 0 );
#endif
		return false; // Unique ID is too long
	}
	if (replyFromTarget && blockOnRPCReply==true)
	{
		// TODO - this should be fixed eventually
		// Prevent a bug where function A calls B (blocking) which calls C back on the sender, which calls D, and C is blocking.
		// blockOnRPCReply is a shared variable so making it unset would unset both blocks, rather than the lowest on the callstack
		// Fix by tracking which function the reply is for.
		return false;
	}

	unsigned *sendList;
//	bool callerAllocationDataUsed;
	unsigned sendListSize;

	// All this code modifies bcs->data and bcs->numberOfBitsToSend in order to transform an RPC request into an actual packet for SendImmediate
	RPCIndex rpcIndex; // Index into the list of RPC calls so we know what number to encode in the packet
//	char *userData; // RPC ID (the name of it) and a pointer to the data sent by the user
//	int extraBuffer; // How many data bytes were allocated to hold the RPC header
	unsigned remoteSystemIndex, sendListIndex; // Iterates into the list of remote systems
//	int dataBlockAllocationLength; // Total number of bytes to allocate for the packet
//	char *writeTarget; // Used to hold either a block of allocated data or the externally allocated data

	sendListSize=0;
	bool routeSend;
	routeSend=false;

	if (broadcast==false)
	{

		sendList=(unsigned *)alloca(sizeof(unsigned));



		remoteSystemIndex=GetIndexFromSystemAddress( systemIdentifier.systemAddress, false );
		if (remoteSystemIndex!=(unsigned)-1 &&
			remoteSystemList[remoteSystemIndex].isActive &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ASAP &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ON_NO_ACK)
		{
			sendList[0]=remoteSystemIndex;
			sendListSize=1;
		}
		else if (router)
			routeSend=true;
	}
	else
	{

		sendList=(unsigned *)alloca(sizeof(unsigned)*maximumNumberOfPeers);




		for ( remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; remoteSystemIndex++ )
		{
			if ( remoteSystemList[ remoteSystemIndex ].isActive && remoteSystemList[ remoteSystemIndex ].systemAddress != systemIdentifier.systemAddress )
				sendList[sendListSize++]=remoteSystemIndex;
		}
	}

	if (sendListSize==0 && routeSend==false)
	{




		return false;
	}
	if (routeSend)
		sendListSize=1;

	RakNet::BitStream outgoingBitStream;
	// remoteSystemList in network thread
	for (sendListIndex=0; sendListIndex < (unsigned)sendListSize; sendListIndex++)
	{
		outgoingBitStream.ResetWritePointer(); // Let us write at the start of the data block, rather than at the end

		if (includedTimestamp)
		{
			outgoingBitStream.Write((MessageID)ID_TIMESTAMP);
			outgoingBitStream.Write(*includedTimestamp);
		}
		outgoingBitStream.Write((MessageID)ID_RPC);
		if (routeSend)
			rpcIndex=UNDEFINED_RPC_INDEX;
		else
			rpcIndex=remoteSystemList[sendList[sendListIndex]].rpcMap.GetIndexFromFunctionName(uniqueID); // Lots of trouble but we can only use remoteSystem->[whatever] in this thread so that is why this command was buffered
		if (rpcIndex!=UNDEFINED_RPC_INDEX)
		{
			// We have an RPC name to an index mapping, so write the index
			outgoingBitStream.Write(false);
			outgoingBitStream.WriteCompressed(rpcIndex);
		}
		else
		{
			// No mapping, so write the encoded RPC name
			outgoingBitStream.Write(true);
			stringCompressor->EncodeString(uniqueID, 256, &outgoingBitStream);
		}
		outgoingBitStream.Write((bool) ((replyFromTarget!=0)==true));
		outgoingBitStream.WriteCompressed( bitLength );
		if (networkID==UNASSIGNED_NETWORK_ID)
		{
			// No object ID
			outgoingBitStream.Write(false);
		}
		else
		{
			// Encode an object ID.  This will use pointer to class member RPC
			outgoingBitStream.Write(true);
			outgoingBitStream.Write(networkID);
		}


		if ( bitLength > 0 )
			outgoingBitStream.WriteBits( (const unsigned char *) data, bitLength, false ); // Last param is false to write the raw data originally from another bitstream, rather than shifting from user data
		else
			outgoingBitStream.WriteCompressed( ( unsigned int ) 0 );

		if (routeSend)
			router->Send((const char*)outgoingBitStream.GetData(), outgoingBitStream.GetNumberOfBitsUsed(), priority,reliability,orderingChannel,systemIdentifier.systemAddress);
		else
			Send(&outgoingBitStream, priority, reliability, orderingChannel, remoteSystemList[sendList[sendListIndex]].systemAddress, false);
	}





	if (replyFromTarget)
	{
		blockOnRPCReply=true;
		// 04/20/06 Just do this transparently.
		// We have to be able to read blocking packets out of order.  Otherwise, if two systems were to send blocking RPC calls to each other at the same time,
		// and they also had ordered packets waiting before the block, it would be impossible to unblock.
		// RakAssert(reliability==RELIABLE || reliability==UNRELIABLE);
		replyFromTargetBS=replyFromTarget;
		replyFromTargetPlayer=systemIdentifier.systemAddress;
		replyFromTargetBroadcast=broadcast;
	}

	// Do not enter this loop on blockOnRPCReply because it is a global which could be set to true by an RPC higher on the callstack, where one RPC was called while waiting for another RPC
	if (replyFromTarget)
//	if (blockOnRPCReply)
	{
//		Packet *p;
		RakNetTime stopWaitingTime=RakNet::GetTime()+30000;
//		RPCIndex arrivedRPCIndex;
//		char uniqueIdentifier[256];
		if (reliability==UNRELIABLE)
			if (systemIdentifier.systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
				stopWaitingTime=RakNet::GetTime()+1500; // Lets guess the ave. ping is 500.  Not important to be very accurate
			else
				stopWaitingTime=RakNet::GetTime()+GetAveragePing(systemIdentifier.systemAddress)*3;

		// For reliable messages, block until we get a reply or the connection is lost
		// For unreliable messages, block until we get a reply, the connection is lost, or 3X the ping passes
		while (blockOnRPCReply &&
			((
			reliability==RELIABLE ||
			reliability==RELIABLE_ORDERED ||
			reliability==RELIABLE_SEQUENCED ||
			reliability==RELIABLE_WITH_ACK_RECEIPT ||
			reliability==RELIABLE_ORDERED_WITH_ACK_RECEIPT
//			||
//			reliability==RELIABLE_SEQUENCED_WITH_ACK_RECEIPT
			) ||
			RakNet::GetTime() < stopWaitingTime))
		{

			RakSleep(30);

			if (routeSend==false && ValidSendTarget(systemIdentifier.systemAddress, broadcast)==false)
				return false;

			unsigned i;
			i=0;


			packetReturnMutex.Lock();
			while (i < packetReturnQueue.Size())
			{
				if ((unsigned char) packetReturnQueue[i]->data[ 0 ] == ID_RPC_REPLY )
				{
					HandleRPCReplyPacket( ( char* ) packetReturnQueue[i]->data, packetReturnQueue[i]->length, packetReturnQueue[i]->systemAddress );
					DeallocatePacket( packetReturnQueue[i] );
					packetReturnQueue.RemoveAtIndex(i);
				}
				else
					i++;
			}
			packetReturnMutex.Unlock();

			/*
			// Scan for RPC reply packets to break out of this loop
			while (i < packetPool.Size())
			{
				if ((unsigned char) packetPool[i]->data[ 0 ] == ID_RPC_REPLY )
				{
					HandleRPCReplyPacket( ( char* ) packetPool[i]->data, packetPool[i]->length, packetPool[i]->systemAddress );
					DeallocatePacket( packetPool[i] );
					packetPool.RemoveAtIndex(i);
				}
				else
					i++;
			}
#ifdef _RAKNET_THREADSAFE
				rakPeerMutexes[packetPool_Mutex].Unlock();
#endif
				*/

			PushBackPacket(ReceiveIgnoreRPC(), false);


			// I might not support processing other RPCs while blocking on one due to complexities I can't control
			// Problem is FuncA calls FuncB which calls back to the sender FuncC. Sometimes it is desirable to call FuncC before returning a return value
			// from FuncB - sometimes not.  There is also a problem with recursion where FuncA calls FuncB which calls FuncA - sometimes valid if
			// a different control path is taken in FuncA. (This can take many different forms)
			/*
			// Same as Receive, but doesn't automatically do RPCs
			p = ReceiveIgnoreRPC();
			if (p)
			{
				// Process all RPC calls except for those calling the function we are currently blocking in (to prevent recursion).
				if ( p->data[ 0 ] == ID_RPC )
				{
					RakNet::BitStream temp((unsigned char *) p->data, p->length, false);
					RPCNode *rpcNode;
					temp.IgnoreBits(8);
					bool nameIsEncoded;
					temp.Read(nameIsEncoded);
					if (nameIsEncoded)
					{
						stringCompressor->DecodeString((char*)uniqueIdentifier, 256, &temp);
					}
					else
					{
						temp.ReadCompressed( arrivedRPCIndex );
						rpcNode=rpcMap.GetNodeFromIndex( arrivedRPCIndex );
						if (rpcNode==0)
						{
							// Invalid RPC format
#ifdef _DEBUG
							RakAssert(0);
#endif
							DeallocatePacket(p);
							continue;
						}
						else
							strcpy(uniqueIdentifier, rpcNode->uniqueIdentifier);
					}

					if (strcmp(uniqueIdentifier, uniqueID)!=0)
					{
						HandleRPCPacket( ( char* ) p->data, p->length, p->systemAddress );
						DeallocatePacket(p);
					}
					else
					{
						PushBackPacket(p, false);
					}
				}
				else
				{
					PushBackPacket(p, false);
				}
			}
			*/
		}

		blockOnRPCReply=false;
	}

	return true;
}


#ifdef _MSC_VER
#pragma warning( disable : 4701 ) // warning C4701: local variable <variable name> may be used without having been initialized
#endif
bool RakPeer::RPC( const char* uniqueID, const RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RakNetTime *includedTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget )
{
	if (bitStream)
		return RPC(uniqueID, (const char*) bitStream->GetData(), bitStream->GetNumberOfBitsUsed(), priority, reliability, orderingChannel, systemIdentifier.systemAddress, broadcast, includedTimestamp, networkID, replyFromTarget);
	else
		return RPC(uniqueID, 0,0, priority, reliability, orderingChannel, systemIdentifier.systemAddress, broadcast, includedTimestamp, networkID, replyFromTarget);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Close the connection to another host (if we initiated the connection it will disconnect, if they did it will kick them out).
//
// Parameters:
// target: Which connection to close
// sendDisconnectionNotification: True to send ID_DISCONNECTION_NOTIFICATION to the recipient. False to close it silently.
// channel: If blockDuration > 0, the disconnect packet will be sent on this channel
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CloseConnection( const SystemAddress target, bool sendDisconnectionNotification, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority )
{
	// This only be called from the user thread, for the user shutting down.
	// From the network thread, this should occur because of ID_DISCONNECTION_NOTIFICATION and ID_CONNECTION_LOST
	unsigned j;
	for (j=0; j < messageHandlerList.Size(); j++)
	{
		messageHandlerList[j]->OnClosedConnection(target, GetGuidFromSystemAddress(target), LCR_CLOSED_BY_USER);
	}

	CloseConnectionInternal(target, sendDisconnectionNotification, false, orderingChannel, disconnectionNotificationPriority);

	// 12/14/09 Return ID_CONNECTION_LOST when calling CloseConnection with sendDisconnectionNotification==false, elsewise it is never returned
	if (sendDisconnectionNotification==false && IsConnected(target,false,false))
	{
		Packet *packet=AllocPacket(sizeof( char ), __FILE__, __LINE__);
		packet->data[ 0 ] = ID_CONNECTION_LOST; // DeadConnection
		packet->guid = GetGuidFromSystemAddress(target);
		packet->systemAddress = target;
		packet->systemAddress.systemIndex = (SystemIndex) GetIndexFromSystemAddress(target,false);
		packet->guid.systemIndex=packet->systemAddress.systemIndex;
		AddPacketToProducer(packet);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Cancel a pending connection attempt
// If we are already connected, the connection stays open
// \param[in] target Which system to cancel
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CancelConnectionAttempt( const SystemAddress target )
{
	unsigned int i;

	// Cancel pending connection attempt, if there is one
	i=0;
	requestedConnectionQueueMutex.Lock();
	while (i < requestedConnectionQueue.Size())
	{
		if (requestedConnectionQueue[i]->systemAddress==target)
		{
			RakNet::OP_DELETE(requestedConnectionQueue[i], __FILE__, __LINE__ );
			requestedConnectionQueue.RemoveAtIndex(i);
			break;
		}
		else
			i++;
	}
	requestedConnectionQueueMutex.Unlock();

}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsConnectionAttemptPending( const SystemAddress systemAddress )
{
	unsigned int i=0;
	requestedConnectionQueueMutex.Lock();
	for (; i < requestedConnectionQueue.Size(); i++)
	{
		if (requestedConnectionQueue[i]->systemAddress==systemAddress)
		{
			requestedConnectionQueueMutex.Unlock();
			return true;
		}
	}
	requestedConnectionQueueMutex.Unlock();

	int index = GetIndexFromSystemAddress(systemAddress, false);
	return index!=-1 && remoteSystemList[index].isActive &&
		(((remoteSystemList[index].connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
		remoteSystemList[index].connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
		remoteSystemList[index].connectMode==RemoteSystemStruct::UNVERIFIED_SENDER ||
		remoteSystemList[index].connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET))
		)
		;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns if a particular systemAddress is connected to us
// \param[in] systemAddress The SystemAddress we are referring to
// \return True if this system is connected and active, false otherwise.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsConnected( const AddressOrGUID systemIdentifier, bool includeInProgress, bool includeDisconnecting )
{
	if (includeInProgress && systemIdentifier.systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		unsigned int i=0;
		requestedConnectionQueueMutex.Lock();
		for (; i < requestedConnectionQueue.Size(); i++)
		{
			if (requestedConnectionQueue[i]->systemAddress==systemIdentifier.systemAddress)
			{
				requestedConnectionQueueMutex.Unlock();
				return true;
			}
		}
		requestedConnectionQueueMutex.Unlock();
	}


	int index;
	if (systemIdentifier.systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		if (IsLoopbackAddress(systemIdentifier.systemAddress,true))
			return true;
		index = GetIndexFromSystemAddress(systemIdentifier.systemAddress, false);
		return index!=-1 && remoteSystemList[index].isActive &&
			(((includeInProgress && (remoteSystemList[index].connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::UNVERIFIED_SENDER ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET))
			||
			(includeDisconnecting && (
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ASAP ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ON_NO_ACK))
			||
			remoteSystemList[index].connectMode==RemoteSystemStruct::CONNECTED))
			;
	}
	else
	{
		index = GetIndexFromGuid(systemIdentifier.rakNetGuid);
		return index!=-1 && remoteSystemList[index].isActive &&
			(((includeInProgress && (remoteSystemList[index].connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::UNVERIFIED_SENDER ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET))
			||
			(includeDisconnecting && (
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ASAP ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY ||
			remoteSystemList[index].connectMode==RemoteSystemStruct::DISCONNECT_ON_NO_ACK))
			||
			remoteSystemList[index].connectMode==RemoteSystemStruct::CONNECTED))
			;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Given a systemAddress, returns an index from 0 to the maximum number of players allowed - 1.
//
// Parameters
// systemAddress - The systemAddress to search for
//
// Returns
// An integer from 0 to the maximum number of peers -1, or -1 if that player is not found
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetIndexFromSystemAddress( const SystemAddress systemAddress ) const
{
	return GetIndexFromSystemAddress(systemAddress, false);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// This function is only useful for looping through all players.
//
// Parameters
// index - an integer between 0 and the maximum number of players allowed - 1.
//
// Returns
// A valid systemAddress or UNASSIGNED_SYSTEM_ADDRESS if no such player at that index
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SystemAddress RakPeer::GetSystemAddressFromIndex( int index )
{
	// remoteSystemList in user thread
	//if ( index >= 0 && index < remoteSystemListSize )
	if ( index >= 0 && index < maximumNumberOfPeers )
		if (remoteSystemList[index].isActive && remoteSystemList[ index ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED) // Don't give the user players that aren't fully connected, since sends will fail
			return remoteSystemList[ index ].systemAddress;

	return UNASSIGNED_SYSTEM_ADDRESS;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Same as GetSystemAddressFromIndex but returns RakNetGUID
// \param[in] index Index should range between 0 and the maximum number of players allowed - 1.
// \return The RakNetGUID
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNetGUID RakPeer::GetGUIDFromIndex( int index )
{
	// remoteSystemList in user thread
	//if ( index >= 0 && index < remoteSystemListSize )
	if ( index >= 0 && index < maximumNumberOfPeers )
		if (remoteSystemList[index].isActive && remoteSystemList[ index ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED) // Don't give the user players that aren't fully connected, since sends will fail
			return remoteSystemList[ index ].guid;

	return UNASSIGNED_RAKNET_GUID;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Same as calling GetSystemAddressFromIndex and GetGUIDFromIndex for all systems, but more efficient
// Indices match each other, so \a addresses[0] and \a guids[0] refer to the same system
// \param[out] addresses All system addresses. Size of the list is the number of connections. Size of the list will match the size of the \a guids list.
// \param[out] guids All guids. Size of the list is the number of connections. Size of the list will match the size of the \a addresses list.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GetSystemList(DataStructures::List<SystemAddress> &addresses, DataStructures::List<RakNetGUID> &guids)
{
	addresses.Clear(false, __FILE__, __LINE__);
	guids.Clear(false, __FILE__, __LINE__);
	int index;
	for (index=0; index < maximumNumberOfPeers; index++)
	{
		 // Don't give the user players that aren't fully connected, since sends will fail
		if (remoteSystemList[index].isActive && remoteSystemList[ index ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED)
		{
			addresses.Push(remoteSystemList[index].systemAddress, __FILE__, __LINE__ );
			guids.Push(remoteSystemList[index].guid, __FILE__, __LINE__ );
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Bans an IP from connecting. Banned IPs persist between connections.
//
// Parameters
// IP - Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
// All IP addresses starting with 128.0.0
// milliseconds - how many ms for a temporary ban.  Use 0 for a permanent ban
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AddToBanList( const char *IP, RakNetTime milliseconds )
{
	unsigned index;
	RakNetTime time = RakNet::GetTime();

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return ;

	// If this guy is already in the ban list, do nothing
	index = 0;

	banListMutex.Lock();

	for ( ; index < banList.Size(); index++ )
	{
		if ( strcmp( IP, banList[ index ]->IP ) == 0 )
		{
			// Already in the ban list.  Just update the time
			if (milliseconds==0)
				banList[ index ]->timeout=0; // Infinite
			else
				banList[ index ]->timeout=time+milliseconds;
			banListMutex.Unlock();
			return;
		}
	}

	banListMutex.Unlock();

	BanStruct *banStruct = RakNet::OP_NEW<BanStruct>( __FILE__, __LINE__ );
	banStruct->IP = (char*) rakMalloc_Ex( 16, __FILE__, __LINE__ );
	if (milliseconds==0)
		banStruct->timeout=0; // Infinite
	else
		banStruct->timeout=time+milliseconds;
	strcpy( banStruct->IP, IP );
	banListMutex.Lock();
	banList.Insert( banStruct, __FILE__, __LINE__ );
	banListMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allows a previously banned IP to connect.
//
// Parameters
// IP - Dotted IP address.  Can use * as a wildcard, such as 128.0.0.* will ban
// All IP addresses starting with 128.0.0
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RemoveFromBanList( const char *IP )
{
	unsigned index;
	BanStruct *temp;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return ;

	index = 0;
	temp=0;

	banListMutex.Lock();

	for ( ; index < banList.Size(); index++ )
	{
		if ( strcmp( IP, banList[ index ]->IP ) == 0 )
		{
			temp = banList[ index ];
			banList[ index ] = banList[ banList.Size() - 1 ];
			banList.RemoveAtIndex( banList.Size() - 1 );
			break;
		}
	}

	banListMutex.Unlock();

	if (temp)
	{
		rakFree_Ex(temp->IP, __FILE__, __LINE__ );
		RakNet::OP_DELETE(temp, __FILE__, __LINE__);
	}

}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allows all previously banned IPs to connect.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearBanList( void )
{
	unsigned index;
	index = 0;
	banListMutex.Lock();

	for ( ; index < banList.Size(); index++ )
	{
		rakFree_Ex(banList[ index ]->IP, __FILE__, __LINE__ );
		RakNet::OP_DELETE(banList[ index ], __FILE__, __LINE__);
	}

	banList.Clear(false, __FILE__, __LINE__);

	banListMutex.Unlock();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetLimitIPConnectionFrequency(bool b)
{
	limitConnectionFrequencyFromTheSameIP=b;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Determines if a particular IP is banned.
//
// Parameters
// IP - Complete dotted IP address
//
// Returns
// True if IP matches any IPs in the ban list, accounting for any wildcards.
// False otherwise.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsBanned( const char *IP )
{
	unsigned banListIndex, characterIndex;
	RakNetTime time;
	BanStruct *temp;

	if ( IP == 0 || IP[ 0 ] == 0 || strlen( IP ) > 15 )
		return false;

	banListIndex = 0;

	if ( banList.Size() == 0 )
		return false; // Skip the mutex if possible

	time = RakNet::GetTime();

	banListMutex.Lock();

	while ( banListIndex < banList.Size() )
	{
		if (banList[ banListIndex ]->timeout>0 && banList[ banListIndex ]->timeout<time)
		{
			// Delete expired ban
			temp = banList[ banListIndex ];
			banList[ banListIndex ] = banList[ banList.Size() - 1 ];
			banList.RemoveAtIndex( banList.Size() - 1 );
			rakFree_Ex(temp->IP, __FILE__, __LINE__ );
			RakNet::OP_DELETE(temp, __FILE__, __LINE__);
		}
		else
		{
			characterIndex = 0;

#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
			while ( true )
			{
				if ( banList[ banListIndex ]->IP[ characterIndex ] == IP[ characterIndex ] )
				{
					// Equal characters

					if ( IP[ characterIndex ] == 0 )
					{
						banListMutex.Unlock();
						// End of the string and the strings match

						return true;
					}

					characterIndex++;
				}

				else
				{
					if ( banList[ banListIndex ]->IP[ characterIndex ] == 0 || IP[ characterIndex ] == 0 )
					{
						// End of one of the strings
						break;
					}

					// Characters do not match
					if ( banList[ banListIndex ]->IP[ characterIndex ] == '*' )
					{
						banListMutex.Unlock();

						// Domain is banned.
						return true;
					}

					// Characters do not match and it is not a *
					break;
				}
			}

			banListIndex++;
		}
	}

	banListMutex.Unlock();

	// No match found.
	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Send a ping to the specified connected system.
//
// Parameters:
// target - who to ping
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::Ping( const SystemAddress target )
{
	PingInternal(target, false, UNRELIABLE);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Send a ping to the specified unconnected system.
// The remote system, if it is Initialized, will respond with ID_PONG.
// The final ping time will be encoded in the following sizeof(RakNetTime) bytes.  (Default is 4 bytes - See __GET_TIME_64BIT in RakNetTypes.h
//
// Parameters:
// host: Either a dotted IP address or a domain name.  Can be 255.255.255.255 for LAN broadcast.
// remotePort: Which port to connect to on the remote machine.
// onlyReplyOnAcceptingConnections: Only request a reply if the remote system has open connections
// connectionSocketIndex Index into the array of socket descriptors passed to socketDescriptors in RakPeer::Startup() to send on.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::Ping( const char* host, unsigned short remotePort, bool onlyReplyOnAcceptingConnections, unsigned connectionSocketIndex )
{
	if ( host == 0 )
		return false;

	// If this assert hits then Startup wasn't called or the call failed.
	RakAssert(connectionSocketIndex < socketList.Size());

//	if ( IsActive() == false )
//		return;

	if ( NonNumericHostString( host ) )
	{
		host = ( char* ) SocketLayer::DomainNameToIP( host );
		if (host==0)
			return false;
	}

	SystemAddress systemAddress;
	systemAddress.SetBinaryAddress(host);
	systemAddress.port=remotePort;

	RakNet::BitStream bitStream( sizeof(unsigned char) + sizeof(RakNetTime) );
	if ( onlyReplyOnAcceptingConnections )
		bitStream.Write((MessageID)ID_PING_OPEN_CONNECTIONS);
	else
		bitStream.Write((MessageID)ID_PING);

	bitStream.Write(RakNet::GetTime());

	bitStream.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));

	unsigned i;
	for (i=0; i < messageHandlerList.Size(); i++)
		messageHandlerList[i]->OnDirectSocketSend((const char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), systemAddress);
	// No timestamp for 255.255.255.255
	unsigned int realIndex = GetRakNetSocketFromUserConnectionSocketIndex(connectionSocketIndex);
	SocketLayer::SendTo( socketList[realIndex]->s, (const char*)bitStream.GetData(), (int) bitStream.GetNumberOfBytesUsed(), ( char* ) host, remotePort, socketList[realIndex]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the average of all ping times read for a specified target
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetAveragePing( const AddressOrGUID systemIdentifier )
{
	int sum, quantity;
	RemoteSystemStruct *remoteSystem = GetRemoteSystem( systemIdentifier, false, false );

	if ( remoteSystem == 0 )
		return -1;

	for ( sum = 0, quantity = 0; quantity < PING_TIMES_ARRAY_SIZE; quantity++ )
	{
		if ( remoteSystem->pingAndClockDifferential[ quantity ].pingTime == 65535 )
			break;
		else
			sum += remoteSystem->pingAndClockDifferential[ quantity ].pingTime;
	}

	if ( quantity > 0 )
		return sum / quantity;
	else
		return -1;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the last ping time read for the specific player or -1 if none read yet
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetLastPing( const AddressOrGUID systemIdentifier ) const
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystem( systemIdentifier, false, false );

	if ( remoteSystem == 0 )
		return -1;

//	return (int)(remoteSystem->reliabilityLayer.GetAckPing()/(RakNetTimeUS)1000);

	if ( remoteSystem->pingAndClockDifferentialWriteIndex == 0 )
		return remoteSystem->pingAndClockDifferential[ PING_TIMES_ARRAY_SIZE - 1 ].pingTime;
	else
		return remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex - 1 ].pingTime;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the lowest ping time read or -1 if none read yet
//
// Parameters:
// target - whose time to read
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetLowestPing( const AddressOrGUID systemIdentifier ) const
{
	RemoteSystemStruct * remoteSystem = GetRemoteSystem( systemIdentifier, false, false );

	if ( remoteSystem == 0 )
		return -1;

	return remoteSystem->lowestPing;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Ping the remote systems every so often.  This is off by default
// This will work anytime
//
// Parameters:
// doPing - True to start occasional pings.  False to stop them.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetOccasionalPing( bool doPing )
{
	occasionalPing = doPing;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Length should be under 400 bytes, as a security measure against flood attacks
// Sets the data to send with an  (LAN server discovery) /(offline ping) response
// See the Ping sample project for how this is used.
// data: a block of data to store, or 0 for none
// length: The length of data in bytes, or 0 for none
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetOfflinePingResponse( const char *data, const unsigned int length )
{
	RakAssert(length < 400);

	rakPeerMutexes[ offlinePingResponse_Mutex ].Lock();
	offlinePingResponse.Reset();

	if ( data && length > 0 )
		offlinePingResponse.Write( data, length );

	rakPeerMutexes[ offlinePingResponse_Mutex ].Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns pointers to a copy of the data passed to SetOfflinePingResponse
// \param[out] data A pointer to a copy of the data passed to \a SetOfflinePingResponse()
// \param[out] length A pointer filled in with the length parameter passed to SetOfflinePingResponse()
// \sa SetOfflinePingResponse
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GetOfflinePingResponse( char **data, unsigned int *length )
{
	rakPeerMutexes[ offlinePingResponse_Mutex ].Lock();
	*data = (char*) offlinePingResponse.GetData();
	*length = (int) offlinePingResponse.GetNumberOfBytesUsed();
	rakPeerMutexes[ offlinePingResponse_Mutex ].Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the unique SystemAddress that represents you on the the network
// Note that unlike in previous versions, this is a struct and is not sequential
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SystemAddress RakPeer::GetInternalID( const SystemAddress systemAddress, const int index ) const
{
	if (systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
	{
		return mySystemAddress[index];
	}
	else
	{

//		SystemAddress returnValue;
		RemoteSystemStruct * remoteSystem = GetRemoteSystemFromSystemAddress( systemAddress, false, true );
		if (remoteSystem==0)
			return UNASSIGNED_SYSTEM_ADDRESS;

		return remoteSystem->theirInternalSystemAddress[index];
		/*
		sockaddr_in sa;
		socklen_t len = sizeof(sa);
		if (getsockname(connectionSockets[remoteSystem->connectionSocketIndex], (sockaddr*)&sa, &len)!=0)
			return UNASSIGNED_SYSTEM_ADDRESS;
		returnValue.port=ntohs(sa.sin_port);
		returnValue.binaryAddress=sa.sin_addr.s_addr;
		return returnValue;
*/



	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Return the unique address identifier that represents you on the the network and is based on your external
// IP / port (the IP / port the specified player uses to communicate with you)
// Note that unlike in previous versions, this is a struct and is not sequential
//
// Parameters:
// target: Which remote system you are referring to for your external ID
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SystemAddress RakPeer::GetExternalID( const SystemAddress target ) const
{
	unsigned i;
	SystemAddress inactiveExternalId;

	inactiveExternalId=UNASSIGNED_SYSTEM_ADDRESS;

	if (target==UNASSIGNED_SYSTEM_ADDRESS)
		return firstExternalID;

	// First check for active connection with this systemAddress
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if (remoteSystemList[ i ].systemAddress == target )
		{
			if ( remoteSystemList[ i ].isActive )
				return remoteSystemList[ i ].myExternalSystemAddress;
			else if (remoteSystemList[ i ].myExternalSystemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
				inactiveExternalId=remoteSystemList[ i ].myExternalSystemAddress;
		}
	}

	return inactiveExternalId;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

const RakNetGUID& RakPeer::GetGuidFromSystemAddress( const SystemAddress input ) const
{
	if (input==UNASSIGNED_SYSTEM_ADDRESS)
		return myGuid;

	if (input.systemIndex!=(SystemIndex)-1 && input.systemIndex<maximumNumberOfPeers && remoteSystemList[ input.systemIndex ].systemAddress == input)
		return remoteSystemList[ input.systemIndex ].guid;

	unsigned int i;
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if (remoteSystemList[ i ].systemAddress == input )
		{
			return remoteSystemList[ i ].guid;
		}
	}

	return UNASSIGNED_RAKNET_GUID;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

unsigned int RakPeer::GetSystemIndexFromGuid( const RakNetGUID input ) const
{
	if (input==UNASSIGNED_RAKNET_GUID)
		return (unsigned int) -1;

	if (input==myGuid)
		return (unsigned int) -1;

	if (input.systemIndex!=(SystemIndex)-1 && input.systemIndex<maximumNumberOfPeers && remoteSystemList[ input.systemIndex ].guid == input)
		return input.systemIndex;

	unsigned int i;
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if (remoteSystemList[ i ].guid == input )
		{
			return i;
		}
	}

	return (unsigned int) -1;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

SystemAddress RakPeer::GetSystemAddressFromGuid( const RakNetGUID input ) const
{
	if (input==UNASSIGNED_RAKNET_GUID)
		return UNASSIGNED_SYSTEM_ADDRESS;

	if (input==myGuid)
		return GetInternalID(UNASSIGNED_SYSTEM_ADDRESS);

	if (input.systemIndex!=(SystemIndex)-1 && input.systemIndex<maximumNumberOfPeers && remoteSystemList[ input.systemIndex ].guid == input)
		return remoteSystemList[ input.systemIndex ].systemAddress;

	unsigned int i;
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if (remoteSystemList[ i ].guid == input )
		{
			return remoteSystemList[ i ].systemAddress;
		}
	}

	return UNASSIGNED_SYSTEM_ADDRESS;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Set the time, in MS, to use before considering ourselves disconnected after not being able to deliver a reliable packet
// \param[in] time Time, in MS
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetTimeoutTime( RakNetTime timeMS, const SystemAddress target )
{
	if (target==UNASSIGNED_SYSTEM_ADDRESS)
	{
		defaultTimeoutTime=timeMS;

		unsigned i;
		for ( i = 0; i < maximumNumberOfPeers; i++ )
		{
			if (remoteSystemList[ i ].isActive)
			{
				if ( remoteSystemList[ i ].isActive )
					remoteSystemList[ i ].reliabilityLayer.SetTimeoutTime(timeMS);
			}
		}
	}
	else
	{
		RemoteSystemStruct * remoteSystem = GetRemoteSystemFromSystemAddress( target, false, true );

		if ( remoteSystem != 0 )
			remoteSystem->reliabilityLayer.SetTimeoutTime(timeMS);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

RakNetTime RakPeer::GetTimeoutTime( const SystemAddress target )
{
	if (target==UNASSIGNED_SYSTEM_ADDRESS)
	{
		return defaultTimeoutTime;
	}
	else
	{
		RemoteSystemStruct * remoteSystem = GetRemoteSystemFromSystemAddress( target, false, true );

		if ( remoteSystem != 0 )
			remoteSystem->reliabilityLayer.GetTimeoutTime();
	}
	return defaultTimeoutTime;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Change the MTU size in order to improve performance when sending large packets
// This can only be called when not connected.
// A too high of value will cause packets not to arrive at worst and be fragmented at best.
// A too low of value will split packets unnecessarily.
//
// Parameters:
// size: Set according to the following table:
// 1500. The largest Ethernet packet size
// This is the typical setting for non-PPPoE, non-VPN connections. The default value for NETGEAR routers, adapters and switches.
// 1492. The size PPPoE prefers.
// 1472. Maximum size to use for pinging. (Bigger packets are fragmented.)
// 1468. The size DHCP prefers.
// 1460. Usable by AOL if you don't have large email attachments, etc.
// 1430. The size VPN and PPTP prefer.
// 1400. Maximum size for AOL DSL.
// 576. Typical value to connect to dial-up ISPs. (Default)
//
// Returns:
// False on failure (we are connected).  True on success.  Maximum allowed size is MAXIMUM_MTU_SIZE
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
bool RakPeer::SetMTUSize( int size, const SystemAddress target )
{
	if ( IsActive() )
		return false;

	if ( size < 512 )
		size = 512;
	else if ( size > MAXIMUM_MTU_SIZE )
		size = MAXIMUM_MTU_SIZE;

	if (target==UNASSIGNED_SYSTEM_ADDRESS)
	{
		defaultMTUSize = size;

		int i;
		// Active connections take priority.  But if there are no active connections, return the first systemAddress match found
		for ( i = 0; i < maximumNumberOfPeers; i++ )
		{
			remoteSystemList[i].MTUSize=size;
		}
	}
	else
	{
		RemoteSystemStruct *rss=GetRemoteSystemFromSystemAddress(target, false, true);
		if (rss)
			rss->MTUSize=size;
	}

	return true;
	}
*/

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the current MTU size
//
// Returns:
// The MTU sized specified in SetMTUSize
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetMTUSize( const SystemAddress target ) const
{
	if (target!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		RemoteSystemStruct *rss=GetRemoteSystemFromSystemAddress(target, false, true);
		if (rss)
			return rss->MTUSize;
	}
	return defaultMTUSize;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the number of IP addresses we have
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GetNumberOfAddresses( void )
{

	int i = 0;

	while ( ipList[ i ][ 0 ] )
		i++;

	return i;




}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns an IP address at index 0 to GetNumberOfAddresses-1
// \param[in] index index into the list of IP addresses
// \return The local IP address at this index
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
const char* RakPeer::GetLocalIP( unsigned int index )
{
	if (IsActive()==false)
	{
	// Fill out ipList structure

	memset( ipList, 0, sizeof( char ) * 16 * MAXIMUM_NUMBER_OF_INTERNAL_IDS );
	SocketLayer::GetMyIP( ipList,binaryAddresses );

	}


	return ipList[ index ];




}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Is this a local IP?
// \param[in] An IP address to check
// \return True if this is one of the IP addresses returned by GetLocalIP
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsLocalIP( const char *ip )
{
	if (ip==0 || ip[0]==0)
		return false;


	if (strcmp(ip, "127.0.0.1")==0 || strcmp(ip, "localhost")==0)
		return true;

	int num = GetNumberOfAddresses();
	int i;
	for (i=0; i < num; i++)
	{
		if (strcmp(ip, GetLocalIP(i))==0)
			return true;
	}




	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Allow or disallow connection responses from any IP. Normally this should be false, but may be necessary
// when connection to servers with multiple IP addresses
//
// Parameters:
// allow - True to allow this behavior, false to not allow.  Defaults to false.  Value persists between connections
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AllowConnectionResponseIPMigration( bool allow )
{
	allowConnectionResponseIPMigration = allow;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Sends a message ID_ADVERTISE_SYSTEM to the remote unconnected system.
// This will tell the remote system our external IP outside the LAN, and can be used for NAT punch through
//
// Requires:
// The sender and recipient must already be started via a successful call to Initialize
//
// host: Either a dotted IP address or a domain name
// remotePort: Which port to connect to on the remote machine.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::AdvertiseSystem( const char *host, unsigned short remotePort, const char *data, int dataLength, unsigned connectionSocketIndex )
{
	RakNet::BitStream bs;
	bs.Write((MessageID)ID_ADVERTISE_SYSTEM);
	bs.WriteAlignedBytes((const unsigned char*) data,dataLength);
	return SendOutOfBand(host, remotePort, (const char*) bs.GetData(), bs.GetNumberOfBytesUsed(), connectionSocketIndex );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Controls how often to return ID_DOWNLOAD_PROGRESS for large message downloads.
// ID_DOWNLOAD_PROGRESS is returned to indicate a new partial message chunk, roughly the MTU size, has arrived
// As it can be slow or cumbersome to get this notification for every chunk, you can set the interval at which it is returned.
// Defaults to 0 (never return this notification)
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetSplitMessageProgressInterval(int interval)
{
	RakAssert(interval>=0);
	splitMessageProgressInterval=interval;
	for ( unsigned short i = 0; i < maximumNumberOfPeers; i++ )
		remoteSystemList[ i ].reliabilityLayer.SetSplitMessageProgressInterval(splitMessageProgressInterval);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns what was passed to SetSplitMessageProgressInterval()
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetSplitMessageProgressInterval(void) const
{
	return splitMessageProgressInterval;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Set how long to wait before giving up on sending an unreliable message
// Useful if the network is clogged up.
// Set to 0 or less to never timeout.  Defaults to 0.
// timeoutMS How many ms to wait before simply not sending an unreliable message.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetUnreliableTimeout(RakNetTime timeoutMS)
{
	unreliableTimeout=timeoutMS;
	for ( unsigned short i = 0; i < maximumNumberOfPeers; i++ )
		remoteSystemList[ i ].reliabilityLayer.SetUnreliableTimeout(unreliableTimeout);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Send a message to host, with the IP socket option TTL set to 3
// This message will not reach the host, but will open the router.
// Used for NAT-Punchthrough
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendTTL( const char* host, unsigned short remotePort, int ttl, unsigned connectionSocketIndex )
{
	char fakeData[2];
	fakeData[0]=0;
	fakeData[1]=1;
	unsigned int realIndex = GetRakNetSocketFromUserConnectionSocketIndex(connectionSocketIndex);
	SocketLayer::SendToTTL( socketList[realIndex]->s, (char*)fakeData, 2, (char*) host, remotePort, ttl );
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Enables or disables our tracking of bytes input to and output from the network.
// This is required to get a frequency table, which is used to generate a new compression layer.
// You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only track
// part of the values sent over the network.
// This value persists between connect calls and defaults to false (no frequency tracking)
//
// Parameters:
// doCompile - true to track bytes.  Defaults to false
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetCompileFrequencyTable( bool doCompile )
{
	trackFrequencyTable = doCompile;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Returns the frequency of outgoing bytes into outputFrequencyTable
// The purpose is to save to file as either a master frequency table from a sample game session for passing to
// GenerateCompressionLayer(false)
// You should only call this when disconnected.
// Requires that you first enable data frequency tracking by calling SetCompileFrequencyTable(true)
//
// Parameters:
// outputFrequencyTable (out): The frequency of each corresponding byte
//
// Returns:
// Ffalse (failure) if connected or if frequency table tracking is not enabled.  Otherwise true (success)
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GetOutgoingFrequencyTable( unsigned int outputFrequencyTable[ 256 ] )
{
	if ( IsActive() )
		return false;

	if ( trackFrequencyTable == false )
		return false;

	memcpy( outputFrequencyTable, frequencyTable, sizeof( unsigned int ) * 256 );

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Generates the compression layer from the input frequency table.
// You should call this twice - once with inputLayer as true and once as false.
// The frequency table passed here with inputLayer=true should match the frequency table on the recipient with inputLayer=false.
// Likewise, the frequency table passed here with inputLayer=false should match the frequency table on the recipient with inputLayer=true
// Calling this function when there is an existing layer will overwrite the old layer
// You should only call this when disconnected
//
// Parameters:
// inputFrequencyTable: The frequency table returned from GetSendFrequencyTable(...)
// inputLayer - Whether inputFrequencyTable represents incoming data from other systems (true) or outgoing data from this system (false)
//
// Returns:
// False on failure (we are connected).  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer )
{
	if ( IsActive() )
		return false;

	DeleteCompressionLayer( inputLayer );

	if ( inputLayer )
	{
		inputTree = RakNet::OP_NEW<HuffmanEncodingTree>( __FILE__, __LINE__ );
		inputTree->GenerateFromFrequencyTable( inputFrequencyTable );
	}

	else
	{
		outputTree = RakNet::OP_NEW<HuffmanEncodingTree>( __FILE__, __LINE__ );
		outputTree->GenerateFromFrequencyTable( inputFrequencyTable );
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Deletes the output or input layer as specified.  This is not necessary to call and is only valuable for freeing memory
// You should only call this when disconnected
//
// Parameters:
// inputLayer - Specifies the corresponding compression layer generated by GenerateCompressionLayer.
//
// Returns:
// False on failure (we are connected).  True otherwise
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::DeleteCompressionLayer( bool inputLayer )
{
	if ( IsActive() )
		return false;

	if ( inputLayer )
	{
		if ( inputTree )
		{
			RakNet::OP_DELETE(inputTree, __FILE__, __LINE__);
			inputTree = 0;
		}
	}

	else
	{
		if ( outputTree )
		{
			RakNet::OP_DELETE(outputTree, __FILE__, __LINE__);
			outputTree = 0;
		}
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns:
// The compression ratio.  A low compression ratio is good.  Compression is for outgoing data
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float RakPeer::GetCompressionRatio( void ) const
{
	if ( rawBytesSent > 0 )
	{
		return ( float ) compressedBytesSent / ( float ) rawBytesSent;
	}

	else
		return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns:
// The decompression ratio.  A high decompression ratio is good.  Decompression is for incoming data
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
float RakPeer::GetDecompressionRatio( void ) const
{
	if ( rawBytesReceived > 0 )
	{
		return ( float ) compressedBytesReceived / ( float ) rawBytesReceived;
	}

	else
		return 0.0f;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Attatches a Plugin interface to run code automatically on message receipt in the Receive call
//
// \param messageHandler Pointer to a plugin to attach
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::AttachPlugin( PluginInterface2 *plugin )
{
	if (messageHandlerList.GetIndexOf(plugin)==MAX_UNSIGNED_LONG)
	{
		plugin->SetRakPeerInterface(this);
		plugin->OnAttach();
		messageHandlerList.Insert(plugin, __FILE__, __LINE__);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Detaches a Plugin interface to run code automatically on message receipt
//
// \param messageHandler Pointer to a plugin to detach
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DetachPlugin( PluginInterface2 *plugin )
{
	if (plugin==0)
		return;

	unsigned int index;
	index = messageHandlerList.GetIndexOf(plugin);
	if (index!=MAX_UNSIGNED_LONG)
	{
		// Unordered list so delete from end for speed
		messageHandlerList[index]=messageHandlerList[messageHandlerList.Size()-1];
		messageHandlerList.RemoveFromEnd();
		plugin->OnDetach();
		plugin->SetRakPeerInterface(0);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Put a packet back at the end of the receive queue in case you don't want to deal with it immediately
//
// packet The packet you want to push back.
// pushAtHead True to push the packet so that the next receive call returns it.  False to push it at the end of the queue (obviously pushing it at the end makes the packets out of order)
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::PushBackPacket( Packet *packet, bool pushAtHead)
{
	if (packet==0)
		return;

	unsigned i;
	for (i=0; i < messageHandlerList.Size(); i++)
		messageHandlerList[i]->OnPushBackPacket((const char*) packet->data, packet->bitSize, packet->systemAddress);

	packetReturnMutex.Lock();
	if (pushAtHead)
		packetReturnQueue.PushAtHead(packet,0,__FILE__,__LINE__);
	else
		packetReturnQueue.Push(packet,__FILE__,__LINE__);
	packetReturnMutex.Unlock();
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetRouterInterface( RouterInterface *routerInterface )
{
	router=routerInterface;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RemoveRouterInterface( RouterInterface *routerInterface )
{
	if (router==routerInterface)
		router=0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ChangeSystemAddress(RakNetGUID guid, SystemAddress systemAddress)
{
	BufferedCommandStruct *bcs;

#ifdef _RAKNET_THREADSAFE
	bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
#else
	bcs=bufferedCommands.WriteLock();
#endif
	bcs->data = 0;
	bcs->systemIdentifier.systemAddress=systemAddress;
	bcs->systemIdentifier.rakNetGuid=guid;
	bcs->command=BufferedCommandStruct::BCS_CHANGE_SYSTEM_ADDRESS;
#ifdef _RAKNET_THREADSAFE
	bufferedCommands.Push(bcs);
#else
	bufferedCommands.WriteUnlock();
#endif
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Packet* RakPeer::AllocatePacket(unsigned dataSize)
{
	return AllocPacket(dataSize, __FILE__, __LINE__);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNetSmartPtr<RakNetSocket> RakPeer::GetSocket( const SystemAddress target )
{
	// Send a query to the thread to get the socket, and return when we got it
	BufferedCommandStruct *bcs;
#ifdef _RAKNET_THREADSAFE
	bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
	bcs->command=BufferedCommandStruct::BCS_GET_SOCKET;
	bcs->systemIdentifier=target;
	bcs->data=0;
	bufferedCommands.Push(bcs);
#else
	bcs=bufferedCommands.WriteLock();
	bcs->command=BufferedCommandStruct::BCS_GET_SOCKET;
	bcs->systemIdentifier=target;
	bcs->data=0;
	bufferedCommands.WriteUnlock();
#endif

	// Block up to one second to get the socket, although it should actually take virtually no time
	SocketQueryOutput *sqo;
	RakNetTime stopWaiting = RakNet::GetTime()+1000;
	DataStructures::List<RakNetSmartPtr<RakNetSocket> > output;
	while (RakNet::GetTime() < stopWaiting)
	{
		if (isMainLoopThreadActive==false)
			return RakNetSmartPtr<RakNetSocket>();

		RakSleep(0);

#ifdef _RAKNET_THREADSAFE
		sqo = socketQueryOutput.Pop();
		if (sqo)
		{
			output=sqo->sockets;
			sqo->sockets.Clear(false, __FILE__, __LINE__);
			socketQueryOutput.Deallocate(sqo, __FILE__,__LINE__);
			if (output.Size())
				return output[0];
			break;
		}
#else
		sqo = socketQueryOutput.ReadLock();
		if (sqo)
		{
			output=sqo->sockets;
			sqo->sockets.Clear(false, __FILE__, __LINE__);
			socketQueryOutput.ReadUnlock();
			if (output.Size())
				return output[0];
			break;
		}
#endif		
	}
	return RakNetSmartPtr<RakNetSocket>();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GetSockets( DataStructures::List<RakNetSmartPtr<RakNetSocket> > &sockets )
{
	sockets.Clear(false, __FILE__, __LINE__);

	// Send a query to the thread to get the socket, and return when we got it
	BufferedCommandStruct *bcs;

#ifdef _RAKNET_THREADSAFE
	bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
	bcs->command=BufferedCommandStruct::BCS_GET_SOCKET;
	bcs->systemIdentifier=UNASSIGNED_SYSTEM_ADDRESS;
	bcs->data=0;
	bufferedCommands.Push(bcs);
#else
	bcs=bufferedCommands.WriteLock();
	bcs->command=BufferedCommandStruct::BCS_GET_SOCKET;
	bcs->systemIdentifier=UNASSIGNED_SYSTEM_ADDRESS;
	bcs->data=0;
	bufferedCommands.WriteUnlock();
#endif

	// Block up to one second to get the socket, although it should actually take virtually no time
	SocketQueryOutput *sqo;
	RakNetTime stopWaiting = RakNet::GetTime()+1000;
	RakNetSmartPtr<RakNetSocket> output;
	while (RakNet::GetTime() < stopWaiting)
	{
		if (isMainLoopThreadActive==false)
			return;

		RakSleep(0);

#ifdef _RAKNET_THREADSAFE
		sqo = socketQueryOutput.Pop();
		if (sqo)
		{
			sockets=sqo->sockets;
			sqo->sockets.Clear(false, __FILE__, __LINE__);
			socketQueryOutput.Deallocate(sqo, __FILE__,__LINE__);
			return;
		}
#else
		sqo = socketQueryOutput.ReadLock();
		if (sqo)
		{
			sockets=sqo->sockets;
			sqo->sockets.Clear(false, __FILE__, __LINE__);
			socketQueryOutput.ReadUnlock();
			return;
		}
#endif	
	}
	return;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Adds simulated ping and packet loss to the outgoing data flow.
// To simulate bi-directional ping and packet loss, you should call this on both the sender and the recipient, with half the total ping and maxSendBPS value on each.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ApplyNetworkSimulator( float packetloss, unsigned short minExtraPing, unsigned short extraPingVariance)
{
#ifdef _DEBUG
	if (remoteSystemList)
	{
		unsigned short i;
		for (i=0; i < maximumNumberOfPeers; i++)
		//for (i=0; i < remoteSystemListSize; i++)
			remoteSystemList[i].reliabilityLayer.ApplyNetworkSimulator(packetloss, minExtraPing, extraPingVariance);
	}

	_packetloss=packetloss;
	_minExtraPing=minExtraPing;
	_extraPingVariance=extraPingVariance;
#endif
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void RakPeer::SetPerConnectionOutgoingBandwidthLimit( unsigned maxBitsPerSecond )
{
	maxOutgoingBPS=maxBitsPerSecond;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Returns if you previously called ApplyNetworkSimulator
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsNetworkSimulatorActive( void )
{
#ifdef _DEBUG
	return _packetloss>0 || _minExtraPing>0 || _extraPingVariance>0;
#else
	return false;
#endif
}
/*
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Have RakNet use a socket you created yourself
// The socket should not be in use - it is up to you to either shutdown or close the connections using it. Otherwise existing connections on that socket will eventually disconnect
// This socket will be forgotten after calling Shutdown(), so rebind again if you need to.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::UseUserSocket( int socket, bool haveRakNetCloseSocket, unsigned connectionSocketIndex)
{
	BufferedCommandStruct *bcs;
#ifdef _RAKNET_THREADSAFE
	rakPeerMutexes[bufferedCommands_Mutex].Lock();
#endif
	bcs=bufferedCommands.WriteLock();
	bcs->command=BufferedCommandStruct::BCS_USE_USER_SOCKET;
	bcs->data=0;
	bcs->socket=socket;
	bcs->haveRakNetCloseSocket=haveRakNetCloseSocket;
	bcs->connectionSocketIndex=connectionSocketIndex;
	bufferedCommands.WriteUnlock();
#ifdef _RAKNET_THREADSAFE
	rakPeerMutexes[bufferedCommands_Mutex].Unlock();
#endif
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Have RakNet recreate a socket using a different port.
// The socket should not be in use - it is up to you to either shutdown or close the connections using it. Otherwise existing connections on that socket will eventually disconnect
// \param[in] connectionSocketIndex Index into the array of socket descriptors passed to socketDescriptors in RakPeer::Startup() to send on.
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::RebindSocketAddress(unsigned connectionSocketIndex, SocketDescriptor &sd)
{
	BufferedCommandStruct *bcs;
#ifdef _RAKNET_THREADSAFE
	rakPeerMutexes[bufferedCommands_Mutex].Lock();
#endif
	bcs=bufferedCommands.WriteLock();
	bcs->command=BufferedCommandStruct::BCS_REBIND_SOCKET_ADDRESS;
	bcs->data=(char*) rakMalloc_Ex(sizeof(sd.hostAddress), __FILE__, __LINE__);
	memcpy(bcs->data, sd.hostAddress, sizeof(sd.hostAddress));
	bcs->port=sd.port;
	bcs->connectionSocketIndex=connectionSocketIndex;
	bcs->socketType=sd.socketType;
	bufferedCommands.WriteUnlock();
#ifdef _RAKNET_THREADSAFE
	rakPeerMutexes[bufferedCommands_Mutex].Unlock();
#endif
}
*/


// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// For internal use
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
char *RakPeer::GetRPCString( const char *data, const BitSize_t bitSize, const SystemAddress systemAddress)
{
	bool nameIsEncoded=false;
	static char uniqueIdentifier[256];
	RPCIndex rpcIndex;
	RPCMap *_rpcMap;
	RakNet::BitStream rpcDecode((unsigned char*) data, BITS_TO_BYTES(bitSize), false);
	rpcDecode.IgnoreBits(8);
	if (data[0]==ID_TIMESTAMP)
		rpcDecode.IgnoreBits(sizeof(unsigned char)+sizeof(RakNetTime));
	rpcDecode.Read(nameIsEncoded);
	if (nameIsEncoded)
	{
		stringCompressor->DecodeString((char*)uniqueIdentifier, 256, &rpcDecode);
	}
	else
	{
		rpcDecode.ReadCompressed( rpcIndex );
		RPCNode *rpcNode;

		if (systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
			_rpcMap=&rpcMap;
		else
		{
			RemoteSystemStruct *rss=GetRemoteSystemFromSystemAddress(systemAddress, false, true);
			if (rss)
				_rpcMap=&(rss->rpcMap);
			else
				_rpcMap=0;
		}

		if (_rpcMap)
			rpcNode = _rpcMap->GetNodeFromIndex(rpcIndex);
		else
			rpcNode=0;

		if (_rpcMap && rpcNode)
			strcpy((char*)uniqueIdentifier, rpcNode->uniqueIdentifier);
		else
			strcpy((char*)uniqueIdentifier, "[UNKNOWN]");
	}

	return uniqueIdentifier;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::WriteOutOfBandHeader(RakNet::BitStream *bitStream)
{
	bitStream->Write((MessageID)ID_OUT_OF_BAND_INTERNAL);
 	bitStream->Write(myGuid);
	bitStream->WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SetUserUpdateThread(void (*_userUpdateThreadPtr)(RakPeerInterface *, void *), void *_userUpdateThreadData)
{
	userUpdateThreadPtr=_userUpdateThreadPtr;
	userUpdateThreadData=_userUpdateThreadData;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendOutOfBand(const char *host, unsigned short remotePort, const char *data, BitSize_t dataLength, unsigned connectionSocketIndex )
{
	if ( IsActive() == false )
		return false;

	if (host==0  || host[0]==0)
		return false;

	// If this assert hits then Startup wasn't called or the call failed.
	RakAssert(connectionSocketIndex < socketList.Size());

	// This is a security measure.  Don't send data longer than this value
	RakAssert(dataLength <= MAX_OFFLINE_DATA_LENGTH);

	if ( NonNumericHostString( host ) )
	{
		host = ( char* ) SocketLayer::DomainNameToIP( host );

		if (host==0)
			return false;
	}

	if (host==0)
		return false;

	SystemAddress systemAddress;
	systemAddress.SetBinaryAddress(host);
	systemAddress.port=remotePort;

	// 34 bytes
	RakNet::BitStream bitStream;
	WriteOutOfBandHeader(&bitStream);
	
	if (dataLength>0)
	{
		bitStream.Write(data, dataLength);
	}

	if (IsActive())
	{
		BufferedCommandStruct *bcs;
#ifdef _RAKNET_THREADSAFE
		bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
		bcs->command=BufferedCommandStruct::BCS_SEND_OUT_OF_BAND;
		bcs->connectionSocketIndex=connectionSocketIndex;
		bcs->data=(char*) rakMalloc_Ex(bitStream.GetNumberOfBytesUsed(), __FILE__, __LINE__);
		bcs->numberOfBitsToSend=bitStream.GetNumberOfBitsUsed();
		memcpy(bcs->data, bitStream.GetData(), bitStream.GetNumberOfBytesUsed());
		bcs->systemIdentifier.systemAddress.SetBinaryAddress(host);
		bcs->systemIdentifier.systemAddress.port=remotePort;
		bufferedCommands.Push(bcs);
#else
		bcs=bufferedCommands.WriteLock();
		bcs->command=BufferedCommandStruct::BCS_SEND_OUT_OF_BAND;
		bcs->connectionSocketIndex=connectionSocketIndex;
		bcs->data=(char*) rakMalloc_Ex(bitStream.GetNumberOfBytesUsed(), __FILE__, __LINE__);
		bcs->numberOfBitsToSend=bitStream.GetNumberOfBitsUsed();
		memcpy(bcs->data, bitStream.GetData(), bitStream.GetNumberOfBytesUsed());
		bcs->systemIdentifier.systemAddress.SetBinaryAddress(host);
		bcs->systemIdentifier.systemAddress.port=remotePort;
		bufferedCommands.WriteUnlock();
#endif
	}
	else
	{
		unsigned i;
		for (i=0; i < messageHandlerList.Size(); i++)
			messageHandlerList[i]->OnDirectSocketSend((const char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), systemAddress);
		unsigned int realIndex = GetRakNetSocketFromUserConnectionSocketIndex(connectionSocketIndex);
		SocketLayer::SendTo( socketList[realIndex]->s, (const char*)bitStream.GetData(), (int) bitStream.GetNumberOfBytesUsed(), ( char* ) host, remotePort, socketList[realIndex]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakNetStatistics * const RakPeer::GetStatistics( const SystemAddress systemAddress, RakNetStatistics *rns )
{
	static RakNetStatistics staticStatistics;
	RakNetStatistics *systemStats;
	if (rns==0)
		systemStats=&staticStatistics;
	else
		systemStats=rns;

	if (systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
	{
		bool firstWrite=false;
		// Return a crude sum
		for ( unsigned short i = 0; i < maximumNumberOfPeers; i++ )
		{
			if (remoteSystemList[ i ].isActive)
			{
				RakNetStatistics rnsTemp;
				remoteSystemList[ i ].reliabilityLayer.GetStatistics(&rnsTemp);

				if (firstWrite==false)
				{
					memcpy(systemStats, &rnsTemp, sizeof(RakNetStatistics));
					firstWrite=true;
				}
				else
					(*systemStats)+=rnsTemp;
			}
		}
		return systemStats;
	}
	else
	{
		RemoteSystemStruct * rss;
		rss = GetRemoteSystemFromSystemAddress( systemAddress, false, false );
		if ( rss && endThreads==false )
		{
			rss->reliabilityLayer.GetStatistics(systemStats);
			return systemStats;
		}
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::GetStatistics( const int index, RakNetStatistics *rns )
{
	if (index < maximumNumberOfPeers && remoteSystemList[ index ].isActive)
	{
		remoteSystemList[ index ].reliabilityLayer.GetStatistics(rns);
		return true;
	}
	return false;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GetReceiveBufferSize(void)
{
	unsigned int size;
	packetReturnMutex.Lock();
	size=packetReturnQueue.Size();
	packetReturnMutex.Unlock();
	return size;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetIndexFromSystemAddress( const SystemAddress systemAddress, bool calledFromNetworkThread ) const
{
	unsigned i;

	if ( systemAddress == UNASSIGNED_SYSTEM_ADDRESS )
		return -1;

	if (systemAddress.systemIndex!=(SystemIndex)-1 && systemAddress.systemIndex < maximumNumberOfPeers && remoteSystemList[systemAddress.systemIndex].systemAddress==systemAddress && remoteSystemList[ systemAddress.systemIndex ].isActive)
		return systemAddress.systemIndex;
	
	if (calledFromNetworkThread)
	{
		return GetRemoteSystemIndex(systemAddress);
	}
	else
	{
		// remoteSystemList in user and network thread
		for ( i = 0; i < maximumNumberOfPeers; i++ )
			if ( remoteSystemList[ i ].isActive && remoteSystemList[ i ].systemAddress == systemAddress )
				return i;

		// If no active results found, try previously active results.
		for ( i = 0; i < maximumNumberOfPeers; i++ )
			if ( remoteSystemList[ i ].systemAddress == systemAddress )
				return i;
	}

	return -1;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int RakPeer::GetIndexFromGuid( const RakNetGUID guid )
{
	unsigned i;

	if ( guid == UNASSIGNED_RAKNET_GUID )
		return -1;

	if (guid.systemIndex!=(SystemIndex)-1 && guid.systemIndex < maximumNumberOfPeers && remoteSystemList[guid.systemIndex].guid==guid && remoteSystemList[ guid.systemIndex ].isActive)
		return guid.systemIndex;

	// remoteSystemList in user and network thread
	for ( i = 0; i < maximumNumberOfPeers; i++ )
		if ( remoteSystemList[ i ].isActive && remoteSystemList[ i ].guid == guid )
			return i;

	// If no active results found, try previously active results.
	for ( i = 0; i < maximumNumberOfPeers; i++ )
		if ( remoteSystemList[ i ].guid == guid )
			return i;

	return -1;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendConnectionRequest( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex, unsigned int extraData, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime )
{
	RakAssert(passwordDataLength <= 256);
	RakAssert(remotePort!=0);
	SystemAddress systemAddress;
	systemAddress.SetBinaryAddress(host);
	systemAddress.port=remotePort;

	// Already connected?
	if (GetRemoteSystemFromSystemAddress(systemAddress, false, true))
		return false;

	//RequestedConnectionStruct *rcs = (RequestedConnectionStruct *) rakMalloc_Ex(sizeof(RequestedConnectionStruct), __FILE__, __LINE__);
	RequestedConnectionStruct *rcs = RakNet::OP_NEW<RequestedConnectionStruct>(__FILE__,__LINE__);

	rcs->systemAddress=systemAddress;
	rcs->nextRequestTime=RakNet::GetTime();
	rcs->requestsMade=0;
	rcs->data=0;
	rcs->extraData=extraData;
	rcs->socketIndex=connectionSocketIndex;
	rcs->actionToTake=RequestedConnectionStruct::CONNECT;
	rcs->sendConnectionAttemptCount=sendConnectionAttemptCount;
	rcs->timeBetweenSendConnectionAttemptsMS=timeBetweenSendConnectionAttemptsMS;
	memcpy(rcs->outgoingPassword, passwordData, passwordDataLength);
	rcs->outgoingPasswordLength=(unsigned char) passwordDataLength;
	rcs->timeoutTime=timeoutTime;

	// Return false if already pending, else push on queue
	unsigned int i=0;
	requestedConnectionQueueMutex.Lock();
	for (; i < requestedConnectionQueue.Size(); i++)
	{
		if (requestedConnectionQueue[i]->systemAddress==systemAddress)
		{
			requestedConnectionQueueMutex.Unlock();
			RakNet::OP_DELETE(rcs,__FILE__,__LINE__);
			return false;
		}
	}
	requestedConnectionQueue.Push(rcs, __FILE__, __LINE__ );
	requestedConnectionQueueMutex.Unlock();

	return true;
}
bool RakPeer::SendConnectionRequest( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex, unsigned int extraData, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime, RakNetSmartPtr<RakNetSocket> socket )
{
	RakAssert(passwordDataLength <= 256);
	SystemAddress systemAddress;
	systemAddress.SetBinaryAddress(host);
	systemAddress.port=remotePort;

	// Already connected?
	if (GetRemoteSystemFromSystemAddress(systemAddress, false, true))
		return false;

	//RequestedConnectionStruct *rcs = (RequestedConnectionStruct *) rakMalloc_Ex(sizeof(RequestedConnectionStruct), __FILE__, __LINE__);
	RequestedConnectionStruct *rcs = RakNet::OP_NEW<RequestedConnectionStruct>(__FILE__,__LINE__);

	rcs->systemAddress=systemAddress;
	rcs->nextRequestTime=RakNet::GetTime();
	rcs->requestsMade=0;
	rcs->data=0;
	rcs->extraData=extraData;
	rcs->socketIndex=connectionSocketIndex;
	rcs->actionToTake=RequestedConnectionStruct::CONNECT;
	rcs->sendConnectionAttemptCount=sendConnectionAttemptCount;
	rcs->timeBetweenSendConnectionAttemptsMS=timeBetweenSendConnectionAttemptsMS;
	memcpy(rcs->outgoingPassword, passwordData, passwordDataLength);
	rcs->outgoingPasswordLength=(unsigned char) passwordDataLength;
	rcs->timeoutTime=timeoutTime;
	rcs->socket=socket;

	// Return false if already pending, else push on queue
	unsigned int i=0;
	requestedConnectionQueueMutex.Lock();
	for (; i < requestedConnectionQueue.Size(); i++)
	{
		if (requestedConnectionQueue[i]->systemAddress==systemAddress)
		{
			requestedConnectionQueueMutex.Unlock();
			RakNet::OP_DELETE(rcs,__FILE__,__LINE__);
			return false;
		}
	}
	requestedConnectionQueue.Push(rcs, __FILE__, __LINE__ );
	requestedConnectionQueueMutex.Unlock();

	return true;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ValidateRemoteSystemLookup(void) const
{
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct *RakPeer::GetRemoteSystem( const AddressOrGUID systemIdentifier, bool calledFromNetworkThread, bool onlyActive ) const
{
	if (systemIdentifier.rakNetGuid!=UNASSIGNED_RAKNET_GUID)
		return GetRemoteSystemFromGUID(systemIdentifier.rakNetGuid, onlyActive);
	else
		return GetRemoteSystemFromSystemAddress(systemIdentifier.systemAddress, calledFromNetworkThread, onlyActive);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct *RakPeer::GetRemoteSystemFromSystemAddress( const SystemAddress systemAddress, bool calledFromNetworkThread, bool onlyActive ) const
{
	unsigned i;

	if ( systemAddress == UNASSIGNED_SYSTEM_ADDRESS )
		return 0;

	if (calledFromNetworkThread)
	{
		unsigned int index = GetRemoteSystemIndex(systemAddress);
		if (index!=(unsigned int) -1)
		{
			if (onlyActive==false || remoteSystemList[ index ].isActive==true )
			{
				RakAssert(remoteSystemList[index].systemAddress==systemAddress);
				return remoteSystemList + index;
			}
		}
	}
	else
	{
		int deadConnectionIndex=-1;

		// Active connections take priority.  But if there are no active connections, return the first systemAddress match found
		for ( i = 0; i < maximumNumberOfPeers; i++ )
		{
			if (remoteSystemList[ i ].systemAddress == systemAddress)
			{
				if ( remoteSystemList[ i ].isActive )
					return remoteSystemList + i;
				else if (deadConnectionIndex==-1)
					deadConnectionIndex=i;
			}
		}

		if (deadConnectionIndex!=-1 && onlyActive==false)
			return remoteSystemList + deadConnectionIndex;
	}

	return 0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct *RakPeer::GetRemoteSystemFromGUID( const RakNetGUID guid, bool onlyActive ) const
{
	if (guid==UNASSIGNED_RAKNET_GUID)
		return 0;
	
	unsigned i;
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	{
		if (remoteSystemList[ i ].guid == guid && (onlyActive==false || remoteSystemList[ i ].isActive))
		{
			return remoteSystemList + i;
		}
	}
	return 0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ParseConnectionRequestPacket( RakPeer::RemoteSystemStruct *remoteSystem, SystemAddress systemAddress, const char *data, int byteSize )
{
	RakNet::BitStream bs((unsigned char*) data,byteSize,false);
	bs.IgnoreBytes(sizeof(MessageID));
	bs.IgnoreBytes(sizeof(OFFLINE_MESSAGE_DATA_ID));
	RakNetGUID guid;
	bs.Read(guid);
	RakNetTime incomingTimestamp;
	bs.Read(incomingTimestamp);

	// If we are full tell the sender.
	// Not needed
	if ( 0 ) //!AllowIncomingConnections() )
	{
		RakNet::BitStream bs;
		bs.Write((MessageID)ID_NO_FREE_INCOMING_CONNECTIONS);
		bs.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
		bs.Write(GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
		SendImmediate((char*) bs.GetData(), bs.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, RELIABLE, 0, systemAddress, false, false, RakNet::GetTimeNS(), 0);
		remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY;
	}
	else
	{
		const char *password = data + sizeof(MessageID) + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID) + sizeof(RakNetTime);
		int passwordLength = byteSize - (int) (sizeof(MessageID) + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID) + sizeof(RakNetTime));

		if ( incomingPasswordLength == passwordLength &&
			memcmp( password, incomingPassword, incomingPasswordLength ) == 0 )
		{
			remoteSystem->connectMode=RemoteSystemStruct::HANDLING_CONNECTION_REQUEST;


			char str1[64];
			systemAddress.ToString(false, str1);
			if ( usingSecurity == false ||
				IsInSecurityExceptionList(str1))

			{
#ifdef _TEST_AES
				unsigned char AESKey[ 16 ];
				// Save the AES key
				for ( i = 0; i < 16; i++ )
					AESKey[ i ] = i;

				OnConnectionRequest( remoteSystem, AESKey, true );
#else
				// Connect this player assuming we have open slots
				OnConnectionRequest( remoteSystem, 0, false, incomingTimestamp );
#endif
			}

			else
				SecuredConnectionResponse( systemAddress );

		}
		else
		{
			// This one we only send once since we don't care if it arrives.
			RakNet::BitStream bitStream;
			bitStream.Write((MessageID)ID_INVALID_PASSWORD);
			bitStream.Write(GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
			SendImmediate((char*) bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), IMMEDIATE_PRIORITY, RELIABLE, 0, systemAddress, false, false, RakNet::GetTimeNS(), 0);
			remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem, unsigned char *AESKey, bool setAESKey, RakNetTime incomingTimestamp )
{
	// Already handled by caller
	//if ( AllowIncomingConnections() )
	{
		SendConnectionRequestAccepted(remoteSystem, incomingTimestamp);

		// Don't set secure connections immediately because we need the ack from the remote system to know ID_CONNECTION_REQUEST_ACCEPTED
		// As soon as a 16 byte packet arrives, we will turn on AES.  This works because all encrypted packets are multiples of 16 and the
		// packets I happen to be sending are less than 16 bytes
		remoteSystem->setAESKey=setAESKey;
		if ( setAESKey )
		{
			memcpy(remoteSystem->AESKey, AESKey, 16);
			remoteSystem->connectMode=RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET;
		}
	}
	/*
	else
	{
		unsigned char c = ID_NO_FREE_INCOMING_CONNECTIONS;
		//SocketLayer::SendTo( connectionSocket, ( char* ) & c, sizeof( char ), systemAddress.binaryAddress, systemAddress.port );

		SendImmediate((char*)&c, sizeof(c)*8, IMMEDIATE_PRIORITY, RELIABLE, 0, remoteSystem->systemAddress, false, false, RakNet::GetTimeNS());
		remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY;
	}
	*/
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendConnectionRequestAccepted(RakPeer::RemoteSystemStruct *remoteSystem, RakNetTime incomingTimestamp)
{
	RakNet::BitStream bitStream;
	bitStream.Write((MessageID)ID_CONNECTION_REQUEST_ACCEPTED);
	bitStream.Write(remoteSystem->systemAddress);
	SystemIndex systemIndex = (SystemIndex) GetIndexFromSystemAddress( remoteSystem->systemAddress, true );
	RakAssert(systemIndex!=65535);
	bitStream.Write(systemIndex);
	for (unsigned int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
		bitStream.Write(mySystemAddress[i]);
	bitStream.Write(incomingTimestamp);
	bitStream.Write(RakNet::GetTime());

	SendImmediate((char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, remoteSystem->systemAddress, false, false, RakNet::GetTimeNS(), 0);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::NotifyAndFlagForShutdown( const SystemAddress systemAddress, bool performImmediate, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority )
{
	RakNet::BitStream temp( sizeof(unsigned char) );
	temp.Write( (MessageID)ID_DISCONNECTION_NOTIFICATION );
	if (performImmediate)
	{
		SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), disconnectionNotificationPriority, RELIABLE_ORDERED, orderingChannel, systemAddress, false, false, RakNet::GetTimeNS(), 0);
		RemoteSystemStruct *rss=GetRemoteSystemFromSystemAddress(systemAddress, true, true);
		rss->connectMode=RemoteSystemStruct::DISCONNECT_ASAP;
	}
	else
	{
		SendBuffered((const char*)temp.GetData(), temp.GetNumberOfBitsUsed(), disconnectionNotificationPriority, RELIABLE_ORDERED, orderingChannel, systemAddress, false, RemoteSystemStruct::DISCONNECT_ASAP, 0);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned short RakPeer::GetNumberOfRemoteInitiatedConnections( void ) const
{
	unsigned short i, numberOfIncomingConnections;

	if ( remoteSystemList == 0 || endThreads == true )
		return 0;

	numberOfIncomingConnections = 0;

	// remoteSystemList in network thread
	for ( i = 0; i < maximumNumberOfPeers; i++ )
	//for ( i = 0; i < remoteSystemListSize; i++ )
	{
		if ( remoteSystemList[ i ].isActive && remoteSystemList[ i ].weInitiatedTheConnection == false && remoteSystemList[i].connectMode==RemoteSystemStruct::CONNECTED)
			numberOfIncomingConnections++;
	}

	return numberOfIncomingConnections;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct * RakPeer::AssignSystemAddressToRemoteSystemList( const SystemAddress systemAddress, RemoteSystemStruct::ConnectMode connectionMode, RakNetSmartPtr<RakNetSocket> incomingRakNetSocket, bool *thisIPConnectedRecently, SystemAddress bindingAddress, int incomingMTU, RakNetGUID guid )
{
	RemoteSystemStruct * remoteSystem;
	unsigned i,j,assignedIndex;
	RakNetTime time = RakNet::GetTime();
#ifdef _DEBUG
	RakAssert(systemAddress!=UNASSIGNED_SYSTEM_ADDRESS);
#endif

	if (limitConnectionFrequencyFromTheSameIP)
	{
		if (IsLoopbackAddress(systemAddress,false)==false)
		{
			for ( i = 0; i < maximumNumberOfPeers; i++ )
			{
				if ( remoteSystemList[ i ].isActive==true &&
					remoteSystemList[ i ].systemAddress.binaryAddress==systemAddress.binaryAddress &&
					time >= remoteSystemList[ i ].connectionTime &&
					time - remoteSystemList[ i ].connectionTime < 100
					)
				{
					// 4/13/09 Attackers can flood ID_OPEN_CONNECTION_REQUEST and use up all available connection slots
					// Ignore connection attempts if this IP address connected within the last 100 milliseconds
					*thisIPConnectedRecently=true;
					ValidateRemoteSystemLookup();
					return 0;
				}
			}
		}
	}

	// Don't use a different port than what we received on
	bindingAddress.port=incomingRakNetSocket->boundAddress.port;

	*thisIPConnectedRecently=false;
	for ( assignedIndex = 0; assignedIndex < maximumNumberOfPeers; assignedIndex++ )
	{
		if ( remoteSystemList[ assignedIndex ].isActive==false )
		{
			remoteSystem=remoteSystemList+assignedIndex;
			remoteSystem->rpcMap.Clear();
			ReferenceRemoteSystem(systemAddress, assignedIndex);
			remoteSystem->MTUSize=defaultMTUSize;
			remoteSystem->guid=guid;
			remoteSystem->isActive = true; // This one line causes future incoming packets to go through the reliability layer
			// Reserve this reliability layer for ourselves.
			if (incomingMTU > remoteSystem->MTUSize)
				remoteSystem->MTUSize=incomingMTU;
			remoteSystem->reliabilityLayer.Reset(true, remoteSystem->MTUSize);
			remoteSystem->reliabilityLayer.SetSplitMessageProgressInterval(splitMessageProgressInterval);
			remoteSystem->reliabilityLayer.SetUnreliableTimeout(unreliableTimeout);
			remoteSystem->reliabilityLayer.SetTimeoutTime(defaultTimeoutTime);
			remoteSystem->reliabilityLayer.SetEncryptionKey( 0 );
			if (incomingRakNetSocket->boundAddress==bindingAddress)
			{
				remoteSystem->rakNetSocket=incomingRakNetSocket;
			}
			else
			{
				char str[256];
				bindingAddress.ToString(true,str);
				// See if this is an internal IP address.
				// If so, force binding on it so we reply on the same IP address as they sent to.
				unsigned int ipListIndex, foundIndex=(unsigned int)-1;

				for (ipListIndex=0; ipListIndex < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ipListIndex++)
				{
					if (ipList[ipListIndex][0]==0)
						break;
					if (bindingAddress.binaryAddress==binaryAddresses[ipListIndex])
					{
						foundIndex=ipListIndex;
						break;
					}
				}

				// 06/26/09 Unconfirmed report that Vista firewall blocks the reply if we force a binding
				// For now use the incoming socket only
				// Originally this code was to force a machine with multiple IP addresses to reply back on the IP
				// that the datagram came in on
				if (1 || foundIndex==(unsigned int)-1)
				{
					// Must not be an internal LAN address. Just use whatever socket it came in on
					remoteSystem->rakNetSocket=incomingRakNetSocket;
				}
				else
				{
					// Force binding
					unsigned int socketListIndex;
					for (socketListIndex=0; socketListIndex < socketList.Size(); socketListIndex++)
					{
						if (socketList[socketListIndex]->boundAddress==bindingAddress)
						{
							// Force binding with existing socket
							remoteSystem->rakNetSocket=socketList[socketListIndex];
							break;
						}
					}

					if (socketListIndex==socketList.Size())
					{
						// Force binding with new socket
						RakNetSmartPtr<RakNetSocket> rns(RakNet::OP_NEW<RakNetSocket>(__FILE__,__LINE__));
						if (incomingRakNetSocket->remotePortRakNetWasStartedOn_PS3==0)
							rns->s = (unsigned int) SocketLayer::CreateBoundSocket( bindingAddress.port, true, ipList[foundIndex], 0 );
						else
							rns->s = (unsigned int) SocketLayer::CreateBoundSocket_PS3Lobby( bindingAddress.port, true, ipList[foundIndex] );
						if ((SOCKET)rns->s==(SOCKET)-1)
						{
							// Can't bind. Just use whatever socket it came in on
							remoteSystem->rakNetSocket=incomingRakNetSocket;
						}
						else
						{
							rns->boundAddress=bindingAddress;
							rns->remotePortRakNetWasStartedOn_PS3=incomingRakNetSocket->remotePortRakNetWasStartedOn_PS3;
							rns->userConnectionSocketIndex=(unsigned int)-1;
							socketList.Push(rns, __FILE__, __LINE__ );
							remoteSystem->rakNetSocket=rns;

							RakPeerAndIndex rpai;
							rpai.remotePortRakNetWasStartedOn_PS3=rns->remotePortRakNetWasStartedOn_PS3;
							rpai.s=rns->s;
							rpai.rakPeer=this;
#ifdef _WIN32
							int highPriority=THREAD_PRIORITY_ABOVE_NORMAL;
#else
							int highPriority=-10;
#endif

//#if !defined(_XBOX) && !defined(X360)
							highPriority=0;
//#endif
							isRecvFromLoopThreadActive=false;
							int errorCode = RakNet::RakThread::Create(RecvFromLoop, &rpai, highPriority);
							RakAssert(errorCode!=0);
							while (  isRecvFromLoopThreadActive == false )
								RakSleep(10);


							/*
#if defined (_WIN32) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
							if (threadSleepTimer>0)
							{
								rns->recvEvent=CreateEvent(0,FALSE,FALSE,0);
								WSAEventSelect(rns->s,rns->recvEvent,FD_READ);
							}
#endif
							*/
						}
					}
				}
			}

			for ( j = 0; j < (unsigned) PING_TIMES_ARRAY_SIZE; j++ )
			{
				remoteSystem->pingAndClockDifferential[ j ].pingTime = 65535;
				remoteSystem->pingAndClockDifferential[ j ].clockDifferential = 0;
			}

			remoteSystem->connectMode=connectionMode;
			remoteSystem->pingAndClockDifferentialWriteIndex = 0;
			remoteSystem->lowestPing = 65535;
			remoteSystem->nextPingTime = 0; // Ping immediately
			remoteSystem->weInitiatedTheConnection = false;
			remoteSystem->connectionTime = time;
			remoteSystem->myExternalSystemAddress = UNASSIGNED_SYSTEM_ADDRESS;
			remoteSystem->setAESKey=false;
			remoteSystem->lastReliableSend=time;

#ifdef _DEBUG
			int indexLoopupCheck=GetIndexFromSystemAddress( systemAddress, true );
			if (indexLoopupCheck!=assignedIndex)
			{
				RakAssert(indexLoopupCheck==assignedIndex);
			}
#endif

			return remoteSystem;
		}
	}

	return 0;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Adjust the first four bytes (treated as unsigned int) of the pointer
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ShiftIncomingTimestamp( unsigned char *data, SystemAddress systemAddress ) const
{
#ifdef _DEBUG
	RakAssert( IsActive() );
	RakAssert( data );
#endif

	RakNet::BitStream timeBS( data, sizeof(RakNetTime), false);
	RakNetTime encodedTimestamp;
	timeBS.Read(encodedTimestamp);

	encodedTimestamp = encodedTimestamp - GetBestClockDifferential( systemAddress );
	timeBS.SetWriteOffset(0);
	timeBS.Write(encodedTimestamp);
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Thanks to Chris Taylor (cat02e@fsu.edu) for the improved timestamping algorithm
RakNetTime RakPeer::GetBestClockDifferential( const SystemAddress systemAddress ) const
{
	int counter, lowestPingSoFar;
	RakNetTime clockDifferential;
	RemoteSystemStruct *remoteSystem = GetRemoteSystemFromSystemAddress( systemAddress, true, true );

	if ( remoteSystem == 0 )
		return 0;

	lowestPingSoFar = 65535;

	clockDifferential = 0;

	for ( counter = 0; counter < PING_TIMES_ARRAY_SIZE; counter++ )
	{
		if ( remoteSystem->pingAndClockDifferential[ counter ].pingTime == 65535 )
			break;

		if ( remoteSystem->pingAndClockDifferential[ counter ].pingTime < lowestPingSoFar )
		{
			clockDifferential = remoteSystem->pingAndClockDifferential[ counter ].clockDifferential;
			lowestPingSoFar = remoteSystem->pingAndClockDifferential[ counter ].pingTime;
		}
	}

	return clockDifferential;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::RemoteSystemLookupHashIndex(SystemAddress sa) const
{
	unsigned int lastHash = SuperFastHashIncremental ((const char*) & sa.binaryAddress, 4, 4 );
	lastHash = SuperFastHashIncremental ((const char*) & sa.port, 2, lastHash );
	return lastHash % ((unsigned int) maximumNumberOfPeers * REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ReferenceRemoteSystem(SystemAddress sa, unsigned int remoteSystemListIndex)
{
// #ifdef _DEBUG
// 	for ( int remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; ++remoteSystemIndex )
// 	{
// 		if (remoteSystemList[remoteSystemIndex].isActive )
// 		{
// 			unsigned int hashIndex = GetRemoteSystemIndex(remoteSystemList[remoteSystemIndex].systemAddress);
// 			RakAssert(hashIndex==remoteSystemIndex);
// 		}
// 	}
// #endif


	SystemAddress oldAddress = remoteSystemList[remoteSystemListIndex].systemAddress;
	if (oldAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		// The system might be active if rerouting
//		RakAssert(remoteSystemList[remoteSystemListIndex].isActive==false);
		
		// Remove the reference if the reference is pointing to this inactive system
		if (GetRemoteSystem(oldAddress)==&remoteSystemList[remoteSystemListIndex])
			DereferenceRemoteSystem(oldAddress);
	}
	DereferenceRemoteSystem(sa);

// #ifdef _DEBUG
// 	for ( int remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; ++remoteSystemIndex )
// 	{
// 		if (remoteSystemList[remoteSystemIndex].isActive )
// 		{
// 			unsigned int hashIndex = GetRemoteSystemIndex(remoteSystemList[remoteSystemIndex].systemAddress);
// 			if (hashIndex!=remoteSystemIndex)
// 			{
// 				RakAssert(hashIndex==remoteSystemIndex);
// 			}
// 		}
// 	}
// #endif


	remoteSystemList[remoteSystemListIndex].systemAddress=sa;

	unsigned int hashIndex = RemoteSystemLookupHashIndex(sa);
	RemoteSystemIndex *rsi;
	rsi = remoteSystemIndexPool.Allocate(__FILE__,__LINE__);
	if (remoteSystemLookup[hashIndex]==0)
	{
		rsi->next=0;
		rsi->index=remoteSystemListIndex;
		remoteSystemLookup[hashIndex]=rsi;
	}
	else
	{
		RemoteSystemIndex *cur = remoteSystemLookup[hashIndex];
		while (cur->next!=0)
		{
			cur=cur->next;
		}

		rsi = remoteSystemIndexPool.Allocate(__FILE__,__LINE__);
		rsi->next=0;
		rsi->index=remoteSystemListIndex;
		cur->next=rsi;
	}

// #ifdef _DEBUG
// 	for ( int remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; ++remoteSystemIndex )
// 	{
// 		if (remoteSystemList[remoteSystemIndex].isActive )
// 		{
// 			unsigned int hashIndex = GetRemoteSystemIndex(remoteSystemList[remoteSystemIndex].systemAddress);
// 			RakAssert(hashIndex==remoteSystemIndex);
// 		}
// 	}
// #endif


	RakAssert(GetRemoteSystemIndex(sa)==remoteSystemListIndex);	
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DereferenceRemoteSystem(SystemAddress sa)
{
	unsigned int hashIndex = RemoteSystemLookupHashIndex(sa);
	RemoteSystemIndex *cur = remoteSystemLookup[hashIndex];
	RemoteSystemIndex *last = 0;
	while (cur!=0)
	{
		if (remoteSystemList[cur->index].systemAddress==sa)
		{
			if (last==0)
			{
				remoteSystemLookup[hashIndex]=cur->next;
			}
			else
			{
				last->next=cur->next;
			}
			remoteSystemIndexPool.Release(cur,__FILE__,__LINE__);
			break;
		}
		last=cur;
		cur=cur->next;
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GetRemoteSystemIndex(SystemAddress sa) const
{
	unsigned int hashIndex = RemoteSystemLookupHashIndex(sa);
	RemoteSystemIndex *cur = remoteSystemLookup[hashIndex];
	while (cur!=0)
	{
		if (remoteSystemList[cur->index].systemAddress==sa)
			return cur->index;
		cur=cur->next;
	}
	return (unsigned int) -1;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RakPeer::RemoteSystemStruct* RakPeer::GetRemoteSystem(SystemAddress sa) const
{
	unsigned int remoteSystemIndex = GetRemoteSystemIndex(sa);
	if (remoteSystemIndex==(unsigned int)-1)
		return 0;
	return remoteSystemList + remoteSystemIndex;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearRemoteSystemLookup(void)
{
	remoteSystemIndexPool.Clear(__FILE__,__LINE__);
	RakNet::OP_DELETE_ARRAY(remoteSystemLookup,__FILE__,__LINE__);
	remoteSystemLookup=0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::LookupIndexUsingHashIndex(SystemAddress sa) const
{
	unsigned int scanCount=0;
	unsigned int index = RemoteSystemLookupHashIndex(sa);
	if (remoteSystemLookup[index].index==(unsigned int)-1)
		return (unsigned int) -1;
	while (remoteSystemList[remoteSystemLookup[index].index].systemAddress!=sa)
	{
		if (++index==(unsigned int) maximumNumberOfPeers*REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE)
			index=0;
		if (++scanCount>(unsigned int) maximumNumberOfPeers*REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE)
			return (unsigned int) -1;
		if (remoteSystemLookup[index].index==-1)
			return (unsigned int) -1;
	}
	return index;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::RemoteSystemListIndexUsingHashIndex(SystemAddress sa) const
{
	unsigned int index = LookupIndexUsingHashIndex(sa);
	if (index!=(unsigned int) -1)
	{
		return remoteSystemLookup[index].index;
	}
	return (unsigned int) -1;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::FirstFreeRemoteSystemLookupIndex(SystemAddress sa) const
{
//	unsigned int collisionCount=0;
	unsigned int index = RemoteSystemLookupHashIndex(sa);
	while (remoteSystemLookup[index].index!=(unsigned int)-1)
	{
		if (++index==(unsigned int) maximumNumberOfPeers*REMOTE_SYSTEM_LOOKUP_HASH_MULTIPLE)
			index=0;
//		collisionCount++;
	}
//	printf("%i collisions. Using index %i\n", collisionCount, index);
	return index;
}
*/
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Description:
// Handles an RPC packet.  If you get a packet with the ID ID_RPC you should pass it to this function
//
// Parameters:
// packet - A packet returned from Receive with the ID ID_RPC
//
// Returns:
// true on success, false on a bad packet or an unregistered function
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning( disable : 4701 ) // warning C4701: local variable <variable name> may be used without having been initialized
#endif
bool RakPeer::HandleRPCPacket( const char *data, int length, SystemAddress systemAddress )
{
	// RPC BitStream format is
	// ID_RPC - unsigned char
	// Unique identifier string length - unsigned char
	// The unique ID  - string with each letter in upper case, subtracted by 'A' and written in 5 bits.
	// Number of bits of the data (int)
	// The data

	RakNet::BitStream incomingBitStream( (unsigned char *) data, length, false );
	char uniqueIdentifier[ 256 ];
//	BitSize_t bitLength;
	unsigned char *userData;
	//bool hasTimestamp;
	bool nameIsEncoded, networkIDIsEncoded;
	RPCIndex rpcIndex;
	RPCNode *node;
	RPCParameters rpcParms;
	NetworkID networkID;
	bool blockingCommand;
	RakNet::BitStream replyToSender;
	rpcParms.replyToSender=&replyToSender;

	rpcParms.recipient=this;
	rpcParms.sender=systemAddress;

	// Note to self - if I change this format then I have to change the PacketLogger class too
	incomingBitStream.IgnoreBits(8);
	if (data[0]==ID_TIMESTAMP)
	{
		// 11/1/2010 timestamp fix for RPC
// 		incomingBitStream.IgnoreBits(8*(sizeof(RakNetTime)+sizeof(MessageID)));
// 		memcpy(&rpcParms.remoteTimestamp, data+sizeof(MessageID), sizeof(RakNetTime));
		incomingBitStream.Read(rpcParms.remoteTimestamp);
		incomingBitStream.IgnoreBits(8);
	}
	else
		rpcParms.remoteTimestamp=0;
	if ( incomingBitStream.Read( nameIsEncoded ) == false )
	{
#ifdef _DEBUG
		RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
		return false;
	}

	if (nameIsEncoded)
	{
		if ( stringCompressor->DecodeString(uniqueIdentifier, 256, &incomingBitStream) == false )
		{
#ifdef _DEBUG
			RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
			return false;
		}

		rpcIndex = rpcMap.GetIndexFromFunctionName(uniqueIdentifier);
	}
	else
	{
		if ( incomingBitStream.ReadCompressed( rpcIndex ) == false )
		{
#ifdef _DEBUG
			RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
			return false;
		}
	}
	if ( incomingBitStream.Read( blockingCommand ) == false )
	{
#ifdef _DEBUG
		RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
		return false;
	}

	/*
	if ( incomingBitStream.Read( rpcParms.hasTimestamp ) == false )
	{
#ifdef _DEBUG
		RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
		return false;
	}
	*/

	if ( incomingBitStream.ReadCompressed( rpcParms.numberOfBitsOfData ) == false )
	{
#ifdef _DEBUG
		RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
		return false;
	}

	if ( incomingBitStream.Read( networkIDIsEncoded ) == false )
	{
#ifdef _DEBUG
		RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
		return false;
	}

	if (networkIDIsEncoded)
	{
		if ( incomingBitStream.Read( networkID ) == false )
		{
#ifdef _DEBUG
			RakAssert( 0 ); // bitstream was not long enough.  Some kind of internal error
#endif
			return false;
		}
	}

	if (rpcIndex==UNDEFINED_RPC_INDEX)
	{
		// Unregistered function
		RakAssert(0);
		return false;
	}

	node = rpcMap.GetNodeFromIndex(rpcIndex);
	if (node==0)
	{
#ifdef _DEBUG
		RakAssert( 0 ); // Should never happen except perhaps from threading errors?  No harm in checking anyway
#endif
		return false;
	}

	// Make sure the call type matches - if this is a pointer to a class member then networkID must be defined.  Otherwise it must not be defined
	if (node->isPointerToMember==true && networkIDIsEncoded==false)
	{
		// If this hits then this pointer was registered as a class member function but the packet does not have an NetworkID.
		// Most likely this means this system registered a function with REGISTER_CLASS_MEMBER_RPC and the remote system called it
		// using the unique ID for a function registered with REGISTER_STATIC_RPC.
		RakAssert(0);
		return false;
	}

	if (node->isPointerToMember==false && networkIDIsEncoded==true)
	{
		// If this hits then this pointer was not registered as a class member function but the packet does have an NetworkID.
		// Most likely this means this system registered a function with REGISTER_STATIC_RPC and the remote system called it
		// using the unique ID for a function registered with REGISTER_CLASS_MEMBER_RPC.
		RakAssert(0);
		return false;
	}

	if (nameIsEncoded && GetRemoteSystemFromSystemAddress(systemAddress, false, true))
	{
		// Send ID_RPC_MAPPING to the sender so they know what index to use next time
		RakNet::BitStream rpcMapBitStream;
		rpcMapBitStream.Write((MessageID)ID_RPC_MAPPING);
		stringCompressor->EncodeString(node->uniqueIdentifier, 256, &rpcMapBitStream);
        rpcMapBitStream.WriteCompressed(rpcIndex);
		SendBuffered( (const char*)rpcMapBitStream.GetData(), rpcMapBitStream.GetNumberOfBitsUsed(), HIGH_PRIORITY, UNRELIABLE, 0, systemAddress, false, RemoteSystemStruct::NO_ACTION, 0 );
	}

	rpcParms.functionName=node->uniqueIdentifier;

	// Call the function
	if ( rpcParms.numberOfBitsOfData == 0 )
	{
		rpcParms.input=0;
		if (networkIDIsEncoded)
		{
			// If this assert hits, you tried to use object member RPC but didn't call RakPeer::SetNetworkIDManager first as required.
			RakAssert(networkIDManager);
			if (networkIDManager)
			{
				void *object = networkIDManager->GET_OBJECT_FROM_ID(networkID);
			if (object)
				(node->memberFunctionPointer(object, &rpcParms));
		}
		}
		else
		{
			node->staticFunctionPointer( &rpcParms );
		}
	}
	else
	{
		if ( incomingBitStream.GetNumberOfUnreadBits() == 0 )
		{
#ifdef _DEBUG
			RakAssert( 0 );
#endif
			return false; // No data was appended!
		}

		// We have to copy into a new data chunk because the user data might not be byte aligned.
		bool usedAlloca=false;

		if (BITS_TO_BYTES( incomingBitStream.GetNumberOfUnreadBits() ) < MAX_ALLOCA_STACK_ALLOCATION)
		{
			userData = ( unsigned char* ) alloca( (size_t) BITS_TO_BYTES( incomingBitStream.GetNumberOfUnreadBits() ) );
			usedAlloca=true;
		}
		else

			userData = (unsigned char*) rakMalloc_Ex((size_t) BITS_TO_BYTES(incomingBitStream.GetNumberOfUnreadBits()), __FILE__, __LINE__);


		// The false means read out the internal representation of the bitstream data rather than
		// aligning it as we normally would with user data.  This is so the end user can cast the data received
		// into a bitstream for reading
		if ( incomingBitStream.ReadBits( ( unsigned char* ) userData, rpcParms.numberOfBitsOfData, false ) == false )
		{
#ifdef _DEBUG
			RakAssert( 0 );
#endif




			return false; // Not enough data to read
		}

//		if ( rpcParms.hasTimestamp )
//			ShiftIncomingTimestamp( userData, systemAddress );

		// Call the function callback
		rpcParms.input=userData;
		if (networkIDIsEncoded)
		{
			// If this assert hits, you tried to use object member RPC but didn't call RakPeer::SetNetworkIDManager first as required.
			RakAssert(networkIDManager);
			if (networkIDManager)
			{
				void *object = networkIDManager->GET_OBJECT_FROM_ID(networkID);
			if (object)
				(node->memberFunctionPointer(object, &rpcParms));
		}
		}
		else
		{
			node->staticFunctionPointer( &rpcParms );
		}


		if (usedAlloca==false)
			rakFree_Ex(userData, __FILE__, __LINE__ );
	}

	if (blockingCommand)
	{
		RakNet::BitStream reply;
		reply.Write((MessageID)ID_RPC_REPLY);
		reply.Write((char*)replyToSender.GetData(), replyToSender.GetNumberOfBytesUsed());
		Send(&reply, HIGH_PRIORITY, RELIABLE, 0, systemAddress, false);
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/**
* Handles an RPC reply packet.  This is data returned from an RPC call
*
* \param data A packet returned from Receive with the ID ID_RPC
* \param length The size of the packet data
* \param systemAddress The sender of the packet
*
* \return true on success, false on a bad packet or an unregistered function
*/
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::HandleRPCReplyPacket( const char *data, int length, SystemAddress systemAddress )
{
	if (blockOnRPCReply)
	{
		if ((systemAddress==replyFromTargetPlayer && replyFromTargetBroadcast==false) ||
			(systemAddress!=replyFromTargetPlayer && replyFromTargetBroadcast==true))
		{
			replyFromTargetBS->Write(data+1, length-1);
			blockOnRPCReply=false;
		}
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::IsLoopbackAddress(const AddressOrGUID &systemIdentifier, bool matchPort) const
{
	if (systemIdentifier.rakNetGuid!=UNASSIGNED_RAKNET_GUID)
		return systemIdentifier.rakNetGuid==myGuid;

	const SystemAddress sa = systemIdentifier.systemAddress;

	// Used to see if we are sending to ourselves
	char str[64];
	sa.ToString(false,str);

	bool isLoopback=strcmp(str,"127.0.0.1")==0;
	if (matchPort==false && isLoopback)
		return true;
	if (matchPort==false)
	{
		for (int ipIndex=0; ipIndex < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ipIndex++)
			if (mySystemAddress[ipIndex].binaryAddress==sa.binaryAddress)
				return true;
	}
	else
	{
		for (int ipIndex=0; ipIndex < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ipIndex++)
			if (mySystemAddress[ipIndex]==sa ||
				(isLoopback && sa.port==mySystemAddress[ipIndex].port))
				return true;
	}

















	return sa==firstExternalID;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
SystemAddress RakPeer::GetLoopbackAddress(void) const
{

	return mySystemAddress[0];



}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GenerateSYNCookieRandomNumber( void )
{
#if   !defined(_WIN32_WCE) 
	unsigned int number;
	int i;
	memcpy( oldRandomNumber, newRandomNumber, sizeof( newRandomNumber ) );

	for ( i = 0; i < (int) sizeof( newRandomNumber ); i += (int) sizeof( number ) )
	{
		number = randomMT();
		memcpy( newRandomNumber + i, ( char* ) & number, sizeof( number ) );
	}

	randomNumberExpirationTime = RakNet::GetTime() + SYN_COOKIE_OLD_RANDOM_NUMBER_DURATION;
#endif
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SecuredConnectionResponse( const SystemAddress systemAddress )
{
#if   !defined(_WIN32_WCE) 
	CSHA1 sha1;
//	RSA_BIT_SIZE n;
//	big::uint32_t e;
//	unsigned char connectionRequestResponse[ 1 + sizeof( big::uint32_t ) + sizeof( RSA_BIT_SIZE ) + 20 ];
	uint32_t modulus[RAKNET_RSA_FACTOR_LIMBS];
	uint32_t e;
	unsigned char connectionRequestResponse[ 1 + sizeof( e ) + sizeof( modulus ) + 20 ];
	connectionRequestResponse[ 0 ] = ID_SECURED_CONNECTION_RESPONSE;

	if ( randomNumberExpirationTime < RakNet::GetTime() )
		GenerateSYNCookieRandomNumber();

	// Hash the SYN-Cookie
	// s2c syn-cookie = SHA1_HASH(source ip address + source port + random number)
	sha1.Reset();
	sha1.Update( ( unsigned char* ) & systemAddress.binaryAddress, sizeof( systemAddress.binaryAddress ) );
	sha1.Update( ( unsigned char* ) & systemAddress.port, sizeof( systemAddress.port ) );
	sha1.Update( ( unsigned char* ) & ( newRandomNumber ), sizeof(newRandomNumber) );
	sha1.Final();

	// Write the cookie (not endian swapped)
	memcpy( connectionRequestResponse + 1, sha1.GetHash(), 20 );

	// Write the public keys
	e = rsacrypt.getPublicExponent();
	//rsacrypt.getPublicModulus(n);
	rsacrypt.getPublicModulus(modulus);
	//rsacrypt.getPublicKey( e, n );

	if (RakNet::BitStream::DoEndianSwap())
	{
		RakNet::BitStream::ReverseBytesInPlace(( unsigned char* ) & e, sizeof(e));
		for (int i=0; i < RAKNET_RSA_FACTOR_LIMBS; i++)
			RakNet::BitStream::ReverseBytesInPlace((unsigned char*) &modulus[i], sizeof(modulus[i]));
	}

	memcpy( connectionRequestResponse + 1 + 20, ( char* ) & e, sizeof( e ) );
	memcpy( connectionRequestResponse + 1 + 20 + sizeof( e ), modulus, sizeof( modulus ) );

	/*
#ifdef HOST_ENDIAN_IS_BIG
	// Mangle the keys on a Big-endian machine before sending
	BSWAPCPY( (unsigned char *)(connectionRequestResponse + 1 + 20),
		(unsigned char *)&e, sizeof( big::uint32_t ) );
	BSWAPCPY( (unsigned char *)(connectionRequestResponse + 1 + 20 + sizeof( big::uint32_t ) ),
		(unsigned char *)n, sizeof( RSA_BIT_SIZE ) );
#else
	memcpy( connectionRequestResponse + 1 + 20, ( char* ) & e, sizeof( big::uint32_t ) );
	memcpy( connectionRequestResponse + 1 + 20 + sizeof( big::uint32_t ), n, sizeof( RSA_BIT_SIZE ) );
#endif
	*/

	// s2c public key, syn-cookie
	//SocketLayer::SendTo( connectionSocket, ( char* ) connectionRequestResponse, 1 + sizeof( big::uint32_t ) + sizeof( RSA_BIT_SIZE ) + 20, systemAddress.binaryAddress, systemAddress.port );
	// All secure connection requests are unreliable because the entire process needs to be restarted if any part fails.
	// Connection requests are resent periodically
	SendImmediate(( char* ) connectionRequestResponse, (1 + sizeof( e ) + sizeof( modulus ) + 20) * 8, IMMEDIATE_PRIORITY, UNRELIABLE, 0, systemAddress, false, false, RakNet::GetTimeNS(), 0);
#endif
}

void RakPeer::SecuredConnectionConfirmation( RakPeer::RemoteSystemStruct * remoteSystem, char* data )
{
#if   !defined(_WIN32_WCE) 
	int i, j;
	unsigned char randomNumber[ 20 ];
	unsigned int number;
	//bool doSend;
	Packet *packet;
//	big::uint32_t e;
//	RSA_BIT_SIZE n, message, encryptedMessage;
//	big::RSACrypt<RSA_BIT_SIZE> privKeyPncrypt;
	uint32_t e;
	uint32_t n[RAKNET_RSA_FACTOR_LIMBS], message[RAKNET_RSA_FACTOR_LIMBS], encryptedMessage[RAKNET_RSA_FACTOR_LIMBS];
	RSACrypt privKeyPncrypt;

	// Make sure that we still want to connect
	if (remoteSystem->connectMode!=RemoteSystemStruct::REQUESTED_CONNECTION)
		return;

	// Copy out e and n
	/*
#ifdef HOST_ENDIAN_IS_BIG
	BSWAPCPY( (unsigned char *)&e, (unsigned char *)(data + 1 + 20), sizeof( big::uint32_t ) );
	BSWAPCPY( (unsigned char *)n, (unsigned char *)(data + 1 + 20 + sizeof( big::uint32_t )), sizeof( RSA_BIT_SIZE ) );
#else
	memcpy( ( char* ) & e, data + 1 + 20, sizeof( big::uint32_t ) );
	memcpy( n, data + 1 + 20 + sizeof( big::uint32_t ), sizeof( RSA_BIT_SIZE ) );
#endif
	*/

	memcpy( ( char* ) & e, data + 1 + 20, sizeof( e ) );
	memcpy( n, data + 1 + 20 + sizeof( e ), sizeof( n ) );

	if (RakNet::BitStream::DoEndianSwap())
	{
		RakNet::BitStream::ReverseBytesInPlace((unsigned char*) &e, sizeof(e));
		for (int i=0; i < RAKNET_RSA_FACTOR_LIMBS; i++)
			RakNet::BitStream::ReverseBytesInPlace((unsigned char*) &n[i], sizeof(n[i]));
	}


	// If we preset a size and it doesn't match, or the keys do not match, then tell the user
	if ( usingSecurity == true && keysLocallyGenerated == false )
	{
		if ( memcmp( ( char* ) & e, ( char* ) & publicKeyE, sizeof( e ) ) != 0 ||
			memcmp( n, publicKeyN, sizeof( n ) ) != 0 )
		{
			packet=AllocPacket(1, __FILE__, __LINE__);
			packet->data[ 0 ] = ID_RSA_PUBLIC_KEY_MISMATCH;
			packet->bitSize = sizeof( char ) * 8;
			packet->systemAddress = remoteSystem->systemAddress;
			packet->systemAddress.systemIndex = ( SystemIndex ) GetIndexFromSystemAddress( packet->systemAddress, true );
			packet->guid.systemIndex=packet->systemAddress.systemIndex;
			AddPacketToProducer(packet);
			remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY;
			return;
		}
	}

	// Create a random number
	for ( i = 0; i < (int) sizeof( randomNumber ); i += (int) sizeof( number ) )
	{
		number = randomMT();
		memcpy( randomNumber + i, ( char* ) & number, sizeof( number ) );
	}

	memset( message, 0, sizeof( message ) );
	RakAssert( sizeof( message ) >= sizeof( randomNumber ) );

//	if (RakNet::BitStream::DoEndianSwap())
//	{
//		for (int i=0; i < 20; i++)
//			RakNet::BitStream::ReverseBytesInPlace((unsigned char*) randomNumber[i], sizeof(randomNumber[i]));
//	}

	memcpy( message, randomNumber, sizeof( randomNumber ) );

	/*


#ifdef HOST_ENDIAN_IS_BIG
	// Scramble the plaintext message (the random number)
	BSWAPCPY( (unsigned char *)message, randomNumber, sizeof(randomNumber) );
#else
	memcpy( message, randomNumber, sizeof( randomNumber ) );
#endif
	*/
	privKeyPncrypt.setPublicKey( n, RAKNET_RSA_FACTOR_LIMBS, e );
//	privKeyPncrypt.encrypt( message, encryptedMessage );

//	printf("message[0]=%i,%i\n", message[0], message[19]);

	privKeyPncrypt.encrypt( encryptedMessage, message );

//	printf("enc1[0]=%i,%i\n", encryptedMessage[0], encryptedMessage[19]);

	// A big-endian machine needs to scramble the byte order of an outgoing (encrypted) message
	if (RakNet::BitStream::DoEndianSwap())
	{
		for (int i=0; i < RAKNET_RSA_FACTOR_LIMBS; i++)
			RakNet::BitStream::ReverseBytesInPlace((unsigned char*) &encryptedMessage[i], sizeof(encryptedMessage[i]));
	}

//	printf("enc2[0]=%i,%i\n", encryptedMessage[0], encryptedMessage[19]);


	/*
#ifdef HOST_ENDIAN_IS_BIG
	// A big-endian machine needs to scramble the byte order of an outgoing (encrypted) message
	BSWAPSELF( (unsigned char *)encryptedMessage, sizeof( RSA_BIT_SIZE ) );
#endif
	*/

	/*
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Lock();
	for ( i = 0; i < ( int ) requestedConnectionsList.Size(); i++ )
	{
		if ( requestedConnectionsList[ i ]->systemAddress == systemAddress )
		{
			doSend = true;
			// Generate the AES key

			for ( j = 0; j < 16; j++ )
				requestedConnectionsList[ i ]->AESKey[ j ] = data[ 1 + j ] ^ randomNumber[ j ];

			requestedConnectionsList[ i ]->setAESKey = true;

			break;
		}
	}
	rakPeerMutexes[ RakPeer::requestedConnections_MUTEX ].Unlock();
	*/

	// Take the remote system's AESKey (SynCookie) and XOR with our random number.
		for ( j = 0; j < 16; j++ )
			remoteSystem->AESKey[ j ] = data[ 1 + j ] ^ randomNumber[ j ];
	remoteSystem->setAESKey = true;

	/*
//	if ( doSend )
//	{
		char reply[ 1 + 20 + sizeof( RSA_BIT_SIZE ) ];
		// c2s RSA(random number), same syn-cookie
		reply[ 0 ] = ID_SECURED_CONNECTION_CONFIRMATION;
		memcpy( reply + 1, data + 1, 20 );  // Copy the syn-cookie
		memcpy( reply + 1 + 20, encryptedMessage, sizeof( RSA_BIT_SIZE ) ); // Copy the encoded random number

		//SocketLayer::SendTo( connectionSocket, reply, 1 + 20 + sizeof( RSA_BIT_SIZE ), systemAddress.binaryAddress, systemAddress.port );
		// All secure connection requests are unreliable because the entire process needs to be restarted if any part fails.
		// Connection requests are resent periodically
		SendImmediate((char*)reply, (1 + 20 + sizeof( RSA_BIT_SIZE )) * 8, IMMEDIATE_PRIORITY, UNRELIABLE, 0, remoteSystem->systemAddress, false, false, RakNet::GetTimeNS());
//	}
*/

	char reply[ 1 + 20 + sizeof( uint32_t ) * RAKNET_RSA_FACTOR_LIMBS + sizeof(RakNetTime) ];
	// c2s RSA(random number), same syn-cookie
	reply[ 0 ] = ID_SECURED_CONNECTION_CONFIRMATION;
	memcpy( reply + 1, data + 1, 20 );  // Copy the syn-cookie
	memcpy( reply + 1 + 20, encryptedMessage, sizeof( encryptedMessage ) ); // Copy the encoded random number
	RakNet::BitStream bsTimestamp;
	bsTimestamp.Write(RakNet::GetTime());
	memcpy( reply + 1 + 20 + sizeof( uint32_t ) * RAKNET_RSA_FACTOR_LIMBS, bsTimestamp.GetData(), bsTimestamp.GetNumberOfBytesUsed() );

	// All secure connection requests are unreliable because the entire process needs to be restarted if any part fails.
	// Connection requests are resent periodically
	SendImmediate((char*)reply, (1 + 20 + sizeof(uint32_t) * RAKNET_RSA_FACTOR_LIMBS + sizeof(RakNetTime) ) * 8 , IMMEDIATE_PRIORITY, UNRELIABLE, 0, remoteSystem->systemAddress, false, false, RakNet::GetTimeNS(), 0);

#endif
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::AllowIncomingConnections(void) const
{
	return GetNumberOfRemoteInitiatedConnections() < GetMaximumIncomingConnections();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::PingInternal( const SystemAddress target, bool performImmediate, PacketReliability reliability )
{
	if ( IsActive() == false )
		return ;

	RakNet::BitStream bitStream(sizeof(unsigned char)+sizeof(RakNetTime));
	bitStream.Write((MessageID)ID_INTERNAL_PING);
	RakNetTimeUS currentTimeNS = RakNet::GetTimeNS();
	RakNetTime currentTime = RakNet::GetTime();
	bitStream.Write(currentTime);
	if (performImmediate)
		SendImmediate( (char*)bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, reliability, 0, target, false, false, currentTimeNS, 0 );
	else
		Send( &bitStream, IMMEDIATE_PRIORITY, reliability, 0, target, false );
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::CloseConnectionInternal( const AddressOrGUID& systemIdentifier, bool sendDisconnectionNotification, bool performImmediate, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority )
{
#ifdef _DEBUG
	RakAssert(orderingChannel < 32);
#endif

	if (systemIdentifier.IsUndefined())
		return;

	if ( remoteSystemList == 0 || endThreads == true )
		return;

	SystemAddress target;
	if (systemIdentifier.systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
	{
		target=systemIdentifier.systemAddress;
	}
	else
	{
		target=GetSystemAddressFromGuid(systemIdentifier.rakNetGuid);
	}

	if (sendDisconnectionNotification)
	{
		NotifyAndFlagForShutdown(target, performImmediate, orderingChannel, disconnectionNotificationPriority);
	}
	else
	{
		if (performImmediate)
		{
			unsigned int index = GetRemoteSystemIndex(target);
			if (index!=(unsigned int) -1)
			{
				if ( remoteSystemList[index].isActive )
				{
					// Found the index to stop
					remoteSystemList[index].isActive = false;

					remoteSystemList[index].guid=UNASSIGNED_RAKNET_GUID;

					// Reserve this reliability layer for ourselves
					//remoteSystemList[ remoteSystemLookup[index].index ].systemAddress = UNASSIGNED_SYSTEM_ADDRESS;

					// Clear any remaining messages
					remoteSystemList[index].reliabilityLayer.Reset(false, remoteSystemList[index].MTUSize);

					// Not using this socket
					remoteSystemList[index].rakNetSocket.SetNull();
				}
			}
		}
		else
		{
			BufferedCommandStruct *bcs;
#ifdef _RAKNET_THREADSAFE
			bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
			bcs->command=BufferedCommandStruct::BCS_CLOSE_CONNECTION;
			bcs->systemIdentifier=target;
			bcs->data=0;
			bcs->orderingChannel=orderingChannel;
			bcs->priority=disconnectionNotificationPriority;
			bufferedCommands.Push(bcs);
#else
			bcs=bufferedCommands.WriteLock();
			bcs->command=BufferedCommandStruct::BCS_CLOSE_CONNECTION;
			bcs->systemIdentifier=target;
			bcs->data=0;
			bcs->orderingChannel=orderingChannel;
			bcs->priority=disconnectionNotificationPriority;
			bufferedCommands.WriteUnlock();
#endif
		}
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::ValidSendTarget(SystemAddress systemAddress, bool broadcast)
{
	unsigned remoteSystemIndex;

	// remoteSystemList in user thread.  This is slow so only do it in debug
	for ( remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; remoteSystemIndex++ )
	//for ( remoteSystemIndex = 0; remoteSystemIndex < remoteSystemListSize; remoteSystemIndex++ )
	{
		if ( remoteSystemList[ remoteSystemIndex ].isActive &&
			remoteSystemList[ remoteSystemIndex ].connectMode==RakPeer::RemoteSystemStruct::CONNECTED && // Not fully connected players are not valid user-send targets because the reliability layer wasn't reset yet
			( ( broadcast == false && remoteSystemList[ remoteSystemIndex ].systemAddress == systemAddress ) ||
			( broadcast == true && remoteSystemList[ remoteSystemIndex ].systemAddress != systemAddress ) )
			)
			return true;
	}

	return false;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendBuffered( const char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode, uint32_t receipt )
{
	BufferedCommandStruct *bcs;

#ifdef _RAKNET_THREADSAFE
	bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
#else
	bcs=bufferedCommands.WriteLock();
#endif
	bcs->data = (char*) rakMalloc_Ex( (size_t) BITS_TO_BYTES(numberOfBitsToSend), __FILE__, __LINE__ ); // Making a copy doesn't lose efficiency because I tell the reliability layer to use this allocation for its own copy
	if (bcs->data==0)
	{
		notifyOutOfMemory(__FILE__, __LINE__);
#ifdef _RAKNET_THREADSAFE
		bufferedCommands.Deallocate(bcs, __FILE__,__LINE__);
#else
		bufferedCommands.WriteUnlock();
#endif
		return;
	}
	memcpy(bcs->data, data, (size_t) BITS_TO_BYTES(numberOfBitsToSend));
	bcs->numberOfBitsToSend=numberOfBitsToSend;
	bcs->priority=priority;
	bcs->reliability=reliability;
	bcs->orderingChannel=orderingChannel;
	bcs->systemIdentifier=systemIdentifier;
	bcs->broadcast=broadcast;
	bcs->connectionMode=connectionMode;
	bcs->receipt=receipt;
	bcs->command=BufferedCommandStruct::BCS_SEND;
#ifdef _RAKNET_THREADSAFE
	bufferedCommands.Push(bcs);
#else
	bufferedCommands.WriteUnlock();
#endif

	if (priority==IMMEDIATE_PRIORITY)
	{
		// Forces pending sends to go out now, rather than waiting to the next update interval
		quitAndDataEvents.SetEvent();
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::SendBufferedList( const char **data, const int *lengths, const int numParameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode, uint32_t receipt )
{
	BufferedCommandStruct *bcs;
	unsigned int totalLength=0;
	unsigned int lengthOffset;
	int i;
	for (i=0; i < numParameters; i++)
	{
		if (lengths[i]>0)
			totalLength+=lengths[i];
	}
	if (totalLength==0)
		return;

	char *dataAggregate;
	dataAggregate = (char*) rakMalloc_Ex( (size_t) totalLength, __FILE__, __LINE__ ); // Making a copy doesn't lose efficiency because I tell the reliability layer to use this allocation for its own copy
	if (dataAggregate==0)
	{
		notifyOutOfMemory(__FILE__, __LINE__);
		return;
	}
	for (i=0, lengthOffset=0; i < numParameters; i++)
	{
		if (lengths[i]>0)
		{
			memcpy(dataAggregate+lengthOffset, data[i], lengths[i]);
			lengthOffset+=lengths[i];
		}
	}

	if (broadcast==false && IsLoopbackAddress(systemIdentifier,true))
	{
		SendLoopback(dataAggregate,totalLength);
		rakFree_Ex(dataAggregate,__FILE__,__LINE__);
		return;
	}

#ifdef _RAKNET_THREADSAFE
	bcs=bufferedCommands.Allocate( __FILE__, __LINE__ );
#else
	bcs=bufferedCommands.WriteLock();
#endif
	bcs->data = dataAggregate;
	bcs->numberOfBitsToSend=BYTES_TO_BITS(totalLength);
	bcs->priority=priority;
	bcs->reliability=reliability;
	bcs->orderingChannel=orderingChannel;
	bcs->systemIdentifier=systemIdentifier;
	bcs->broadcast=broadcast;
	bcs->connectionMode=connectionMode;
	bcs->receipt=receipt;
	bcs->command=BufferedCommandStruct::BCS_SEND;
#ifdef _RAKNET_THREADSAFE
	bufferedCommands.Push(bcs);
#else
	bufferedCommands.WriteUnlock();
#endif

	if (priority==IMMEDIATE_PRIORITY)
	{
		// Forces pending sends to go out now, rather than waiting to the next update interval
		quitAndDataEvents.SetEvent();
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::SendImmediate( char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, bool useCallerDataAllocation, RakNetTimeUS currentTime, uint32_t receipt )
{
	unsigned *sendList;
	unsigned sendListSize;
	bool callerDataAllocationUsed;
	unsigned int remoteSystemIndex, sendListIndex; // Iterates into the list of remote systems
	unsigned numberOfBytesUsed = (unsigned) BITS_TO_BYTES(numberOfBitsToSend);
	callerDataAllocationUsed=false;

	sendListSize=0;

	if (systemIdentifier.systemAddress!=UNASSIGNED_SYSTEM_ADDRESS)
		remoteSystemIndex=GetIndexFromSystemAddress( systemIdentifier.systemAddress, true );
	else if (systemIdentifier.rakNetGuid!=UNASSIGNED_RAKNET_GUID)
		remoteSystemIndex=GetSystemIndexFromGuid(systemIdentifier.rakNetGuid);
	else
		remoteSystemIndex=(unsigned int) -1;

	// 03/06/06 - If broadcast is false, use the optimized version of GetIndexFromSystemAddress
	if (broadcast==false)
	{
		if (remoteSystemIndex==(unsigned int) -1)
		{
#ifdef _DEBUG
//			int debugIndex = GetRemoteSystemIndex(systemIdentifier.systemAddress);
#endif
			return false;
		}


		sendList=(unsigned *)alloca(sizeof(unsigned));




		if (remoteSystemList[remoteSystemIndex].isActive &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ASAP &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY &&
			remoteSystemList[remoteSystemIndex].connectMode!=RemoteSystemStruct::DISCONNECT_ON_NO_ACK)
		{
			sendList[0]=remoteSystemIndex;
			sendListSize=1;
		}
	}
	else
	{

	//sendList=(unsigned *)alloca(sizeof(unsigned)*remoteSystemListSize);
		sendList=(unsigned *)alloca(sizeof(unsigned)*maximumNumberOfPeers);





		// remoteSystemList in network thread
		unsigned int idx;
		for ( idx = 0; idx < maximumNumberOfPeers; idx++ )
		{
			if (remoteSystemIndex!=(unsigned int) -1 && idx==remoteSystemIndex)
				continue;

			if ( remoteSystemList[ idx ].isActive && remoteSystemList[ idx ].systemAddress != UNASSIGNED_SYSTEM_ADDRESS )
				sendList[sendListSize++]=idx;
		}
	}

	if (sendListSize==0)
	{



		return false;
	}

	for (sendListIndex=0; sendListIndex < sendListSize; sendListIndex++)
	{
		if ( trackFrequencyTable )
		{
			unsigned i;
			// Store output frequency
			for (i=0 ; i < numberOfBytesUsed; i++ )
				frequencyTable[ (unsigned char)(data[i]) ]++;
		}

		if ( outputTree )
		{
			RakNet::BitStream bitStreamCopy( numberOfBytesUsed );
			outputTree->EncodeArray( (unsigned char*) data, numberOfBytesUsed, &bitStreamCopy );
			rawBytesSent += numberOfBytesUsed;
			compressedBytesSent += (unsigned int) bitStreamCopy.GetNumberOfBytesUsed();
			remoteSystemList[sendList[sendListIndex]].reliabilityLayer.Send( (char*) bitStreamCopy.GetData(), bitStreamCopy.GetNumberOfBitsUsed(), priority, reliability, orderingChannel, true, remoteSystemList[sendList[sendListIndex]].MTUSize, currentTime, receipt );
		}
		else
		{
			// Send may split the packet and thus deallocate data.  Don't assume data is valid if we use the callerAllocationData
			bool useData = useCallerDataAllocation && callerDataAllocationUsed==false && sendListIndex+1==sendListSize;
			remoteSystemList[sendList[sendListIndex]].reliabilityLayer.Send( data, numberOfBitsToSend, priority, reliability, orderingChannel, useData==false, remoteSystemList[sendList[sendListIndex]].MTUSize, currentTime, receipt );
			if (useData)
				callerDataAllocationUsed=true;
		}

		if (reliability==RELIABLE ||
			reliability==RELIABLE_ORDERED ||
			reliability==RELIABLE_SEQUENCED ||
			reliability==RELIABLE_WITH_ACK_RECEIPT ||
			reliability==RELIABLE_ORDERED_WITH_ACK_RECEIPT
//			||
//			reliability==RELIABLE_SEQUENCED_WITH_ACK_RECEIPT
			)
			remoteSystemList[sendList[sendListIndex]].lastReliableSend=(RakNetTime)(currentTime/(RakNetTimeUS)1000);
	}





	// Return value only meaningful if true was passed for useCallerDataAllocation.  Means the reliability layer used that data copy, so the caller should not deallocate it
	return callerDataAllocationUsed;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ResetSendReceipt(void)
{
	sendReceiptSerialMutex.Lock();
	sendReceiptSerial=1;
	sendReceiptSerialMutex.Unlock();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::OnConnectedPong(RakNetTime sendPingTime, RakNetTime sendPongTime, RemoteSystemStruct *remoteSystem)
{
	RakNetTime ping, lastPing;
	RakNetTimeUS timeNS = RakNet::GetTimeNS(); // Update the time value to be accurate
	RakNetTimeMS timeMS = (RakNetTime)(timeNS/(RakNetTimeUS)1000);
	if (timeMS > sendPingTime)
		ping = timeMS - sendPingTime;
	else
		ping=0;
	lastPing = remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].pingTime;

	// Ignore super high spikes in the average
	if ( lastPing <= 0 || ( ( ping < ( lastPing * 3 ) ) && ping < 1200 ) )
	{
		remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].pingTime = ( unsigned short ) ping;
		// Thanks to Chris Taylor (cat02e@fsu.edu) for the improved timestamping algorithm
		// Divide each integer by 2, rather than the sum by 2, to prevent overflow
		remoteSystem->pingAndClockDifferential[ remoteSystem->pingAndClockDifferentialWriteIndex ].clockDifferential = sendPongTime - ( timeMS/2 + sendPingTime/2 );

		if ( remoteSystem->lowestPing == (unsigned short)-1 || remoteSystem->lowestPing > (int) ping )
			remoteSystem->lowestPing = (unsigned short) ping;

		// Reliability layer calculates its own ping
		// Most packets should arrive by the ping time.
		//RakAssert(ping < 10000); // Sanity check - could hit due to negative pings causing the var to overflow
		//remoteSystem->reliabilityLayer.SetPing( (unsigned short) ping );

		if ( ++( remoteSystem->pingAndClockDifferentialWriteIndex ) == PING_TIMES_ARRAY_SIZE )
			remoteSystem->pingAndClockDifferentialWriteIndex = 0;

		remoteSystem->reliabilityLayer.OnExternalPing((double) ping);
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearBufferedCommands(void)
{
	BufferedCommandStruct *bcs;

#ifdef _RAKNET_THREADSAFE
	while ((bcs=bufferedCommands.Pop())!=0)
	{
		if (bcs->data)
			rakFree_Ex(bcs->data, __FILE__, __LINE__ );

		bufferedCommands.Deallocate(bcs, __FILE__,__LINE__);
	}
	bufferedCommands.Clear(__FILE__, __LINE__);
#else
	while ((bcs=bufferedCommands.ReadLock())!=0)
	{
		if (bcs->data)
			rakFree_Ex(bcs->data, __FILE__, __LINE__ );

		bufferedCommands.ReadUnlock();
	}
	bufferedCommands.Clear(__FILE__, __LINE__);
#endif
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearBufferedPackets(void)
{
	RecvFromStruct *bcs;

#ifdef _RAKNET_THREADSAFE
	while ((bcs=bufferedPackets.Pop())!=0)
	{
		bufferedPackets.Deallocate(bcs, __FILE__,__LINE__);
	}
	bufferedPackets.Clear(__FILE__, __LINE__);
#else
	while ((bcs=bufferedPackets.ReadLock())!=0)
	{
		bufferedPackets.ReadUnlock();
	}
	bufferedPackets.Clear(__FILE__, __LINE__);
#endif
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearSocketQueryOutput(void)
{
	socketQueryOutput.Clear(__FILE__, __LINE__);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::ClearRequestedConnectionList(void)
{
	DataStructures::Queue<RequestedConnectionStruct*> freeQueue;
	requestedConnectionQueueMutex.Lock();
	while (requestedConnectionQueue.Size())
		freeQueue.Push(requestedConnectionQueue.Pop(), __FILE__, __LINE__ );
	requestedConnectionQueueMutex.Unlock();
	unsigned i;
	for (i=0; i < freeQueue.Size(); i++)
		RakNet::OP_DELETE(freeQueue[i], __FILE__, __LINE__ );
}
inline void RakPeer::AddPacketToProducer(Packet *p)
{
	packetReturnMutex.Lock();
	packetReturnQueue.Push(p,__FILE__,__LINE__);
	packetReturnMutex.Unlock();
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::GenerateGUID(void)
{
	// Mac address is a poor solution because you can't have multiple connections from the same system








#if   defined(_WIN32)
	myGuid.g=RakNet::GetTimeUS();

	RakNetTimeUS lastTime, thisTime;
	int j;
	// Sleep a small random time, then use the last 4 bits as a source of randomness
	for (j=0; j < 8; j++)
	{
		lastTime = RakNet::GetTimeUS();
		RakSleep(1);
		RakSleep(0);
		thisTime = RakNet::GetTimeUS();
		RakNetTimeUS diff = thisTime-lastTime;
		unsigned int diff4Bits = (unsigned int) (diff & 15);
		diff4Bits <<= 32-4;
		diff4Bits >>= j*4;
		((char*)&myGuid.g)[j] ^= diff4Bits;
	}

#else
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	myGuid.g=tv.tv_usec + tv.tv_sec * 1000000;
#endif
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ProcessPortUnreachable( unsigned int binaryAddress, unsigned short port, RakPeer *rakPeer )
{
	(void) binaryAddress;
	(void) port;
	(void) rakPeer;

}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool ProcessOfflineNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetSmartPtr<RakNetSocket> rakNetSocket, bool *isOfflineMessage, RakNetTimeUS timeRead )
{
	(void) timeRead;
	RakPeer::RemoteSystemStruct *remoteSystem;
	Packet *packet;
	unsigned i;


	char str1[64];
	systemAddress.ToString(false, str1);
	if (rakPeer->IsBanned( str1 ))
	{
		for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
			rakPeer->messageHandlerList[i]->OnDirectSocketReceive(data, length*8, systemAddress);

		RakNet::BitStream bs;
		bs.Write((MessageID)ID_CONNECTION_BANNED);
		bs.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
		bs.Write(rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));

		unsigned i;
		for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
			rakPeer->messageHandlerList[i]->OnDirectSocketSend((char*) bs.GetData(), bs.GetNumberOfBitsUsed(), systemAddress);
		SocketLayer::SendTo( rakNetSocket->s, (char*) bs.GetData(), bs.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

		return true;
	}



	// The reason for all this is that the reliability layer has no way to tell between offline messages that arrived late for a player that is now connected,
	// and a regular encoding. So I insert OFFLINE_MESSAGE_DATA_ID into the stream, the encoding of which is essentially impossible to hit by random chance
	if (length <=2)
	{
		*isOfflineMessage=true;
	}
	else if (
		((unsigned char)data[0] == ID_PING ||
		(unsigned char)data[0] == ID_PING_OPEN_CONNECTIONS) &&
		length == sizeof(unsigned char) + sizeof(RakNetTime) + sizeof(OFFLINE_MESSAGE_DATA_ID))
	{
		*isOfflineMessage=memcmp(data+sizeof(unsigned char) + sizeof(RakNetTime), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else if ((unsigned char)data[0] == ID_PONG && (size_t) length >= sizeof(unsigned char) + sizeof(RakNetTime) + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID))
	{
		*isOfflineMessage=memcmp(data+sizeof(unsigned char) + sizeof(RakNetTime) + RakNetGUID::size(), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else if (
		(unsigned char)data[0] == ID_OUT_OF_BAND_INTERNAL	&&
		(size_t) length >= sizeof(MessageID) + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID))
	{
		*isOfflineMessage=memcmp(data+sizeof(MessageID) + RakNetGUID::size(), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else if (
		(unsigned char)data[0] == ID_OPEN_CONNECTION_REQUEST	&&
		(size_t) length >= sizeof(MessageID)*2 + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID))
	{
		*isOfflineMessage=memcmp(data+sizeof(MessageID)*2 + RakNetGUID::size(), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else if (
		(
		(unsigned char)data[0] == ID_OPEN_CONNECTION_REPLY ||
		(unsigned char)data[0] == ID_CONNECTION_ATTEMPT_FAILED ||
		(unsigned char)data[0] == ID_NO_FREE_INCOMING_CONNECTIONS ||
		(unsigned char)data[0] == ID_CONNECTION_BANNED ||
		(unsigned char)data[0] == ID_ALREADY_CONNECTED ||
		(unsigned char)data[0] == ID_IP_RECENTLY_CONNECTED ||
		(unsigned char)data[0] == ID_CONNECTION_REQUEST) &&
		(size_t) length >= sizeof(MessageID) + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID))
	{
		*isOfflineMessage=memcmp(data+sizeof(MessageID), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else if (((unsigned char)data[0] == ID_INCOMPATIBLE_PROTOCOL_VERSION&&
		(size_t) length == sizeof(MessageID)*2 + RakNetGUID::size() + sizeof(OFFLINE_MESSAGE_DATA_ID)))
	{
		*isOfflineMessage=memcmp(data+sizeof(MessageID)*2, OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
	}
	else
	{
		*isOfflineMessage=false;
	}

	if (*isOfflineMessage)
	{
		for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
			rakPeer->messageHandlerList[i]->OnDirectSocketReceive(data, length*8, systemAddress);

		// These are all messages from unconnected systems.  Messages here can be any size, but are never processed from connected systems.
		if ( ( (unsigned char) data[ 0 ] == ID_PING_OPEN_CONNECTIONS
			|| (unsigned char)(data)[0] == ID_PING)	&& length == sizeof(unsigned char)+sizeof(RakNetTime)+sizeof(OFFLINE_MESSAGE_DATA_ID) )
		{
			if ( (unsigned char)(data)[0] == ID_PING ||
				rakPeer->AllowIncomingConnections() ) // Open connections with players
			{
// #if !defined(_XBOX) && !defined(X360)
				RakNet::BitStream inBitStream( (unsigned char *) data, length, false );
				inBitStream.IgnoreBits(8);
				RakNetTime sendPingTime;
				inBitStream.Read(sendPingTime);

				RakNet::BitStream outBitStream;
				outBitStream.Write((MessageID)ID_PONG); // Should be named ID_UNCONNECTED_PONG eventually
				outBitStream.Write(sendPingTime);
				outBitStream.Write(rakPeer->myGuid);
				outBitStream.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));

				rakPeer->rakPeerMutexes[ RakPeer::offlinePingResponse_Mutex ].Lock();
				// They are connected, so append offline ping data
				outBitStream.Write( (char*)rakPeer->offlinePingResponse.GetData(), rakPeer->offlinePingResponse.GetNumberOfBytesUsed() );
				rakPeer->rakPeerMutexes[ RakPeer::offlinePingResponse_Mutex ].Unlock();

				unsigned i;
				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*)outBitStream.GetData(), outBitStream.GetNumberOfBytesUsed(), systemAddress);

				char str1[64];
				systemAddress.ToString(false, str1);
				SocketLayer::SendTo( rakNetSocket->s, (const char*)outBitStream.GetData(), (unsigned int) outBitStream.GetNumberOfBytesUsed(), str1 , systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

				packet=rakPeer->AllocPacket(sizeof(MessageID), __FILE__, __LINE__);
				packet->data[0]=data[0];
				packet->systemAddress = systemAddress;
				packet->guid=UNASSIGNED_RAKNET_GUID;
				packet->systemAddress.systemIndex = ( SystemIndex ) rakPeer->GetIndexFromSystemAddress( systemAddress, true );
				packet->guid.systemIndex=packet->systemAddress.systemIndex;
				rakPeer->AddPacketToProducer(packet);
// #endif
			}
		}
		// UNCONNECTED MESSAGE Pong with no data.
		else if ((unsigned char) data[ 0 ] == ID_PONG && (size_t) length >= sizeof(unsigned char)+sizeof(RakNetTime)+RakNetGUID::size()+sizeof(OFFLINE_MESSAGE_DATA_ID) && (size_t) length < sizeof(unsigned char)+sizeof(RakNetTime)+RakNetGUID::size()+sizeof(OFFLINE_MESSAGE_DATA_ID)+MAX_OFFLINE_DATA_LENGTH)
		{
			packet=rakPeer->AllocPacket((unsigned int) (length-sizeof(OFFLINE_MESSAGE_DATA_ID)-RakNetGUID::size()), __FILE__, __LINE__);
			RakNet::BitStream bs((unsigned char*) data, length, false);
			bs.IgnoreBytes(sizeof(unsigned char)+sizeof(RakNetTime));
			bs.Read(packet->guid);
			packet->data[0]=ID_PONG;
			// Don't endian swap the time, so the user can do so when reading out as a bitstream
			memcpy(packet->data+sizeof(unsigned char), data+sizeof(unsigned char), sizeof(RakNetTime));
// 			RakNetTime test1;
// 			memcpy(&test1,data+sizeof(unsigned char), sizeof(RakNetTime));
// 			RakNetTime test2;
// 			test2=RakNet::GetTime();
			memcpy(packet->data+sizeof(unsigned char)+sizeof(RakNetTime), data+sizeof(unsigned char)+sizeof(RakNetTime)+RakNetGUID::size()+sizeof(OFFLINE_MESSAGE_DATA_ID), length-sizeof(unsigned char)-sizeof(RakNetTime)-RakNetGUID::size()-sizeof(OFFLINE_MESSAGE_DATA_ID));
			packet->bitSize = BYTES_TO_BITS(packet->length);
			packet->systemAddress = systemAddress;
			packet->systemAddress.systemIndex = ( SystemIndex ) rakPeer->GetIndexFromSystemAddress( systemAddress, true );
			packet->guid.systemIndex=packet->systemAddress.systemIndex;
			rakPeer->AddPacketToProducer(packet);
		}
		else if ((unsigned char) data[ 0 ] == ID_OUT_OF_BAND_INTERNAL &&
			(size_t) length < MAX_OFFLINE_DATA_LENGTH+sizeof(OFFLINE_MESSAGE_DATA_ID)+sizeof(MessageID)+RakNetGUID::size())
		{
			unsigned int dataLength = (unsigned int) (length-sizeof(OFFLINE_MESSAGE_DATA_ID)-RakNetGUID::size()-sizeof(MessageID));
			RakAssert(dataLength<1024);
			packet=rakPeer->AllocPacket(dataLength+sizeof(MessageID), __FILE__, __LINE__);
			RakAssert(packet->length<1024);

			RakNet::BitStream bs2((unsigned char*) data, length, false);
			bs2.IgnoreBytes(sizeof(MessageID));
			bs2.Read(packet->guid);

			if (data[sizeof(OFFLINE_MESSAGE_DATA_ID)+sizeof(MessageID) + RakNetGUID::size()]==ID_ADVERTISE_SYSTEM)
			{
				packet->length--;
				packet->bitSize=BYTES_TO_BITS(packet->length);
				packet->data[0]=ID_ADVERTISE_SYSTEM;
				memcpy(packet->data+1, data+sizeof(OFFLINE_MESSAGE_DATA_ID)+sizeof(MessageID)*2 + RakNetGUID::size(), dataLength);
			}
			else
			{
				packet->data[0]=ID_OUT_OF_BAND_INTERNAL;
				memcpy(packet->data+1, data+sizeof(OFFLINE_MESSAGE_DATA_ID)+sizeof(MessageID) + RakNetGUID::size(), dataLength);
			}

			packet->systemAddress = systemAddress;
			packet->systemAddress.systemIndex = ( SystemIndex ) rakPeer->GetIndexFromSystemAddress( systemAddress, true );
			packet->guid.systemIndex=packet->systemAddress.systemIndex;
			rakPeer->AddPacketToProducer(packet);
		}
		else if ((unsigned char)(data)[0] == (MessageID)ID_OPEN_CONNECTION_REPLY)
		{
			for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
				rakPeer->messageHandlerList[i]->OnDirectSocketReceive(data, length*8, systemAddress);

			RakNet::BitStream bs((unsigned char*) data,length,false);
			bs.IgnoreBytes(sizeof(MessageID));
			bs.IgnoreBytes(sizeof(OFFLINE_MESSAGE_DATA_ID));
			RakNetGUID guid;
			bs.Read(guid);
			SystemAddress bindingAddress;
			bs.Read(bindingAddress);

			RakPeer::RequestedConnectionStruct *rcs;
			bool unlock=true;
			unsigned i;
			rakPeer->requestedConnectionQueueMutex.Lock();
			for (i=0; i <  rakPeer->requestedConnectionQueue.Size(); i++)
			{
				rcs=rakPeer->requestedConnectionQueue[i];
				if (rcs->systemAddress==systemAddress)
				{
					rakPeer->requestedConnectionQueueMutex.Unlock();
					unlock=false;

					RakAssert(rcs->actionToTake==RakPeer::RequestedConnectionStruct::CONNECT);
					// You might get this when already connected because of cross-connections
					bool thisIPConnectedRecently=false;
					remoteSystem=rakPeer->GetRemoteSystemFromSystemAddress( systemAddress, true, true );
					// Removeme
					// printf("1 p=%i\n", rakPeer->mySystemAddress->port);
					if (remoteSystem==0)
					{
						// Removeme
						// printf("2 p=%i\n", rakPeer->mySystemAddress->port);
						if (rcs->socket.IsNull())
						{
							// Removeme
							// printf("3 p=%i\n", rakPeer->mySystemAddress->port);
							remoteSystem=rakPeer->AssignSystemAddressToRemoteSystemList(systemAddress, RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER, rakNetSocket, &thisIPConnectedRecently, bindingAddress, length+UDP_HEADER_SIZE, guid);
						}
						else
						{
							// Removeme
							// printf("4 p=%i\n", rakPeer->mySystemAddress->port);
							remoteSystem=rakPeer->AssignSystemAddressToRemoteSystemList(systemAddress, RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER, rcs->socket, &thisIPConnectedRecently, bindingAddress, length+UDP_HEADER_SIZE, guid);
						}


						//						printf("System %i got ID_OPEN_CONNECTION_REPLY from %i\n", rakPeer->mySystemAddress[0].port, systemAddress.port);
					}

					// 4/13/09 Attackers can flood ID_OPEN_CONNECTION_REQUEST and use up all available connection slots
					// Ignore connection attempts if this IP address connected within the last 100 milliseconds
					if (thisIPConnectedRecently==false)
					{
						// Don't check GetRemoteSystemFromGUID, server will verify
						if (remoteSystem)
						{
							// RakNetTimeUS time = RakNet::GetTimeNS();
							remoteSystem->weInitiatedTheConnection=true;
							remoteSystem->connectMode=RakPeer::RemoteSystemStruct::REQUESTED_CONNECTION;
							if (rcs->timeoutTime!=0)
								remoteSystem->reliabilityLayer.SetTimeoutTime(rcs->timeoutTime);


							RakNet::BitStream temp;
							temp.Write( (MessageID)ID_CONNECTION_REQUEST );
							temp.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
							temp.Write(rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
							temp.Write(RakNet::GetTime());

							if ( rcs->outgoingPasswordLength > 0 )
								temp.Write( ( char* ) rcs->outgoingPassword,  rcs->outgoingPasswordLength );

							rakPeer->SendImmediate((char*)temp.GetData(), temp.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, RELIABLE, 0, systemAddress, false, false, timeRead, 0 );
						}
						else
						{
							// Failed, no connections available anymore
							packet=rakPeer->AllocPacket(sizeof( char ), __FILE__, __LINE__);
							packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
							packet->bitSize = ( sizeof( char ) * 8);
							packet->systemAddress = rcs->systemAddress;
							packet->guid=guid;
							rakPeer->AddPacketToProducer(packet);
						}
					}

					rakPeer->requestedConnectionQueueMutex.Lock();
					for (unsigned int k=0; k < rakPeer->requestedConnectionQueue.Size(); k++)
					{
						if (rakPeer->requestedConnectionQueue[k]->systemAddress==systemAddress)
						{
							rakPeer->requestedConnectionQueue.RemoveAtIndex(k);
							break;
						}
					}
					rakPeer->requestedConnectionQueueMutex.Unlock();

					RakNet::OP_DELETE(rcs,__FILE__,__LINE__);

					break;
				}
			}

			if (unlock)
				rakPeer->requestedConnectionQueueMutex.Unlock();

			return true;

		}
		else if ((unsigned char)(data)[0] == (MessageID)ID_CONNECTION_ATTEMPT_FAILED ||
			(unsigned char)(data)[0] == (MessageID)ID_NO_FREE_INCOMING_CONNECTIONS ||
			(unsigned char)(data)[0] == (MessageID)ID_CONNECTION_BANNED ||
			(unsigned char)(data)[0] == (MessageID)ID_ALREADY_CONNECTED ||
			(unsigned char)(data)[0] == (MessageID)ID_INVALID_PASSWORD ||
			(unsigned char)(data)[0] == (MessageID)ID_IP_RECENTLY_CONNECTED ||
			(unsigned char)(data)[0] == (MessageID)ID_INCOMPATIBLE_PROTOCOL_VERSION)
		{

			RakNet::BitStream bs((unsigned char*) data,length,false);
			bs.IgnoreBytes(sizeof(MessageID));
			bs.IgnoreBytes(sizeof(OFFLINE_MESSAGE_DATA_ID));
			if ((unsigned char)(data)[0] == (MessageID)ID_INCOMPATIBLE_PROTOCOL_VERSION)
				bs.IgnoreBytes(sizeof(unsigned char));

			RakNetGUID guid;
			bs.Read(guid);

			RakPeer::RequestedConnectionStruct *rcs;
			bool connectionAttemptCancelled=false;
			unsigned i;
			rakPeer->requestedConnectionQueueMutex.Lock();
			for (i=0; i <  rakPeer->requestedConnectionQueue.Size(); i++)
			{
				rcs=rakPeer->requestedConnectionQueue[i];
				if (rcs->actionToTake==RakPeer::RequestedConnectionStruct::CONNECT && rcs->systemAddress==systemAddress)
				{
					connectionAttemptCancelled=true;




					rakPeer->requestedConnectionQueue.RemoveAtIndex(i);
					RakNet::OP_DELETE(rcs,__FILE__,__LINE__);
					break;
				}
			}

			rakPeer->requestedConnectionQueueMutex.Unlock();

			if (connectionAttemptCancelled)
			{
				// Tell user of connection attempt failed
				packet=rakPeer->AllocPacket(sizeof( char ), __FILE__, __LINE__);
				packet->data[ 0 ] = data[0]; // Attempted a connection and couldn't
				packet->bitSize = ( sizeof( char ) * 8);
				packet->systemAddress = systemAddress;
				packet->guid=guid;
				rakPeer->AddPacketToProducer(packet);
			}
		}
		else if ((unsigned char)(data)[0] == ID_OPEN_CONNECTION_REQUEST && length >= sizeof(unsigned char)*2)
		{
			//			if (rakPeer->mySystemAddress[0].port!=60481)
			//				return;

			unsigned int i;
			//RAKNET_DEBUG_PRINTF("%i:IOCR, ", __LINE__);
			char remoteProtocol=data[1];
			if (remoteProtocol!=RAKNET_PROTOCOL_VERSION)
			{
				RakNet::BitStream bs;
				bs.Write((MessageID)ID_INCOMPATIBLE_PROTOCOL_VERSION);
				bs.Write((unsigned char)RAKNET_PROTOCOL_VERSION);
				bs.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
				bs.Write(rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));

				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((char*)bs.GetData(), bs.GetNumberOfBitsUsed(), systemAddress);

				SocketLayer::SendTo( rakNetSocket->s, (char*)bs.GetData(), bs.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
				return true;
			}

			for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
				rakPeer->messageHandlerList[i]->OnDirectSocketReceive(data, length*8, systemAddress);

			RakNetGUID guid;
			RakNet::BitStream bsOut;
			RakNet::BitStream bs((unsigned char*) data, length, false);
			bs.IgnoreBytes(sizeof(MessageID)*2);
			bs.Read(guid);
			bs.AlignReadToByteBoundary();
			bs.IgnoreBytes(sizeof(OFFLINE_MESSAGE_DATA_ID));
			SystemAddress bindingAddress;
			bs.Read(bindingAddress);

			RakPeer::RemoteSystemStruct *rssFromSA = rakPeer->GetRemoteSystemFromSystemAddress( systemAddress, true, true );
			bool IPAddrInUse = rssFromSA != 0 && rssFromSA->isActive;
			RakPeer::RemoteSystemStruct *rssFromGuid = rakPeer->GetRemoteSystemFromGUID(guid, true);
			bool GUIDInUse = rssFromGuid != 0 && rssFromGuid->isActive;

			// IPAddrInUse, GuidInUse, outcome
			// TRUE,	  , TRUE	 , ID_OPEN_CONNECTION_REPLY if they are the same, else ID_ALREADY_CONNECTED
			// FALSE,     , TRUE     , ID_ALREADY_CONNECTED (someone else took this guid)
			// TRUE,	  , FALSE	 , ID_ALREADY_CONNECTED (silently disconnected, restarted rakNet)
			// FALSE	  , FALSE	 , Allow connection

			int outcome;
			if (IPAddrInUse & GUIDInUse)
			{
				if (rssFromSA==rssFromGuid && rssFromSA->connectMode==RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER)
				{
					// ID_OPEN_CONNECTION_REPLY if they are the same
					outcome=1;
				}
				else
				{
					// ID_ALREADY_CONNECTED (restarted raknet, connected again from same ip, plus someone else took this guid)
					outcome=2;
				}
			}
			else if (IPAddrInUse==false && GUIDInUse==true)
			{
				// ID_ALREADY_CONNECTED (someone else took this guid)
				outcome=3;
			}
			else if (IPAddrInUse==true && GUIDInUse==false)
			{
				// ID_ALREADY_CONNECTED (silently disconnected, restarted rakNet)
				outcome=4;
			}
			else
			{
				// Allow connection
				outcome=0;
			}

			if (outcome==1)
			{
				// Duplicate connection request packet
				bsOut.Write((MessageID)ID_OPEN_CONNECTION_REPLY);
				bsOut.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
				bsOut.Write(rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
				bsOut.Write(systemAddress);
				bsOut.PadWithZeroToByteLength(length); // Pad to the same MTU
				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*) bsOut.GetData(), bsOut.GetNumberOfBitsUsed(), systemAddress);
				SocketLayer::SetDoNotFragment(rakNetSocket->s, 1);
				SocketLayer::SendTo( rakNetSocket->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
				SocketLayer::SetDoNotFragment(rakNetSocket->s, 0);
				return true;
			}
			else if (outcome!=0)
			{
				bsOut.Write((MessageID)ID_ALREADY_CONNECTED);
				bsOut.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
				bsOut.Write(guid);
				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*) bsOut.GetData(), bsOut.GetNumberOfBitsUsed(), systemAddress);
				SocketLayer::SendTo( rakNetSocket->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

				return true;
			}

			if (rakPeer->AllowIncomingConnections()==false)
			{
				bsOut.Write((MessageID)ID_NO_FREE_INCOMING_CONNECTIONS);
				bsOut.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
				bsOut.Write(guid);
				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*) bsOut.GetData(), bsOut.GetNumberOfBitsUsed(), systemAddress);
				SocketLayer::SendTo( rakNetSocket->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

				return true;
			}

			bool thisIPConnectedRecently=false;
			rakPeer->AssignSystemAddressToRemoteSystemList(systemAddress, RakPeer::RemoteSystemStruct::UNVERIFIED_SENDER, rakNetSocket, &thisIPConnectedRecently, bindingAddress, length+UDP_HEADER_SIZE, guid);

			if (thisIPConnectedRecently==true)
			{
				bsOut.Write((MessageID)ID_IP_RECENTLY_CONNECTED);
				bsOut.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
				bsOut.Write(guid);
				for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
					rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*) bsOut.GetData(), bsOut.GetNumberOfBitsUsed(), systemAddress);
				SocketLayer::SendTo( rakNetSocket->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

				return true;
			}

			bsOut.Write((MessageID)ID_OPEN_CONNECTION_REPLY);
			bsOut.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
			bsOut.Write(rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS));
			bsOut.Write(systemAddress);
			bsOut.PadWithZeroToByteLength(length); // Pad to the same MTU
			for (i=0; i < rakPeer->messageHandlerList.Size(); i++)
				rakPeer->messageHandlerList[i]->OnDirectSocketSend((const char*) bsOut.GetData(), bsOut.GetNumberOfBitsUsed(), systemAddress);
			SocketLayer::SetDoNotFragment(rakNetSocket->s, 1);
			SocketLayer::SendTo( rakNetSocket->s, (const char*) bsOut.GetData(), bsOut.GetNumberOfBytesUsed(), systemAddress.binaryAddress, systemAddress.port, rakNetSocket->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
			SocketLayer::SetDoNotFragment(rakNetSocket->s, 0);


		}
		return true;
	}

	return false;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetTimeUS timeRead )
{
	ProcessNetworkPacket(systemAddress,data,length,rakPeer,rakPeer->socketList[0],timeRead);
}
void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetSmartPtr<RakNetSocket> rakNetSocket, RakNetTimeUS timeRead )
{
	RakAssert(systemAddress.port);
	bool isOfflineMessage;
	if (ProcessOfflineNetworkPacket(systemAddress, data, length, rakPeer, rakNetSocket, &isOfflineMessage, timeRead))
	{
		return;
	}

	Packet *packet;
	RakPeer::RemoteSystemStruct *remoteSystem;

	// See if this datagram came from a connected system
	remoteSystem = rakPeer->GetRemoteSystemFromSystemAddress( systemAddress, true, true );
	if ( remoteSystem )
	{
		if (remoteSystem->connectMode==RakPeer::RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET && (length & 15)==0) // & 15 = mod 16
		{
			// Test the key before setting it
			unsigned int newLength;
			char output[ MAXIMUM_MTU_SIZE ];
			DataBlockEncryptor testEncryptor;
			testEncryptor.SetKey(remoteSystem->AESKey);
			//if ( testEncryptor.Decrypt( ( unsigned char* ) data, length, (unsigned char*) output,&newLength ) == true )
			if ( testEncryptor.Decrypt( ( unsigned char* ) data, length, (unsigned char*) output, &newLength ) == true )
				remoteSystem->reliabilityLayer.SetEncryptionKey( remoteSystem->AESKey);
		}

		// Handle regular incoming data
		// HandleSocketReceiveFromConnectedPlayer is only safe to be called from the same thread as Update, which is this thread
		if ( isOfflineMessage==false)
		{
			if (remoteSystem->reliabilityLayer.HandleSocketReceiveFromConnectedPlayer( 
				data, length, systemAddress, rakPeer->messageHandlerList, remoteSystem->MTUSize,
				rakNetSocket->s, &rnr, rakNetSocket->remotePortRakNetWasStartedOn_PS3, timeRead) == false)
			{
				// These kinds of packets may have been duplicated and incorrectly determined to be
				// cheat packets.  Anything else really is a cheat packet
				if ( !(
					( (unsigned char)data[0] == ID_CONNECTION_BANNED  ) ||
					( (unsigned char)data[0] == ID_OPEN_CONNECTION_REQUEST ) ||
					( (unsigned char)data[0] == ID_OPEN_CONNECTION_REPLY ) ||
					( (unsigned char)data[0] == ID_CONNECTION_ATTEMPT_FAILED ) ||
					( (unsigned char)data[0] == ID_IP_RECENTLY_CONNECTED ) ||
					( (unsigned char)data[0] == ID_INCOMPATIBLE_PROTOCOL_VERSION ))
					)
				{
					// Unknown message.  Could be caused by old out of order stuff from unconnected or no longer connected systems, etc.
					packet=rakPeer->AllocPacket(1, __FILE__, __LINE__);
					packet->data[ 0 ] = ID_MODIFIED_PACKET;
					packet->bitSize = sizeof( char ) * 8;
					packet->systemAddress = systemAddress;
					packet->systemAddress.systemIndex = ( SystemIndex ) rakPeer->GetIndexFromSystemAddress( systemAddress, true );
					packet->guid=remoteSystem->guid;
					packet->guid.systemIndex=packet->systemAddress.systemIndex;
					rakPeer->AddPacketToProducer(packet);
				}
			}
		}
	}
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GenerateSeedFromGuid(void)
{
	/*
	// Construct a random seed based on the initial guid value, and the last digits of the difference to each subsequent number
	// This assumes that only the last 3 bits of each guidId integer has a meaningful amount of randomness between it and the prior number
	unsigned int t = guid.g[0];
	unsigned int i;
	for (i=1; i < sizeof(guid.g) / sizeof(guid.g[0]); i++)
	{
		unsigned int diff = guid.g[i]-guid.g[i-1];
		unsigned int diff3Bits = diff & 0x0007;
		diff3Bits <<= 29;
		diff3Bits >>= (i-1)*3;
		t ^= diff3Bits;
	}

	return t;
	*/
	return (unsigned int) ((myGuid.g >> 32) ^ myGuid.g);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void RakPeer::DerefAllSockets(void)
{
	unsigned int i;
	for (i=0; i < socketList.Size(); i++)
	{
		socketList[i].SetNull();
	}
	socketList.Clear(false, __FILE__, __LINE__);
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned int RakPeer::GetRakNetSocketFromUserConnectionSocketIndex(unsigned int userIndex) const
{
	unsigned int i;
	for (i=0; i < socketList.Size(); i++)
	{
		if (socketList[i]->userConnectionSocketIndex==userIndex)
			return i;
	}
	RakAssert("GetRakNetSocketFromUserConnectionSocketIndex failed" && 0);
	return (unsigned int) -1;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool RakPeer::RunUpdateCycle( RakNetTimeUS timeNS, RakNetTime timeMS )
{
	RakPeer::RemoteSystemStruct * remoteSystem;
	unsigned remoteSystemIndex;
	Packet *packet;
	// int currentSentBytes,currentReceivedBytes;
//	unsigned numberOfBytesUsed;
	BitSize_t numberOfBitsUsed;
	//SystemAddress authoritativeClientSystemAddress;
	BitSize_t bitSize;
	unsigned int byteSize;
	unsigned char *data;
	SystemAddress systemAddress;
	BufferedCommandStruct *bcs;
	bool callerDataAllocationUsed;
	RakNetStatistics *rnss;

	/*
	int errorCode;
	int gotData;
	unsigned connectionSocketIndex;
	for (connectionSocketIndex=0; connectionSocketIndex < socketList.Size(); connectionSocketIndex++)
	{
		do
		{
			gotData = SocketLayer::RecvFrom( socketList[connectionSocketIndex]->s, this, &errorCode, socketList[connectionSocketIndex], socketList[connectionSocketIndex]->remotePortRakNetWasStartedOn_PS3 );

			if ( gotData == -1 )
			{
#ifdef _WIN32
				if ( errorCode == WSAECONNRESET )
				{
					gotData=false;
				}
				else
					if ( errorCode != 0 && endThreads == false )
					{
#ifdef _DO_PRINTF
						RAKNET_DEBUG_PRINTF( "Server RecvFrom critical failure!\n" );
#endif
						// Some kind of critical error
						// peer->isRecvfromThreadActive=false;
						endThreads = true;
						Shutdown( 0, 0 );
						return false;
					}

#else
				if ( errorCode == -1 )
				{
					// isRecvfromThreadActive=false;
					endThreads = true;
					Shutdown( 0 );
					return false;
				}
#endif
			}

			if ( endThreads )
				return false;
		}
		while ( gotData>0 ); // Read until there is nothing left
	}
	*/

	// This is here so RecvFromBlocking actually gets data from the same thread
	if (SocketLayer::GetSocketLayerOverride())
	{
		SystemAddress sender;
		char dataOut[ MAXIMUM_MTU_SIZE ];
		int len = SocketLayer::GetSocketLayerOverride()->RakNetRecvFrom(socketList[0]->s,this,dataOut,&sender,true);
		if (len>0)
		{
			ProcessNetworkPacket( sender, dataOut, len, this, socketList[0], RakNet::GetTimeUS() );
			return 1;
		}
	}

	unsigned int socketListIndex;
	RakPeer::RecvFromStruct *recvFromStruct;
	while ((recvFromStruct=bufferedPackets.PopInaccurate())!=0)
	{
		for (socketListIndex=0; socketListIndex < socketList.Size(); socketListIndex++)
		{
			if (socketList[socketListIndex]->s==recvFromStruct->s)
				break;
		}
		if (socketListIndex!=socketList.Size())
			ProcessNetworkPacket(recvFromStruct->systemAddress, recvFromStruct->data, recvFromStruct->bytesRead, this, socketList[socketListIndex], recvFromStruct->timeRead);
		bufferedPackets.Deallocate(recvFromStruct, __FILE__,__LINE__);
	}

#ifdef _RAKNET_THREADSAFE
	while ((bcs=bufferedCommands.PopInaccurate())!=0)
#else
	// Process all the deferred user thread Send and connect calls
	while ((bcs=bufferedCommands.ReadLock())!=0)
#endif
	{
		if (bcs->command==BufferedCommandStruct::BCS_SEND)
		{
			// GetTime is a very slow call so do it once and as late as possible
			if (timeNS==0)
			{
				timeNS = RakNet::GetTimeNS();
				timeMS = (RakNetTime)(timeNS/(RakNetTimeUS)1000);
			}

			callerDataAllocationUsed=SendImmediate((char*)bcs->data, bcs->numberOfBitsToSend, bcs->priority, bcs->reliability, bcs->orderingChannel, bcs->systemIdentifier, bcs->broadcast, true, timeNS, bcs->receipt);
			if ( callerDataAllocationUsed==false )
			{
				rakFree_Ex(bcs->data, __FILE__, __LINE__ );
				bcs->data=0;
			}

			// Set the new connection state AFTER we call sendImmediate in case we are setting it to a disconnection state, which does not allow further sends
			if (bcs->connectionMode!=RemoteSystemStruct::NO_ACTION )
			{
				remoteSystem=GetRemoteSystem( bcs->systemIdentifier, true, true );
				if (remoteSystem)
					remoteSystem->connectMode=bcs->connectionMode;
			}
		}
		else if (bcs->command==BufferedCommandStruct::BCS_SEND_OUT_OF_BAND)
		{
			char host[128];
			bcs->systemIdentifier.systemAddress.ToString(false,host);

			unsigned i;
			for (i=0; i < messageHandlerList.Size(); i++)
				messageHandlerList[i]->OnDirectSocketSend(bcs->data, bcs->numberOfBitsToSend, systemAddress);
			unsigned int realIndex = GetRakNetSocketFromUserConnectionSocketIndex(bcs->connectionSocketIndex);
			SocketLayer::SendTo( socketList[realIndex]->s, bcs->data, (int) BITS_TO_BYTES(bcs->numberOfBitsToSend), ( char* ) host, bcs->systemIdentifier.systemAddress.port, socketList[realIndex]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );

			rakFree_Ex(bcs->data, __FILE__, __LINE__ );
			bcs->data=0;
		}
		else if (bcs->command==BufferedCommandStruct::BCS_CLOSE_CONNECTION)
		{
			CloseConnectionInternal(bcs->systemIdentifier, false, true, bcs->orderingChannel, bcs->priority);
		}
		else if (bcs->command==BufferedCommandStruct::BCS_CHANGE_SYSTEM_ADDRESS)
		{
			// Reroute
			RakPeer::RemoteSystemStruct *rssFromGuid = GetRemoteSystem(bcs->systemIdentifier.rakNetGuid,true,true);
			if (rssFromGuid!=0)
			{
				unsigned int existingSystemIndex = GetRemoteSystemIndex(rssFromGuid->systemAddress);
				ReferenceRemoteSystem(bcs->systemIdentifier.systemAddress, existingSystemIndex);
			}
		}
		else if (bcs->command==BufferedCommandStruct::BCS_GET_SOCKET)
		{
			SocketQueryOutput *sqo;
			if (bcs->systemIdentifier.IsUndefined())
			{
#ifdef _RAKNET_THREADSAFE
				sqo = socketQueryOutput.Allocate( __FILE__, __LINE__ );
				sqo->sockets=socketList;
				socketQueryOutput.Push(sqo);
#else
				sqo = socketQueryOutput.WriteLock();
				sqo->sockets=socketList;
				socketQueryOutput.WriteUnlock();
#endif
			}
			else
			{
				remoteSystem=GetRemoteSystem( bcs->systemIdentifier, true, true );
#ifdef _RAKNET_THREADSAFE
				sqo = socketQueryOutput.Allocate( __FILE__, __LINE__ );
#else
				sqo = socketQueryOutput.WriteLock();
#endif
				sqo->sockets.Clear(false, __FILE__, __LINE__);
				if (remoteSystem)
				{
					sqo->sockets.Push(remoteSystem->rakNetSocket, __FILE__, __LINE__ );
				}
				else
				{
					// Leave empty smart pointer
				}
#ifdef _RAKNET_THREADSAFE
				socketQueryOutput.Push(sqo);
#else
				socketQueryOutput.WriteUnlock();
#endif
			}

		}


#ifdef _RAKNET_THREADSAFE
		bufferedCommands.Deallocate(bcs, __FILE__,__LINE__);
#else
		bufferedCommands.ReadUnlock();
#endif
	}

	if (requestedConnectionQueue.IsEmpty()==false)
	{
		if (timeNS==0)
		{
			timeNS = RakNet::GetTimeNS();
			timeMS = (RakNetTime)(timeNS/(RakNetTimeUS)1000);
		}

		bool condition1, condition2;
		RequestedConnectionStruct *rcs;
		unsigned requestedConnectionQueueIndex=0;
		requestedConnectionQueueMutex.Lock();
		while (requestedConnectionQueueIndex < requestedConnectionQueue.Size())
		{
			rcs = requestedConnectionQueue[requestedConnectionQueueIndex];
			requestedConnectionQueueMutex.Unlock();
			if (rcs->nextRequestTime < timeMS)
			{
				condition1=rcs->requestsMade==rcs->sendConnectionAttemptCount+1;
				condition2=(bool)((rcs->systemAddress==UNASSIGNED_SYSTEM_ADDRESS)==1);
				// If too many requests made or a hole then remove this if possible, otherwise invalidate it
				if (condition1 || condition2)
				{
					if (rcs->data)
					{
						rakFree_Ex(rcs->data, __FILE__, __LINE__ );
						rcs->data=0;
					}

					if (condition1 && !condition2 && rcs->actionToTake==RequestedConnectionStruct::CONNECT)
					{
						// Tell user of connection attempt failed
						packet=AllocPacket(sizeof( char ), __FILE__, __LINE__);
						packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
						packet->bitSize = ( sizeof(	 char ) * 8);
						packet->systemAddress = rcs->systemAddress;
						AddPacketToProducer(packet);
					}

					RakNet::OP_DELETE(rcs,__FILE__,__LINE__);

					requestedConnectionQueueMutex.Lock();
					for (unsigned int k=0; k < requestedConnectionQueue.Size(); k++)
					{
						if (requestedConnectionQueue[k]==rcs)
						{
							requestedConnectionQueue.RemoveAtIndex(k);
							break;
						}
					}
					requestedConnectionQueueMutex.Unlock();
				}
				else
				{

					int MTUSizeIndex = rcs->requestsMade / (rcs->sendConnectionAttemptCount/NUM_MTU_SIZES);
					if (MTUSizeIndex>=NUM_MTU_SIZES)
						MTUSizeIndex=NUM_MTU_SIZES-1;
					rcs->requestsMade++;
					rcs->nextRequestTime=timeMS+rcs->timeBetweenSendConnectionAttemptsMS;
		//			char c[MAXIMUM_MTU_SIZE];
		//			c[0] = ID_OPEN_CONNECTION_REQUEST;
		//			c[1] = RAKNET_PROTOCOL_VERSION;

					RakNet::BitStream bitStream;
					bitStream.Write((MessageID)ID_OPEN_CONNECTION_REQUEST);
					bitStream.Write((MessageID)RAKNET_PROTOCOL_VERSION);
					bitStream.Write(myGuid);
					bitStream.WriteAlignedBytes((const unsigned char*) OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID));
					bitStream.Write(rcs->systemAddress);
					// Pad out to MTU test size
					bitStream.PadWithZeroToByteLength(mtuSizes[MTUSizeIndex]-UDP_HEADER_SIZE);

// 					bool isProperOfflineMessage=memcmp(bitStream.GetData()+sizeof(MessageID)*2 + RakNetGUID::size(), OFFLINE_MESSAGE_DATA_ID, sizeof(OFFLINE_MESSAGE_DATA_ID))==0;
// 					RakAssert(isProperOfflineMessage);

					char str[256];
					rcs->systemAddress.ToString(true,str);

					//RAKNET_DEBUG_PRINTF("%i:IOCR, ", __LINE__);

					unsigned i;
					for (i=0; i < messageHandlerList.Size(); i++)
						messageHandlerList[i]->OnDirectSocketSend((const char*) bitStream.GetData(), bitStream.GetNumberOfBitsUsed(), rcs->systemAddress);

					if (rcs->socket.IsNull())
					{
						SocketLayer::SetDoNotFragment(socketList[rcs->socketIndex]->s, 1);
						RakNetTime sendToStart=RakNet::GetTime();
						if (SocketLayer::SendTo( socketList[rcs->socketIndex]->s, (const char*) bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), rcs->systemAddress.binaryAddress, rcs->systemAddress.port, socketList[rcs->socketIndex]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  )==-10040)
						{
							if (rcs->requestsMade==rcs->sendConnectionAttemptCount+1)
							{
								// every size is returning -10040
								rcs->requestsMade=rcs->sendConnectionAttemptCount+1;
							}
							else
							{
								// Don't use this MTU size again
								rcs->requestsMade = (unsigned char) ((MTUSizeIndex + 1) * (rcs->sendConnectionAttemptCount/NUM_MTU_SIZES));
								rcs->nextRequestTime=timeMS;
							}
						}
						else
						{
							RakNetTime sendToEnd=RakNet::GetTime();
							if (sendToEnd-sendToStart>100)
							{
								// Drop to lowest MTU
								int lowestMtuIndex = rcs->sendConnectionAttemptCount/NUM_MTU_SIZES * (NUM_MTU_SIZES - 1);
								if (lowestMtuIndex > rcs->requestsMade)
								{
									rcs->requestsMade = lowestMtuIndex;
									rcs->nextRequestTime=timeMS;
								}
								else
									rcs->requestsMade=rcs->sendConnectionAttemptCount+1;
							}
						}
						SocketLayer::SetDoNotFragment(socketList[rcs->socketIndex]->s, 0);
					}
					else
					{
						SocketLayer::SetDoNotFragment(rcs->socket->s, 1);
						RakNetTime sendToStart=RakNet::GetTime();
						if (SocketLayer::SendTo( rcs->socket->s, (const char*) bitStream.GetData(), bitStream.GetNumberOfBytesUsed(), rcs->systemAddress.binaryAddress, rcs->systemAddress.port, socketList[rcs->socketIndex]->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  )==-10040)
						{
							if (rcs->requestsMade==rcs->sendConnectionAttemptCount+1)
							{
								// every size is returning -10040
								rcs->requestsMade=rcs->sendConnectionAttemptCount+1;
							}
							else
							{
								// Don't use this MTU size again
								rcs->requestsMade = (unsigned char) ((MTUSizeIndex + 1) * (rcs->sendConnectionAttemptCount/NUM_MTU_SIZES));
								rcs->nextRequestTime=timeMS;
							}
						}
						else
						{
							RakNetTime sendToEnd=RakNet::GetTime();
							if (sendToEnd-sendToStart>100)
							{
								// Drop to lowest MTU
								int lowestMtuIndex = rcs->sendConnectionAttemptCount/NUM_MTU_SIZES * (NUM_MTU_SIZES - 1);
								if (lowestMtuIndex > rcs->requestsMade)
								{
									rcs->requestsMade = lowestMtuIndex;
									rcs->nextRequestTime=timeMS;
								}
								else
									rcs->requestsMade=rcs->sendConnectionAttemptCount+1;
							}
						}
						SocketLayer::SetDoNotFragment(socketList[rcs->socketIndex]->s, 0);
					}
				//	printf("ID_OPEN_CONNECTION_REQUEST\n");

					requestedConnectionQueueIndex++;
				}
			}
			else
				requestedConnectionQueueIndex++;

			requestedConnectionQueueMutex.Lock();
		}
		requestedConnectionQueueMutex.Unlock();
	}

	// remoteSystemList in network thread
	for ( remoteSystemIndex = 0; remoteSystemIndex < maximumNumberOfPeers; ++remoteSystemIndex )
	//for ( remoteSystemIndex = 0; remoteSystemIndex < remoteSystemListSize; ++remoteSystemIndex )
	{
		// I'm using systemAddress from remoteSystemList but am not locking it because this loop is called very frequently and it doesn't
		// matter if we miss or do an extra update.  The reliability layers themselves never care which player they are associated with
		//systemAddress = remoteSystemList[ remoteSystemIndex ].systemAddress;
		// Allow the systemAddress for this remote system list to change.  We don't care if it changes now.
	//	remoteSystemList[ remoteSystemIndex ].allowSystemAddressAssigment=true;
		if ( remoteSystemList[ remoteSystemIndex ].isActive )
		{
			systemAddress = remoteSystemList[ remoteSystemIndex ].systemAddress;
			RakAssert(systemAddress!=UNASSIGNED_SYSTEM_ADDRESS);

			// Found an active remote system
			remoteSystem = remoteSystemList + remoteSystemIndex;
			// Update is only safe to call from the same thread that calls HandleSocketReceiveFromConnectedPlayer,
			// which is this thread

			if (timeNS==0)
			{
				timeNS = RakNet::GetTimeNS();
				timeMS = (RakNetTime)(timeNS/(RakNetTimeUS)1000);
				//RAKNET_DEBUG_PRINTF("timeNS = %I64i timeMS=%i\n", timeNS, timeMS);
			}


			if (timeMS > remoteSystem->lastReliableSend && timeMS-remoteSystem->lastReliableSend > defaultTimeoutTime/2 && remoteSystem->connectMode==RemoteSystemStruct::CONNECTED)
			{
				// If no reliable packets are waiting for an ack, do a one byte reliable send so that disconnections are noticed
				RakNetStatistics rakNetStatistics;
				rnss=remoteSystem->reliabilityLayer.GetStatistics(&rakNetStatistics);
				if (rnss->messagesInResendBuffer==0)
				{
					PingInternal( systemAddress, true, RELIABLE );
					/*
					unsigned char keepAlive=ID_DETECT_LOST_CONNECTIONS;
					SendImmediate((char*)&keepAlive,8,LOW_PRIORITY, RELIABLE, 0, remoteSystem->systemAddress, false, false, timeNS);
					*/
					//remoteSystem->lastReliableSend=timeMS+remoteSystem->reliabilityLayer.GetTimeoutTime();
					remoteSystem->lastReliableSend=timeMS;
				}
			}

			remoteSystem->reliabilityLayer.Update( remoteSystem->rakNetSocket->s, systemAddress, remoteSystem->MTUSize, timeNS, maxOutgoingBPS, messageHandlerList, &rnr, remoteSystem->rakNetSocket->remotePortRakNetWasStartedOn_PS3 ); // systemAddress only used for the internet simulator test

			// Check for failure conditions
			if ( remoteSystem->reliabilityLayer.IsDeadConnection() ||
				((remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP || remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY) && remoteSystem->reliabilityLayer.IsOutgoingDataWaiting()==false) ||
				(remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ON_NO_ACK && (remoteSystem->reliabilityLayer.AreAcksWaiting()==false || remoteSystem->reliabilityLayer.AckTimeout(timeMS)==true)) ||
				((
				(remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
				remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
				remoteSystem->connectMode==RemoteSystemStruct::UNVERIFIED_SENDER ||
				remoteSystem->connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET)
				&& timeMS > remoteSystem->connectionTime && timeMS - remoteSystem->connectionTime > 10000))
				)
			{
			//	RAKNET_DEBUG_PRINTF("timeMS=%i remoteSystem->connectionTime=%i\n", timeMS, remoteSystem->connectionTime );

				// Failed.  Inform the user?
				// TODO - RakNet 4.0 - Return a different message identifier for DISCONNECT_ASAP_SILENTLY and DISCONNECT_ASAP than for DISCONNECT_ON_NO_ACK
				// The first two mean we called CloseConnection(), the last means the other system sent us ID_DISCONNECTION_NOTIFICATION
				if (remoteSystem->connectMode==RemoteSystemStruct::CONNECTED || remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION
					|| remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY || remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP || remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ON_NO_ACK)
				{

//					RakNet::BitStream undeliveredMessages;
//					remoteSystem->reliabilityLayer.GetUndeliveredMessages(&undeliveredMessages,remoteSystem->MTUSize);

//					packet=AllocPacket(sizeof( char ) + undeliveredMessages.GetNumberOfBytesUsed());
					packet=AllocPacket(sizeof( char ), __FILE__, __LINE__);
					if (remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION)
						packet->data[ 0 ] = ID_CONNECTION_ATTEMPT_FAILED; // Attempted a connection and couldn't
					else if (remoteSystem->connectMode==RemoteSystemStruct::CONNECTED)
						packet->data[ 0 ] = ID_CONNECTION_LOST; // DeadConnection
					else
						packet->data[ 0 ] = ID_DISCONNECTION_NOTIFICATION; // DeadConnection

//					memcpy(packet->data+1, undeliveredMessages.GetData(), undeliveredMessages.GetNumberOfBytesUsed());

					packet->guid = remoteSystem->guid;
					packet->systemAddress = systemAddress;
					packet->systemAddress.systemIndex = ( SystemIndex ) remoteSystemIndex;
					packet->guid.systemIndex=packet->systemAddress.systemIndex;

					AddPacketToProducer(packet);
				}
				// else connection shutting down, don't bother telling the user

#ifdef _DO_PRINTF
				RAKNET_DEBUG_PRINTF("Connection dropped for player %i:%i\n", systemAddress.binaryAddress, systemAddress.port);
#endif
				CloseConnectionInternal( systemAddress, false, true, 0, LOW_PRIORITY );
				continue;
			}

			// Did the reliability layer detect a modified packet?
			if ( remoteSystem->reliabilityLayer.IsCheater() )
			{
				packet=AllocPacket(sizeof(char), __FILE__, __LINE__);
				packet->bitSize=8;
				packet->data[ 0 ] = (MessageID)ID_MODIFIED_PACKET;
				packet->systemAddress = systemAddress;
				packet->systemAddress.systemIndex = ( SystemIndex ) remoteSystemIndex;
				packet->guid = remoteSystem->guid;
				packet->guid.systemIndex=packet->systemAddress.systemIndex;
				AddPacketToProducer(packet);
				continue;
			}

			// Ping this guy if it is time to do so
			if ( remoteSystem->connectMode==RemoteSystemStruct::CONNECTED && timeMS > remoteSystem->nextPingTime && ( occasionalPing || remoteSystem->lowestPing == (unsigned short)-1 ) )
			{
				remoteSystem->nextPingTime = timeMS + 5000;
				PingInternal( systemAddress, true, UNRELIABLE );

				// Update again immediately after this tick so the ping goes out right away
				quitAndDataEvents.SetEvent();
			}

			// Find whoever has the lowest player ID
			//if (systemAddress < authoritativeClientSystemAddress)
			// authoritativeClientSystemAddress=systemAddress;

			// Does the reliability layer have any packets waiting for us?
			// To be thread safe, this has to be called in the same thread as HandleSocketReceiveFromConnectedPlayer
			bitSize = remoteSystem->reliabilityLayer.Receive( &data );

			while ( bitSize > 0 )
			{
				// These types are for internal use and should never arrive from a network packet
				if (data[0]==ID_CONNECTION_ATTEMPT_FAILED || data[0]==ID_MODIFIED_PACKET)
				{
					RakAssert(0);
					bitSize=0;
					continue;
				}

				// Put the input through compression if necessary
				if ( inputTree )
				{
					RakNet::BitStream dataBitStream( MAXIMUM_MTU_SIZE );
					// Since we are decompressing input, we need to copy to a bitstream, decompress, then copy back to a probably
					// larger data block.  It's slow, but the user should have known that anyway
					dataBitStream.Reset();
					dataBitStream.WriteAlignedBytes( ( unsigned char* ) data, BITS_TO_BYTES( bitSize ) );
					rawBytesReceived += (unsigned int) dataBitStream.GetNumberOfBytesUsed();

//					numberOfBytesUsed = dataBitStream.GetNumberOfBytesUsed();
					numberOfBitsUsed = dataBitStream.GetNumberOfBitsUsed();
					//rawBytesReceived += numberOfBytesUsed;
					// Decompress the input data.

					if (numberOfBitsUsed>0)
					{
						unsigned char *dataCopy = (unsigned char*) rakMalloc_Ex( (unsigned int) dataBitStream.GetNumberOfBytesUsed(), __FILE__, __LINE__ );
						memcpy( dataCopy, dataBitStream.GetData(), (size_t) dataBitStream.GetNumberOfBytesUsed() );
						dataBitStream.Reset();
						inputTree->DecodeArray( dataCopy, numberOfBitsUsed, &dataBitStream );
						compressedBytesReceived += (unsigned int) dataBitStream.GetNumberOfBytesUsed();
						rakFree_Ex(dataCopy, __FILE__, __LINE__ );

						byteSize = (unsigned int) dataBitStream.GetNumberOfBytesUsed();

						if ( byteSize > BITS_TO_BYTES( bitSize ) )   // Probably the case - otherwise why decompress?
						{
							rakFree_Ex(data, __FILE__, __LINE__ );
							data = (unsigned char*) rakMalloc_Ex( (size_t) byteSize, __FILE__, __LINE__ );
						}
						bitSize = (BitSize_t) dataBitStream.GetNumberOfBitsUsed();
						memcpy( data, dataBitStream.GetData(), byteSize );
					}
					else
						byteSize=0;
				}
				else
					// Fast and easy - just use the data that was returned
					byteSize = (unsigned int) BITS_TO_BYTES( bitSize );

				// For unknown senders we only accept a few specific packets
				if (remoteSystem->connectMode==RemoteSystemStruct::UNVERIFIED_SENDER)
				{
					if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST )
					{
						ParseConnectionRequestPacket(remoteSystem, systemAddress, (const char*)data, byteSize);
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else
					{
						CloseConnectionInternal( systemAddress, false, true, 0, LOW_PRIORITY );
#ifdef _DO_PRINTF
						RAKNET_DEBUG_PRINTF("Temporarily banning %i:%i for sending nonsense data\n", systemAddress.binaryAddress, systemAddress.port);
#endif

						char str1[64];
						systemAddress.ToString(false, str1);
						AddToBanList(str1, remoteSystem->reliabilityLayer.GetTimeoutTime());


						rakFree_Ex(data, __FILE__, __LINE__ );
					}
				}
				else
				{
					// However, if we are connected we still take a connection request in case both systems are trying to connect to each other
					// at the same time
					if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST )
					{
						// 04/27/06 This is wrong.  With cross connections, we can both have initiated the connection are in state REQUESTED_CONNECTION
						// 04/28/06 Downgrading connections from connected will close the connection due to security at ((remoteSystem->connectMode!=RemoteSystemStruct::CONNECTED && time > remoteSystem->connectionTime && time - remoteSystem->connectionTime > 10000))
						if (remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION)
						{
							ParseConnectionRequestPacket(remoteSystem, systemAddress, (const char*)data, byteSize);
						}
						else
						{

							RakNet::BitStream bs((unsigned char*) data,byteSize,false);
							bs.IgnoreBytes(sizeof(MessageID));
							bs.IgnoreBytes(sizeof(OFFLINE_MESSAGE_DATA_ID));
							bs.IgnoreBytes(RakNetGUID::size());
							RakNetTime incomingTimestamp;
							bs.Read(incomingTimestamp);

							// Got a connection request message from someone we are already connected to. Just reply normally.
							// This can happen due to race conditions with the fully connected mesh
							SendConnectionRequestAccepted(remoteSystem, incomingTimestamp);
						}
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else if ( (unsigned char) data[ 0 ] == ID_NEW_INCOMING_CONNECTION && byteSize > sizeof(unsigned char)+sizeof(unsigned int)+sizeof(unsigned short)+sizeof(RakNetTime)*2 )
					{

						if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
							// WHy was this here? In CrossConnectinoTest it resulted in returning both ID_NEW_INCOMING_CONNECTION and ID_CONNECTION_REQUEST_ACCEPTED
				//			remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
							remoteSystem->connectMode==RemoteSystemStruct::SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET)
						{
							// Removeme
//							static int count5=1;
//							printf("Processed ID_NEW_INCOMING_CONNECTION count=%i\n", count5++);

							remoteSystem->connectMode=RemoteSystemStruct::CONNECTED;
							PingInternal( systemAddress, true, UNRELIABLE );

							// Update again immediately after this tick so the ping goes out right away
							quitAndDataEvents.SetEvent();

							RakNet::BitStream inBitStream((unsigned char *) data, byteSize, false);
							SystemAddress bsSystemAddress;

							inBitStream.IgnoreBits(8);
							inBitStream.Read(bsSystemAddress);
							for (unsigned int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
								inBitStream.Read(remoteSystem->theirInternalSystemAddress[i]);

							RakNetTime sendPingTime, sendPongTime;
							inBitStream.Read(sendPingTime);
							inBitStream.Read(sendPongTime);
							OnConnectedPong(sendPingTime,sendPongTime,remoteSystem);

							// Overwrite the data in the packet
							//					NewIncomingConnectionStruct newIncomingConnectionStruct;
							//					RakNet::BitStream nICS_BS( data, NewIncomingConnectionStruct_Size, false );
							//					newIncomingConnectionStruct.Deserialize( nICS_BS );

							remoteSystem->myExternalSystemAddress = bsSystemAddress;
							firstExternalID=bsSystemAddress;

							// Send this info down to the game
							packet=AllocPacket(byteSize, data, __FILE__, __LINE__);
							packet->bitSize = bitSize;
							packet->systemAddress = systemAddress;
							packet->systemAddress.systemIndex = ( SystemIndex ) remoteSystemIndex;
							packet->guid = remoteSystem->guid;
							packet->guid.systemIndex=packet->systemAddress.systemIndex;
							AddPacketToProducer(packet);
						}
					}
					else if ( (unsigned char) data[ 0 ] == ID_CONNECTED_PONG && byteSize == sizeof(unsigned char)+sizeof(RakNetTime)*2 )
					{
						RakNetTime sendPingTime, sendPongTime;

						// Copy into the ping times array the current time - the value returned
						// First extract the sent ping
						RakNet::BitStream inBitStream( (unsigned char *) data, byteSize, false );
						//PingStruct ps;
						//ps.Deserialize(psBS);
						inBitStream.IgnoreBits(8);
						inBitStream.Read(sendPingTime);
						inBitStream.Read(sendPongTime);

						OnConnectedPong(sendPingTime,sendPongTime,remoteSystem);

						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else if ( (unsigned char)data[0] == ID_INTERNAL_PING && byteSize == sizeof(unsigned char)+sizeof(RakNetTime) )
					{
						RakNet::BitStream inBitStream( (unsigned char *) data, byteSize, false );
 						inBitStream.IgnoreBits(8);
						RakNetTime sendPingTime;
						inBitStream.Read(sendPingTime);

						RakNet::BitStream outBitStream;
						outBitStream.Write((MessageID)ID_CONNECTED_PONG);
						outBitStream.Write(sendPingTime);
						timeMS = RakNet::GetTime();
						timeNS = RakNet::GetTimeNS();
						outBitStream.Write(timeMS);
						SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, UNRELIABLE, 0, systemAddress, false, false, timeNS, 0 );

						// Update again immediately after this tick so the ping goes out right away
						quitAndDataEvents.SetEvent();

						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else if ( (unsigned char) data[ 0 ] == ID_DISCONNECTION_NOTIFICATION )
					{
						// We shouldn't close the connection immediately because we need to ack the ID_DISCONNECTION_NOTIFICATION
						remoteSystem->connectMode=RemoteSystemStruct::DISCONNECT_ON_NO_ACK;
						rakFree_Ex(data, __FILE__, __LINE__ );

					//	AddPacketToProducer(packet);
					}
					else if ( (unsigned char) data[ 0 ] == ID_RPC_MAPPING )
					{
						RakNet::BitStream inBitStream( (unsigned char *) data, byteSize, false );
						RPCIndex index;
						char output[256];
						inBitStream.IgnoreBits(8);
						stringCompressor->DecodeString(output, 255, &inBitStream);
						inBitStream.ReadCompressed(index);
                        remoteSystem->rpcMap.AddIdentifierAtIndex((char*)output,index);
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
#if   !defined(_WIN32_WCE) 
					else if ( (unsigned char)(data)[0] == ID_SECURED_CONNECTION_RESPONSE &&
						byteSize == 1 + sizeof( uint32_t ) + sizeof( uint32_t ) * RAKNET_RSA_FACTOR_LIMBS + 20 )
					{
						SecuredConnectionConfirmation( remoteSystem, (char*)data );
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else if ( (unsigned char)(data)[0] == ID_SECURED_CONNECTION_CONFIRMATION &&
						byteSize == 1 + 20 + sizeof( uint32_t ) * RAKNET_RSA_FACTOR_LIMBS + sizeof(RakNetTime) )
					{
						CSHA1 sha1;
						bool confirmedHash;
						//bool newRandNumber;

						confirmedHash = false;

						// Hash the SYN-Cookie
						// s2c syn-cookie = SHA1_HASH(source ip address + source port + random number)
						sha1.Reset();
						sha1.Update( ( unsigned char* ) & systemAddress.binaryAddress, sizeof( systemAddress.binaryAddress ) );
						sha1.Update( ( unsigned char* ) & systemAddress.port, sizeof( systemAddress.port ) );
						sha1.Update( ( unsigned char* ) & ( newRandomNumber ), 20 );
						sha1.Final();

						//newRandNumber = false;

						// Confirm if
						//syn-cookie ?= HASH(source ip address + source port + last random number)
						//syn-cookie ?= HASH(source ip address + source port + current random number)
						if ( memcmp( sha1.GetHash(), data + 1, 20 ) == 0 )
						{
							confirmedHash = true;
						}
						else
						{
							sha1.Reset();
							sha1.Update( ( unsigned char* ) & systemAddress.binaryAddress, sizeof( systemAddress.binaryAddress ) );
							sha1.Update( ( unsigned char* ) & systemAddress.port, sizeof( systemAddress.port ) );
							sha1.Update( ( unsigned char* ) & ( oldRandomNumber ), 20 );
							sha1.Final();

							if ( memcmp( sha1.GetHash(), data + 1, 20 ) == 0 )
								confirmedHash = true;
						}
						if ( confirmedHash )
						{
							int i;
							unsigned char AESKey[ 16 ];
							//RSA_BIT_SIZE message, encryptedMessage;
							uint32_t message[RAKNET_RSA_FACTOR_LIMBS], encryptedMessage[RAKNET_RSA_FACTOR_LIMBS];

							// On connection accept, AES key is c2s RSA_Decrypt(random number) XOR s2c syn-cookie
							// Get the random number first

							memcpy( encryptedMessage, data + 1 + 20, sizeof( encryptedMessage ) );

					//		printf("enc3[0]=%i,%i\n", encryptedMessage[0], encryptedMessage[19]);

							if (RakNet::BitStream::DoEndianSwap())
							{
								for (int i=0; i < RAKNET_RSA_FACTOR_LIMBS; i++)
									RakNet::BitStream::ReverseBytesInPlace((unsigned char*) &encryptedMessage[i], sizeof(encryptedMessage[i]));
							}

				//			printf("enc4[0]=%i,%i\n", encryptedMessage[0], encryptedMessage[19]);

							// rsacrypt.decrypt( encryptedMessage, message );
							rsacrypt.decrypt( message, encryptedMessage );

						//	printf("message[0]=%i,%i\n", message[0], message[19]);



//							if (RakNet::BitStream::DoEndianSwap())
//							{
								// The entire message is endian swapped, then just the random number
//								unsigned char randomNumber[ 20 ];
//								if (RakNet::BitStream::DoEndianSwap())
//								{
//									for (int i=0; i < 20; i++)
//										RakNet::BitStream::ReverseBytesInPlace((unsigned char*) message[i], sizeof(message[i]));
//								}
//							}

							/*
							// On connection accept, AES key is c2s RSA_Decrypt(random number) XOR s2c syn-cookie
							// Get the random number first
							#ifdef HOST_ENDIAN_IS_BIG
								BSWAPCPY( (unsigned char *) encryptedMessage, (unsigned char *)(data + 1 + 20), sizeof( RSA_BIT_SIZE ) );
							#else
								memcpy( encryptedMessage, data + 1 + 20, sizeof( RSA_BIT_SIZE ) );
							#endif
							rsacrypt.decrypt( encryptedMessage, message );
							#ifdef HOST_ENDIAN_IS_BIG
								BSWAPSELF( (unsigned char *) message, sizeof( RSA_BIT_SIZE ) );
							#endif
							*/

							// Save the AES key
							for ( i = 0; i < 16; i++ )
								AESKey[ i ] = data[ 1 + i ] ^ ( ( unsigned char* ) ( message ) ) [ i ];

							RakNet::BitStream bsTimestamp(data+1 + 20 + sizeof( uint32_t ) * RAKNET_RSA_FACTOR_LIMBS,sizeof(RakNetTime),false);
							RakNetTime timeStamp;
							bsTimestamp.Read(timeStamp);

							// Connect this player assuming we have open slots
							OnConnectionRequest( remoteSystem, AESKey, true, timeStamp );
						}
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
#endif // #if !defined(_XBOX) && !defined(_WIN32_WCE)
					else if ( (unsigned char)(data)[0] == ID_DETECT_LOST_CONNECTIONS && byteSize == sizeof(unsigned char) )
					{
						// Do nothing
						rakFree_Ex(data, __FILE__, __LINE__ );
					}
					else if ( (unsigned char)(data)[0] == ID_CONNECTION_REQUEST_ACCEPTED )
					{
						if (byteSize > sizeof(MessageID)+sizeof(unsigned int)+sizeof(unsigned short)+sizeof(SystemIndex)+sizeof(RakNetTime)*2)
						{
							// Make sure this connection accept is from someone we wanted to connect to
							bool allowConnection, alreadyConnected;

							if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST ||
								remoteSystem->connectMode==RemoteSystemStruct::REQUESTED_CONNECTION ||
								allowConnectionResponseIPMigration)
								allowConnection=true;
							else
								allowConnection=false;

							if (remoteSystem->connectMode==RemoteSystemStruct::HANDLING_CONNECTION_REQUEST)
								alreadyConnected=true;
							else
								alreadyConnected=false;

							if ( allowConnection )
							{
								SystemAddress externalID;
								SystemIndex systemIndex;
//								SystemAddress internalID;

								RakNet::BitStream inBitStream((unsigned char *) data, byteSize, false);
								inBitStream.IgnoreBits(8); // ID_CONNECTION_REQUEST_ACCEPTED
								//	inBitStream.Read(remotePort);
								inBitStream.Read(externalID);
								inBitStream.Read(systemIndex);
								for (unsigned int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
									inBitStream.Read(remoteSystem->theirInternalSystemAddress[i]);
							
								RakNetTime sendPingTime, sendPongTime;
								inBitStream.Read(sendPingTime);
								inBitStream.Read(sendPongTime);
								OnConnectedPong(sendPingTime, sendPongTime, remoteSystem);

								// Find a free remote system struct to use
								//						RakNet::BitStream casBitS(data, byteSize, false);
								//						ConnectionAcceptStruct cas;
								//						cas.Deserialize(casBitS);
								//	systemAddress.port = remotePort;

								// The remote system told us our external IP, so save it
								remoteSystem->myExternalSystemAddress = externalID;
								remoteSystem->connectMode=RemoteSystemStruct::CONNECTED;

								firstExternalID=externalID;

								if (alreadyConnected==false)
								{
									// Use the stored encryption key
									if (remoteSystem->setAESKey)
										remoteSystem->reliabilityLayer.SetEncryptionKey( remoteSystem->AESKey );
									else
										remoteSystem->reliabilityLayer.SetEncryptionKey( 0 );
								}

								// Send the connection request complete to the game
								packet=AllocPacket(byteSize, data, __FILE__, __LINE__);
								packet->bitSize = byteSize * 8;
								packet->systemAddress = systemAddress;
								packet->systemAddress.systemIndex = ( SystemIndex ) GetIndexFromSystemAddress( systemAddress, true );
								packet->guid = remoteSystem->guid;
								packet->guid.systemIndex=packet->systemAddress.systemIndex;
								AddPacketToProducer(packet);


								// Removeme
//								static int count3=1;
//								printf("Send ID_NEW_INCOMING_CONNECTION count=%i\n", count3++);

								RakNet::BitStream outBitStream;
								outBitStream.Write((MessageID)ID_NEW_INCOMING_CONNECTION);
								outBitStream.Write(systemAddress);
								for (unsigned int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
									outBitStream.Write(mySystemAddress[i]);
								outBitStream.Write(sendPongTime);
								outBitStream.Write(RakNet::GetTime());
								

								// We turned on encryption with SetEncryptionKey.  This pads packets to up to a multiple of 16 bytes.
								// As soon as a multiple of 16 byte packet arrives on the remote system, we will turn on AES.  This works because all encrypted packets are multiples of 16 and the
								// packets I happen to be sending before this are not a multiple of 16 bytes.  Otherwise there is no way to know if a packet that arrived is
								// encrypted or not so the other side won't know to turn on encryption or not.
								RakAssert((outBitStream.GetNumberOfBytesUsed()&15)!=0);
								SendImmediate( (char*)outBitStream.GetData(), outBitStream.GetNumberOfBitsUsed(), IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, false, false, RakNet::GetTimeNS(), 0 );

								if (alreadyConnected==false)
								{
									PingInternal( systemAddress, true, UNRELIABLE );
								}
							}
							else
							{
								// Ignore, already connected
								rakFree_Ex(data, __FILE__, __LINE__ );
							}
						}
						else
						{
							// Version mismatch error?
							RakAssert(0);
							rakFree_Ex(data, __FILE__, __LINE__ );
						}
					}
					else
					{
						// What do I do if I get a message from a system, before I am fully connected?
						// I can either ignore it or give it to the user
						// It seems like giving it to the user is a better option
						if (data[0]>=(MessageID)ID_RPC &&
							remoteSystem->isActive
							/*
							(remoteSystem->connectMode==RemoteSystemStruct::CONNECTED ||
							remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP ||
							remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ASAP_SILENTLY ||
							remoteSystem->connectMode==RemoteSystemStruct::DISCONNECT_ON_NO_ACK)
							*/
							)
						{
							packet=AllocPacket(byteSize, data, __FILE__, __LINE__);
							packet->bitSize = bitSize;
							packet->systemAddress = systemAddress;
							packet->systemAddress.systemIndex = ( SystemIndex ) remoteSystemIndex;
							packet->guid = remoteSystem->guid;
							packet->guid.systemIndex=packet->systemAddress.systemIndex;
							AddPacketToProducer(packet);
						}
						else
						{
							rakFree_Ex(data, __FILE__, __LINE__ );
						}
					}
				}

				// Does the reliability layer have any more packets waiting for us?
				// To be thread safe, this has to be called in the same thread as HandleSocketReceiveFromConnectedPlayer
				bitSize = remoteSystem->reliabilityLayer.Receive( &data );
			}
		}
	}

	return true;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RAK_THREAD_DECLARATION(RecvFromLoop)
{
	RakPeerAndIndex *rpai = ( RakPeerAndIndex * ) arguments;
	RakPeer * rakPeer = rpai->rakPeer;
	SOCKET s = rpai->s;
	unsigned short remotePortRakNetWasStartedOn_PS3 = rpai->remotePortRakNetWasStartedOn_PS3;

	rakPeer->isRecvFromLoopThreadActive = true;
	
	RakPeer::RecvFromStruct *recvFromStruct;
	while ( rakPeer->endThreads == false )
	{
		recvFromStruct=rakPeer->bufferedPackets.Allocate( __FILE__, __LINE__ );
		recvFromStruct->s=s;
		recvFromStruct->remotePortRakNetWasStartedOn_PS3=remotePortRakNetWasStartedOn_PS3;
		SocketLayer::RecvFromBlocking(s, rakPeer, remotePortRakNetWasStartedOn_PS3, recvFromStruct->data, &recvFromStruct->bytesRead, &recvFromStruct->systemAddress, &recvFromStruct->timeRead);
		if (recvFromStruct->bytesRead>0)
		{
			RakAssert(recvFromStruct->systemAddress.port);
			rakPeer->bufferedPackets.Push(recvFromStruct);

			rakPeer->quitAndDataEvents.SetEvent();
		}
		else
		{
			rakPeer->bufferedPackets.Deallocate(recvFromStruct, __FILE__,__LINE__);
		}
	}
	rakPeer->isRecvFromLoopThreadActive = false;
	return 0;
}
// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
RAK_THREAD_DECLARATION(UpdateNetworkLoop)
{
	RakPeer * rakPeer = ( RakPeer * ) arguments;

/*
	// 11/15/05 - this is slower than Sleep()
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	// Lets see if these timers give better performance than Sleep
	HANDLE timerHandle;
	LARGE_INTEGER dueTime;

	if ( rakPeer->threadSleepTimer <= 0 )
		rakPeer->threadSleepTimer = 1;

	// 2nd parameter of false means synchronization timer instead of manual-reset timer
	timerHandle = CreateWaitableTimer( NULL, FALSE, 0 );

	RakAssert( timerHandle );

	dueTime.QuadPart = -10000 * rakPeer->threadSleepTimer; // 10000 is 1 ms?

	BOOL success = SetWaitableTimer( timerHandle, &dueTime, rakPeer->threadSleepTimer, NULL, NULL, FALSE );
    (void) success;
	RakAssert( success );

#endif
#endif
*/

	RakNetTimeUS timeNS;
	RakNetTimeMS timeMS;

	rakPeer->isMainLoopThreadActive = true;

// #ifdef _DEBUG
// 	RakNetTime lastCall=RakNet::GetTime();
// #endif
	while ( rakPeer->endThreads == false )
	{
		// Set inside RunUpdateCycle() itself, this is here for testing
		timeNS=0;
		timeMS=0;

// #ifdef _DEBUG
// 		// Sanity check, make sure RunUpdateCycle does not block or not otherwise get called for a long time
// 		RakNetTime thisCall=RakNet::GetTime();
// 		RakAssert(thisCall-lastCall<250);
// 		lastCall=thisCall;
// #endif
		if (rakPeer->userUpdateThreadPtr)
			rakPeer->userUpdateThreadPtr(rakPeer, rakPeer->userUpdateThreadData);

		rakPeer->RunUpdateCycle(timeNS, timeMS);

		// Pending sends go out this often, unless quitAndDataEvents is set
		rakPeer->quitAndDataEvents.WaitOnEvent(10);

		/*

// #if ((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)) &&
#if defined(USE_WAIT_FOR_MULTIPLE_EVENTS) && defined(_WIN32)

		if (rakPeer->threadSleepTimer>0)
		{
			WSAEVENT eventArray[256];
			unsigned int i, eventArrayIndex;
			for (i=0,eventArrayIndex=0; i < rakPeer->socketList.Size(); i++)
			{
				if (rakPeer->socketList[i]->recvEvent!=INVALID_HANDLE_VALUE)
				{
					eventArray[eventArrayIndex]=rakPeer->socketList[i]->recvEvent;
					eventArrayIndex++;
					if (eventArrayIndex==256)
						break;
				}
			}
			WSAWaitForMultipleEvents(eventArrayIndex,(const HANDLE*) &eventArray,FALSE,rakPeer->threadSleepTimer,FALSE);
		}
		else
		{
			RakSleep(0);
		}

#else // ((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)) && defined(USE_WAIT_FOR_MULTIPLE_EVENTS)
		#pragma message("-- RakNet: Using Sleep(). Uncomment USE_WAIT_FOR_MULTIPLE_EVENTS in RakNetDefines.h if you want to use WaitForSingleObject instead. --")

		RakSleep( rakPeer->threadSleepTimer );
#endif
		*/
	}

	rakPeer->isMainLoopThreadActive = false;

	/*
#ifdef _WIN32
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	CloseHandle(timerHandle);
#endif
#endif
	*/

	return 0;
}

#if defined(RMO_NEW_UNDEF_ALLOCATING_QUEUE)
#pragma pop_macro("new")
#undef RMO_NEW_UNDEF_ALLOCATING_QUEUE
#endif


#ifdef _MSC_VER
#pragma warning( pop )
#endif
