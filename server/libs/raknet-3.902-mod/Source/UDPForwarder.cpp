#include "UDPForwarder.h"
#include "GetTime.h"
#include "MTUSize.h"
#include "SocketLayer.h"
#include "WSAStartupSingleton.h"
#include "RakSleep.h"
#include "DS_OrderedList.h"

using namespace RakNet;
static const unsigned short DEFAULT_MAX_FORWARD_ENTRIES=64;

RAK_THREAD_DECLARATION(UpdateUDPForwarder);

bool operator<( const DataStructures::MLKeyRef<UDPForwarder::SrcAndDest> &inputKey, const UDPForwarder::ForwardEntry *cls )
{
	return inputKey.Get().source < cls->srcAndDest.source ||
		(inputKey.Get().source == cls->srcAndDest.source && inputKey.Get().dest < cls->srcAndDest.dest);
}
bool operator>( const DataStructures::MLKeyRef<UDPForwarder::SrcAndDest> &inputKey, const UDPForwarder::ForwardEntry *cls )
{
	return inputKey.Get().source > cls->srcAndDest.source ||
		(inputKey.Get().source == cls->srcAndDest.source && inputKey.Get().dest > cls->srcAndDest.dest);
}
bool operator==( const DataStructures::MLKeyRef<UDPForwarder::SrcAndDest> &inputKey, const UDPForwarder::ForwardEntry *cls )
{
	return inputKey.Get().source == cls->srcAndDest.source && inputKey.Get().dest == cls->srcAndDest.dest;
}


UDPForwarder::ForwardEntry::ForwardEntry() {socket=INVALID_SOCKET; timeLastDatagramForwarded=RakNet::GetTimeMS(); updatedSourcePort=false; updatedDestPort=false;}
UDPForwarder::ForwardEntry::~ForwardEntry() {
	if (socket!=INVALID_SOCKET)
		closesocket(socket);
}

