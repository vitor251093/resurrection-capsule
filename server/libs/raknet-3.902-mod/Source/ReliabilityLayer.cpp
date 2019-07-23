/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "ReliabilityLayer.h"
#include "GetTime.h"
#include "SocketLayer.h"
#include "PluginInterface2.h"
#include "RakAssert.h"
#include "Rand.h"
#include "MessageIdentifiers.h"
#include <math.h>

// Can't figure out which library has this function on the PS3
double Ceil(double d) {if (((double)((int)d))==d) return d; return (int) (d+1.0);}

#if defined(new)
#pragma push_macro("new")
#undef new
#define RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE
#endif


//#define _DEBUG_LOGGER

#if CC_TIME_TYPE_BYTES==4
static const CCTimeType MAX_TIME_BETWEEN_PACKETS= 350; // 350 milliseconds
static const CCTimeType HISTOGRAM_RESTART_CYCLE=10000; // Every 10 seconds reset the histogram
#else
static const CCTimeType MAX_TIME_BETWEEN_PACKETS= 350000; // 350 milliseconds
//static const CCTimeType HISTOGRAM_RESTART_CYCLE=10000000; // Every 10 seconds reset the histogram
#endif
static const int DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE=512;
static const CCTimeType STARTING_TIME_BETWEEN_PACKETS=MAX_TIME_BETWEEN_PACKETS;
//static const long double TIME_BETWEEN_PACKETS_INCREASE_MULTIPLIER_DEFAULT=.02;
//static const long double TIME_BETWEEN_PACKETS_DECREASE_MULTIPLIER_DEFAULT=1.0 / 9.0;

typedef uint32_t BitstreamLengthEncoding;

#ifdef _MSC_VER
#pragma warning( push )
#endif


BPSTracker::TimeAndValue2::TimeAndValue2() {}
BPSTracker::TimeAndValue2::~TimeAndValue2() {}
BPSTracker::TimeAndValue2::TimeAndValue2(RakNetTimeUS t, uint64_t v1) : time(t), value1(v1) {}
//BPSTracker::TimeAndValue2::TimeAndValue2(RakNetTimeUS t, uint64_t v1, uint64_t v2) : time(t), value1(v1), value2(v2) {}
BPSTracker::BPSTracker() {Reset(__FILE__,__LINE__);}
BPSTracker::~BPSTracker() {}
//void BPSTracker::Reset(const char *file, unsigned int line) {total1=total2=lastSec1=lastSec2=0; dataQueue.Clear(file,line);}
void BPSTracker::Reset(const char *file, unsigned int line) {total1=lastSec1=0; dataQueue.Clear(file,line);}
void BPSTracker::Push1(RakNetTimeUS time, uint64_t value1) {
	ClearExpired1(time);
	dataQueue.Push(TimeAndValue2(time,value1),__FILE__,__LINE__); total1+=value1; lastSec1+=value1;
}
//void BPSTracker::Push2(RakNetTimeUS time, uint64_t value1, uint64_t value2) {dataQueue.Push(TimeAndValue2(time,value1,value2),__FILE__,__LINE__); total1+=value1; lastSec1+=value1;  total2+=value2; lastSec2+=value2;}
uint64_t BPSTracker::GetBPS1(RakNetTimeUS time) {ClearExpired1(time); return lastSec1;}
uint64_t BPSTracker::GetBPS1Threadsafe(RakNetTimeUS time) {(void) time; return lastSec1;}
//uint64_t BPSTracker::GetBPS2(RakNetTimeUS time) {ClearExpired2(time); return lastSec2;}
//void BPSTracker::GetBPS1And2(RakNetTimeUS time, uint64_t &out1, uint64_t &out2) {ClearExpired2(time); out1=lastSec1; out2=lastSec2;}
uint64_t BPSTracker::GetTotal1(void) const {return total1;}
//uint64_t BPSTracker::GetTotal2(void) const {return total2;}

// void BPSTracker::ClearExpired2(RakNetTimeUS time) {
// 	RakNetTimeUS threshold=time;
// 	if (threshold < 1000000)
// 		return;
// 	threshold-=1000000;
// 	while (dataQueue.IsEmpty()==false && dataQueue.Peek().time < threshold)
// 	{
// 		lastSec1-=dataQueue.Peek().value1;
// 		lastSec2-=dataQueue.Peek().value2;
// 		dataQueue.Pop();
// 	}
// }
void BPSTracker::ClearExpired1(RakNetTimeUS time)
{
	while (dataQueue.IsEmpty()==false &&
#if CC_TIME_TYPE_BYTES==8
		dataQueue.Peek().time+1000000 < time
#else
		dataQueue.Peek().time+1000 < time
#endif
		)
	{
		lastSec1-=dataQueue.Peek().value1;
		dataQueue.Pop();
	}
}

struct DatagramHeaderFormat
{
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
	CCTimeType sourceSystemTime;
#endif
	DatagramSequenceNumberType datagramNumber;

	// Use floats to save bandwidth
	//	float B; // Link capacity
	float AS; // Data arrival rate
	bool isACK;
	bool isNAK;
	bool isPacketPair;
	bool hasBAndAS;
	bool isContinuousSend;
	bool needsBAndAs;
	bool isValid; // To differentiate between what I serialized, and offline data

	static BitSize_t GetDataHeaderBitLength()
	{
		return BYTES_TO_BITS(GetDataHeaderByteLength());
	}

	static unsigned int GetDataHeaderByteLength()
	{
		//return 2 + 3 + sizeof(RakNetTimeMS) + sizeof(float)*2;
		return 2 + 3 +
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
			sizeof(RakNetTimeMS) +
#endif
			sizeof(float)*1;
	}

	void Serialize(RakNet::BitStream *b)
	{
		// Not endian safe
		//		RakAssert(GetDataHeaderByteLength()==sizeof(DatagramHeaderFormat));
		//		b->WriteAlignedBytes((const unsigned char*) this, sizeof(DatagramHeaderFormat));
		//		return;

		b->Write(true); // IsValid
		if (isACK)
		{
			b->Write(true);
			b->Write(hasBAndAS);
			b->AlignWriteToByteBoundary();
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
			RakNetTimeMS timeMSLow=(RakNetTimeMS) sourceSystemTime&0xFFFFFFFF; b->Write(timeMSLow);
#endif
			if (hasBAndAS)
			{
				//		b->Write(B);
				b->Write(AS);
			}
		}
		else if (isNAK)
		{
			b->Write(false);
			b->Write(true);
		}
		else
		{
			b->Write(false);
			b->Write(false);
			b->Write(isPacketPair);
			b->Write(isContinuousSend);
			b->Write(needsBAndAs);
			b->AlignWriteToByteBoundary();
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
			RakNetTimeMS timeMSLow=(RakNetTimeMS) sourceSystemTime&0xFFFFFFFF; b->Write(timeMSLow);
#endif
			b->Write(datagramNumber);
		}
	}
	void Deserialize(RakNet::BitStream *b)
	{
		// Not endian safe
		//		b->ReadAlignedBytes((unsigned char*) this, sizeof(DatagramHeaderFormat));
		//		return;

		b->Read(isValid);
		b->Read(isACK);
		if (isACK)
		{
			isNAK=false;
			isPacketPair=false;
			b->Read(hasBAndAS);
			b->AlignReadToByteBoundary();
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
			RakNetTimeMS timeMS; b->Read(timeMS); sourceSystemTime=(CCTimeType) timeMS;
#endif
			if (hasBAndAS)
			{
				//			b->Read(B);
				b->Read(AS);
			}
		}
		else
		{
			b->Read(isNAK);
			if (isNAK)
			{
				isPacketPair=false;
			}
			else
			{
				b->Read(isPacketPair);
				b->Read(isContinuousSend);
				b->Read(needsBAndAs);
				b->AlignReadToByteBoundary();
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
				RakNetTimeMS timeMS; b->Read(timeMS); sourceSystemTime=(CCTimeType) timeMS;
#endif
				b->Read(datagramNumber);
			}
		}
	}
};

#pragma warning(disable:4702)   // unreachable code

#ifdef _WIN32
//#define _DEBUG_LOGGER
#ifdef _DEBUG_LOGGER
#include "WindowsIncludes.h"
#endif
#endif

//#define DEBUG_SPLIT_PACKET_PROBLEMS
#if defined (DEBUG_SPLIT_PACKET_PROBLEMS)
static int waitFlag=-1;
#endif

using namespace RakNet;

int SplitPacketChannelComp( SplitPacketIdType const &key, SplitPacketChannel* const &data )
{
#if PREALLOCATE_LARGE_MESSAGES==1
	if (key < data->returnedPacket->splitPacketId)
		return -1;
	if (key == data->returnedPacket->splitPacketId)
		return 0;
#else
	if (key < data->splitPacketList[0]->splitPacketId)
		return -1;
	if (key == data->splitPacketList[0]->splitPacketId)
		return 0;
#endif
	return 1;
}

// DEFINE_MULTILIST_PTR_TO_MEMBER_COMPARISONS( InternalPacket, SplitPacketIndexType, splitPacketIndex )

bool operator<( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() < cls->splitPacketIndex;
}
bool operator>( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() > cls->splitPacketIndex;
}
bool operator==( const DataStructures::MLKeyRef<SplitPacketIndexType> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get() == cls->splitPacketIndex;
}
/// Semi-hack: This is necessary to call Sort()
bool operator<( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex < cls->splitPacketIndex;
}
bool operator>( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex > cls->splitPacketIndex;
}
bool operator==( const DataStructures::MLKeyRef<InternalPacket *> &inputKey, const InternalPacket *cls )
{
	return inputKey.Get()->splitPacketIndex == cls->splitPacketIndex;
}

int SplitPacketIndexComp( SplitPacketIndexType const &key, InternalPacket* const &data )
{
if (key < data->splitPacketIndex)
return -1;
if (key == data->splitPacketIndex)
return 0;
return 1;
}

//-------------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------------
// Add 21 to the default MTU so if we encrypt it can hold potentially 21 more bytes of extra data + padding.
ReliabilityLayer::ReliabilityLayer() :
updateBitStream( MAXIMUM_MTU_SIZE + 21 )   // preallocate the update bitstream so we can avoid a lot of reallocs at runtime
{
	freeThreadedMemoryOnNextUpdate = false;


#ifdef _DEBUG
	// Wait longer to disconnect in debug so I don't get disconnected while tracing
	timeoutTime=30000;
#else
	timeoutTime=10000;
#endif

#ifdef _DEBUG
	minExtraPing=extraPingVariance=0;
	packetloss=(double) minExtraPing;	
#endif

	InitializeVariables(MAXIMUM_MTU_SIZE);

	datagramHistoryMessagePool.SetPageSize(sizeof(MessageNumberNode)*128);
	internalPacketPool.SetPageSize(sizeof(InternalPacket)*128);
	refCountedDataPool.SetPageSize(sizeof(InternalPacket)*32);
}

//-------------------------------------------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::~ReliabilityLayer()
{
	FreeMemory( true ); // Free all memory immediately
}
//-------------------------------------------------------------------------------------------------------
// Resets the layer for reuse
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Reset( bool resetVariables, int MTUSize )
{
	FreeMemory( true ); // true because making a memory reset pending in the update cycle causes resets after reconnects.  Instead, just call Reset from a single thread
	if (resetVariables)
	{
		InitializeVariables(MTUSize);

		if ( encryptor.IsKeySet() )
			congestionManager.Init(RakNet::GetTimeUS(), MTUSize - UDP_HEADER_SIZE);
		else
			congestionManager.Init(RakNet::GetTimeUS(), MTUSize - UDP_HEADER_SIZE);
	}
}

//-------------------------------------------------------------------------------------------------------
// Sets up encryption
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetEncryptionKey( const unsigned char* key )
{
	if ( key )
		encryptor.SetKey( key );
	else
		encryptor.UnsetKey();
}

//-------------------------------------------------------------------------------------------------------
// Set the time, in MS, to use before considering ourselves disconnected after not being able to deliver a reliable packet
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetTimeoutTime( RakNetTimeMS time )
{
	timeoutTime=time;
}

//-------------------------------------------------------------------------------------------------------
// Returns the value passed to SetTimeoutTime. or the default if it was never called
//-------------------------------------------------------------------------------------------------------
RakNetTimeMS ReliabilityLayer::GetTimeoutTime(void)
{
	return timeoutTime;
}

//-------------------------------------------------------------------------------------------------------
// Initialize the variables
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InitializeVariables( int MTUSize )
{
	(void) MTUSize;

	memset( waitingForOrderedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType));
	memset( waitingForSequencedPacketReadIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForOrderedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( waitingForSequencedPacketWriteIndex, 0, NUMBER_OF_ORDERED_STREAMS * sizeof(OrderingIndexType) );
	memset( &statistics, 0, sizeof( statistics ) );
	statistics.connectionStartTime = RakNet::GetTimeUS();
	splitPacketId = 0;
	elapsedTimeSinceLastUpdate=0;
	throughputCapCountdown=0;
	sendReliableMessageNumberIndex = 0;
	internalOrderIndex=0;
	timeToNextUnreliableCull=0;
	unreliableLinkedListHead=0;
	lastUpdateTime= RakNet::GetTimeNS();
	bandwidthExceededStatistic=false;
	remoteSystemTime=0;
	unreliableTimeout=0;
	lastBpsClear=0;

	// Disable packet pairs
	countdownToNextPacketPair=15;

	nextAllowedThroughputSample=0;
	deadConnection = cheater = false;
	timeOfLastContinualSend=0;

	// timeResendQueueNonEmpty = 0;
	timeLastDatagramArrived=RakNet::GetTimeMS();
	//	packetlossThisSample=false;
	//	backoffThisSample=0;
	//	packetlossThisSampleResendCount=0;
	//	lastPacketlossTime=0;
	statistics.messagesInResendBuffer=0;
	statistics.bytesInResendBuffer=0;

	receivedPacketsBaseIndex=0;
	resetReceivedPackets=true;
	receivePacketCount=0; 

	//	SetPing( 1000 );

	timeBetweenPackets=STARTING_TIME_BETWEEN_PACKETS;

	ackPingIndex=0;
	ackPingSum=(CCTimeType)0;

	nextSendTime=lastUpdateTime;
	//nextLowestPingReset=(CCTimeType)0;
	//	continuousSend=false;

	//	histogramStart=(CCTimeType)0;
	//	histogramBitsSent=0;
	unacknowledgedBytes=0;
	resendLinkedListHead=0;
	totalUserDataBytesAcked=0;

	datagramHistoryPopCount=0;

	InitHeapWeights();
	for (int i=0; i < NUMBER_OF_PRIORITIES; i++)
	{
		statistics.messageInSendBuffer[i]=0;
		statistics.bytesInSendBuffer[i]=0.0;
	}

	for (int i=0; i < RNS_PER_SECOND_METRICS_COUNT; i++)
	{
		bpsMetrics[i].Reset(__FILE__,__LINE__);
	}
}

//-------------------------------------------------------------------------------------------------------
// Frees all allocated memory
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::FreeMemory( bool freeAllImmediately )
{
	if ( freeAllImmediately )
	{
		FreeThreadedMemory();
		FreeThreadSafeMemory();
	}
	else
	{
		FreeThreadSafeMemory();
		freeThreadedMemoryOnNextUpdate = true;
	}
}

void ReliabilityLayer::FreeThreadedMemory( void )
{
}

