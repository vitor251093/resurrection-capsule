#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_TCPInterface==1

/// \file
/// \brief A simple TCP based server allowing sends and receives.  Can be connected to by a telnet client.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "TCPInterface.h"
#ifdef _WIN32
typedef int socklen_t;
#else
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#endif
#include <string.h>
#include "RakAssert.h"
#include <stdio.h>
#include "RakAssert.h"
#include "RakSleep.h"
#include "StringCompressor.h"
#include "StringTable.h"

#ifdef _DO_PRINTF
#endif

#ifdef _WIN32
#include "WSAStartupSingleton.h"
#endif

RAK_THREAD_DECLARATION(UpdateTCPInterfaceLoop);
RAK_THREAD_DECLARATION(ConnectionAttemptLoop);

#ifdef _MSC_VER
#pragma warning( push )
#endif

TCPInterface::TCPInterface()
{
	isStarted=false;
	threadRunning=false;
	listenSocket=(SOCKET) -1;
	remoteClients=0;
	remoteClientsLength=0;

	StringCompressor::AddReference();
	RakNet::StringTable::AddReference();

#if OPEN_SSL_CLIENT_SUPPORT==1
	ctx=0;
	meth=0;
#endif

#ifdef _WIN32
	WSAStartupSingleton::AddRef();
#endif
}
TCPInterface::~TCPInterface()
{
	Stop();
#ifdef _WIN32
	WSAStartupSingleton::Deref();
#endif

	RakNet::OP_DELETE_ARRAY(remoteClients,__FILE__,__LINE__);

	StringCompressor::RemoveReference();
	RakNet::StringTable::RemoveReference();
}
bool TCPInterface::Start(unsigned short port, unsigned short maxIncomingConnections, unsigned short maxConnections, int _threadPriority)
{
	if (isStarted)
		return false;

	threadPriority=_threadPriority;

	if (threadPriority==-99999)
	{


#if   defined(_WIN32)
		threadPriority=0;


#else
		threadPriority=1000;
#endif
	}

	isStarted=true;
	if (maxConnections==0)
		maxConnections=maxIncomingConnections;
	if (maxConnections==0)
		maxConnections=1;
	remoteClientsLength=maxConnections;
	remoteClients=RakNet::OP_NEW_ARRAY<RemoteClient>(maxConnections,__FILE__,__LINE__);

	if (maxIncomingConnections>0)
	{
		listenSocket = socket(AF_INET, SOCK_STREAM, 0);
		if ((int)listenSocket ==-1)
			return false;

		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddress.sin_port = htons(port);

		if (bind(listenSocket,(struct sockaddr *) &serverAddress,sizeof(serverAddress)) < 0)
			return false;

		listen(listenSocket, maxIncomingConnections);
	}

	// Start the update thread
	int errorCode = RakNet::RakThread::Create(UpdateTCPInterfaceLoop, this, threadPriority);
	if (errorCode!=0)
		return false;

	while (threadRunning==false)
		RakSleep(0);

	return true;
}
void TCPInterface::Stop(void)
{
	if (isStarted==false)
		return;

	unsigned i;
#if OPEN_SSL_CLIENT_SUPPORT==1
	for (i=0; i < remoteClientsLength; i++)
		remoteClients[i].DisconnectSSL();
#endif

	isStarted=false;

	if (listenSocket!=(SOCKET) -1)
	{
#ifdef _WIN32
		shutdown(listenSocket, SD_BOTH);
#else		
		shutdown(listenSocket, SHUT_RDWR);
#endif
		closesocket(listenSocket);
		listenSocket=(SOCKET) -1;
	}

	// Abort waiting connect calls
	blockingSocketListMutex.Lock();
	for (i=0; i < blockingSocketList.Size(); i++)
	{
		closesocket(blockingSocketList[i]);
	}
	blockingSocketListMutex.Unlock();

	// Wait for the thread to stop
	while ( threadRunning )
		RakSleep(15);

	RakSleep(100);

	// Stuff from here on to the end of the function is not threadsafe
	for (i=0; i < (unsigned int) remoteClientsLength; i++)
	{
		closesocket(remoteClients[i].socket);
#if OPEN_SSL_CLIENT_SUPPORT==1
		remoteClients[i].FreeSSL();
#endif
	}
	remoteClientsLength=0;
	RakNet::OP_DELETE_ARRAY(remoteClients,__FILE__,__LINE__);
	remoteClients=0;

	incomingMessages.Clear(__FILE__, __LINE__);
	newIncomingConnections.Clear(__FILE__, __LINE__);
	newRemoteClients.Clear(__FILE__, __LINE__);
	lostConnections.Clear(__FILE__, __LINE__);
	requestedCloseConnections.Clear(__FILE__, __LINE__);
	failedConnectionAttempts.Clear(__FILE__, __LINE__);
	completedConnectionAttempts.Clear(__FILE__, __LINE__);
	failedConnectionAttempts.Clear(__FILE__, __LINE__);
	for (i=0; i < headPush.Size(); i++)
		DeallocatePacket(headPush[i]);
	headPush.Clear(__FILE__, __LINE__);
	for (i=0; i < tailPush.Size(); i++)
		DeallocatePacket(tailPush[i]);
	tailPush.Clear(__FILE__, __LINE__);

#if OPEN_SSL_CLIENT_SUPPORT==1
	SSL_CTX_free (ctx);
	startSSL.Clear(__FILE__, __LINE__);
	activeSSLConnections.Clear(false, __FILE__, __LINE__);
#endif
}
SystemAddress TCPInterface::Connect(const char* host, unsigned short remotePort, bool block)
{
	if (threadRunning==false)
		return UNASSIGNED_SYSTEM_ADDRESS;

	int newRemoteClientIndex=-1;
	for (newRemoteClientIndex=0; newRemoteClientIndex < remoteClientsLength; newRemoteClientIndex++)
	{
		remoteClients[newRemoteClientIndex].isActiveMutex.Lock();
		if (remoteClients[newRemoteClientIndex].isActive==false)
		{
			remoteClients[newRemoteClientIndex].SetActive(true);
			remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();
			break;
		}
		remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();
	}
	if (newRemoteClientIndex==-1)
		return UNASSIGNED_SYSTEM_ADDRESS;

	if (block)
	{
		SystemAddress systemAddress;
		systemAddress.binaryAddress=inet_addr(host);
		systemAddress.port=remotePort;
		systemAddress.systemIndex=(SystemIndex) newRemoteClientIndex;

		SOCKET sockfd = SocketConnect(host, remotePort);
		if (sockfd==(SOCKET)-1)
		{
			remoteClients[newRemoteClientIndex].isActiveMutex.Lock();
			remoteClients[newRemoteClientIndex].SetActive(false);
			remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();

			failedConnectionAttemptMutex.Lock();
			failedConnectionAttempts.Push(systemAddress, __FILE__, __LINE__ );
			failedConnectionAttemptMutex.Unlock();

			return UNASSIGNED_SYSTEM_ADDRESS;
		}

		remoteClients[newRemoteClientIndex].socket=sockfd;
		remoteClients[newRemoteClientIndex].systemAddress=systemAddress;

		completedConnectionAttemptMutex.Lock();
		completedConnectionAttempts.Push(remoteClients[newRemoteClientIndex].systemAddress, __FILE__, __LINE__ );
		completedConnectionAttemptMutex.Unlock();

		return remoteClients[newRemoteClientIndex].systemAddress;
	}
	else
	{
		ThisPtrPlusSysAddr *s = RakNet::OP_NEW<ThisPtrPlusSysAddr>( __FILE__, __LINE__ );
		s->systemAddress.SetBinaryAddress(host);
		s->systemAddress.port=remotePort;
		s->systemAddress.systemIndex=(SystemIndex) newRemoteClientIndex;
		s->tcpInterface=this;

		// Start the connection thread
		int errorCode = RakNet::RakThread::Create(ConnectionAttemptLoop, s, threadPriority);
		if (errorCode!=0)
		{
			RakNet::OP_DELETE(s, __FILE__, __LINE__);
			failedConnectionAttempts.Push(s->systemAddress, __FILE__, __LINE__ );
		}
		return UNASSIGNED_SYSTEM_ADDRESS;
	}	
}
#if OPEN_SSL_CLIENT_SUPPORT==1
void TCPInterface::StartSSLClient(SystemAddress systemAddress)
{
	if (ctx==0)
	{
		sharedSslMutex.Lock();
		SSLeay_add_ssl_algorithms();
		meth = SSLv2_client_method();
		SSL_load_error_strings();
		ctx = SSL_CTX_new (meth);
		RakAssert(ctx!=0);
		sharedSslMutex.Unlock();
	}

	SystemAddress *id = startSSL.Allocate( __FILE__, __LINE__ );
	*id=systemAddress;
	startSSL.Push(id);
	unsigned index = activeSSLConnections.GetIndexOf(systemAddress);
	if (index==(unsigned)-1)
		activeSSLConnections.Insert(systemAddress,__FILE__,__LINE__);
}
bool TCPInterface::IsSSLActive(SystemAddress systemAddress)
{
	return activeSSLConnections.GetIndexOf(systemAddress)!=-1;
}
#endif
void TCPInterface::Send( const char *data, unsigned length, SystemAddress systemAddress, bool broadcast )
{
	SendList( &data, &length, 1, systemAddress,broadcast );
}
bool TCPInterface::SendList( const char **data, const unsigned int *lengths, const int numParameters, SystemAddress systemAddress, bool broadcast )
{
	if (isStarted==false)
		return false;
	if (data==0)
		return false;
	if (systemAddress==UNASSIGNED_SYSTEM_ADDRESS && broadcast==false)
		return false;
	unsigned int totalLength=0;
	int i;
	for (i=0; i < numParameters; i++)
	{
		if (lengths[i]>0)
			totalLength+=lengths[i];
	}
	if (totalLength==0)
		return false;

	if (broadcast)
	{
		// Send to all, possible exception system
		for (i=0; i < remoteClientsLength; i++)
		{
			if (remoteClients[i].systemAddress!=systemAddress)
			{
				remoteClients[i].SendOrBuffer(data, lengths, numParameters);
			}
		}
	}
	else
	{
		// Send to this player
		if (systemAddress.systemIndex<remoteClientsLength &&
			remoteClients[systemAddress.systemIndex].systemAddress==systemAddress)
		{
			remoteClients[systemAddress.systemIndex].SendOrBuffer(data, lengths, numParameters);
		}
		else
		{
			for (i=0; i < remoteClientsLength; i++)
			{
				if (remoteClients[i].systemAddress==systemAddress )
				{
					remoteClients[i].SendOrBuffer(data, lengths, numParameters);
				}
			}
		}
	}


	return true;
}
bool TCPInterface::ReceiveHasPackets( void )
{
	return headPush.IsEmpty()==false || incomingMessages.IsEmpty()==false || tailPush.IsEmpty()==false;
}
Packet* TCPInterface::Receive( void )
{
	if (isStarted==false)
		return 0;
	if (headPush.IsEmpty()==false)
		return headPush.Pop();
	Packet *p = incomingMessages.PopInaccurate();
	if (p)
		return p;
	if (tailPush.IsEmpty()==false)
		return tailPush.Pop();
	return 0;
}
void TCPInterface::CloseConnection( SystemAddress systemAddress )
{
	if (isStarted==false)
		return;
	if (systemAddress==UNASSIGNED_SYSTEM_ADDRESS)
		return;
	
	if (systemAddress.systemIndex<remoteClientsLength && remoteClients[systemAddress.systemIndex].systemAddress==systemAddress)
	{
		remoteClients[systemAddress.systemIndex].isActiveMutex.Lock();
		remoteClients[systemAddress.systemIndex].SetActive(false);
		remoteClients[systemAddress.systemIndex].isActiveMutex.Unlock();
	}
	else
	{
		for (int i=0; i < remoteClientsLength; i++)
		{
			remoteClients[i].isActiveMutex.Lock();
			if (remoteClients[i].isActive && remoteClients[i].systemAddress==systemAddress)
			{
				remoteClients[systemAddress.systemIndex].SetActive(false);
				remoteClients[i].isActiveMutex.Unlock();
				break;
			}
			remoteClients[i].isActiveMutex.Unlock();
		}
	}


#if OPEN_SSL_CLIENT_SUPPORT==1
	unsigned index = activeSSLConnections.GetIndexOf(systemAddress);
	if (index!=(unsigned)-1)
		activeSSLConnections.RemoveAtIndex(index);
#endif
}
void TCPInterface::DeallocatePacket( Packet *packet )
{
	if (packet==0)
		return;
	if (packet->deleteData)
	{
		rakFree_Ex(packet->data, __FILE__, __LINE__ );
		incomingMessages.Deallocate(packet, __FILE__,__LINE__);
	}
	else
	{
		// Came from userspace AllocatePacket
		rakFree_Ex(packet->data, __FILE__, __LINE__ );
		RakNet::OP_DELETE(packet, __FILE__, __LINE__);
	}
}
Packet* TCPInterface::AllocatePacket(unsigned dataSize)
{
	Packet*p = RakNet::OP_NEW<Packet>(__FILE__,__LINE__);
	p->data=(unsigned char*) rakMalloc_Ex(dataSize,__FILE__,__LINE__);
	p->length=dataSize;
	p->bitSize=BYTES_TO_BITS(dataSize);
	p->deleteData=false;
	p->guid=UNASSIGNED_RAKNET_GUID;
	p->systemAddress=UNASSIGNED_SYSTEM_ADDRESS;
	p->systemAddress.systemIndex=(SystemIndex)-1;
	return p;
}
void TCPInterface::PushBackPacket( Packet *packet, bool pushAtHead )
{
	if (pushAtHead)
		headPush.Push(packet, __FILE__, __LINE__ );
	else
		tailPush.Push(packet, __FILE__, __LINE__ );
}
SystemAddress TCPInterface::HasCompletedConnectionAttempt(void)
{
	SystemAddress sysAddr=UNASSIGNED_SYSTEM_ADDRESS;
	completedConnectionAttemptMutex.Lock();
	if (completedConnectionAttempts.IsEmpty()==false)
		sysAddr=completedConnectionAttempts.Pop();
	completedConnectionAttemptMutex.Unlock();
	return sysAddr;
}
SystemAddress TCPInterface::HasFailedConnectionAttempt(void)
{
	SystemAddress sysAddr=UNASSIGNED_SYSTEM_ADDRESS;
	failedConnectionAttemptMutex.Lock();
	if (failedConnectionAttempts.IsEmpty()==false)
		sysAddr=failedConnectionAttempts.Pop();
	failedConnectionAttemptMutex.Unlock();
	return sysAddr;
}
SystemAddress TCPInterface::HasNewIncomingConnection(void)
{
	SystemAddress *out, out2;
	out = newIncomingConnections.PopInaccurate();
	if (out)
	{
		out2=*out;
		newIncomingConnections.Deallocate(out, __FILE__,__LINE__);
		return *out;
	}
	else
	{
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
}
SystemAddress TCPInterface::HasLostConnection(void)
{
	SystemAddress *out, out2;
	out = lostConnections.PopInaccurate();
	if (out)
	{
		out2=*out;
		lostConnections.Deallocate(out, __FILE__,__LINE__);
		return *out;
	}
	else
	{
		return UNASSIGNED_SYSTEM_ADDRESS;
	}
}
void TCPInterface::GetConnectionList( SystemAddress *remoteSystems, unsigned short *numberOfSystems ) const
{
	unsigned short systemCount=0;
	unsigned short maxToWrite=*numberOfSystems;
	for (int i=0; i < remoteClientsLength; i++)
	{
		if (remoteClients[i].isActive)
		{
			if (systemCount < maxToWrite)
				remoteSystems[systemCount]=remoteClients[i].systemAddress;
			systemCount++;
		}
	}
	*numberOfSystems=systemCount;
}
unsigned short TCPInterface::GetConnectionCount(void) const
{
	unsigned short systemCount=0;
	for (int i=0; i < remoteClientsLength; i++)
	{
		if (remoteClients[i].isActive)
			systemCount++;
	}
	return systemCount;
}

unsigned int TCPInterface::GetOutgoingDataBufferSize(SystemAddress systemAddress) const
{
	unsigned bytesWritten=0;
	if (systemAddress.systemIndex<remoteClientsLength &&
		remoteClients[systemAddress.systemIndex].isActive &&
		remoteClients[systemAddress.systemIndex].systemAddress==systemAddress)
	{
		remoteClients[systemAddress.systemIndex].outgoingDataMutex.Lock();
		bytesWritten=remoteClients[systemAddress.systemIndex].outgoingData.GetBytesWritten();
		remoteClients[systemAddress.systemIndex].outgoingDataMutex.Unlock();
		return bytesWritten;
	}

	for (int i=0; i < remoteClientsLength; i++)
	{
		if (remoteClients[i].isActive && remoteClients[i].systemAddress==systemAddress)
		{
			remoteClients[i].outgoingDataMutex.Lock();
			bytesWritten+=remoteClients[i].outgoingData.GetBytesWritten();
			remoteClients[i].outgoingDataMutex.Unlock();
		}
	}
	return bytesWritten;
}
SOCKET TCPInterface::SocketConnect(const char* host, unsigned short remotePort)
{
	sockaddr_in serverAddress;


	struct hostent * server;
	server = gethostbyname(host);
	if (server == NULL)
		return (SOCKET) -1;


	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		return (SOCKET) -1;

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons( remotePort );

	/*
#ifdef _WIN32
	unsigned long nonblocking = 1;
	ioctlsocket( sockfd, FIONBIO, &nonblocking );
#elif defined(_PS3) || defined(__PS3__) || defined(SN_TARGET_PS3)
	int sock_opt=1;
	setsockopt(sockfd, SOL_SOCKET, SO_NBIO, ( char * ) & sock_opt, sizeof ( sock_opt ) );
#else
	fcntl( sockfd, F_SETFL, O_NONBLOCK );
#endif
	*/

	int sock_opt=1024*256;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );



	memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)server->h_addr, server->h_length);




	blockingSocketListMutex.Lock();
	blockingSocketList.Insert(sockfd, __FILE__, __LINE__);
	blockingSocketListMutex.Unlock();

	// This is blocking
	int connectResult = connect( sockfd, ( struct sockaddr * ) &serverAddress, sizeof( struct sockaddr ) );

	unsigned sockfdIndex;
	blockingSocketListMutex.Lock();
	sockfdIndex=blockingSocketList.GetIndexOf(sockfd);
	if (sockfdIndex!=(unsigned)-1)
		blockingSocketList.RemoveAtIndexFast(sockfdIndex);
	blockingSocketListMutex.Unlock();

	if (connectResult==-1)
	{
		closesocket(sockfd);
		return (SOCKET) -1;
	}

	return sockfd;
}
RAK_THREAD_DECLARATION(ConnectionAttemptLoop)
{
	TCPInterface::ThisPtrPlusSysAddr *s = (TCPInterface::ThisPtrPlusSysAddr *) arguments;
	SystemAddress systemAddress = s->systemAddress;
	TCPInterface *tcpInterface = s->tcpInterface;
	int newRemoteClientIndex=systemAddress.systemIndex;
	RakNet::OP_DELETE(s, __FILE__, __LINE__);

	char str1[64];
	systemAddress.ToString(false, str1);
	SOCKET sockfd = tcpInterface->SocketConnect(str1, systemAddress.port);
	if (sockfd==(SOCKET)-1)
	{
		tcpInterface->remoteClients[newRemoteClientIndex].isActiveMutex.Lock();
		tcpInterface->remoteClients[newRemoteClientIndex].SetActive(false);
		tcpInterface->remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();

		tcpInterface->failedConnectionAttemptMutex.Lock();
		tcpInterface->failedConnectionAttempts.Push(systemAddress, __FILE__, __LINE__ );
		tcpInterface->failedConnectionAttemptMutex.Unlock();
		return 0;
	}

	tcpInterface->remoteClients[newRemoteClientIndex].socket=sockfd;
	tcpInterface->remoteClients[newRemoteClientIndex].systemAddress=systemAddress;

	// Notify user that the connection attempt has completed.
	if (tcpInterface->threadRunning)
	{
		tcpInterface->completedConnectionAttemptMutex.Lock();
		tcpInterface->completedConnectionAttempts.Push(systemAddress, __FILE__, __LINE__ );
		tcpInterface->completedConnectionAttemptMutex.Unlock();
	}	

	return 0;
}