UDPForwarder::UDPForwarder()
{
#ifdef _WIN32
	WSAStartupSingleton::AddRef();
#endif

	maxForwardEntries=DEFAULT_MAX_FORWARD_ENTRIES;
	isRunning=false;
	threadRunning=false;
}
UDPForwarder::~UDPForwarder()
{
	Shutdown();

#ifdef _WIN32
	WSAStartupSingleton::Deref();
#endif
}
void UDPForwarder::Startup(void)
{
	if (isRunning==true)
		return;

	isRunning=true;
	threadRunning=false;

#ifdef UDP_FORWARDER_EXECUTE_THREADED
	int errorCode = RakNet::RakThread::Create(UpdateUDPForwarder, this);
	if ( errorCode != 0 )
	{
		RakAssert(0);
		return;
	}

	while (threadRunning==false)
		RakSleep(30);
#endif
}
void UDPForwarder::Shutdown(void)
{
	if (isRunning==false)
		return;

	isRunning=false;

#ifdef UDP_FORWARDER_EXECUTE_THREADED
	while (threadRunning==true)
		RakSleep(30);
#endif

	forwardList.ClearPointers(true,__FILE__,__LINE__);
}
void UDPForwarder::Update(void)
{
#ifndef UDP_FORWARDER_EXECUTE_THREADED
	UpdateThreaded();
#endif

}
void UDPForwarder::UpdateThreaded(void)
{
	fd_set      readFD;
	//fd_set exceptionFD;
	FD_ZERO(&readFD);
//	FD_ZERO(&exceptionFD);
	timeval tv;
	int selectResult;
	tv.tv_sec=0;
	tv.tv_usec=0;

	RakNetTimeMS curTime = RakNet::GetTimeMS();

	SOCKET largestDescriptor=0;
	DataStructures::DefaultIndexType i;

	// Remove unused entries
	i=0;
	while (i < forwardList.GetSize())
	{
		if (curTime > forwardList[i]->timeLastDatagramForwarded && // Account for timestamp wrap
			curTime > forwardList[i]->timeLastDatagramForwarded+forwardList[i]->timeoutOnNoDataMS)
		{
			RakNet::OP_DELETE(forwardList[i],__FILE__,__LINE__);
			forwardList.RemoveAtIndex(i,__FILE__,__LINE__);
		}
		else
			i++;
	}

	if (forwardList.GetSize()==0)
		return;

	for (i=0; i < forwardList.GetSize(); i++)
	{
#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif
		FD_SET(forwardList[i]->socket, &readFD);
//		FD_SET(forwardList[i]->readSocket, &exceptionFD);

		if (forwardList[i]->socket > largestDescriptor)
			largestDescriptor = forwardList[i]->socket;
	}




	selectResult=(int) select((int) largestDescriptor+1, &readFD, 0, 0, &tv);


	char data[ MAXIMUM_MTU_SIZE ];
	sockaddr_in sa;
	socklen_t len2;

	if (selectResult > 0)
	{
		DataStructures::Queue<ForwardEntry*> entriesToRead;
		ForwardEntry *forwardEntry;

		for (i=0; i < forwardList.GetSize(); i++)
		{
			forwardEntry = forwardList[i];
			// I do this because I'm updating the forwardList, and don't want to lose FD_ISSET as the list is no longer in order
			if (FD_ISSET(forwardEntry->socket, &readFD))
				entriesToRead.Push(forwardEntry,__FILE__,__LINE__);
		}

		while (entriesToRead.IsEmpty()==false)
		{
			forwardEntry=entriesToRead.Pop();

			const int flag=0;
			int receivedDataLen, len=0;
			unsigned short portnum=0;
			len2 = sizeof( sa );
			sa.sin_family = AF_INET;
			receivedDataLen = recvfrom( forwardEntry->socket, data, MAXIMUM_MTU_SIZE, flag, ( sockaddr* ) & sa, ( socklen_t* ) & len2 );

			if (receivedDataLen<0)
			{
#if defined(_WIN32) && defined(_DEBUG) 
				DWORD dwIOError = WSAGetLastError();

				if (dwIOError!=WSAECONNRESET && dwIOError!=WSAEINTR && dwIOError!=WSAETIMEDOUT)
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
				continue;
			}

			portnum = ntohs( sa.sin_port );
			if (forwardEntry->srcAndDest.source.binaryAddress==sa.sin_addr.s_addr && forwardEntry->updatedSourcePort==false && forwardEntry->srcAndDest.dest.port!=portnum)
			{
				forwardEntry->updatedSourcePort=true;

				if (forwardEntry->srcAndDest.source.port!=portnum)
				{
					DataStructures::DefaultIndexType index;
					SrcAndDest srcAndDest(forwardEntry->srcAndDest.dest, forwardEntry->srcAndDest.source);
					index=forwardList.GetIndexOf(srcAndDest);
					forwardList.RemoveAtIndex(index,__FILE__,__LINE__);
					forwardEntry->srcAndDest.source.port=portnum;
					forwardList.Push(forwardEntry,forwardEntry->srcAndDest,__FILE__,__LINE__);
				}
			}
			
			if (forwardEntry->srcAndDest.source.binaryAddress==sa.sin_addr.s_addr && forwardEntry->srcAndDest.source.port==portnum)
			{
				// Forward to dest
				len=0;
				sockaddr_in saOut;
				saOut.sin_port = htons( forwardEntry->srcAndDest.dest.port ); // User port
				saOut.sin_addr.s_addr = forwardEntry->srcAndDest.dest.binaryAddress;
				saOut.sin_family = AF_INET;
				do
				{
					len = sendto( forwardEntry->socket, data, receivedDataLen, 0, ( const sockaddr* ) & saOut, sizeof( saOut ) );
				}
				while ( len == 0 );

				// printf("1. Forwarding after %i ms\n", curTime-forwardEntry->timeLastDatagramForwarded);

				forwardEntry->timeLastDatagramForwarded=curTime;
			}

			if (forwardEntry->srcAndDest.dest.binaryAddress==sa.sin_addr.s_addr && forwardEntry->updatedDestPort==false && forwardEntry->srcAndDest.source.port!=portnum)
			{
				forwardEntry->updatedDestPort=true;

				if (forwardEntry->srcAndDest.dest.port!=portnum)
				{
					DataStructures::DefaultIndexType index;
					SrcAndDest srcAndDest(forwardEntry->srcAndDest.source, forwardEntry->srcAndDest.dest);
					index=forwardList.GetIndexOf(srcAndDest);
					forwardList.RemoveAtIndex(index,__FILE__,__LINE__);
					forwardEntry->srcAndDest.dest.port=portnum;
					forwardList.Push(forwardEntry,forwardEntry->srcAndDest,__FILE__,__LINE__);
				}
			}

			if (forwardEntry->srcAndDest.dest.binaryAddress==sa.sin_addr.s_addr && forwardEntry->srcAndDest.dest.port==portnum)
			{
				// Forward to source
				len=0;
				sockaddr_in saOut;
				saOut.sin_port = htons( forwardEntry->srcAndDest.source.port ); // User port
				saOut.sin_addr.s_addr = forwardEntry->srcAndDest.source.binaryAddress;
				saOut.sin_family = AF_INET;
				do
				{
					len = sendto( forwardEntry->socket, data, receivedDataLen, 0, ( const sockaddr* ) & saOut, sizeof( saOut ) );
				}
				while ( len == 0 );

				// printf("2. Forwarding after %i ms\n", curTime-forwardEntry->timeLastDatagramForwarded);

				forwardEntry->timeLastDatagramForwarded=curTime;
			}
		}
	}


}
void UDPForwarder::SetMaxForwardEntries(unsigned short maxEntries)
{
	RakAssert(maxEntries>0 && maxEntries<65535/2);
	maxForwardEntries=maxEntries;
}
int UDPForwarder::GetMaxForwardEntries(void) const
{
	return maxForwardEntries;
}
int UDPForwarder::GetUsedForwardEntries(void) const
{
	return (int) forwardList.GetSize();
}
UDPForwarderResult UDPForwarder::AddForwardingEntry(SrcAndDest srcAndDest, RakNetTimeMS timeoutOnNoDataMS, unsigned short *port, const char *forceHostAddress)
{
	DataStructures::DefaultIndexType insertionIndex;
	insertionIndex = forwardList.GetInsertionIndex(srcAndDest);
	if (insertionIndex!=(DataStructures::DefaultIndexType)-1)
	{
		int sock_opt;
		sockaddr_in listenerSocketAddress;
		listenerSocketAddress.sin_port = 0;
		ForwardEntry *fe = RakNet::OP_NEW<UDPForwarder::ForwardEntry>(__FILE__,__LINE__);
		fe->srcAndDest=srcAndDest;
		fe->timeoutOnNoDataMS=timeoutOnNoDataMS;
		fe->socket = socket( AF_INET, SOCK_DGRAM, 0 );

		//printf("Made socket %i\n", fe->readSocket);

		// This doubles the max throughput rate
		sock_opt=1024*256;
		setsockopt(fe->socket, SOL_SOCKET, SO_RCVBUF, ( char * ) & sock_opt, sizeof ( sock_opt ) );

		// Immediate hard close. Don't linger the readSocket, or recreating the readSocket quickly on Vista fails.
		sock_opt=0;
		setsockopt(fe->socket, SOL_SOCKET, SO_LINGER, ( char * ) & sock_opt, sizeof ( sock_opt ) );

		listenerSocketAddress.sin_family = AF_INET;

		if (forceHostAddress && forceHostAddress[0])
		{
			listenerSocketAddress.sin_addr.s_addr = inet_addr( forceHostAddress );
		}
		else
		{
			listenerSocketAddress.sin_addr.s_addr = INADDR_ANY;
		}

		int ret = bind( fe->socket, ( struct sockaddr * ) & listenerSocketAddress, sizeof( listenerSocketAddress ) );
		if (ret==-1)
		{
			RakNet::OP_DELETE(fe,__FILE__,__LINE__);
			return UDPFORWARDER_BIND_FAILED;
		}

		DataStructures::DefaultIndexType oldSize = forwardList.GetSize();
		forwardList.InsertAtIndex(fe,insertionIndex,__FILE__,__LINE__);
		RakAssert(forwardList.GetIndexOf(fe->srcAndDest)!=(unsigned int) -1);
		RakAssert(forwardList.GetSize()==oldSize+1);
		*port = SocketLayer::GetLocalPort ( fe->socket );
		return UDPFORWARDER_SUCCESS;
	}

	return UDPFORWARDER_FORWARDING_ALREADY_EXISTS;
}
UDPForwarderResult UDPForwarder::StartForwarding(SystemAddress source, SystemAddress destination, RakNetTimeMS timeoutOnNoDataMS, const char *forceHostAddress,
								  unsigned short *forwardingPort, SOCKET *forwardingSocket)
{
	// Invalid parameters?
	if (timeoutOnNoDataMS == 0 || timeoutOnNoDataMS > UDP_FORWARDER_MAXIMUM_TIMEOUT || source==UNASSIGNED_SYSTEM_ADDRESS || destination==UNASSIGNED_SYSTEM_ADDRESS)
		return UDPFORWARDER_INVALID_PARAMETERS;

#ifdef UDP_FORWARDER_EXECUTE_THREADED
	ThreadOperation threadOperation;
	threadOperation.source=source;
	threadOperation.destination=destination;
	threadOperation.timeoutOnNoDataMS=timeoutOnNoDataMS;
	threadOperation.forceHostAddress=forceHostAddress;
	threadOperation.operation=ThreadOperation::TO_START_FORWARDING;
	threadOperationIncomingMutex.Lock();
	threadOperationIncomingQueue.Push(threadOperation, __FILE__, __LINE__ );
	threadOperationIncomingMutex.Unlock();

	while (1)
	{
		RakSleep(0);
		threadOperationOutgoingMutex.Lock();
		if (threadOperationOutgoingQueue.Size()!=0)
		{
			threadOperation=threadOperationOutgoingQueue.Pop();
			threadOperationOutgoingMutex.Unlock();
			if (forwardingPort)
				*forwardingPort=threadOperation.forwardingPort;
			if (forwardingSocket)
				*forwardingSocket=threadOperation.forwardingSocket;
			return threadOperation.result;
		}
		threadOperationOutgoingMutex.Unlock();

	}
#else
	return StartForwardingThreaded(source, destination, timeoutOnNoDataMS, forceHostAddress, srcToDestPort, destToSourcePort, srcToDestSocket, destToSourceSocket);
#endif

}
UDPForwarderResult UDPForwarder::StartForwardingThreaded(SystemAddress source, SystemAddress destination, RakNetTimeMS timeoutOnNoDataMS, const char *forceHostAddress,
								  unsigned short *forwardingPort, SOCKET *forwardingSocket)
{
	SrcAndDest srcAndDest(source, destination);
	
	UDPForwarderResult result = AddForwardingEntry(srcAndDest, timeoutOnNoDataMS, forwardingPort, forceHostAddress);

	if (result!=UDPFORWARDER_SUCCESS)
		return result;

	if (*forwardingSocket)
	{
		DataStructures::DefaultIndexType idx;
		idx = forwardList.GetIndexOf(srcAndDest);

		*forwardingSocket=forwardList[idx]->socket;
	}

	return UDPFORWARDER_SUCCESS;
}
void UDPForwarder::StopForwarding(SystemAddress source, SystemAddress destination)
{
#ifdef UDP_FORWARDER_EXECUTE_THREADED
	ThreadOperation threadOperation;
	threadOperation.source=source;
	threadOperation.destination=destination;
	threadOperation.operation=ThreadOperation::TO_STOP_FORWARDING;
	threadOperationIncomingMutex.Lock();
	threadOperationIncomingQueue.Push(threadOperation, __FILE__, __LINE__ );
	threadOperationIncomingMutex.Unlock();
#else
	StopForwardingThreaded(source, destination);
#endif
}
void UDPForwarder::StopForwardingThreaded(SystemAddress source, SystemAddress destination)
{
	SrcAndDest srcAndDest(destination,source);

	DataStructures::DefaultIndexType idx = forwardList.GetIndexOf(srcAndDest);
	if (idx!=(DataStructures::DefaultIndexType)-1)
	{
		RakNet::OP_DELETE(forwardList[idx],__FILE__,__LINE__);
		forwardList.RemoveAtIndex(idx,__FILE__,__LINE__);
	}
}
#ifdef UDP_FORWARDER_EXECUTE_THREADED
RAK_THREAD_DECLARATION(UpdateUDPForwarder)
{
	UDPForwarder * udpForwarder = ( UDPForwarder * ) arguments;
	udpForwarder->threadRunning=true;
	UDPForwarder::ThreadOperation threadOperation;
	while (udpForwarder->isRunning)
	{
		udpForwarder->threadOperationIncomingMutex.Lock();
		while (udpForwarder->threadOperationIncomingQueue.Size())
		{
			threadOperation=udpForwarder->threadOperationIncomingQueue.Pop();
			udpForwarder->threadOperationIncomingMutex.Unlock();
			if (threadOperation.operation==UDPForwarder::ThreadOperation::TO_START_FORWARDING)
			{
				threadOperation.result=udpForwarder->StartForwardingThreaded(threadOperation.source, threadOperation.destination, threadOperation.timeoutOnNoDataMS,
					threadOperation.forceHostAddress, &threadOperation.forwardingPort, &threadOperation.forwardingSocket);
				udpForwarder->threadOperationOutgoingMutex.Lock();
				udpForwarder->threadOperationOutgoingQueue.Push(threadOperation, __FILE__, __LINE__ );
				udpForwarder->threadOperationOutgoingMutex.Unlock();
			}
			else
			{
				udpForwarder->StopForwardingThreaded(threadOperation.source, threadOperation.destination);
			}


			udpForwarder->threadOperationIncomingMutex.Lock();
		}
		udpForwarder->threadOperationIncomingMutex.Unlock();

		udpForwarder->UpdateThreaded();
		// Avoid 100% reported CPU usage
		RakSleep(15);
	}
	udpForwarder->threadRunning=false;
	return 0;
}
#endif