void ReliabilityLayer::FreeThreadSafeMemory( void )
{
	unsigned i,j;
	InternalPacket *internalPacket;

	ClearPacketsAndDatagrams(false);

	for (i=0; i < splitPacketChannelList.Size(); i++)
	{
		for (j=0; j < splitPacketChannelList[i]->splitPacketList.Size(); j++)
		{
			FreeInternalPacketData(splitPacketChannelList[i]->splitPacketList[j], __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( splitPacketChannelList[i]->splitPacketList[j] );
		}
#if PREALLOCATE_LARGE_MESSAGES==1
		if (splitPacketChannelList[i]->returnedPacket)
		{
			FreeInternalPacketData(splitPacketChannelList[i]->returnedPacket, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( splitPacketChannelList[i]->returnedPacket );
		}
#endif
		RakNet::OP_DELETE(splitPacketChannelList[i], __FILE__, __LINE__);
	}
	splitPacketChannelList.Clear(false, __FILE__, __LINE__);

	while ( outputQueue.Size() > 0 )
	{
		internalPacket = outputQueue.Pop();
		FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );
	}

	outputQueue.ClearAndForceAllocation( 32, __FILE__,__LINE__ );

	for ( i = 0; i < orderingList.Size(); i++ )
	{
		if ( orderingList[ i ] )
		{
			DataStructures::LinkedList<InternalPacket*>* theList = orderingList[ i ];

			if ( theList )
			{
				while ( theList->Size() )
				{
					internalPacket = orderingList[ i ]->Pop();
					FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
					ReleaseToInternalPacketPool( internalPacket );
				}

				RakNet::OP_DELETE(theList, __FILE__, __LINE__);
			}
		}
	}

	orderingList.Clear(false, __FILE__, __LINE__);

	//resendList.ForEachData(DeleteInternalPacket);
	//	resendTree.Clear(__FILE__, __LINE__);
	memset(resendBuffer, 0, sizeof(resendBuffer));
	statistics.messagesInResendBuffer=0;
	statistics.bytesInResendBuffer=0;

	if (resendLinkedListHead)
	{
		InternalPacket *prev;
		InternalPacket *iter = resendLinkedListHead;
		while (1)
		{
			if (iter->data)
				FreeInternalPacketData(iter, __FILE__, __LINE__ );
			prev=iter;
			iter=iter->resendNext;
			if (iter==resendLinkedListHead)
			{
				ReleaseToInternalPacketPool(prev);
				break;
			}
			ReleaseToInternalPacketPool(prev);
		}
		resendLinkedListHead=0;
	}
	unacknowledgedBytes=0;

	//	acknowlegements.Clear(__FILE__, __LINE__);

	for ( j=0 ; j < outgoingPacketBuffer.Size(); j++ )
	{
		if ( outgoingPacketBuffer[ j ]->data)
			FreeInternalPacketData( outgoingPacketBuffer[ j ], __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( outgoingPacketBuffer[ j ] );
	}

	outgoingPacketBuffer.Clear(true, __FILE__,__LINE__);


#ifdef _DEBUG
	for (unsigned i = 0; i < delayList.Size(); i++ )
		RakNet::OP_DELETE(delayList[ i ], __FILE__, __LINE__);
	delayList.Clear(__FILE__, __LINE__);
#endif

	packetsToSendThisUpdate.Clear(false, __FILE__, __LINE__);
	packetsToSendThisUpdate.Preallocate(512, __FILE__, __LINE__);
	packetsToDeallocThisUpdate.Clear(false, __FILE__, __LINE__);
	packetsToDeallocThisUpdate.Preallocate(512, __FILE__, __LINE__);
	packetsToSendThisUpdateDatagramBoundaries.Clear(false, __FILE__, __LINE__);
	packetsToSendThisUpdateDatagramBoundaries.Preallocate(128, __FILE__, __LINE__);
	datagramSizesInBytes.Clear(false, __FILE__, __LINE__);
	datagramSizesInBytes.Preallocate(128, __FILE__, __LINE__);

	internalPacketPool.Clear(__FILE__, __LINE__);

	refCountedDataPool.Clear(__FILE__,__LINE__);

	/*
	DataStructures::Page<DatagramSequenceNumberType, DatagramMessageIDList*, RESEND_TREE_ORDER> *cur = datagramMessageIDTree.GetListHead();
	while (cur)
	{
	int treeIndex;
	for (treeIndex=0; treeIndex < cur->size; treeIndex++)
	ReleaseToDatagramMessageIDPool(cur->data[treeIndex]);
	cur=cur->resendNext;
	}
	datagramMessageIDTree.Clear(__FILE__, __LINE__);
	datagramMessageIDPool.Clear(__FILE__, __LINE__);
	*/

	while (datagramHistory.Size())
	{
		RemoveFromDatagramHistory(datagramHistoryPopCount);
		datagramHistory.Pop();
		datagramHistoryPopCount++;
	}
	datagramHistoryMessagePool.Clear(__FILE__,__LINE__);
	datagramHistoryPopCount=0;

	acknowlegements.Clear();
	NAKs.Clear();

	unreliableLinkedListHead=0;
}

//-------------------------------------------------------------------------------------------------------
// Packets are read directly from the socket layer and skip the reliability
//layer  because unconnected players do not use the reliability layer
// This function takes packet data after a player has been confirmed as
//connected.  The game should not use that data directly
// because some data is used internally, such as packet acknowledgment and
//split packets
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::HandleSocketReceiveFromConnectedPlayer(
	const char *buffer, unsigned int length, SystemAddress systemAddress, DataStructures::List<PluginInterface2*> &messageHandlerList, int MTUSize,
	SOCKET s, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3, CCTimeType timeRead)
{
#ifdef _DEBUG
	RakAssert( !( buffer == 0 ) );
#endif

#if CC_TIME_TYPE_BYTES==4
	timeRead/=1000;
#endif

	bpsMetrics[(int) ACTUAL_BYTES_RECEIVED].Push1(timeRead,length);

	(void) MTUSize;

	if ( length <= 2 || buffer == 0 )   // Length of 1 is a connection request resend that we just ignore
	{
		for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
			messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("length <= 2 || buffer == 0", BYTES_TO_BITS(length), systemAddress);
		return true;
	}

	timeLastDatagramArrived=RakNet::GetTimeMS();

	//	CCTimeType time;
	bool indexFound;
	int count, size;
	DatagramSequenceNumberType holeCount;
	unsigned i;
	//	bool hasAcks=false;

	UpdateThreadedMemory();

	// decode this whole chunk if the decoder is defined.
	if ( encryptor.IsKeySet() )
	{
		if ( encryptor.Decrypt( ( unsigned char* ) buffer, length, ( unsigned char* ) buffer, &length ) == false )
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("Decryption failed", BYTES_TO_BITS(length), systemAddress);

			return false;
		}
	}
	RakNet::BitStream socketData( (unsigned char*) buffer, length, false ); // Convert the incoming data to a bitstream for easy parsing
	//	time = RakNet::GetTimeNS();

	// Set to the current time if it is not zero, and we get incoming data
	// 	if (timeResendQueueNonEmpty!=0)
	// 		timeResendQueueNonEmpty=timeRead;

	DatagramHeaderFormat dhf;
	dhf.Deserialize(&socketData);
	if (dhf.isValid==false)
	{
		for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
			messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("dhf.isValid==false", BYTES_TO_BITS(length), systemAddress);

		return true;
	}
	if (dhf.isACK)
	{
		DatagramSequenceNumberType datagramNumber;
		// datagramNumber=dhf.datagramNumber;

#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
		RakNetTimeMS timeMSLow=(RakNetTimeMS) timeRead&0xFFFFFFFF;
		CCTimeType rtt = timeMSLow-dhf.sourceSystemTime;
#if CC_TIME_TYPE_BYTES==4
		if (rtt > 10000)
#else
		if (rtt > 10000000)
#endif
		{
			// Sanity check. This could happen due to type overflow, especially since I only send the low 4 bytes to reduce bandwidth
			rtt=(CCTimeType) congestionManager.GetRTT();
		}
		//	RakAssert(rtt < 500000);
		//	printf("%i ", (RakNetTimeMS)(rtt/1000));
		ackPing=rtt;
#endif

#ifdef _DEBUG
		if (dhf.hasBAndAS==false)
		{
			//			dhf.B=0;
			dhf.AS=0;
		}
#endif
		//		congestionManager.OnAck(timeRead, rtt, dhf.hasBAndAS, dhf.B, dhf.AS, totalUserDataBytesAcked );


		incomingAcks.Clear();
		if (incomingAcks.Deserialize(&socketData)==false)
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("incomingAcks.Deserialize failed", BYTES_TO_BITS(length), systemAddress);

			return false;
		}
		for (i=0; i<incomingAcks.ranges.Size();i++)
		{
			if (incomingAcks.ranges[i].minIndex>incomingAcks.ranges[i].maxIndex)
			{
				RakAssert(incomingAcks.ranges[i].minIndex<=incomingAcks.ranges[i].maxIndex);

				for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
					messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("incomingAcks minIndex > maxIndex", BYTES_TO_BITS(length), systemAddress);
				return false;
			}
			for (datagramNumber=incomingAcks.ranges[i].minIndex; datagramNumber >= incomingAcks.ranges[i].minIndex && datagramNumber <= incomingAcks.ranges[i].maxIndex; datagramNumber++)
			{
	//			bool isReliable;
				MessageNumberNode *messageNumberNode = GetMessageNumberNodeByDatagramIndex(datagramNumber/*, &isReliable*/);
				if (messageNumberNode)
				{
				//	printf("%p Got ack for %i\n", this, datagramNumber.val);
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
					congestionManager.OnAck(timeRead, rtt, dhf.hasBAndAS, 0, dhf.AS, totalUserDataBytesAcked, bandwidthExceededStatistic, datagramNumber );
#else
					congestionManager.OnAck(timeRead, 0, dhf.hasBAndAS, 0, dhf.AS, totalUserDataBytesAcked, bandwidthExceededStatistic, datagramNumber );
#endif

					while (messageNumberNode)
					{
						RemovePacketFromResendListAndDeleteOlderReliableSequenced( messageNumberNode->messageNumber, timeRead, messageHandlerList, systemAddress );
						messageNumberNode=messageNumberNode->next;
					}

					RemoveFromDatagramHistory(datagramNumber);
				}
// 				else if (isReliable)
// 				{
// 					// Previously used slot, rather than empty unreliable slot
// 					printf("%p Ack %i is duplicate\n", this, datagramNumber.val);
// 
//  					congestionManager.OnDuplicateAck(timeRead, datagramNumber);
// 				}
			}
		}
	}
	else if (dhf.isNAK)
	{
		DatagramSequenceNumberType messageNumber;
		DataStructures::RangeList<DatagramSequenceNumberType> incomingNAKs;
		if (incomingNAKs.Deserialize(&socketData)==false)
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("incomingNAKs.Deserialize failed", BYTES_TO_BITS(length), systemAddress);			

			return false;
		}
		for (i=0; i<incomingNAKs.ranges.Size();i++)
		{
			if (incomingNAKs.ranges[i].minIndex>incomingNAKs.ranges[i].maxIndex)
			{
				RakAssert(incomingNAKs.ranges[i].minIndex<=incomingNAKs.ranges[i].maxIndex);

				for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
					messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("incomingNAKs minIndex>maxIndex", BYTES_TO_BITS(length), systemAddress);			

				return false;
			}
			// Sanity check
			RakAssert(incomingNAKs.ranges[i].maxIndex.val-incomingNAKs.ranges[i].minIndex.val<1000);
			for (messageNumber=incomingNAKs.ranges[i].minIndex; messageNumber >= incomingNAKs.ranges[i].minIndex && messageNumber <= incomingNAKs.ranges[i].maxIndex; messageNumber++)
			{
				congestionManager.OnNAK(timeRead, messageNumber);

				// REMOVEME
				//				printf("%p NAK %i\n", this, dhf.datagramNumber.val);

		//		bool isReliable;
				MessageNumberNode *messageNumberNode = GetMessageNumberNodeByDatagramIndex(messageNumber/*, &isReliable*/);
				while (messageNumberNode)
				{
					// Update timers so resends occur immediately
					InternalPacket *internalPacket = resendBuffer[messageNumberNode->messageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK];
					if (internalPacket)
					{
						if (internalPacket->nextActionTime!=0)
						{
							internalPacket->nextActionTime=timeRead;
						}
					}				

					messageNumberNode=messageNumberNode->next;
				}
			}
		}
	}
	else
	{
		uint32_t skippedMessageCount;
		if (!congestionManager.OnGotPacket(dhf.datagramNumber, dhf.isContinuousSend, timeRead, length, &skippedMessageCount))
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("congestionManager.OnGotPacket failed", BYTES_TO_BITS(length), systemAddress);			

			return true;
		}
		if (dhf.isPacketPair)
			congestionManager.OnGotPacketPair(dhf.datagramNumber, length, timeRead);

		DatagramHeaderFormat dhfNAK;
		dhfNAK.isNAK=true;
		uint32_t skippedMessageOffset;
		for (skippedMessageOffset=skippedMessageCount; skippedMessageOffset > 0; skippedMessageOffset--)
		{
			NAKs.Insert(dhf.datagramNumber-skippedMessageOffset);
		}
		remoteSystemNeedsBAndAS=dhf.needsBAndAs;

		// Ack dhf.datagramNumber
		// Ack even unreliable messages for congestion control, just don't resend them on no ack
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
		SendAcknowledgementPacket( dhf.datagramNumber, dhf.sourceSystemTime);
#else
		SendAcknowledgementPacket( dhf.datagramNumber, 0);
#endif

		InternalPacket* internalPacket = CreateInternalPacketFromBitStream( &socketData, timeRead );
		if (internalPacket==0)
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
				messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("CreateInternalPacketFromBitStream failed", BYTES_TO_BITS(length), systemAddress);			

			return true;
		}

		while ( internalPacket )
		{
			for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
			{
#if CC_TIME_TYPE_BYTES==4
				messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, receivePacketCount, systemAddress, timeRead, false);
#else
				messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, receivePacketCount, systemAddress, (RakNetTime)(timeRead/(CCTimeType)1000), false);
#endif
			}

			{

				// resetReceivedPackets is set from a non-threadsafe function.
				// We do the actual reset in this function so the data is not modified by multiple threads
				if (resetReceivedPackets)
				{
					hasReceivedPacketQueue.ClearAndForceAllocation(DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE, __FILE__,__LINE__);
					receivedPacketsBaseIndex=0;
					resetReceivedPackets=false;
				}

				// 8/12/09 was previously not checking if the message was reliable. However, on packetloss this would mean you'd eventually exceed the
				// hole count because unreliable messages were never resent, and you'd stop getting messages
				if (internalPacket->reliability == RELIABLE || internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == RELIABLE_ORDERED )
				{
					// If the following conditional is true then this either a duplicate packet
					// or an older out of order packet
					// The subtraction unsigned overflow is intentional
					holeCount = (DatagramSequenceNumberType)(internalPacket->reliableMessageNumber-receivedPacketsBaseIndex);
					const DatagramSequenceNumberType typeRange = (DatagramSequenceNumberType)(const uint32_t)-1;

					if (holeCount==(DatagramSequenceNumberType) 0)
					{
						// Got what we were expecting
						if (hasReceivedPacketQueue.Size())
							hasReceivedPacketQueue.Pop();
						++receivedPacketsBaseIndex;
					}
					else if (holeCount > typeRange/(DatagramSequenceNumberType) 2)
					{
						// Duplicate packet
						FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );

						bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}
					else if ((unsigned int) holeCount<hasReceivedPacketQueue.Size())
					{

						// Got a higher count out of order packet that was missing in the sequence or we already got
						if (hasReceivedPacketQueue[holeCount]!=false) // non-zero means this is a hole
						{
							// Fill in the hole
							hasReceivedPacketQueue[holeCount]=false; // We got the packet at holeCount
						}
						else
						{
							// Duplicate packet
							FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
							ReleaseToInternalPacketPool( internalPacket );

							bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

							goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
						}
					}
					else // holeCount>=receivedPackets.Size()
					{
						if (holeCount > (DatagramSequenceNumberType) 1000000)
						{
							RakAssert("Hole count too high. See ReliabilityLayer.h" && 0);

							for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
								messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("holeCount > 1000000", BYTES_TO_BITS(length), systemAddress);			

							// Would crash due to out of memory!
							FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
							ReleaseToInternalPacketPool( internalPacket );

							bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

							goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
						}


						// Fix - sending on a higher priority gives us a very very high received packets base index if we formerly had pre-split a lot of messages and
						// used that as the message number.  Because of this, a lot of time is spent in this linear loop and the timeout time expires because not
						// all of the message is sent in time.
						// Fixed by late assigning message IDs on the sender

						// Add 0 times to the queue until (reliableMessageNumber - baseIndex) < queue size.
						while ((unsigned int)(holeCount) > hasReceivedPacketQueue.Size())
							hasReceivedPacketQueue.Push(true, __FILE__, __LINE__ ); // time+(CCTimeType)60 * (CCTimeType)1000 * (CCTimeType)1000); // Didn't get this packet - set the time to give up waiting
						hasReceivedPacketQueue.Push(false, __FILE__, __LINE__ ); // Got the packet
#ifdef _DEBUG
						// If this assert hits then DatagramSequenceNumberType has overflowed
						RakAssert(hasReceivedPacketQueue.Size() < (unsigned int)((DatagramSequenceNumberType)(const uint32_t)(-1)));
#endif
					}

					while ( hasReceivedPacketQueue.Size()>0 && hasReceivedPacketQueue.Peek()==false )
					{
						hasReceivedPacketQueue.Pop();
						++receivedPacketsBaseIndex;
					}
				}

				// If the allocated buffer is > DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE and it is 3x greater than the number of elements actually being used
				if (hasReceivedPacketQueue.AllocationSize() > (unsigned int) DEFAULT_HAS_RECEIVED_PACKET_QUEUE_SIZE && hasReceivedPacketQueue.AllocationSize() > hasReceivedPacketQueue.Size() * 3)
					hasReceivedPacketQueue.Compress(__FILE__,__LINE__);

				if ( internalPacket->reliability == RELIABLE_SEQUENCED || internalPacket->reliability == UNRELIABLE_SEQUENCED )
				{
#ifdef _DEBUG
					RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

					if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
					{

						FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );

						for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
							messageHandlerList[messageHandlerIndex]->OnReliabilityLayerPacketError("internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS", BYTES_TO_BITS(length), systemAddress);			

						bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					if ( IsOlderOrderedPacket( internalPacket->orderingIndex, waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] ) == false )
					{
						// Is this a split packet?
						if ( internalPacket->splitPacketCount > 0 )
						{
							// Generate the split
							// Verify some parameters to make sure we don't get junk data


							// Check for a rebuilt packet
							InsertIntoSplitPacketList( internalPacket, timeRead );
							bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_PROCESSED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

							// Sequenced
							internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, timeRead,
								s, systemAddress, rnr, remotePortRakNetWasStartedOn_PS3);

							if ( internalPacket )
							{
								// Update our index to the newest packet
								waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + (OrderingIndexType)1;

								// If there is a rebuilt packet, add it to the output queue
								outputQueue.Push( internalPacket, __FILE__, __LINE__  );
								internalPacket = 0;
							}

							// else don't have all the parts yet
						}
						else
						{
							// Update our index to the newest packet
							waitingForSequencedPacketReadIndex[ internalPacket->orderingChannel ] = internalPacket->orderingIndex + (OrderingIndexType)1;

							// Not a split packet. Add the packet to the output queue
							bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_PROCESSED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));
							outputQueue.Push( internalPacket, __FILE__, __LINE__  );
							internalPacket = 0;
						}
					}
					else
					{
						// Older sequenced packet. Discard it
						FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );

						bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

					}

					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}

				// Is this an unsequenced split packet?
				if ( internalPacket->splitPacketCount > 0 )
				{
					// An unsequenced split packet.  May be ordered though.

					// Check for a rebuilt packet
					if ( internalPacket->reliability != RELIABLE_ORDERED )
						internalPacket->orderingChannel = 255; // Use 255 to designate not sequenced and not ordered

					InsertIntoSplitPacketList( internalPacket, timeRead );

					internalPacket = BuildPacketFromSplitPacketList( internalPacket->splitPacketId, timeRead,
						s, systemAddress, rnr, remotePortRakNetWasStartedOn_PS3);

					if ( internalPacket == 0 )
					{

						// Don't have all the parts yet
						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					// else continue down to handle RELIABLE_ORDERED

				}

				if ( internalPacket->reliability == RELIABLE_ORDERED )
				{
#ifdef _DEBUG
					RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

					if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
					{
						// Invalid packet
						FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
						ReleaseToInternalPacketPool( internalPacket );

						bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_IGNORED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

						goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
					}

					bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_PROCESSED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

					if ( waitingForOrderedPacketReadIndex[ internalPacket->orderingChannel ] == internalPacket->orderingIndex )
					{
						// Get the list to hold ordered packets for this stream
						DataStructures::LinkedList<InternalPacket*> *orderingListAtOrderingStream;
						unsigned char orderingChannelCopy = internalPacket->orderingChannel;

						// Push the packet for the user to read
						outputQueue.Push( internalPacket, __FILE__, __LINE__  );
						internalPacket = 0; // Don't reference this any longer since other threads access it

						// Wait for the resendNext ordered packet in sequence
						waitingForOrderedPacketReadIndex[ orderingChannelCopy ] ++; // This wraps

						orderingListAtOrderingStream = GetOrderingListAtOrderingStream( orderingChannelCopy );

						if ( orderingListAtOrderingStream != 0)
						{
							while ( orderingListAtOrderingStream->Size() > 0 )
							{
								// Cycle through the list until nothing is found
								orderingListAtOrderingStream->Beginning();
								indexFound=false;
								size=orderingListAtOrderingStream->Size();
								count=0;

								while (count++ < size)
								{
									if ( orderingListAtOrderingStream->Peek()->orderingIndex == waitingForOrderedPacketReadIndex[ orderingChannelCopy ] )
									{
										outputQueue.Push( orderingListAtOrderingStream->Pop(), __FILE__, __LINE__  );
										waitingForOrderedPacketReadIndex[ orderingChannelCopy ]++;
										indexFound=true;
									}
									else
										(*orderingListAtOrderingStream)++;
								}

								if (indexFound==false)
									break;
							}
						}
						internalPacket = 0;
					}
					else
					{
						// This is a newer ordered packet than we are waiting for. Store it for future use
						AddToOrderingList( internalPacket );
					}


					goto CONTINUE_SOCKET_DATA_PARSE_LOOP;
				}

				bpsMetrics[(int) USER_MESSAGE_BYTES_RECEIVED_PROCESSED].Push1(timeRead,BITS_TO_BYTES(internalPacket->dataBitLength));

				// Nothing special about this packet.  Add it to the output queue
				outputQueue.Push( internalPacket, __FILE__, __LINE__  );

				internalPacket = 0;
			}

			// Used for a goto to jump to the resendNext packet immediately