RAK_THREAD_DECLARATION(UpdateTCPInterfaceLoop)
{
	TCPInterface * sts = ( TCPInterface * ) arguments;
//	const int BUFF_SIZE=8096;
	const int BUFF_SIZE=1048576;
	//char data[ BUFF_SIZE ];
	char * data = (char*) rakMalloc_Ex(BUFF_SIZE,__FILE__,__LINE__);
	Packet *incomingMessage;
	fd_set      readFD, exceptionFD, writeFD;
	sts->threadRunning=true;

	sockaddr_in sockAddr;
	int sockAddrSize = sizeof(sockAddr);

	int len;
	SOCKET newSock;
	timeval tv;
	int selectResult;
	tv.tv_sec=0;
	tv.tv_usec=50000;

	while (sts->isStarted)
	{
#if OPEN_SSL_CLIENT_SUPPORT==1
		SystemAddress *sslSystemAddress;
		sslSystemAddress = sts->startSSL.PopInaccurate();
		if (sslSystemAddress)
		{
			if (sslSystemAddress->systemIndex>=0 &&
				sslSystemAddress->systemIndex<sts->remoteClientsLength &&
				sts->remoteClients[sslSystemAddress->systemIndex].systemAddress==*sslSystemAddress)
			{
				sts->remoteClients[sslSystemAddress->systemIndex].InitSSL(sts->ctx,sts->meth);
			}
			else
			{
				for (int i=0; i < sts->remoteClientsLength; i++)
				{
					sts->remoteClients[i].isActiveMutex.Lock();
					if (sts->remoteClients[i].isActive && sts->remoteClients[i].systemAddress==*sslSystemAddress)
					{
						if (sts->remoteClients[i].ssl==0)
							sts->remoteClients[i].InitSSL(sts->ctx,sts->meth);
					}
					sts->remoteClients[i].isActiveMutex.Unlock();
				}
			}
			sts->startSSL.Deallocate(sslSystemAddress,__FILE__,__LINE__);
		}
#endif


		SOCKET largestDescriptor=0; // see select()'s first parameter's documentation under linux


		// Linux' select() implementation changes the timeout
		tv.tv_sec=0;
		tv.tv_usec=500000;

		while (1)
		{
			// Reset readFD, writeFD, and exceptionFD since select seems to clear it
			FD_ZERO(&readFD);
			FD_ZERO(&exceptionFD);
			FD_ZERO(&writeFD);
#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
			largestDescriptor=0;
			if (sts->listenSocket!=(SOCKET) -1)
			{
				FD_SET(sts->listenSocket, &readFD);
				FD_SET(sts->listenSocket, &exceptionFD);
				largestDescriptor = sts->listenSocket; // @see largestDescriptor def
			}

			unsigned i;
			for (i=0; i < (unsigned int) sts->remoteClientsLength; i++)
			{
				sts->remoteClients[i].isActiveMutex.Lock();
				if (sts->remoteClients[i].isActive && sts->remoteClients[i].socket!=INVALID_SOCKET)
				{
					FD_SET(sts->remoteClients[i].socket, &readFD);
					FD_SET(sts->remoteClients[i].socket, &exceptionFD);
					if (sts->remoteClients[i].outgoingData.GetBytesWritten()>0)
						FD_SET(sts->remoteClients[i].socket, &writeFD);
					if(sts->remoteClients[i].socket > largestDescriptor) // @see largestDescriptorDef
						largestDescriptor = sts->remoteClients[i].socket;
				}
				sts->remoteClients[i].isActiveMutex.Unlock();
			}

#ifdef _MSC_VER
#pragma warning( disable : 4244 ) // warning C4127: conditional expression is constant
#endif



			selectResult=(int) select(largestDescriptor+1, &readFD, &writeFD, &exceptionFD, &tv);		


			if (selectResult<=0)
				break;

			if (sts->listenSocket!=(SOCKET) -1 && FD_ISSET(sts->listenSocket, &readFD))
			{
				newSock = accept(sts->listenSocket, (sockaddr*)&sockAddr, (socklen_t*)&sockAddrSize);

				if (newSock != (SOCKET) -1)
				{
					int newRemoteClientIndex=-1;
					for (newRemoteClientIndex=0; newRemoteClientIndex < sts->remoteClientsLength; newRemoteClientIndex++)
					{
						sts->remoteClients[newRemoteClientIndex].isActiveMutex.Lock();
						if (sts->remoteClients[newRemoteClientIndex].isActive==false)
						{
							sts->remoteClients[newRemoteClientIndex].socket=newSock;
							sts->remoteClients[newRemoteClientIndex].systemAddress.binaryAddress=sockAddr.sin_addr.s_addr;
							sts->remoteClients[newRemoteClientIndex].systemAddress.port=ntohs( sockAddr.sin_port);
							sts->remoteClients[newRemoteClientIndex].systemAddress.systemIndex=newRemoteClientIndex;

							sts->remoteClients[newRemoteClientIndex].SetActive(true);
							sts->remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();


							SystemAddress *newConnectionSystemAddress=sts->newIncomingConnections.Allocate( __FILE__, __LINE__ );
							*newConnectionSystemAddress=sts->remoteClients[newRemoteClientIndex].systemAddress;
							sts->newIncomingConnections.Push(newConnectionSystemAddress);

							break;
						}
						sts->remoteClients[newRemoteClientIndex].isActiveMutex.Unlock();
					}
					if (newRemoteClientIndex==-1)
					{
						closesocket(sts->listenSocket);
					}
				}
				else
				{
#ifdef _DO_PRINTF
					RAKNET_DEBUG_PRINTF("Error: connection failed\n");
#endif
				}
			}
			else if (sts->listenSocket!=(SOCKET) -1 && FD_ISSET(sts->listenSocket, &exceptionFD))
			{
#ifdef _DO_PRINTF
				int err;
				int errlen = sizeof(err);
				getsockopt(sts->listenSocket, SOL_SOCKET, SO_ERROR,(char*)&err, &errlen);
				RAKNET_DEBUG_PRINTF("Socket error %s on listening socket\n", err);
#endif
			}
			
			{
				i=0;
				while (i < (unsigned int) sts->remoteClientsLength)
				{
					if (sts->remoteClients[i].isActive==false)
					{
						i++;
						continue;
					}

					if (FD_ISSET(sts->remoteClients[i].socket, &exceptionFD))
					{
#ifdef _DO_PRINTF
						if (sts->listenSocket!=-1)
						{
							int err;
							int errlen = sizeof(err);
							getsockopt(sts->listenSocket, SOL_SOCKET, SO_ERROR,(char*)&err, &errlen);
							in_addr in;
							in.s_addr = sts->remoteClients[i].systemAddress.binaryAddress;
							RAKNET_DEBUG_PRINTF("Socket error %i on %s:%i\n", err,inet_ntoa( in ), sts->remoteClients[i].systemAddress.port );
						}
						
#endif
						// Connection lost abruptly
						SystemAddress *lostConnectionSystemAddress=sts->lostConnections.Allocate( __FILE__, __LINE__ );
						*lostConnectionSystemAddress=sts->remoteClients[i].systemAddress;
						sts->lostConnections.Push(lostConnectionSystemAddress);
						sts->remoteClients[i].isActiveMutex.Lock();
						sts->remoteClients[i].SetActive(false);
						sts->remoteClients[i].isActiveMutex.Unlock();
					}
					else
					{
						if (FD_ISSET(sts->remoteClients[i].socket, &readFD))
						{
							// if recv returns 0 this was a graceful close
							len = sts->remoteClients[i].Recv(data,BUFF_SIZE);
							if (len>0)
							{
								incomingMessage=sts->incomingMessages.Allocate( __FILE__, __LINE__ );
								incomingMessage->data = (unsigned char*) rakMalloc_Ex( len+1, __FILE__, __LINE__ );
								memcpy(incomingMessage->data, data, len);
								incomingMessage->data[len]=0; // Null terminate this so we can print it out as regular strings.  This is different from RakNet which does not do this.
//								printf("RECV: %s\n",incomingMessage->data);
								incomingMessage->length=len;
								incomingMessage->deleteData=true; // actually means came from SPSC, rather than AllocatePacket
								incomingMessage->systemAddress=sts->remoteClients[i].systemAddress;
								sts->incomingMessages.Push(incomingMessage);
							}
							else
							{
								// Connection lost gracefully
								SystemAddress *lostConnectionSystemAddress=sts->lostConnections.Allocate( __FILE__, __LINE__ );
								*lostConnectionSystemAddress=sts->remoteClients[i].systemAddress;
								sts->lostConnections.Push(lostConnectionSystemAddress);
								sts->remoteClients[i].isActiveMutex.Lock();
								sts->remoteClients[i].SetActive(false);
								sts->remoteClients[i].isActiveMutex.Unlock();
								continue;
							}
						}
						if (FD_ISSET(sts->remoteClients[i].socket, &writeFD))
						{
							RemoteClient *rc = &sts->remoteClients[i];
							unsigned int bytesInBuffer;
							int bytesAvailable;
							int bytesSent;
							rc->outgoingDataMutex.Lock();
							bytesInBuffer=rc->outgoingData.GetBytesWritten();
							if (bytesInBuffer>0)
							{
								unsigned int contiguousLength;
								char* contiguousBytesPointer = rc->outgoingData.PeekContiguousBytes(&contiguousLength);
								if (contiguousLength < BUFF_SIZE && contiguousLength<bytesInBuffer)
								{
									if (bytesInBuffer > BUFF_SIZE)
										bytesAvailable=BUFF_SIZE;
									else
										bytesAvailable=bytesInBuffer;
									rc->outgoingData.ReadBytes(data,bytesAvailable,true);
									bytesSent=rc->Send(data,bytesAvailable);
								}
								else
								{
									bytesSent=rc->Send(contiguousBytesPointer,contiguousLength);
								}

								rc->outgoingData.IncrementReadOffset(bytesSent);
								bytesInBuffer=rc->outgoingData.GetBytesWritten();
							}
							rc->outgoingDataMutex.Unlock();
						}
							
						i++; // Nothing deleted so increment the index
					}
				}
			}
		}

		// Sleep 0 on Linux monopolizes the CPU
		RakSleep(30);
	}
	sts->threadRunning=false;

	rakFree_Ex(data,__FILE__,__LINE__);

	return 0;
}
void RemoteClient::SetActive(bool a)
{
	if (isActive != a)
	{
		isActive=a;
		Reset();
		if (isActive==false && socket!=INVALID_SOCKET)
		{
			closesocket(socket);
			socket=INVALID_SOCKET;
		}
	}
}
void RemoteClient::SendOrBuffer(const char **data, const unsigned int *lengths, const int numParameters)
{
	// True can save memory and buffer copies, but gives worse performance overall
	// Do not use true for the XBOX, as it just locks up
	const bool ALLOW_SEND_FROM_USER_THREAD=false;

	int parameterIndex;
	if (isActive==false)
		return;
	parameterIndex=0;
	for (; parameterIndex < numParameters; parameterIndex++)
	{
		outgoingDataMutex.Lock();
		if (ALLOW_SEND_FROM_USER_THREAD && outgoingData.GetBytesWritten()==0)
		{
			outgoingDataMutex.Unlock();
			int bytesSent = Send(data[parameterIndex],lengths[parameterIndex]);
			if (bytesSent<(int) lengths[parameterIndex])
			{
				// Push remainder
				outgoingDataMutex.Lock();
				outgoingData.WriteBytes(data[parameterIndex]+bytesSent,lengths[parameterIndex]-bytesSent,__FILE__,__LINE__);
				outgoingDataMutex.Unlock();
			}
		}
		else
		{
			outgoingData.WriteBytes(data[parameterIndex],lengths[parameterIndex],__FILE__,__LINE__);
			outgoingDataMutex.Unlock();
		}
	}
}
#if OPEN_SSL_CLIENT_SUPPORT==1
void RemoteClient::InitSSL(SSL_CTX* ctx, SSL_METHOD *meth)
{
	(void) meth;

	ssl = SSL_new (ctx);                         
	RakAssert(ssl);    
	SSL_set_fd (ssl, socket);
	SSL_connect (ssl);
}
void RemoteClient::DisconnectSSL(void)
{
	if (ssl)
		SSL_shutdown (ssl);  /* send SSL/TLS close_notify */
}
void RemoteClient::FreeSSL(void)
{
	if (ssl)
		SSL_free (ssl);
}
int RemoteClient::Send(const char *data, unsigned int length)
{
	int err;
	if (ssl)
	{
		return SSL_write (ssl, data, length);
	}
	else
		return send(socket, data, length, 0);
}
int RemoteClient::Recv(char *data, const int dataSize)
{
	if (ssl)
		return SSL_read (ssl, data, dataSize);
	else
		return recv(socket, data, dataSize, 0);
}
#else
int RemoteClient::Send(const char *data, unsigned int length)
{
	return send(socket, data, length, 0);
}
int RemoteClient::Recv(char *data, const int dataSize)
{
	return recv(socket, data, dataSize, 0);
}
#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // _RAKNET_SUPPORT_*
