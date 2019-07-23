/// \file
/// \brief A structure that holds all statistical data returned by RakNet.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.



#ifndef __RAK_NET_STATISTICS_H
#define __RAK_NET_STATISTICS_H

#include "PacketPriority.h"
#include "Export.h"
#include "RakNetTypes.h"

enum RNSPerSecondMetrics
{
	USER_MESSAGE_BYTES_PUSHED,
	USER_MESSAGE_BYTES_SENT,
	USER_MESSAGE_BYTES_RESENT,
	USER_MESSAGE_BYTES_RECEIVED_PROCESSED,
	USER_MESSAGE_BYTES_RECEIVED_IGNORED,
	ACTUAL_BYTES_SENT,
	ACTUAL_BYTES_RECEIVED,
	RNS_PER_SECOND_METRICS_COUNT
};

/// \brief Network Statisics Usage 
///
/// Store Statistics information related to network usage 
struct RAK_DLL_EXPORT RakNetStatistics
{
	uint64_t valueOverLastSecond[RNS_PER_SECOND_METRICS_COUNT];
	uint64_t runningTotal[RNS_PER_SECOND_METRICS_COUNT];
	
	RakNetTimeUS connectionStartTime;

	uint64_t BPSLimitByCongestionControl;
	bool isLimitedByCongestionControl;

	uint64_t BPSLimitByOutgoingBandwidthLimit;
	bool isLimitedByOutgoingBandwidthLimit;

	unsigned int messageInSendBuffer[NUMBER_OF_PRIORITIES];
	double bytesInSendBuffer[NUMBER_OF_PRIORITIES];

	unsigned int messagesInResendBuffer;
	uint64_t bytesInResendBuffer;

	float packetlossLastSecond, packetlossTotal;

	RakNetStatistics& operator +=(const RakNetStatistics& other)
	{
		unsigned i;
		for (i=0; i < NUMBER_OF_PRIORITIES; i++)
		{
			messageInSendBuffer[i]+=other.messageInSendBuffer[i];
			bytesInSendBuffer[i]+=other.bytesInSendBuffer[i];
		}

		for (i=0; i < RNS_PER_SECOND_METRICS_COUNT; i++)
		{
			valueOverLastSecond[i]+=other.valueOverLastSecond[i];
			runningTotal[i]+=other.runningTotal[i];
		}

		return *this;
	}
};

/// Verbosity level currently supports 0 (low), 1 (medium), 2 (high)
/// \param[in] s The Statistical information to format out
/// \param[in] buffer The buffer containing a formated report
/// \param[in] verbosityLevel 
/// 0 low
/// 1 medium 
/// 2 high 
/// 3 debugging congestion control
void RAK_DLL_EXPORT StatisticsToString( RakNetStatistics *s, char *buffer, int verbosityLevel );

#endif