CONTINUE_SOCKET_DATA_PARSE_LOOP:
			// Parse the bitstream to create an internal packet
			internalPacket = CreateInternalPacketFromBitStream( &socketData, timeRead );
		}

	}


	receivePacketCount++;

	return true;
}

//-------------------------------------------------------------------------------------------------------
// This gets an end-user packet already parsed out. Returns number of BITS put into the buffer
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::Receive( unsigned char **data )
{
	// Wait until the clear occurs
	if (freeThreadedMemoryOnNextUpdate)
		return 0;

	InternalPacket * internalPacket;

	if ( outputQueue.Size() > 0 )
	{
		//  #ifdef _DEBUG
		//  RakAssert(bitStream->GetNumberOfBitsUsed()==0);
		//  #endif
		internalPacket = outputQueue.Pop();

		BitSize_t bitLength;
		*data = internalPacket->data;
		bitLength = internalPacket->dataBitLength;
		ReleaseToInternalPacketPool( internalPacket );
		return bitLength;
	}

	else
	{
		return 0;
	}

}

//-------------------------------------------------------------------------------------------------------
// Puts data on the send queue
// bitStream contains the data to send
// priority is what priority to send the data at
// reliability is what reliability to use
// ordering channel is from 0 to 255 and specifies what stream to use
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::Send( char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, unsigned char orderingChannel, bool makeDataCopy, int MTUSize, CCTimeType currentTime, uint32_t receipt )
{
#ifdef _DEBUG
	RakAssert( !( reliability >= NUMBER_OF_RELIABILITIES || reliability < 0 ) );
	RakAssert( !( priority > NUMBER_OF_PRIORITIES || priority < 0 ) );
	RakAssert( !( orderingChannel >= NUMBER_OF_ORDERED_STREAMS ) );
	RakAssert( numberOfBitsToSend > 0 );
#endif

#if CC_TIME_TYPE_BYTES==4
	currentTime/=1000;
#endif

	(void) MTUSize;

	//	int a = BITS_TO_BYTES(numberOfBitsToSend);

	// Fix any bad parameters
	if ( reliability > RELIABLE_ORDERED_WITH_ACK_RECEIPT || reliability < 0 )
		reliability = RELIABLE;

	if ( priority > NUMBER_OF_PRIORITIES || priority < 0 )
		priority = HIGH_PRIORITY;

	if ( orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
		orderingChannel = 0;

	unsigned int numberOfBytesToSend=(unsigned int) BITS_TO_BYTES(numberOfBitsToSend);
	if ( numberOfBitsToSend == 0 )
	{
		return false;
	}
	InternalPacket * internalPacket = AllocateFromInternalPacketPool();
	if (internalPacket==0)
	{
		notifyOutOfMemory(__FILE__, __LINE__);
		return false; // Out of memory
	}

	bpsMetrics[(int) USER_MESSAGE_BYTES_PUSHED].Push1(currentTime,numberOfBytesToSend);

	internalPacket->creationTime = currentTime;

	if ( makeDataCopy )
	{
		AllocInternalPacketData(internalPacket, numberOfBytesToSend, __FILE__, __LINE__ );
		//internalPacket->data = (unsigned char*) rakMalloc_Ex( numberOfBytesToSend, __FILE__, __LINE__ );
		memcpy( internalPacket->data, data, numberOfBytesToSend );
	}
	else
	{
		// Allocated the data elsewhere, delete it in here
		//internalPacket->data = ( unsigned char* ) data;
		AllocInternalPacketData(internalPacket, (unsigned char*) data );
	}

	internalPacket->dataBitLength = numberOfBitsToSend;
	internalPacket->messageInternalOrder = internalOrderIndex++;
	internalPacket->priority = priority;
	internalPacket->reliability = reliability;
	internalPacket->sendReceiptSerial=receipt;

	// Calculate if I need to split the packet
	//	int headerLength = BITS_TO_BYTES( GetMessageHeaderLengthBits( internalPacket, true ) );

	unsigned int maxDataSizeBytes = GetMaxDatagramSizeExcludingMessageHeaderBytes() - BITS_TO_BYTES(GetMaxMessageHeaderLengthBits());

	bool splitPacket = numberOfBytesToSend > maxDataSizeBytes;

	// If a split packet, we might have to upgrade the reliability
	if ( splitPacket )
	{
		// Split packets cannot be unreliable, in case that one part doesn't arrive and the whole cannot be reassembled.
		// One part could not arrive either due to packetloss or due to unreliable discard
		if (internalPacket->reliability==UNRELIABLE)
			internalPacket->reliability=RELIABLE;
		else if (internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT)
			internalPacket->reliability=RELIABLE_WITH_ACK_RECEIPT;
		else if (internalPacket->reliability==UNRELIABLE_SEQUENCED)
			internalPacket->reliability=RELIABLE_SEQUENCED;
//		else if (internalPacket->reliability==UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT)
//			internalPacket->reliability=RELIABLE_SEQUENCED_WITH_ACK_RECEIPT;
	}

	//	++sendMessageNumberIndex;

	if ( internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == UNRELIABLE_SEQUENCED
//		||
//		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
//		internalPacket->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
		)
	{
		// Assign the sequence stream and index
		internalPacket->orderingChannel = orderingChannel;
		internalPacket->orderingIndex = waitingForSequencedPacketWriteIndex[ orderingChannel ] ++;

		// This packet supersedes all other sequenced packets on the same ordering channel
		// Delete all packets in all send lists that are sequenced and on the same ordering channel
		// UPDATE:
		// Disabled.  We don't have enough info to consistently do this.  Sometimes newer data does supercede
		// older data such as with constantly declining health, but not in all cases.
		// For example, with sequenced unreliable sound packets just because you send a newer one doesn't mean you
		// don't need the older ones because the odds are they will still arrive in order
		/*
		for (int i=0; i < NUMBER_OF_PRIORITIES; i++)
		{
		DeleteSequencedPacketsInList(orderingChannel, sendQueue[i]);
		}
		*/
	}
	else if ( internalPacket->reliability == RELIABLE_ORDERED || internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT )
	{
		// Assign the ordering channel and index
		internalPacket->orderingChannel = orderingChannel;
		internalPacket->orderingIndex = waitingForOrderedPacketWriteIndex[ orderingChannel ] ++;
	}



	if ( splitPacket )   // If it uses a secure header it will be generated here
	{
		// Must split the packet.  This will also generate the SHA1 if it is required. It also adds it to the send list.
		//InternalPacket packetCopy;
		//memcpy(&packetCopy, internalPacket, sizeof(InternalPacket));
		//sendPacketSet[priority].CancelWriteLock(internalPacket);
		//SplitPacket( &packetCopy, MTUSize );
		SplitPacket( internalPacket );
		//RakNet::OP_DELETE_ARRAY(packetCopy.data, __FILE__, __LINE__);
		return true;
	}

	RakAssert(internalPacket->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
	AddToUnreliableLinkedList(internalPacket);

	RakAssert(internalPacket->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
	RakAssert(internalPacket->messageNumberAssigned==false);
	outgoingPacketBuffer.Push( GetNextWeight(internalPacket->priority), internalPacket, __FILE__, __LINE__  );
	RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
	statistics.messageInSendBuffer[(int)internalPacket->priority]++;
	statistics.bytesInSendBuffer[(int)internalPacket->priority]+=(double) BITS_TO_BYTES(internalPacket->dataBitLength);

	//	sendPacketSet[priority].WriteUnlock();
	return true;
}
//-------------------------------------------------------------------------------------------------------
// Run this once per game cycle.  Handles internal lists and actually does the send
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::Update( SOCKET s, SystemAddress systemAddress, int MTUSize, CCTimeType time,
							  unsigned bitsPerSecondLimit,
							  DataStructures::List<PluginInterface2*> &messageHandlerList,
							  RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)

{
	(void) bitsPerSecondLimit;
	(void) MTUSize;

	RakNetTimeMS timeMs;
#if CC_TIME_TYPE_BYTES==4
	time/=1000;
	timeMs=time;
#else
	timeMs=time/1000;
#endif

	//	int i;

#ifdef _DEBUG
	while (delayList.Size())
	{
		if (delayList.Peek()->sendTime <= timeMs)
		{
			DataAndTime *dat = delayList.Pop();
			SocketLayer::SendTo( dat->s, dat->data, dat->length, systemAddress.binaryAddress, systemAddress.port, dat->remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
			RakNet::OP_DELETE(dat,__FILE__,__LINE__);
		}
		break;
	}
#endif

	// This line is necessary because the timer isn't accurate
	if (time <= lastUpdateTime)
	{
		// Always set the last time in case of overflow
		lastUpdateTime=time;
		return;
	}

	CCTimeType timeSinceLastTick = time - lastUpdateTime;
	lastUpdateTime=time;
#if CC_TIME_TYPE_BYTES==4
	if (timeSinceLastTick>100)
		timeSinceLastTick=100;
#else
	if (timeSinceLastTick>100000)
		timeSinceLastTick=100000;
#endif

	if (unreliableTimeout>0)
	{
		if (timeSinceLastTick>=timeToNextUnreliableCull)
		{
			if (unreliableLinkedListHead)
			{
				// Cull out all unreliable messages that have exceeded the timeout
				InternalPacket *cur = unreliableLinkedListHead;
				InternalPacket *end = unreliableLinkedListHead->unreliablePrev;
				while (1)
				{
					if (time > cur->creationTime+(CCTimeType)unreliableTimeout)
					{
						// Flag invalid, and clear the memory. Still needs to be removed from the sendPacketSet later
						// This fixes a problem where a remote system disconnects, but we don't know it yet, and memory consumption increases to a huge value
						FreeInternalPacketData(cur, __FILE__, __LINE__ );
						cur->data=0;
						InternalPacket *next = cur->unreliableNext;
						RemoveFromUnreliableLinkedList(cur);

						if (cur==end)
							break;

						cur=next;
					}
					else
					{
						// 						if (cur==end)
						// 							break;
						// 
						// 						cur=cur->unreliableNext;

						// They should be inserted in-order, so no need to iterate past the first failure
						break;
					}
				}
			}

			timeToNextUnreliableCull=unreliableTimeout/(CCTimeType)2;
		}
		else
		{
			timeToNextUnreliableCull-=timeSinceLastTick;
		}
	}


	// Due to thread vagarities and the way I store the time to avoid slow calls to RakNet::GetTime
	// time may be less than lastAck
#if CC_TIME_TYPE_BYTES==4
	if ( statistics.messagesInResendBuffer!=0 && AckTimeout(time) )
#else
	if ( statistics.messagesInResendBuffer!=0 && AckTimeout(time/1000) )
#endif
	{
		// SHOW - dead connection
		// We've waited a very long time for a reliable packet to get an ack and it never has
		deadConnection = true;
		return;
	}

	if (congestionManager.ShouldSendACKs(time,timeSinceLastTick))
	{
		SendACKs(s, systemAddress, time, rnr, remotePortRakNetWasStartedOn_PS3);
	}

	if (NAKs.Size()>0)
	{
		updateBitStream.Reset();
		DatagramHeaderFormat dhfNAK;
		dhfNAK.isNAK=true;
		dhfNAK.isACK=false;
		dhfNAK.isPacketPair=false;
		dhfNAK.Serialize(&updateBitStream);
		NAKs.Serialize(&updateBitStream, GetMaxDatagramSizeExcludingMessageHeaderBits(), true);
		SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3, time );
	}

	DatagramHeaderFormat dhf;
	dhf.needsBAndAs=congestionManager.GetIsInSlowStart();
	dhf.isContinuousSend=bandwidthExceededStatistic;
	// 	bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
	// 		sendPacketSet[1].IsEmpty()==false ||
	// 		sendPacketSet[2].IsEmpty()==false ||
	// 		sendPacketSet[3].IsEmpty()==false;
	bandwidthExceededStatistic=outgoingPacketBuffer.Size()>0;

	const bool hasDataToSendOrResend = IsResendQueueEmpty()==false || bandwidthExceededStatistic;
	RakAssert(NUMBER_OF_PRIORITIES==4);
	congestionManager.Update(time, hasDataToSendOrResend);

	uint64_t actualBPS = bpsMetrics[(int) ACTUAL_BYTES_SENT].GetBPS1(time);
	statistics.BPSLimitByOutgoingBandwidthLimit = BITS_TO_BYTES(bitsPerSecondLimit);
	statistics.BPSLimitByCongestionControl = congestionManager.GetBytesPerSecondLimitByCongestionControl();
	if (statistics.BPSLimitByOutgoingBandwidthLimit > 0 && statistics.BPSLimitByOutgoingBandwidthLimit < actualBPS)
	{
		statistics.BPSLimitByOutgoingBandwidthLimit=true;
		return;
	}
	else
	{
		statistics.BPSLimitByOutgoingBandwidthLimit=false;
	}

	unsigned int i;
	if (time > lastBpsClear+
#if CC_TIME_TYPE_BYTES==4
		100
#else
		100000
#endif
		)
	{
		for (i=0; i < RNS_PER_SECOND_METRICS_COUNT; i++)
		{
			bpsMetrics[i].ClearExpired1(time);
		}

		lastBpsClear=time;
	}

	if (hasDataToSendOrResend==true)
	{
		InternalPacket *internalPacket;
		//		bool forceSend=false;
		bool pushedAnything;
		BitSize_t nextPacketBitLength;
		dhf.isACK=false;
		dhf.isNAK=false;
		dhf.hasBAndAS=false;
		ResetPacketsAndDatagrams();

		int transmissionBandwidth = congestionManager.GetTransmissionBandwidth(time, timeSinceLastTick, unacknowledgedBytes,dhf.isContinuousSend);
		int retransmissionBandwidth = congestionManager.GetRetransmissionBandwidth(time, timeSinceLastTick, unacknowledgedBytes,dhf.isContinuousSend);
		if (retransmissionBandwidth>0 || transmissionBandwidth>0)
		{
			statistics.isLimitedByCongestionControl=false;

			allDatagramSizesSoFar=0;

			// Keep filling datagrams until we exceed retransmission bandwidth
			while ((int)BITS_TO_BYTES(allDatagramSizesSoFar)<retransmissionBandwidth)
			{
				pushedAnything=false;

				// Fill one datagram, then break
				while ( IsResendQueueEmpty()==false )
				{
					internalPacket = resendLinkedListHead;
					RakAssert(internalPacket->messageNumberAssigned==true);

					if ( internalPacket->nextActionTime < time )
					{
						// If this is unreliable, then it was just in this list for a receipt. Don't actually resend, and remove from the list
						if (internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT
						//	|| internalPacket->reliability==UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
							)
						{
							PopListHead(false);

							InternalPacket *ackReceipt = AllocateFromInternalPacketPool();
							AllocInternalPacketData(ackReceipt, 5,  __FILE__, __LINE__ );
							ackReceipt->dataBitLength=BYTES_TO_BITS(5);
							ackReceipt->data[0]=(MessageID)ID_SND_RECEIPT_LOSS;
							ackReceipt->allocationScheme=InternalPacket::NORMAL;
							memcpy(ackReceipt->data+sizeof(MessageID), &internalPacket->sendReceiptSerial, sizeof(internalPacket->sendReceiptSerial));
							outputQueue.Push(ackReceipt, __FILE__, __LINE__ );

							statistics.messagesInResendBuffer--;
							statistics.bytesInResendBuffer-=BITS_TO_BYTES(internalPacket->dataBitLength);

							ReleaseToInternalPacketPool( internalPacket );

							resendBuffer[internalPacket->reliableMessageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK] = 0;
							

							continue;
						}

						nextPacketBitLength = internalPacket->headerLength + internalPacket->dataBitLength;
						if ( datagramSizeSoFar + nextPacketBitLength > GetMaxDatagramSizeExcludingMessageHeaderBits() )
						{
							// Gathers all PushPackets()
							PushDatagram();
							break;
						}

						PopListHead(false);

						CC_DEBUG_PRINTF_2("Rs %i ", internalPacket->reliableMessageNumber.val);

						bpsMetrics[(int) USER_MESSAGE_BYTES_RESENT].Push1(time,BITS_TO_BYTES(internalPacket->dataBitLength));
						PushPacket(time,internalPacket,true); // Affects GetNewTransmissionBandwidth()
						internalPacket->timesSent++;
						internalPacket->nextActionTime = congestionManager.GetRTOForRetransmission()+time;
#if CC_TIME_TYPE_BYTES==4
						if (internalPacket->nextActionTime-time > 10000)
#else
						if (internalPacket->nextActionTime-time > 10000000)
#endif
						{
							//							int a=5;
							RakAssert(0);
						}

						congestionManager.OnResend(time);

						pushedAnything=true;

						for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
						{
#if CC_TIME_TYPE_BYTES==4
							messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, packetsToSendThisUpdateDatagramBoundaries.Size()+congestionManager.GetNextDatagramSequenceNumber(), systemAddress, time, true);
#else
							messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, packetsToSendThisUpdateDatagramBoundaries.Size()+congestionManager.GetNextDatagramSequenceNumber(), systemAddress, (RakNetTime)(time/(CCTimeType)1000), true);
#endif
						}

						// Put the packet back into the resend list at the correct spot
						// Don't make a copy since I'm reinserting an allocated struct
						InsertPacketIntoResendList( internalPacket, time, false, false );

						// Removeme
						//						printf("Resend:%i ", internalPacket->reliableMessageNumber);
					}
					else
					{
						// Filled one datagram.
						// If the 2nd and it's time to send a datagram pair, will be marked as a pair
						PushDatagram();
						break;
					}
				}

				if (pushedAnything==false)
					break;
			}
		}
		else
		{
			statistics.isLimitedByCongestionControl=true;
		}

// 		CCTimeType timeSinceLastContinualSend;
// 		if (timeOfLastContinualSend!=0)
// 			timeSinceLastContinualSend=time-timeOfLastContinualSend;
// 		else
// 			timeSinceLastContinualSend=0;


		//RakAssert(bufferOverflow==false); // If asserts, buffer is too small. We are using a slot that wasn't acked yet
		if ((int)BITS_TO_BYTES(allDatagramSizesSoFar)<transmissionBandwidth)
		{
			//	printf("S+ ");
			allDatagramSizesSoFar=0;

			// Keep filling datagrams until we exceed retransmission bandwidth
			while (
				ResendBufferOverflow()==false &&
				((int)BITS_TO_BYTES(allDatagramSizesSoFar)<transmissionBandwidth ||
				// This condition means if we want to send a datagram pair, and only have one datagram buffered, exceed bandwidth to add another
				(countdownToNextPacketPair==0 &&
				datagramsToSendThisUpdateIsPair.Size()==1))
				)
			{
				// Fill with packets until MTU is reached
				//	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
				//	{
				pushedAnything=false;


				while (outgoingPacketBuffer.Size())
					//while ( sendPacketSet[ i ].Size() )
				{
					internalPacket=outgoingPacketBuffer.Peek();
					RakAssert(internalPacket->messageNumberAssigned==false);
					RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));

					// internalPacket = sendPacketSet[ i ].Peek();
					if (internalPacket->data==0)
					{
						//sendPacketSet[ i ].Pop();
						outgoingPacketBuffer.Pop(0);
						RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
						statistics.messageInSendBuffer[(int)internalPacket->priority]--;
						statistics.bytesInSendBuffer[(int)internalPacket->priority]-=(double) BITS_TO_BYTES(internalPacket->dataBitLength);
						ReleaseToInternalPacketPool( internalPacket );
						continue;
					}

					internalPacket->headerLength=GetMessageHeaderLengthBits(internalPacket);
					nextPacketBitLength = internalPacket->headerLength + internalPacket->dataBitLength;
					if ( datagramSizeSoFar + nextPacketBitLength > GetMaxDatagramSizeExcludingMessageHeaderBits() )
					{
						// Hit MTU. May still push packets if smaller ones exist at a lower priority
						RakAssert(internalPacket->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
						break;
					}

					bool isReliable;
					if ( internalPacket->reliability == RELIABLE ||
						internalPacket->reliability == RELIABLE_SEQUENCED ||
						internalPacket->reliability == RELIABLE_ORDERED ||
						internalPacket->reliability == RELIABLE_WITH_ACK_RECEIPT  ||
//						internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT  ||
						internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
						)
						isReliable = true;
					else
						isReliable = false;

					//sendPacketSet[ i ].Pop();
					outgoingPacketBuffer.Pop(0);
					RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
					RakAssert(internalPacket->messageNumberAssigned==false);
					statistics.messageInSendBuffer[(int)internalPacket->priority]--;
					statistics.bytesInSendBuffer[(int)internalPacket->priority]-=(double) BITS_TO_BYTES(internalPacket->dataBitLength);
					if (isReliable
						/*
						I thought about this and agree that UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT and RELIABLE_SEQUENCED_WITH_ACK_RECEIPT is not useful unless you also know if the message was discarded.

						The problem is that internally, message numbers are only assigned to reliable messages, because message numbers are only used to discard duplicate message receipt and only reliable messages get sent more than once. However, without message numbers getting assigned and transmitted, there is no way to tell the sender about which messages were discarded. In fact, in looking this over I realized that UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT introduced a bug, because the remote system assumes all message numbers are used (no holes). With that send type, on packetloss, a permanent hole would have been created which eventually would cause the system to discard all further packets.

						So I have two options. Either do not support ack receipts when sending sequenced, or write complex and major new systems. UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT would need to send the message ID number on a special channel which allows for non-delivery. And both of them would need to have a special range list to indicate which message numbers were not delivered, so when acks are sent that can be indicated as well. A further problem is that the ack itself can be lost - it is possible that the message can arrive but be discarded, yet the ack is lost. On resend, the resent message would be ignored as duplicate, and you'd never get the discard message either (unless I made a special buffer for that case too). 
*/
//						||
						// If needs an ack receipt, keep the internal packet around in the list
//						internalPacket->reliability == UNRELIABLE_WITH_ACK_RECEIPT ||
//						internalPacket->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
						)
					{
						internalPacket->messageNumberAssigned=true;
						internalPacket->reliableMessageNumber=sendReliableMessageNumberIndex;
						internalPacket->nextActionTime = congestionManager.GetRTOForRetransmission()+time;
#if CC_TIME_TYPE_BYTES==4
						const CCTimeType threshhold = 10000;
#else
						const CCTimeType threshhold = 10000000;
#endif
						if (internalPacket->nextActionTime-time > threshhold)
						{
							//								int a=5;
							RakAssert(time-internalPacket->nextActionTime < threshhold);
						}
						//resendTree.Insert( internalPacket->reliableMessageNumber, internalPacket);
						if (resendBuffer[internalPacket->reliableMessageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK]!=0)
						{
							//								bool overflow = ResendBufferOverflow();
							RakAssert(0);
						}
						resendBuffer[internalPacket->reliableMessageNumber & (uint32_t) RESEND_BUFFER_ARRAY_MASK] = internalPacket;
						statistics.messagesInResendBuffer++;
						statistics.bytesInResendBuffer+=BITS_TO_BYTES(internalPacket->dataBitLength);

						//		printf("pre:%i ", unacknowledgedBytes);

						InsertPacketIntoResendList( internalPacket, time, true, isReliable);


						//		printf("post:%i ", unacknowledgedBytes);
						sendReliableMessageNumberIndex++;
					}
					internalPacket->timesSent=1;
					// If isReliable is false, the packet and its contents will be added to a list to be freed in ClearPacketsAndDatagrams
					// However, the internalPacket structure will remain allocated and be in the resendBuffer list if it requires a receipt
					bpsMetrics[(int) USER_MESSAGE_BYTES_SENT].Push1(time,BITS_TO_BYTES(internalPacket->dataBitLength));
					PushPacket(time,internalPacket, isReliable);

					for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
					{
#if CC_TIME_TYPE_BYTES==4
						messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, packetsToSendThisUpdateDatagramBoundaries.Size()+congestionManager.GetNextDatagramSequenceNumber(), systemAddress, time, true);
#else
						messageHandlerList[messageHandlerIndex]->OnInternalPacket(internalPacket, packetsToSendThisUpdateDatagramBoundaries.Size()+congestionManager.GetNextDatagramSequenceNumber(), systemAddress, (RakNetTime)(time/(CCTimeType)1000), true);
#endif
					}
					pushedAnything=true;

					if (ResendBufferOverflow())
						break;
				}
				//	if (ResendBufferOverflow())
				//		break;
				//	}z

				// No datagrams pushed?
				if (datagramSizeSoFar==0)
					break;

				// Filled one datagram.
				// If the 2nd and it's time to send a datagram pair, will be marked as a pair
				PushDatagram();
			}
		}
		else
		{
			//	printf("S- ");

		}



		for (unsigned int datagramIndex=0; datagramIndex < packetsToSendThisUpdateDatagramBoundaries.Size(); datagramIndex++)
		{
			if (datagramIndex>0)
				dhf.isContinuousSend=true;
			MessageNumberNode* messageNumberNode = 0;
			dhf.datagramNumber=congestionManager.GetAndIncrementNextDatagramSequenceNumber();
			dhf.isPacketPair=datagramsToSendThisUpdateIsPair[datagramIndex];

			//printf("%p pushing datagram %i\n", this, dhf.datagramNumber.val);

			bool isSecondOfPacketPair=dhf.isPacketPair && datagramIndex>0 &&  datagramsToSendThisUpdateIsPair[datagramIndex-1];
			unsigned int msgIndex, msgTerm;
			if (datagramIndex==0)
			{
				msgIndex=0;
				msgTerm=packetsToSendThisUpdateDatagramBoundaries[0];
			}
			else
			{
				msgIndex=packetsToSendThisUpdateDatagramBoundaries[datagramIndex-1];
				msgTerm=packetsToSendThisUpdateDatagramBoundaries[datagramIndex];
			}

			// More accurate time to reset here
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
			dhf.sourceSystemTime=RakNet::GetTimeUS();
#endif
			updateBitStream.Reset();
			dhf.Serialize(&updateBitStream);
			CC_DEBUG_PRINTF_2("S%i ",dhf.datagramNumber.val);

			while (msgIndex < msgTerm)
			{
				// If reliable or needs receipt
				if ( packetsToSendThisUpdate[msgIndex]->reliability != UNRELIABLE &&
					packetsToSendThisUpdate[msgIndex]->reliability != UNRELIABLE_SEQUENCED
					)
				{
					if (messageNumberNode==0)
					{
						messageNumberNode = AddFirstToDatagramHistory(dhf.datagramNumber, packetsToSendThisUpdate[msgIndex]->reliableMessageNumber);
					}
					else
					{
						messageNumberNode = AddSubsequentToDatagramHistory(messageNumberNode, packetsToSendThisUpdate[msgIndex]->reliableMessageNumber);
					}
				}

				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
				WriteToBitStreamFromInternalPacket( &updateBitStream, packetsToSendThisUpdate[msgIndex], time );
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
				msgIndex++;
			}

			if (isSecondOfPacketPair)
			{
				// Pad to size of first datagram
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
				updateBitStream.PadWithZeroToByteLength(datagramSizesInBytes[datagramIndex-1]);
				RakAssert(updateBitStream.GetNumberOfBytesUsed()<=MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
			}

			if (messageNumberNode==0)
			{
				// Unreliable, add dummy node
				AddFirstToDatagramHistory(dhf.datagramNumber);
			}

			// Store what message ids were sent with this datagram
			//	datagramMessageIDTree.Insert(dhf.datagramNumber,idList);

			congestionManager.OnSendBytes(time,UDP_HEADER_SIZE+DatagramHeaderFormat::GetDataHeaderByteLength());

			SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3, time );

			bandwidthExceededStatistic=outgoingPacketBuffer.Size()>0;
			// 			bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
			// 				sendPacketSet[1].IsEmpty()==false ||
			// 				sendPacketSet[2].IsEmpty()==false ||
			// 				sendPacketSet[3].IsEmpty()==false;



			if (bandwidthExceededStatistic==true)
				timeOfLastContinualSend=time;
			else
				timeOfLastContinualSend=0;
		}

		ClearPacketsAndDatagrams(true);

		// Any data waiting to send after attempting to send, then bandwidth is exceeded
		bandwidthExceededStatistic=outgoingPacketBuffer.Size()>0;
		// 		bandwidthExceededStatistic=sendPacketSet[0].IsEmpty()==false ||
		// 			sendPacketSet[1].IsEmpty()==false ||
		// 			sendPacketSet[2].IsEmpty()==false ||
		// 			sendPacketSet[3].IsEmpty()==false;
	}


	// Keep on top of deleting old unreliable split packets so they don't clog the list.
	//DeleteOldUnreliableSplitPackets( time );
}

//-------------------------------------------------------------------------------------------------------
// Writes a bitstream to the socket
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendBitStream( SOCKET s, SystemAddress systemAddress, RakNet::BitStream *bitStream, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3, CCTimeType currentTime)
{
	(void) systemAddress;

	unsigned int length;


	if ( encryptor.IsKeySet() )
	{
		length = (unsigned int) bitStream->GetNumberOfBytesUsed();

		encryptor.Encrypt( ( unsigned char* ) bitStream->GetData(), length, ( unsigned char* ) bitStream->GetData(), &length, rnr );

		RakAssert( ( length % 16 ) == 0 );
	}
	else
	{
		length = (unsigned int) bitStream->GetNumberOfBytesUsed();
	}

#ifdef _DEBUG
	if (packetloss > 0.0)
	{
		if (frandomMT() < packetloss)
			return;
	}

	if (minExtraPing > 0 || extraPingVariance > 0)
	{
		RakNetTimeMS delay = minExtraPing;
		if (extraPingVariance>0)
			delay += (randomMT() % extraPingVariance);
		if (delay > 0)
		{
			DataAndTime *dat = RakNet::OP_NEW<DataAndTime>(__FILE__,__LINE__);
			memcpy(dat->data, ( char* ) bitStream->GetData(), length );
			dat->s=s;
			dat->length=length;
			dat->sendTime = RakNet::GetTimeMS() + delay;
			dat->remotePortRakNetWasStartedOn_PS3=remotePortRakNetWasStartedOn_PS3;
			for (unsigned int i=0; i < delayList.Size(); i++)
			{
				if (dat->sendTime < delayList[i]->sendTime)
				{
					delayList.PushAtHead(dat, i, __FILE__, __LINE__);
					dat=0;
					break;
				}
			}
			if (dat!=0)
				delayList.Push(dat,__FILE__,__LINE__);
			return;
		}
	}
#endif

	// Test packetloss
// 	if (frandomMT()<.1)
// 		return;

	//	printf("%i/%i\n", length,congestionManager.GetMTU());

	bpsMetrics[(int) ACTUAL_BYTES_SENT].Push1(currentTime,length);

	RakAssert(length <= congestionManager.GetMTU());
	SocketLayer::SendTo( s, ( char* ) bitStream->GetData(), length, systemAddress.binaryAddress, systemAddress.port, remotePortRakNetWasStartedOn_PS3, __FILE__, __LINE__  );
}

//-------------------------------------------------------------------------------------------------------
// Are we waiting for any data to be sent out or be processed by the player?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsOutgoingDataWaiting(void)
{
	if (outgoingPacketBuffer.Size()>0)
		return true;

	// 	unsigned i;
	// 	for ( i = 0; i < NUMBER_OF_PRIORITIES; i++ )
	// 	{
	// 		if (sendPacketSet[ i ].Size() > 0)
	// 			return true;
	// 	}

	return 
		//acknowlegements.Size() > 0 ||
		//resendTree.IsEmpty()==false;// || outputQueue.Size() > 0 || orderingList.Size() > 0 || splitPacketChannelList.Size() > 0;
		statistics.messagesInResendBuffer!=0;
}
bool ReliabilityLayer::AreAcksWaiting(void)
{
	return acknowlegements.Size() > 0;
}

//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ApplyNetworkSimulator( double _packetloss, RakNetTime _minExtraPing, RakNetTime _extraPingVariance )
{
#ifdef _DEBUG
	packetloss=_packetloss;
	minExtraPing=_minExtraPing;
	extraPingVariance=_extraPingVariance;
	//	if (ping < (unsigned int)(minExtraPing+extraPingVariance)*2)
	//		ping=(minExtraPing+extraPingVariance)*2;
#endif
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetSplitMessageProgressInterval(int interval)
{
	splitMessageProgressInterval=interval;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SetUnreliableTimeout(RakNetTimeMS timeoutMS)
{
#if CC_TIME_TYPE_BYTES==4
	unreliableTimeout=timeoutMS;
#else
	unreliableTimeout=(CCTimeType)timeoutMS*(CCTimeType)1000;
#endif
}

//-------------------------------------------------------------------------------------------------------
// This will return true if we should not send at this time
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsSendThrottled( int MTUSize )
{
	(void) MTUSize;

	return false;
	//	return resendList.Size() > windowSize;

	// Disabling this, because it can get stuck here forever
	/*
	unsigned packetsWaiting;
	unsigned resendListDataSize=0;
	unsigned i;
	for (i=0; i < resendList.Size(); i++)
	{
	if (resendList[i])
	resendListDataSize+=resendList[i]->dataBitLength;
	}
	packetsWaiting = 1 + ((BITS_TO_BYTES(resendListDataSize)) / (MTUSize - UDP_HEADER_SIZE - 10)); // 10 to roughly estimate the raknet header

	return packetsWaiting >= windowSize;
	*/
}

//-------------------------------------------------------------------------------------------------------
// We lost a packet
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateWindowFromPacketloss( CCTimeType time )
{
	(void) time;
}

//-------------------------------------------------------------------------------------------------------
// Increase the window size
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateWindowFromAck( CCTimeType time )
{
	(void) time;
}

//-------------------------------------------------------------------------------------------------------
// Does what the function name says
//-------------------------------------------------------------------------------------------------------
unsigned ReliabilityLayer::RemovePacketFromResendListAndDeleteOlderReliableSequenced( const MessageNumberType messageNumber, CCTimeType time, DataStructures::List<PluginInterface2*> &messageHandlerList, SystemAddress systemAddress )
{
	(void) time;
	(void) messageNumber;
	InternalPacket * internalPacket;
	//InternalPacket *temp;
//	PacketReliability reliability; // What type of reliability algorithm to use with this packet
//	unsigned char orderingChannel; // What ordering channel this packet is on, if the reliability type uses ordering channels
	OrderingIndexType orderingIndex; // The ID used as identification for ordering channels
	//	unsigned j;

	for (unsigned int messageHandlerIndex=0; messageHandlerIndex < messageHandlerList.Size(); messageHandlerIndex++)
	{
#if CC_TIME_TYPE_BYTES==4
		messageHandlerList[messageHandlerIndex]->OnAck(messageNumber, systemAddress, time);
#else
		messageHandlerList[messageHandlerIndex]->OnAck(messageNumber, systemAddress, (RakNetTime)(time/(CCTimeType)1000));
#endif
	}

	//	bool deleted;
	//	deleted=resendTree.Delete(messageNumber, internalPacket);
	internalPacket = resendBuffer[messageNumber & RESEND_BUFFER_ARRAY_MASK];
	// May ask to remove twice, for example resend twice, then second ack
	if (internalPacket && internalPacket->reliableMessageNumber==messageNumber)
	{
		ValidateResendList();
		resendBuffer[messageNumber & RESEND_BUFFER_ARRAY_MASK]=0;
		CC_DEBUG_PRINTF_2("AckRcv %i ", messageNumber);

		statistics.messagesInResendBuffer--;
		statistics.bytesInResendBuffer-=BITS_TO_BYTES(internalPacket->dataBitLength);

		orderingIndex = internalPacket->orderingIndex;
		totalUserDataBytesAcked+=(double) BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);

		// Return receipt if asked for
		if (internalPacket->reliability>=UNRELIABLE_WITH_ACK_RECEIPT && 
			(internalPacket->splitPacketCount==0 || internalPacket->splitPacketIndex+1==internalPacket->splitPacketCount)
			)
		{
			InternalPacket *ackReceipt = AllocateFromInternalPacketPool();
			AllocInternalPacketData(ackReceipt, 5,  __FILE__, __LINE__ );
			ackReceipt->dataBitLength=BYTES_TO_BITS(5);
			ackReceipt->data[0]=(MessageID)ID_SND_RECEIPT_ACKED;
			ackReceipt->allocationScheme=InternalPacket::NORMAL;
			memcpy(ackReceipt->data+sizeof(MessageID), &internalPacket->sendReceiptSerial, sizeof(internalPacket->sendReceiptSerial));
			outputQueue.Push(ackReceipt, __FILE__, __LINE__ );
		}

		bool isReliable;
		if ( internalPacket->reliability == RELIABLE ||
			internalPacket->reliability == RELIABLE_SEQUENCED ||
			internalPacket->reliability == RELIABLE_ORDERED ||
			internalPacket->reliability == RELIABLE_WITH_ACK_RECEIPT  ||
//			internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT  ||
			internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
			)
			isReliable = true;
		else
			isReliable = false;

		RemoveFromList(internalPacket, isReliable);
		FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );


		return 0;
	}
	else
	{

	}

	return (unsigned)-1;
}

//-------------------------------------------------------------------------------------------------------
// Acknowledge receipt of the packet with the specified messageNumber
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendAcknowledgementPacket( const DatagramSequenceNumberType messageNumber, CCTimeType time )
{

	// REMOVEME
	// printf("%p Send ack %i\n", this, messageNumber.val);

	nextAckTimeToSend=time;
	acknowlegements.Insert(messageNumber);

	//printf("ACK_DG:%i ", messageNumber.val);

	CC_DEBUG_PRINTF_2("AckPush %i ", messageNumber);

}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and figure out how many header bits would be
// written.  Returns that number
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::GetMaxMessageHeaderLengthBits( void )
{
	InternalPacket ip;
	ip.reliability=RELIABLE_ORDERED;
	ip.splitPacketCount=1;
	return GetMessageHeaderLengthBits(&ip);
}
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::GetMessageHeaderLengthBits( const InternalPacket *const internalPacket )
{	
	BitSize_t bitLength;

	//	bitStream->AlignWriteToByteBoundary(); // Potentially unaligned
	//	tempChar=(unsigned char)internalPacket->reliability; bitStream->WriteBits( (const unsigned char *)&tempChar, 3, true ); // 3 bits to write reliability.
	//	bool hasSplitPacket = internalPacket->splitPacketCount>0; bitStream->Write(hasSplitPacket); // Write 1 bit to indicate if splitPacketCount>0
	bitLength = 8*1;

	//	bitStream->AlignWriteToByteBoundary();
	//	RakAssert(internalPacket->dataBitLength < 65535);
	//	unsigned short s; s = (unsigned short) internalPacket->dataBitLength; bitStream->WriteAlignedVar16((const char*)& s);
	bitLength += 8*2;

	if ( internalPacket->reliability == RELIABLE ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED ||
		internalPacket->reliability == RELIABLE_WITH_ACK_RECEIPT ||
//		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
		bitLength += 8*3; // bitStream->Write(internalPacket->reliableMessageNumber); // Message sequence number
	// bitStream->AlignWriteToByteBoundary(); // Potentially nothing else to write
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED ||
//		internalPacket->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
//		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
	{
		bitLength += 8*3; // bitStream->Write(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED.
		bitLength += 8*1; // tempChar=internalPacket->orderingChannel; bitStream->WriteAlignedVar8((const char*)& tempChar); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, write one byte
	}
	if (internalPacket->splitPacketCount>0)
	{
		bitLength += 8*4; // bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketCount); RakAssert(sizeof(SplitPacketIndexType)==4); // Only needed if splitPacketCount>0. 4 bytes
		bitLength += 8*sizeof(SplitPacketIdType); // bitStream->WriteAlignedVar16((const char*)& internalPacket->splitPacketId); RakAssert(sizeof(SplitPacketIdType)==2); // Only needed if splitPacketCount>0.
		bitLength += 8*4; // bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
	}

	return bitLength;
}

//-------------------------------------------------------------------------------------------------------
// Parse an internalPacket and create a bitstream to represent this data
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::WriteToBitStreamFromInternalPacket( RakNet::BitStream *bitStream, const InternalPacket *const internalPacket, CCTimeType curTime )
{
	(void) curTime;

	BitSize_t start = bitStream->GetNumberOfBitsUsed();
	unsigned char tempChar;

	// (Incoming data may be all zeros due to padding)
	bitStream->AlignWriteToByteBoundary(); // Potentially unaligned
	if (internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT)
		tempChar=UNRELIABLE;
	else if (internalPacket->reliability==RELIABLE_WITH_ACK_RECEIPT)
		tempChar=RELIABLE;
	else if (internalPacket->reliability==RELIABLE_ORDERED_WITH_ACK_RECEIPT)
		tempChar=RELIABLE_ORDERED;
	else
		tempChar=(unsigned char)internalPacket->reliability; 
	bitStream->WriteBits( (const unsigned char *)&tempChar, 3, true ); // 3 bits to write reliability.

	bool hasSplitPacket = internalPacket->splitPacketCount>0; bitStream->Write(hasSplitPacket); // Write 1 bit to indicate if splitPacketCount>0
	bitStream->AlignWriteToByteBoundary();
	RakAssert(internalPacket->dataBitLength < 65535);
	unsigned short s; s = (unsigned short) internalPacket->dataBitLength; bitStream->WriteAlignedVar16((const char*)& s);
	if ( internalPacket->reliability == RELIABLE ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED ||
		internalPacket->reliability == RELIABLE_WITH_ACK_RECEIPT ||
//		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
		bitStream->Write(internalPacket->reliableMessageNumber); // Message sequence number
	bitStream->AlignWriteToByteBoundary(); // Potentially nothing else to write
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED ||
	//	internalPacket->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
	//	internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
	{
		bitStream->Write(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED.
		tempChar=internalPacket->orderingChannel; bitStream->WriteAlignedVar8((const char*)& tempChar); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, write one byte
	}
	if (internalPacket->splitPacketCount>0)
	{
		bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketCount); RakAssert(sizeof(SplitPacketIndexType)==4); // Only needed if splitPacketCount>0. 4 bytes
		bitStream->WriteAlignedVar16((const char*)& internalPacket->splitPacketId); RakAssert(sizeof(SplitPacketIdType)==2); // Only needed if splitPacketCount>0.
		bitStream->WriteAlignedVar32((const char*)& internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
	}

	// Write the actual data.
	bitStream->WriteAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );

	return bitStream->GetNumberOfBitsUsed() - start;
}

//-------------------------------------------------------------------------------------------------------
// Parse a bitstream and create an internal packet to represent this data
//-------------------------------------------------------------------------------------------------------
InternalPacket* ReliabilityLayer::CreateInternalPacketFromBitStream( RakNet::BitStream *bitStream, CCTimeType time )
{
	bool bitStreamSucceeded;
	InternalPacket* internalPacket;
	unsigned char tempChar;
	bool hasSplitPacket;
	bool readSuccess;

	if ( bitStream->GetNumberOfUnreadBits() < (int) sizeof( internalPacket->reliableMessageNumber ) * 8 )
		return 0; // leftover bits

	internalPacket = AllocateFromInternalPacketPool();
	if (internalPacket==0)
	{
		// Out of memory
		RakAssert(0);
		return 0;
	}
	internalPacket->creationTime = time;

	// (Incoming data may be all zeros due to padding)
	bitStream->AlignReadToByteBoundary(); // Potentially unaligned
	bitStream->ReadBits( ( unsigned char* ) ( &( tempChar ) ), 3 );
	internalPacket->reliability = ( const PacketReliability ) tempChar;
	readSuccess=bitStream->Read(hasSplitPacket); // Read 1 bit to indicate if splitPacketCount>0
	bitStream->AlignReadToByteBoundary();
	unsigned short s; bitStream->ReadAlignedVar16((char*)&s); internalPacket->dataBitLength=s; // Length of message (2 bytes)
	if ( internalPacket->reliability == RELIABLE ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED
		// I don't write ACK_RECEIPT to the remote system
// 		||
// 		internalPacket->reliability == RELIABLE_WITH_ACK_RECEIPT ||
// 		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
// 		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
		bitStream->Read(internalPacket->reliableMessageNumber); // Message sequence number
	else
		internalPacket->reliableMessageNumber=(MessageNumberType)(const uint32_t)-1;
	bitStream->AlignReadToByteBoundary(); // Potentially nothing else to Read
	if ( internalPacket->reliability == UNRELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_SEQUENCED ||
		internalPacket->reliability == RELIABLE_ORDERED
		// I don't write ACK_RECEIPT to the remote system
// 		||
// 		internalPacket->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
// 		internalPacket->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
// 		internalPacket->reliability == RELIABLE_ORDERED_WITH_ACK_RECEIPT
		)
	{
		bitStream->Read(internalPacket->orderingIndex); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 4 bytes.
		readSuccess=bitStream->ReadAlignedVar8((char*)& internalPacket->orderingChannel); // Used for UNRELIABLE_SEQUENCED, RELIABLE_SEQUENCED, RELIABLE_ORDERED. 5 bits needed, Read one byte
	}
	else
		internalPacket->orderingChannel=0;
	if (hasSplitPacket)
	{
		bitStream->ReadAlignedVar32((char*)& internalPacket->splitPacketCount); // Only needed if splitPacketCount>0. 4 bytes
		bitStream->ReadAlignedVar16((char*)& internalPacket->splitPacketId); // Only needed if splitPacketCount>0.
		readSuccess=bitStream->ReadAlignedVar32((char*)& internalPacket->splitPacketIndex); // Only needed if splitPacketCount>0. 4 bytes
		RakAssert(readSuccess);
	}
	else
	{
		internalPacket->splitPacketCount=0;
	}

	if (readSuccess==false ||
		internalPacket->dataBitLength==0 ||
		internalPacket->reliability>=NUMBER_OF_RELIABILITIES ||
		internalPacket->orderingChannel>=32 || 
		(hasSplitPacket && (internalPacket->splitPacketIndex >= internalPacket->splitPacketCount)))
	{
		// If this assert hits, encoding is garbage
		RakAssert(internalPacket->dataBitLength==0);
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	// Allocate memory to hold our data
	AllocInternalPacketData(internalPacket, BITS_TO_BYTES( internalPacket->dataBitLength ),  __FILE__, __LINE__ );
	RakAssert(BITS_TO_BYTES( internalPacket->dataBitLength )<MAXIMUM_MTU_SIZE);

	if (internalPacket->data == 0)
	{
		RakAssert("Out of memory in ReliabilityLayer::CreateInternalPacketFromBitStream" && 0);
		notifyOutOfMemory(__FILE__, __LINE__);
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	// Set the last byte to 0 so if ReadBits does not read a multiple of 8 the last bits are 0'ed out
	internalPacket->data[ BITS_TO_BYTES( internalPacket->dataBitLength ) - 1 ] = 0;

	// Read the data the packet holds
	bitStreamSucceeded = bitStream->ReadAlignedBytes( ( unsigned char* ) internalPacket->data, BITS_TO_BYTES( internalPacket->dataBitLength ) );

	if ( bitStreamSucceeded == false )
	{
		// If this hits, most likely the variable buff is too small in RunUpdateCycle in RakPeer.cpp
		RakAssert("Couldn't read all the data"  && 0);

		FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool( internalPacket );
		return 0;
	}

	return internalPacket;
}


//-------------------------------------------------------------------------------------------------------
// Get the SHA1 code
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::GetSHA1( unsigned char * const buffer, unsigned int
							   nbytes, char code[ SHA1_LENGTH ] )
{
	CSHA1 sha1;

	sha1.Reset();
	sha1.Update( ( unsigned char* ) buffer, nbytes );
	sha1.Final();
	memcpy( code, sha1.GetHash(), SHA1_LENGTH );
}

//-------------------------------------------------------------------------------------------------------
// Check the SHA1 code
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::CheckSHA1( char code[ SHA1_LENGTH ], unsigned char *
								 const buffer, unsigned int nbytes )
{
	char code2[ SHA1_LENGTH ];
	GetSHA1( buffer, nbytes, code2 );

	for ( int i = 0; i < SHA1_LENGTH; i++ )
		if ( code[ i ] != code2[ i ] )
			return false;

	return true;
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets on the specified ordering
// stream, optionally skipping those with splitPacketId, and delete them
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, DataStructures::List<InternalPacket*>&theList, int splitPacketId )
{
	unsigned i = 0;

	while ( i < theList.Size() )
	{
		if ( ( 
			theList[ i ]->reliability == RELIABLE_SEQUENCED ||
			theList[ i ]->reliability == UNRELIABLE_SEQUENCED 
//			||
//			theList[ i ]->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
//			theList[ i ]->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
			) &&
			theList[ i ]->orderingChannel == orderingChannel && ( splitPacketId == -1 || theList[ i ]->splitPacketId != (unsigned int) splitPacketId ) )
		{
			InternalPacket * internalPacket = theList[ i ];
			theList.RemoveAtIndex( i );
			FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( internalPacket );
		}

		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Search the specified list for sequenced packets with a value less than orderingIndex and delete them
// Note - I added functionality so you can use the Queue as a list (in this case for searching) but it is less efficient to do so than a regular list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::DeleteSequencedPacketsInList( unsigned char orderingChannel, DataStructures::Queue<InternalPacket*>&theList )
{
	InternalPacket * internalPacket;
	int listSize = theList.Size();
	int i = 0;

	while ( i < listSize )
	{
		if ( (
			theList[ i ]->reliability == RELIABLE_SEQUENCED ||
			theList[ i ]->reliability == UNRELIABLE_SEQUENCED
//			||
//			theList[ i ]->reliability == RELIABLE_SEQUENCED_WITH_ACK_RECEIPT ||
//			theList[ i ]->reliability == UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
			) && theList[ i ]->orderingChannel == orderingChannel )
		{
			internalPacket = theList[ i ];
			theList.RemoveAtIndex( i );
			FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
			ReleaseToInternalPacketPool( internalPacket );
			listSize--;
		}

		else
			i++;
	}
}

//-------------------------------------------------------------------------------------------------------
// Returns true if newPacketOrderingIndex is older than the waitingForPacketOrderingIndex
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsOlderOrderedPacket( OrderingIndexType newPacketOrderingIndex, OrderingIndexType waitingForPacketOrderingIndex )
{
	OrderingIndexType maxRange = (OrderingIndexType) (const uint32_t)-1;

	if ( waitingForPacketOrderingIndex > maxRange/(OrderingIndexType)2 )
	{
		if ( newPacketOrderingIndex >= waitingForPacketOrderingIndex - maxRange/(OrderingIndexType)2+(OrderingIndexType)1 && newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}
	}

	else
		if ( newPacketOrderingIndex >= ( OrderingIndexType ) ( waitingForPacketOrderingIndex - (( OrderingIndexType ) maxRange/(OrderingIndexType)2+(OrderingIndexType)1) ) ||
			newPacketOrderingIndex < waitingForPacketOrderingIndex )
		{
			return true;
		}

		// Old packet
		return false;
}

//-------------------------------------------------------------------------------------------------------
// Split the passed packet into chunks under MTU_SIZEbytes (including headers) and save those new chunks
// Optimized version
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SplitPacket( InternalPacket *internalPacket )
{
	// Doing all sizes in bytes in this function so I don't write partial bytes with split packets
	internalPacket->splitPacketCount = 1; // This causes GetMessageHeaderLengthBits to account for the split packet header
	unsigned int headerLength = (unsigned int) BITS_TO_BYTES( GetMessageHeaderLengthBits( internalPacket ) );
	unsigned int dataByteLength = (unsigned int) BITS_TO_BYTES( internalPacket->dataBitLength );
	int maximumSendBlockBytes, byteOffset, bytesToSend;
	SplitPacketIndexType splitPacketIndex;
	int i;
	InternalPacket **internalPacketArray;

	maximumSendBlockBytes = GetMaxDatagramSizeExcludingMessageHeaderBytes() - BITS_TO_BYTES(GetMaxMessageHeaderLengthBits());

	// Calculate how many packets we need to create
	internalPacket->splitPacketCount = ( ( dataByteLength - 1 ) / ( maximumSendBlockBytes ) + 1 );

	// Optimization
	// internalPacketArray = RakNet::OP_NEW<InternalPacket*>(internalPacket->splitPacketCount, __FILE__, __LINE__ );
	bool usedAlloca=false;

	if (sizeof( InternalPacket* ) * internalPacket->splitPacketCount < MAX_ALLOCA_STACK_ALLOCATION)
	{
		internalPacketArray = ( InternalPacket** ) alloca( sizeof( InternalPacket* ) * internalPacket->splitPacketCount );
		usedAlloca=true;
	}
	else

		internalPacketArray = (InternalPacket**) rakMalloc_Ex( sizeof(InternalPacket*) * internalPacket->splitPacketCount, __FILE__, __LINE__ );

	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		internalPacketArray[ i ] = AllocateFromInternalPacketPool();

		//internalPacketArray[ i ] = (InternalPacket*) alloca( sizeof( InternalPacket ) );
		//		internalPacketArray[ i ] = sendPacketSet[internalPacket->priority].WriteLock();
		*internalPacketArray[ i ]=*internalPacket;
		internalPacketArray[ i ]->messageNumberAssigned=false;

		if (i!=0)
			internalPacket->messageInternalOrder = internalOrderIndex++;
	}

	// This identifies which packet this is in the set
	splitPacketIndex = 0;

	InternalPacketRefCountedData *refCounter=0;

	// Do a loop to send out all the packets
	do
	{
		byteOffset = splitPacketIndex * maximumSendBlockBytes;
		bytesToSend = dataByteLength - byteOffset;

		if ( bytesToSend > maximumSendBlockBytes )
			bytesToSend = maximumSendBlockBytes;

		// Copy over our chunk of data

		AllocInternalPacketData(internalPacketArray[ splitPacketIndex ], &refCounter, internalPacket->data, internalPacket->data + byteOffset);
		//		internalPacketArray[ splitPacketIndex ]->data = (unsigned char*) rakMalloc_Ex( bytesToSend, __FILE__, __LINE__ );
		//		memcpy( internalPacketArray[ splitPacketIndex ]->data, internalPacket->data + byteOffset, bytesToSend );

		if ( bytesToSend != maximumSendBlockBytes )
			internalPacketArray[ splitPacketIndex ]->dataBitLength = internalPacket->dataBitLength - splitPacketIndex * ( maximumSendBlockBytes << 3 );
		else
			internalPacketArray[ splitPacketIndex ]->dataBitLength = bytesToSend << 3;

		internalPacketArray[ splitPacketIndex ]->splitPacketIndex = splitPacketIndex;
		internalPacketArray[ splitPacketIndex ]->splitPacketId = splitPacketId;
		internalPacketArray[ splitPacketIndex ]->splitPacketCount = internalPacket->splitPacketCount;
		RakAssert(internalPacketArray[ splitPacketIndex ]->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
	} while ( ++splitPacketIndex < internalPacket->splitPacketCount );

	splitPacketId++; // It's ok if this wraps to 0

	//	InternalPacket *workingPacket;

	// Tell the heap we are going to push a list of elements where each element in the list follows the heap order
	RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
	outgoingPacketBuffer.StartSeries();

	// Copy all the new packets into the split packet list
	for ( i = 0; i < ( int ) internalPacket->splitPacketCount; i++ )
	{
		internalPacketArray[ i ]->headerLength=headerLength;
		RakAssert(internalPacketArray[ i ]->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
		AddToUnreliableLinkedList(internalPacketArray[ i ]);
		//		sendPacketSet[ internalPacket->priority ].Push( internalPacketArray[ i ], __FILE__, __LINE__  );
		RakAssert(internalPacketArray[ i ]->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
		RakAssert(internalPacketArray[ i ]->messageNumberAssigned==false);
		outgoingPacketBuffer.PushSeries(GetNextWeight(internalPacketArray[ i ]->priority), internalPacketArray[ i ], __FILE__, __LINE__);
		RakAssert(outgoingPacketBuffer.Size()==0 || outgoingPacketBuffer.Peek()->dataBitLength<BYTES_TO_BITS(MAXIMUM_MTU_SIZE));
		statistics.messageInSendBuffer[(int)internalPacketArray[ i ]->priority]++;
		statistics.bytesInSendBuffer[(int)(int)internalPacketArray[ i ]->priority]+=(double) BITS_TO_BYTES(internalPacketArray[ i ]->dataBitLength);
		//		workingPacket=sendPacketSet[internalPacket->priority].WriteLock();
		//		memcpy(workingPacket, internalPacketArray[ i ], sizeof(InternalPacket));
		//		sendPacketSet[internalPacket->priority].WriteUnlock();
	}

	// Do not delete, original is referenced by all split packets to avoid numerous allocations. See AllocInternalPacketData above
	//	FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
	ReleaseToInternalPacketPool( internalPacket );

	if (usedAlloca==false)
		rakFree_Ex(internalPacketArray, __FILE__, __LINE__ );
}

//-------------------------------------------------------------------------------------------------------
// Insert a packet into the split packet list
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertIntoSplitPacketList( InternalPacket * internalPacket, CCTimeType time )
{
	bool objectExists;
	unsigned index;
	index=splitPacketChannelList.GetIndexFromKey(internalPacket->splitPacketId, &objectExists);
	if (objectExists==false)
	{
		SplitPacketChannel *newChannel = RakNet::OP_NEW<SplitPacketChannel>( __FILE__, __LINE__ );
#if PREALLOCATE_LARGE_MESSAGES==1
		index=splitPacketChannelList.Insert(internalPacket->splitPacketId, newChannel, true, __FILE__,__LINE__);
		newChannel->returnedPacket=CreateInternalPacketCopy( internalPacket, 0, 0, time );
		newChannel->gotFirstPacket=false;
		newChannel->splitPacketsArrived=0;
		AllocInternalPacketData(newChannel->returnedPacket, BITS_TO_BYTES( internalPacket->dataBitLength*internalPacket->splitPacketCount ),  __FILE__, __LINE__ );
		RakAssert(newChannel->returnedPacket->data);
#else
		newChannel->firstPacket=0;
		index=splitPacketChannelList.Insert(internalPacket->splitPacketId, newChannel, true, __FILE__,__LINE__);
		// Preallocate to the final size, to avoid runtime copies
		newChannel->splitPacketList.Preallocate(internalPacket->splitPacketCount, __FILE__,__LINE__);

#endif
	}

#if PREALLOCATE_LARGE_MESSAGES==1
	splitPacketChannelList[index]->lastUpdateTime=time;
	splitPacketChannelList[index]->splitPacketsArrived++;
	splitPacketChannelList[index]->returnedPacket->dataBitLength+=internalPacket->dataBitLength;

	bool dealloc;
	if (internalPacket->splitPacketIndex==0)
	{
		splitPacketChannelList[index]->gotFirstPacket=true;
		splitPacketChannelList[index]->stride=BITS_TO_BYTES(internalPacket->dataBitLength);

		for (unsigned int j=0; j < splitPacketChannelList[index]->splitPacketList.Size(); j++)
		{
			memcpy(splitPacketChannelList[index]->returnedPacket->data+internalPacket->splitPacketIndex*splitPacketChannelList[index]->stride, internalPacket->data, (size_t) BITS_TO_BYTES(internalPacket->dataBitLength));
			FreeInternalPacketData(splitPacketChannelList[index]->splitPacketList[j], __FILE__, __LINE__ );
			ReleaseToInternalPacketPool(splitPacketChannelList[index]->splitPacketList[j]);
		}

		memcpy(splitPacketChannelList[index]->returnedPacket->data, internalPacket->data, (size_t) BITS_TO_BYTES(internalPacket->dataBitLength));
		splitPacketChannelList[index]->splitPacketList.Clear(true,__FILE__,__LINE__);
		dealloc=true;
	}
	else
	{
		if (splitPacketChannelList[index]->gotFirstPacket==true)
		{
			memcpy(splitPacketChannelList[index]->returnedPacket->data+internalPacket->splitPacketIndex*splitPacketChannelList[index]->stride, internalPacket->data, (size_t) BITS_TO_BYTES(internalPacket->dataBitLength));
			dealloc=true;
		}
		else
		{
			splitPacketChannelList[index]->splitPacketList.Push(internalPacket,__FILE__,__LINE__);
			dealloc=false;
		}
	}

	if (splitPacketChannelList[index]->gotFirstPacket==true &&
		splitMessageProgressInterval &&
		// 		splitPacketChannelList[index]->firstPacket &&
		// 		splitPacketChannelList[index]->splitPacketList.Size()!=splitPacketChannelList[index]->firstPacket->splitPacketCount &&
		// 		(splitPacketChannelList[index]->splitPacketList.Size()%splitMessageProgressInterval)==0
		splitPacketChannelList[index]->gotFirstPacket &&
		splitPacketChannelList[index]->splitPacketsArrived!=splitPacketChannelList[index]->returnedPacket->splitPacketCount &&
		(splitPacketChannelList[index]->splitPacketsArrived%splitMessageProgressInterval)==0
		)
	{
		// Return ID_DOWNLOAD_PROGRESS
		// Write splitPacketIndex (SplitPacketIndexType)
		// Write splitPacketCount (SplitPacketIndexType)
		// Write byteLength (4)
		// Write data, splitPacketChannelList[index]->splitPacketList[0]->data
		InternalPacket *progressIndicator = AllocateFromInternalPacketPool();
		//		unsigned int len = sizeof(MessageID) + sizeof(unsigned int)*2 + sizeof(unsigned int) + (unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		unsigned int l = splitPacketChannelList[index]->stride;
		const unsigned int len = sizeof(MessageID) + sizeof(unsigned int)*2 + sizeof(unsigned int) + l;
		AllocInternalPacketData(progressIndicator, len,  __FILE__, __LINE__ );
		progressIndicator->dataBitLength=BYTES_TO_BITS(len);
		progressIndicator->data[0]=(MessageID)ID_DOWNLOAD_PROGRESS;
		progressIndicator->allocationScheme=InternalPacket::NORMAL;
		unsigned int temp;
		//	temp=splitPacketChannelList[index]->splitPacketList.Size();
		temp=splitPacketChannelList[index]->splitPacketsArrived;
		memcpy(progressIndicator->data+sizeof(MessageID), &temp, sizeof(unsigned int));
		temp=(unsigned int)internalPacket->splitPacketCount;
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*1, &temp, sizeof(unsigned int));
		//		temp=(unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		temp=(unsigned int) BITS_TO_BYTES(l);
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*2, &temp, sizeof(unsigned int));
		//memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*3, splitPacketChannelList[index]->firstPacket->data, (size_t) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength));
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*3, splitPacketChannelList[index]->returnedPacket->data, (size_t) BITS_TO_BYTES(l));
		outputQueue.Push(progressIndicator, __FILE__, __LINE__ );
	}

	if (dealloc)
	{
		FreeInternalPacketData(internalPacket, __FILE__, __LINE__ );
		ReleaseToInternalPacketPool(internalPacket);
	}
#else
	splitPacketChannelList[index]->splitPacketList.Insert(internalPacket, __FILE__, __LINE__ );
	splitPacketChannelList[index]->lastUpdateTime=time;

	if (internalPacket->splitPacketIndex==0)
		splitPacketChannelList[index]->firstPacket=internalPacket;
	
	if (splitMessageProgressInterval &&
		splitPacketChannelList[index]->firstPacket &&
		splitPacketChannelList[index]->splitPacketList.Size()!=splitPacketChannelList[index]->firstPacket->splitPacketCount &&
		(splitPacketChannelList[index]->splitPacketList.Size()%splitMessageProgressInterval)==0)
	{
		// Return ID_DOWNLOAD_PROGRESS
		// Write splitPacketIndex (SplitPacketIndexType)
		// Write splitPacketCount (SplitPacketIndexType)
		// Write byteLength (4)
		// Write data, splitPacketChannelList[index]->splitPacketList[0]->data
		InternalPacket *progressIndicator = AllocateFromInternalPacketPool();
		unsigned int length = sizeof(MessageID) + sizeof(unsigned int)*2 + sizeof(unsigned int) + (unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		AllocInternalPacketData(progressIndicator, length,  __FILE__, __LINE__ );
		progressIndicator->dataBitLength=BYTES_TO_BITS(length);
		progressIndicator->data[0]=(MessageID)ID_DOWNLOAD_PROGRESS;
		progressIndicator->allocationScheme=InternalPacket::NORMAL;
		unsigned int temp;
		temp=splitPacketChannelList[index]->splitPacketList.Size();
		memcpy(progressIndicator->data+sizeof(MessageID), &temp, sizeof(unsigned int));
		temp=(unsigned int)internalPacket->splitPacketCount;
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*1, &temp, sizeof(unsigned int));
		temp=(unsigned int) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength);
		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*2, &temp, sizeof(unsigned int));

		memcpy(progressIndicator->data+sizeof(MessageID)+sizeof(unsigned int)*3, splitPacketChannelList[index]->firstPacket->data, (size_t) BITS_TO_BYTES(splitPacketChannelList[index]->firstPacket->dataBitLength));
		outputQueue.Push(progressIndicator, __FILE__, __LINE__ );
	}

#endif
}

//-------------------------------------------------------------------------------------------------------
// Take all split chunks with the specified splitPacketId and try to
//reconstruct a packet.  If we can, allocate and return it.  Otherwise return 0
// Optimized version
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::BuildPacketFromSplitPacketList( SplitPacketChannel *splitPacketChannel, CCTimeType time )
{
#if PREALLOCATE_LARGE_MESSAGES==1
	InternalPacket *returnedPacket=splitPacketChannel->returnedPacket;
	RakNet::OP_DELETE(splitPacketChannel, __FILE__, __LINE__);
	(void) time;
	return returnedPacket;
#else
	unsigned int j;
	InternalPacket * internalPacket, *splitPacket;
	int splitPacketPartLength;

	// Reconstruct
	internalPacket = CreateInternalPacketCopy( splitPacketChannel->splitPacketList[0], 0, 0, time );
	internalPacket->dataBitLength=0;
	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
		internalPacket->dataBitLength+=splitPacketChannel->splitPacketList[j]->dataBitLength;
	splitPacketPartLength=BITS_TO_BYTES(splitPacketChannel->firstPacket->dataBitLength);

	internalPacket->data = (unsigned char*) rakMalloc_Ex( (size_t) BITS_TO_BYTES( internalPacket->dataBitLength ), __FILE__, __LINE__ );

	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
	{
		splitPacket=splitPacketChannel->splitPacketList[j];
		memcpy(internalPacket->data+splitPacket->splitPacketIndex*splitPacketPartLength, splitPacket->data, (size_t) BITS_TO_BYTES(splitPacketChannel->splitPacketList[j]->dataBitLength));
	}

	for (j=0; j < splitPacketChannel->splitPacketList.Size(); j++)
	{
		FreeInternalPacketData(splitPacketChannel->splitPacketList[j], __FILE__, __LINE__ );
		ReleaseToInternalPacketPool(splitPacketChannel->splitPacketList[j]);
	}
	RakNet::OP_DELETE(splitPacketChannel, __FILE__, __LINE__);

	return internalPacket;
#endif
}
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::BuildPacketFromSplitPacketList( SplitPacketIdType splitPacketId, CCTimeType time,
																  SOCKET s, SystemAddress systemAddress, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)
{
	unsigned int i;
	bool objectExists;
	SplitPacketChannel *splitPacketChannel;
	InternalPacket * internalPacket;

	i=splitPacketChannelList.GetIndexFromKey(splitPacketId, &objectExists);
	splitPacketChannel=splitPacketChannelList[i];
	
#if PREALLOCATE_LARGE_MESSAGES==1
	if (splitPacketChannel->splitPacketsArrived==splitPacketChannel->returnedPacket->splitPacketCount)
#else
	if (splitPacketChannel->splitPacketList.Size()==splitPacketChannel->splitPacketList[0]->splitPacketCount)
#endif
	{
		// Ack immediately, because for large files this can take a long time
		SendACKs(s, systemAddress, time, rnr, remotePortRakNetWasStartedOn_PS3);
		internalPacket=BuildPacketFromSplitPacketList(splitPacketChannel,time);
		splitPacketChannelList.RemoveAtIndex(i);
		return internalPacket;
	}
	else
	{
		return 0;
	}
}
/*
//-------------------------------------------------------------------------------------------------------
// Delete any unreliable split packets that have long since expired
void ReliabilityLayer::DeleteOldUnreliableSplitPackets( CCTimeType time )
{
unsigned i,j;
i=0;
while (i < splitPacketChannelList.Size())
{
#if CC_TIME_TYPE_BYTES==4
if (time > splitPacketChannelList[i]->lastUpdateTime + timeoutTime &&
#else
if (time > splitPacketChannelList[i]->lastUpdateTime + (CCTimeType)timeoutTime*(CCTimeType)1000 &&
#endif
(splitPacketChannelList[i]->splitPacketList[0]->reliability==UNRELIABLE || splitPacketChannelList[i]->splitPacketList[0]->reliability==UNRELIABLE_SEQUENCED))
{
for (j=0; j < splitPacketChannelList[i]->splitPacketList.Size(); j++)
{
RakNet::OP_DELETE_ARRAY(splitPacketChannelList[i]->splitPacketList[j]->data, __FILE__, __LINE__);
ReleaseToInternalPacketPool(splitPacketChannelList[i]->splitPacketList[j]);
}
RakNet::OP_DELETE(splitPacketChannelList[i], __FILE__, __LINE__);
splitPacketChannelList.RemoveAtIndex(i);
}
else
i++;
}
}
*/

//-------------------------------------------------------------------------------------------------------
// Creates a copy of the specified internal packet with data copied from the original starting at dataByteOffset for dataByteLength bytes.
// Does not copy any split data parameters as that information is always generated does not have any reason to be copied
//-------------------------------------------------------------------------------------------------------
InternalPacket * ReliabilityLayer::CreateInternalPacketCopy( InternalPacket *original, int dataByteOffset, int dataByteLength, CCTimeType time )
{
	InternalPacket * copy = AllocateFromInternalPacketPool();
#ifdef _DEBUG
	// Remove accessing undefined memory error
	memset( copy, 255, sizeof( InternalPacket ) );
#endif
	// Copy over our chunk of data

	if ( dataByteLength > 0 )
	{
		AllocInternalPacketData(copy, BITS_TO_BYTES(dataByteLength ),  __FILE__, __LINE__ );
		memcpy( copy->data, original->data + dataByteOffset, dataByteLength );
	}
	else
		copy->data = 0;

	copy->dataBitLength = dataByteLength << 3;
	copy->creationTime = time;
	copy->nextActionTime = 0;
	copy->orderingIndex = original->orderingIndex;
	copy->orderingChannel = original->orderingChannel;
	copy->reliableMessageNumber = original->reliableMessageNumber;
	copy->priority = original->priority;
	copy->reliability = original->reliability;
#if PREALLOCATE_LARGE_MESSAGES==1
	copy->splitPacketCount = original->splitPacketCount;
	copy->splitPacketId = original->splitPacketId;
	copy->splitPacketIndex = original->splitPacketIndex;
#endif

	return copy;
}

//-------------------------------------------------------------------------------------------------------
// Get the specified ordering list
//-------------------------------------------------------------------------------------------------------
DataStructures::LinkedList<InternalPacket*> *ReliabilityLayer::GetOrderingListAtOrderingStream( unsigned char orderingChannel )
{
	if ( orderingChannel >= orderingList.Size() )
		return 0;

	return orderingList[ orderingChannel ];
}

//-------------------------------------------------------------------------------------------------------
// Add the internal packet to the ordering list in order based on order index
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToOrderingList( InternalPacket * internalPacket )
{
#ifdef _DEBUG
	RakAssert( internalPacket->orderingChannel < NUMBER_OF_ORDERED_STREAMS );
#endif

	if ( internalPacket->orderingChannel >= NUMBER_OF_ORDERED_STREAMS )
	{
		return;
	}

	DataStructures::LinkedList<InternalPacket*> *theList;

	if ( internalPacket->orderingChannel >= orderingList.Size() || orderingList[ internalPacket->orderingChannel ] == 0 )
	{
		// Need a linked list in this index
		orderingList.Replace( RakNet::OP_NEW<DataStructures::LinkedList<InternalPacket*> >(__FILE__,__LINE__) , 0, internalPacket->orderingChannel, __FILE__,__LINE__ );
		theList=orderingList[ internalPacket->orderingChannel ];
	}
	else
	{
		// Have a linked list in this index
		if ( orderingList[ internalPacket->orderingChannel ]->Size() == 0 )
		{
			theList=orderingList[ internalPacket->orderingChannel ];
		}
		else
		{
			theList = GetOrderingListAtOrderingStream( internalPacket->orderingChannel );
		}
	}

	theList->End();
	theList->Add(internalPacket);
}

//-------------------------------------------------------------------------------------------------------
// Inserts a packet into the resend list in order
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InsertPacketIntoResendList( InternalPacket *internalPacket, CCTimeType time, bool firstResend, bool modifyUnacknowledgedBytes )
{
	(void) firstResend;
	(void) time;
	(void) internalPacket;

	AddToListTail(internalPacket, modifyUnacknowledgedBytes);
	RakAssert(internalPacket->nextActionTime!=0);

}

//-------------------------------------------------------------------------------------------------------
// If Read returns -1 and this returns true then a modified packet was detected
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsCheater( void ) const
{
	return cheater;
}

//-------------------------------------------------------------------------------------------------------
//  Were you ever unable to deliver a packet despite retries?
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsDeadConnection( void ) const
{
	return deadConnection;
}

//-------------------------------------------------------------------------------------------------------
//  Causes IsDeadConnection to return true
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::KillConnection( void )
{
	deadConnection=true;
}


//-------------------------------------------------------------------------------------------------------
// Statistics
//-------------------------------------------------------------------------------------------------------
RakNetStatistics * const ReliabilityLayer::GetStatistics( RakNetStatistics *rns )
{
	unsigned i;
	RakNetTimeUS time = RakNet::GetTimeUS();
	uint64_t uint64Denominator;
	double doubleDenominator;

	for (i=0; i < RNS_PER_SECOND_METRICS_COUNT; i++)
	{
		statistics.valueOverLastSecond[i]=bpsMetrics[i].GetBPS1Threadsafe(time);
		statistics.runningTotal[i]=bpsMetrics[i].GetTotal1();
	}

	memcpy(rns, &statistics, sizeof(statistics));

	if (rns->valueOverLastSecond[USER_MESSAGE_BYTES_SENT]+rns->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT]>0)
		rns->packetlossLastSecond=(float)((double) rns->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT]/((double) rns->valueOverLastSecond[USER_MESSAGE_BYTES_SENT]+(double) rns->valueOverLastSecond[USER_MESSAGE_BYTES_RESENT]));
	else
		rns->packetlossLastSecond=0.0f;

	rns->packetlossTotal=0.0f;
	uint64Denominator=(rns->runningTotal[USER_MESSAGE_BYTES_SENT]+rns->runningTotal[USER_MESSAGE_BYTES_RESENT]);
	if (uint64Denominator!=0&&rns->runningTotal[USER_MESSAGE_BYTES_SENT]/uint64Denominator>0)
	{
		doubleDenominator=((double) rns->runningTotal[USER_MESSAGE_BYTES_SENT]+(double) rns->runningTotal[USER_MESSAGE_BYTES_RESENT]);
		if(doubleDenominator!=0)
		{
			rns->packetlossTotal=(float)((double) rns->runningTotal[USER_MESSAGE_BYTES_RESENT]/doubleDenominator);
		}
	}

	return rns;
}

//-------------------------------------------------------------------------------------------------------
// Returns the number of packets in the resend queue, not counting holes
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::GetResendListDataSize(void) const
{
	// Not accurate but thread-safe.  The commented version might crash if the queue is cleared while we loop through it
	// return resendTree.Size();
	return statistics.messagesInResendBuffer;
}

//-------------------------------------------------------------------------------------------------------
// Process threaded commands
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::UpdateThreadedMemory(void)
{
	if ( freeThreadedMemoryOnNextUpdate )
	{
		freeThreadedMemoryOnNextUpdate = false;
		FreeThreadedMemory();
	}
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::AckTimeout(RakNetTimeMS curTime)
{
	// I check timeLastDatagramArrived-curTime because with threading it is possible that timeLastDatagramArrived is
	// slightly greater than curTime, in which case this is NOT an ack timeout
	return (timeLastDatagramArrived-curTime)>10000 && curTime-timeLastDatagramArrived>timeoutTime;
}
//-------------------------------------------------------------------------------------------------------
CCTimeType ReliabilityLayer::GetNextSendTime(void) const
{
	return nextSendTime;
}
//-------------------------------------------------------------------------------------------------------
CCTimeType ReliabilityLayer::GetTimeBetweenPackets(void) const
{
	return timeBetweenPackets;
}
//-------------------------------------------------------------------------------------------------------
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
CCTimeType ReliabilityLayer::GetAckPing(void) const
{
	return ackPing;
}
#endif
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::OnExternalPing(double pingMS)
{
	congestionManager.OnExternalPing(pingMS);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ResetPacketsAndDatagrams(void)
{
	packetsToSendThisUpdate.Clear(true, __FILE__, __LINE__);
	packetsToDeallocThisUpdate.Clear(true, __FILE__, __LINE__);
	packetsToSendThisUpdateDatagramBoundaries.Clear(true, __FILE__, __LINE__);
	datagramsToSendThisUpdateIsPair.Clear(true, __FILE__, __LINE__);
	datagramSizesInBytes.Clear(true, __FILE__, __LINE__);
	datagramSizeSoFar=0;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PushPacket(CCTimeType time, InternalPacket *internalPacket, bool isReliable)
{
	BitSize_t bitsForThisPacket=BYTES_TO_BITS(BITS_TO_BYTES(internalPacket->dataBitLength)+BITS_TO_BYTES(internalPacket->headerLength));
	datagramSizeSoFar+=bitsForThisPacket;
	RakAssert(BITS_TO_BYTES(datagramSizeSoFar)<MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
	allDatagramSizesSoFar+=bitsForThisPacket;
	packetsToSendThisUpdate.Push(internalPacket, __FILE__, __LINE__ );
	packetsToDeallocThisUpdate.Push(isReliable==false, __FILE__, __LINE__ );
	RakAssert(internalPacket->headerLength==GetMessageHeaderLengthBits(internalPacket));
// This code tells me how much time elapses between when you send, and when the message actually goes out
// 	if (internalPacket->data[0]==0)
// 	{
// 		RakNetTime t;
// 		RakNet::BitStream bs(internalPacket->data+1,sizeof(t),false);
// 		bs.Read(t);
// 		RakNetTime curTime=RakNet::GetTime();
// 		RakNetTime diff = curTime-t;
// 	}

	congestionManager.OnSendBytes(time, BITS_TO_BYTES(internalPacket->dataBitLength)+BITS_TO_BYTES(internalPacket->headerLength));
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PushDatagram(void)
{
	if (datagramSizeSoFar>0)
	{
		packetsToSendThisUpdateDatagramBoundaries.Push(packetsToSendThisUpdate.Size(), __FILE__, __LINE__ );
		datagramsToSendThisUpdateIsPair.Push(false, __FILE__, __LINE__ );
		RakAssert(BITS_TO_BYTES(datagramSizeSoFar)<MAXIMUM_MTU_SIZE-UDP_HEADER_SIZE);
		datagramSizesInBytes.Push(BITS_TO_BYTES(datagramSizeSoFar), __FILE__, __LINE__ );
		datagramSizeSoFar=0;

		// Disable packet pairs
		/*
		if (countdownToNextPacketPair==0)
		{
		if (TagMostRecentPushAsSecondOfPacketPair())
		countdownToNextPacketPair=15;
		}
		else
		countdownToNextPacketPair--;
		*/
	}
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::TagMostRecentPushAsSecondOfPacketPair(void)
{
	if (datagramsToSendThisUpdateIsPair.Size()>=2)
	{
		datagramsToSendThisUpdateIsPair[datagramsToSendThisUpdateIsPair.Size()-2]=true;
		datagramsToSendThisUpdateIsPair[datagramsToSendThisUpdateIsPair.Size()-1]=true;
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ClearPacketsAndDatagrams(bool keepInternalPacketIfNeedsAck)
{
	unsigned int i;
	for (i=0; i < packetsToDeallocThisUpdate.Size(); i++)
	{
		// packetsToDeallocThisUpdate holds a boolean indicating if packetsToSendThisUpdate at this index should be freed
		if (packetsToDeallocThisUpdate[i])
		{
			RemoveFromUnreliableLinkedList(packetsToSendThisUpdate[i]);
			FreeInternalPacketData(packetsToSendThisUpdate[i], __FILE__, __LINE__ );
			if (keepInternalPacketIfNeedsAck==false || packetsToSendThisUpdate[i]->reliability<UNRELIABLE_WITH_ACK_RECEIPT)
				ReleaseToInternalPacketPool( packetsToSendThisUpdate[i] );
		}
	}
	packetsToDeallocThisUpdate.Clear(true, __FILE__, __LINE__);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::MoveToListHead(InternalPacket *internalPacket)
{
	if ( internalPacket == resendLinkedListHead )
		return;
	if (resendLinkedListHead==0)
	{
		internalPacket->resendNext=internalPacket;
		internalPacket->resendPrev=internalPacket;
		resendLinkedListHead=internalPacket;
		return;
	}
	internalPacket->resendPrev->resendNext = internalPacket->resendNext;
	internalPacket->resendNext->resendPrev = internalPacket->resendPrev;
	internalPacket->resendNext=resendLinkedListHead;
	internalPacket->resendPrev=resendLinkedListHead->resendPrev;
	internalPacket->resendPrev->resendNext=internalPacket;
	resendLinkedListHead->resendPrev=internalPacket;
	resendLinkedListHead=internalPacket;
	RakAssert(internalPacket->headerLength+internalPacket->dataBitLength>0);

	ValidateResendList();
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::RemoveFromList(InternalPacket *internalPacket, bool modifyUnacknowledgedBytes)
{
	InternalPacket *newPosition;
	internalPacket->resendPrev->resendNext = internalPacket->resendNext;
	internalPacket->resendNext->resendPrev = internalPacket->resendPrev;
	newPosition = internalPacket->resendNext;
	if ( internalPacket == resendLinkedListHead )
		resendLinkedListHead = newPosition;
	if (resendLinkedListHead==internalPacket)
		resendLinkedListHead=0;

	if (modifyUnacknowledgedBytes)
	{
		RakAssert(unacknowledgedBytes>=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength));
		unacknowledgedBytes-=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);
		// printf("-unacknowledgedBytes:%i ", unacknowledgedBytes);


		ValidateResendList();
	}
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToListTail(InternalPacket *internalPacket, bool modifyUnacknowledgedBytes)
{
	if (modifyUnacknowledgedBytes)
	{
		unacknowledgedBytes+=BITS_TO_BYTES(internalPacket->headerLength+internalPacket->dataBitLength);
		// printf("+unacknowledgedBytes:%i ", unacknowledgedBytes);
	}

	if (resendLinkedListHead==0)
	{
		internalPacket->resendNext=internalPacket;
		internalPacket->resendPrev=internalPacket;
		resendLinkedListHead=internalPacket;
		return;
	}
	internalPacket->resendNext=resendLinkedListHead;
	internalPacket->resendPrev=resendLinkedListHead->resendPrev;
	internalPacket->resendPrev->resendNext=internalPacket;
	resendLinkedListHead->resendPrev=internalPacket;

	ValidateResendList();

}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::PopListHead(bool modifyUnacknowledgedBytes)
{
	RakAssert(resendLinkedListHead!=0);
	RemoveFromList(resendLinkedListHead, modifyUnacknowledgedBytes);
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::IsResendQueueEmpty(void) const
{
	return resendLinkedListHead==0;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::SendACKs(SOCKET s, SystemAddress systemAddress, CCTimeType time, RakNetRandom *rnr, unsigned short remotePortRakNetWasStartedOn_PS3)
{
	BitSize_t maxDatagramPayload = GetMaxDatagramSizeExcludingMessageHeaderBits();

	while (acknowlegements.Size()>0)
	{
		// Send acks
		updateBitStream.Reset();
		DatagramHeaderFormat dhf;
		dhf.isACK=true;
		dhf.isNAK=false;
		dhf.isPacketPair=false;
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
		dhf.sourceSystemTime=time;
#endif
		double B;
		double AS;
		bool hasBAndAS;
		if (remoteSystemNeedsBAndAS)
		{
			congestionManager.OnSendAckGetBAndAS(time, &hasBAndAS,&B,&AS);
			dhf.AS=(float)AS;
			dhf.hasBAndAS=hasBAndAS;
		}
		else
			dhf.hasBAndAS=false;
#if INCLUDE_TIMESTAMP_WITH_DATAGRAMS==1
		dhf.sourceSystemTime=nextAckTimeToSend;
#endif
		//		dhf.B=(float)B;
		updateBitStream.Reset();
		dhf.Serialize(&updateBitStream);
		CC_DEBUG_PRINTF_1("AckSnd ");
		acknowlegements.Serialize(&updateBitStream, maxDatagramPayload, true);
		SendBitStream( s, systemAddress, &updateBitStream, rnr, remotePortRakNetWasStartedOn_PS3, time );
		congestionManager.OnSendAck(time,updateBitStream.GetNumberOfBytesUsed());

		// I think this is causing a bug where if the estimated bandwidth is very low for the recipient, only acks ever get sent
		//	congestionManager.OnSendBytes(time,UDP_HEADER_SIZE+updateBitStream.GetNumberOfBytesUsed());
	}


}
/*
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::DatagramMessageIDList* ReliabilityLayer::AllocateFromDatagramMessageIDPool(void)
{
DatagramMessageIDList*s;
s=datagramMessageIDPool.Allocate( __FILE__, __LINE__ );
// Call new operator, memoryPool doesn't do this
s = new ((void*)s) DatagramMessageIDList;
return s;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ReleaseToDatagramMessageIDPool(DatagramMessageIDList* d)
{
d->~DatagramMessageIDList();
datagramMessageIDPool.Release(d);
}
*/
//-------------------------------------------------------------------------------------------------------
InternalPacket* ReliabilityLayer::AllocateFromInternalPacketPool(void)
{
	InternalPacket *ip = internalPacketPool.Allocate( __FILE__, __LINE__ );
	ip->reliableMessageNumber = (MessageNumberType) (const uint32_t)-1;
	ip->messageNumberAssigned=false;
	ip->nextActionTime = 0;
	ip->splitPacketCount = 0;
	ip->allocationScheme=InternalPacket::NORMAL;
	ip->data=0;
	return ip;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ReleaseToInternalPacketPool(InternalPacket *ip)
{
	internalPacketPool.Release(ip, __FILE__,__LINE__);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::RemoveFromUnreliableLinkedList(InternalPacket *internalPacket)
{
	if (internalPacket->reliability==UNRELIABLE ||
		internalPacket->reliability==UNRELIABLE_SEQUENCED ||
		internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT
//		||
//		internalPacket->reliability==UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
		)
	{
		InternalPacket *newPosition;
		internalPacket->unreliablePrev->unreliableNext = internalPacket->unreliableNext;
		internalPacket->unreliableNext->unreliablePrev = internalPacket->unreliablePrev;
		newPosition = internalPacket->unreliableNext;
		if ( internalPacket == unreliableLinkedListHead )
			unreliableLinkedListHead = newPosition;
		if (unreliableLinkedListHead==internalPacket)
			unreliableLinkedListHead=0;
	}
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddToUnreliableLinkedList(InternalPacket *internalPacket)
{
	if (internalPacket->reliability==UNRELIABLE ||
		internalPacket->reliability==UNRELIABLE_SEQUENCED ||
		internalPacket->reliability==UNRELIABLE_WITH_ACK_RECEIPT
//		||
//		internalPacket->reliability==UNRELIABLE_SEQUENCED_WITH_ACK_RECEIPT
		)
	{
		if (unreliableLinkedListHead==0)
		{
			internalPacket->unreliableNext=internalPacket;
			internalPacket->unreliablePrev=internalPacket;
			unreliableLinkedListHead=internalPacket;
			return;
		}
		internalPacket->unreliableNext=unreliableLinkedListHead;
		internalPacket->unreliablePrev=unreliableLinkedListHead->unreliablePrev;
		internalPacket->unreliablePrev->unreliableNext=internalPacket;
		unreliableLinkedListHead->unreliablePrev=internalPacket;
	}
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::ValidateResendList(void) const
{
	/*
	unsigned int count1=0, count2=0;
	for (unsigned int i=0; i < RESEND_BUFFER_ARRAY_LENGTH; i++)
	if (resendBuffer[i])
	count1++;

	if (resendLinkedListHead)
	{
	InternalPacket *internalPacket = resendLinkedListHead;
	do 
	{
	count2++;
	internalPacket=internalPacket->resendNext;
	} while (internalPacket!=resendLinkedListHead);
	}
	RakAssert(count1==count2);
	RakAssert(count2<=RESEND_BUFFER_ARRAY_LENGTH);
	*/
}
//-------------------------------------------------------------------------------------------------------
bool ReliabilityLayer::ResendBufferOverflow(void) const
{
	int index1 = sendReliableMessageNumberIndex & (uint32_t) RESEND_BUFFER_ARRAY_MASK;
	//	int index2 = (sendReliableMessageNumberIndex+(uint32_t)1) & (uint32_t) RESEND_BUFFER_ARRAY_MASK;
	RakAssert(index1<RESEND_BUFFER_ARRAY_LENGTH);
	return resendBuffer[index1]!=0; // || resendBuffer[index2]!=0;

}
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::MessageNumberNode* ReliabilityLayer::GetMessageNumberNodeByDatagramIndex(DatagramSequenceNumberType index/*, bool *isReliable*/)
{
//	*isReliable=false;
	if (datagramHistory.IsEmpty())
		return 0;

	if (congestionManager.LessThan(index, datagramHistoryPopCount))
		return 0;

	DatagramSequenceNumberType offsetIntoList = index - datagramHistoryPopCount;
	if (offsetIntoList >= datagramHistory.Size())
		return 0;

	// *isReliable=datagramHistory[offsetIntoList].isReliable;
	return datagramHistory[offsetIntoList].head;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::RemoveFromDatagramHistory(DatagramSequenceNumberType index)
{
	DatagramSequenceNumberType offsetIntoList = index - datagramHistoryPopCount;
	MessageNumberNode *mnm = datagramHistory[offsetIntoList].head;
	MessageNumberNode *next;
	while (mnm)
	{
		next=mnm->next;
		datagramHistoryMessagePool.Release(mnm, __FILE__,__LINE__);
		mnm=next;
	}
	datagramHistory[offsetIntoList].head=0;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AddFirstToDatagramHistory(DatagramSequenceNumberType datagramNumber)
{
	(void) datagramNumber;
	if (datagramHistory.Size()>DATAGRAM_MESSAGE_ID_ARRAY_LENGTH)
	{
		RemoveFromDatagramHistory(datagramHistoryPopCount);
		datagramHistory.Pop();
		datagramHistoryPopCount++;
	}

	datagramHistory.Push(DatagramHistoryNode(0/*,false*/), __FILE__,__LINE__);
	// printf("%p Pushed empty DatagramHistoryNode to datagram history at index %i\n", this, datagramHistory.Size()-1);
}
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::MessageNumberNode* ReliabilityLayer::AddFirstToDatagramHistory(DatagramSequenceNumberType datagramNumber, DatagramSequenceNumberType messageNumber)
{
	(void) datagramNumber;
//	RakAssert(datagramHistoryPopCount+(unsigned int) datagramHistory.Size()==datagramNumber);
	if (datagramHistory.Size()>DATAGRAM_MESSAGE_ID_ARRAY_LENGTH)
	{
		RemoveFromDatagramHistory(datagramHistoryPopCount);
		datagramHistory.Pop();
		datagramHistoryPopCount++;
	}

	MessageNumberNode *mnm = datagramHistoryMessagePool.Allocate(__FILE__,__LINE__);
	mnm->next=0;
	mnm->messageNumber=messageNumber;
	datagramHistory.Push(DatagramHistoryNode(mnm/*,true*/), __FILE__,__LINE__);
	// printf("%p Pushed message %i to DatagramHistoryNode to datagram history at index %i\n", this, messageNumber.val, datagramHistory.Size()-1);
	return mnm;
}
//-------------------------------------------------------------------------------------------------------
ReliabilityLayer::MessageNumberNode* ReliabilityLayer::AddSubsequentToDatagramHistory(MessageNumberNode *messageNumberNode, DatagramSequenceNumberType messageNumber)
{
	messageNumberNode->next=datagramHistoryMessagePool.Allocate(__FILE__,__LINE__);
	messageNumberNode->next->messageNumber=messageNumber;
	messageNumberNode->next->next=0;
	return messageNumberNode->next;		
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AllocInternalPacketData(InternalPacket *internalPacket, InternalPacketRefCountedData **refCounter, unsigned char *externallyAllocatedPtr, unsigned char *ourOffset)
{
	internalPacket->allocationScheme=InternalPacket::REF_COUNTED;
	internalPacket->data=ourOffset;
	if (*refCounter==0)
	{
		*refCounter = refCountedDataPool.Allocate(__FILE__,__LINE__);
		// *refCounter = RakNet::OP_NEW<InternalPacketRefCountedData>(__FILE__,__LINE__);
		(*refCounter)->refCount=1;
		(*refCounter)->sharedDataBlock=externallyAllocatedPtr;
	}
	else
		(*refCounter)->refCount++;
	internalPacket->refCountedData=(*refCounter);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AllocInternalPacketData(InternalPacket *internalPacket, unsigned char *externallyAllocatedPtr)
{
	internalPacket->allocationScheme=InternalPacket::NORMAL;
	internalPacket->data=externallyAllocatedPtr;
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::AllocInternalPacketData(InternalPacket *internalPacket, unsigned int numBytes, const char *file, unsigned int line)
{
	internalPacket->allocationScheme=InternalPacket::NORMAL;
	internalPacket->data=(unsigned char*) rakMalloc_Ex(numBytes,file,line);
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::FreeInternalPacketData(InternalPacket *internalPacket, const char *file, unsigned int line)
{
	if (internalPacket==0)
		return;

	if (internalPacket->allocationScheme==InternalPacket::REF_COUNTED)
	{
		if (internalPacket->refCountedData==0)
			return;

		internalPacket->refCountedData->refCount--;
		if (internalPacket->refCountedData->refCount==0)
		{
			rakFree_Ex(internalPacket->refCountedData->sharedDataBlock, file, line );
			internalPacket->refCountedData->sharedDataBlock=0;
			// RakNet::OP_DELETE(internalPacket->refCountedData,file, line);
			refCountedDataPool.Release(internalPacket->refCountedData,file, line);
			internalPacket->refCountedData=0;
		}
	}
	else
	{
		if (internalPacket->data==0)
			return;

		rakFree_Ex(internalPacket->data, file, line );
		internalPacket->data=0;
	}
}
//-------------------------------------------------------------------------------------------------------
unsigned int ReliabilityLayer::GetMaxDatagramSizeExcludingMessageHeaderBytes(void)
{
	// When using encryption, the data may be padded by up to 15 bytes in order to be a multiple of 16.
	// I don't know how many exactly, it depends on the datagram header serialization
	if (encryptor.IsKeySet())
		return congestionManager.GetMTU() - DatagramHeaderFormat::GetDataHeaderByteLength() - 15;
	else
		return congestionManager.GetMTU() - DatagramHeaderFormat::GetDataHeaderByteLength();
}
//-------------------------------------------------------------------------------------------------------
BitSize_t ReliabilityLayer::GetMaxDatagramSizeExcludingMessageHeaderBits(void)
{
	return BYTES_TO_BITS(GetMaxDatagramSizeExcludingMessageHeaderBytes());
}
//-------------------------------------------------------------------------------------------------------
void ReliabilityLayer::InitHeapWeights(void)
{
	for (int priorityLevel=0; priorityLevel < NUMBER_OF_PRIORITIES; priorityLevel++)
		outgoingPacketBufferNextWeights[priorityLevel]=(1<<priorityLevel)*priorityLevel+priorityLevel;
}
//-------------------------------------------------------------------------------------------------------
reliabilityHeapWeightType ReliabilityLayer::GetNextWeight(int priorityLevel)
{
	uint64_t next = outgoingPacketBufferNextWeights[priorityLevel];
	if (outgoingPacketBuffer.Size()>0)
	{
		int peekPL = outgoingPacketBuffer.Peek()->priority;
		reliabilityHeapWeightType weight = outgoingPacketBuffer.PeekWeight();
		reliabilityHeapWeightType min = weight - (1<<peekPL)*peekPL+peekPL;
		if (next<min)
			next=min + (1<<priorityLevel)*priorityLevel+priorityLevel;
		outgoingPacketBufferNextWeights[priorityLevel]=next+(1<<priorityLevel)*(priorityLevel+1)+priorityLevel;
	}
	else
	{
		InitHeapWeights();
	}
	return next;
}

//-------------------------------------------------------------------------------------------------------
#if defined(RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE)
#pragma pop_macro("new")
#undef RELIABILITY_LAYER_NEW_UNDEF_ALLOCATING_QUEUE
#endif


#ifdef _MSC_VER
#pragma warning( pop )
#endif
