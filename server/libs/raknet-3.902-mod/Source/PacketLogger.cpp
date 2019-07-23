#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_PacketLogger==1

#include "PacketLogger.h"
#include "BitStream.h"
#include "DS_List.h"
#include "InternalPacket.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "StringCompressor.h"
#include "RPCMap.h"
#include "GetTime.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Itoa.h"
#include <time.h>
#include "SocketIncludes.h"
#include "gettimeofday.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

PacketLogger::PacketLogger()
{
	printId=true;
	printAcks=true;
	prefix[0]=0;
	suffix[0]=0;
	logDirectMessages=true;
}
PacketLogger::~PacketLogger()
{
}
void PacketLogger::FormatLine(
char* into, const char* dir, const char* type, unsigned int packet, unsigned int frame, unsigned char id
, const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex)
{
	char numericID[16];
	const char* idToPrint = NULL;
	if(printId)
	{
		if (splitPacketCount>0 && splitPacketCount!=(unsigned int)-1)
			idToPrint="(SPLIT PACKET)";
		else
			idToPrint =	IDTOString(id);
	}
	// If printId is false, idToPrint will be NULL, as it will
	// in the case of an unrecognized id. Testing printId for false
	// would just be redundant.
	if(idToPrint == NULL)
	{
		sprintf(numericID, "%5u", id);
		idToPrint = numericID;
	}

	FormatLine(into, dir, type, packet, frame, idToPrint, bitLen, time, local, remote,splitPacketId,splitPacketIndex,splitPacketCount, orderingIndex);
}

void PacketLogger::FormatLine(
char* into, const char* dir, const char* type, unsigned int packet, unsigned int frame, const char* idToPrint
, const BitSize_t bitLen, unsigned long long time, const SystemAddress& local, const SystemAddress& remote,
unsigned int splitPacketId, unsigned int splitPacketIndex, unsigned int splitPacketCount, unsigned int orderingIndex)
{
	char str1[64], str2[62];
	local.ToString(true, str1);
	remote.ToString(true, str2);
	char localtime[128];
	GetLocalTime(localtime);

	sprintf(into, "%s,%s%s,%s,%5u,%5u,%s,%u,%" PRINTF_64_BIT_MODIFIER "u,%s,%s,%i,%i,%i,%i,%s,"
					, localtime
					, prefix
					, dir
					, type
					, packet
					, frame
					, idToPrint
					, bitLen
					, time
					, str1
					, str2
					, splitPacketId
					, splitPacketIndex
					, splitPacketCount
					, orderingIndex
					, suffix
					);
}
void PacketLogger::OnDirectSocketSend(const char *data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	if (logDirectMessages==false)
		return;

	char str[256];
	FormatLine(str, "Snd", "Raw", 0, 0, data[0], bitsUsed, RakNet::GetTime(), rakPeerInterface->GetExternalID(remoteSystemAddress), remoteSystemAddress, (unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}

void PacketLogger::LogHeader(void)
{
	// Last 5 are splitpacket id, split packet index, split packet count, ordering index, suffix
	AddToLog("Clock,S|R,Typ,Pckt#,Frm #,PktID,BitLn,Time     ,Local IP:Port   ,RemoteIP:Port,SPID,SPIN,SPCO,OI,Suffix,Miscellaneous\n");
}
void PacketLogger::OnDirectSocketReceive(const char *data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	if (logDirectMessages==false)
		return;

	char str[256];
	FormatLine(str, "Rcv", "Raw", 0, 0, data[0], bitsUsed, RakNet::GetTime(), rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS), remoteSystemAddress,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}
void PacketLogger::OnReliabilityLayerPacketError(const char *errorMessage, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	char str[1024];
	FormatLine(str, "RcvErr", errorMessage, 0, 0, "", bitsUsed, RakNet::GetTime(), rakPeerInterface->GetInternalID(UNASSIGNED_SYSTEM_ADDRESS), remoteSystemAddress,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1,(unsigned int)-1);
	AddToLog(str);
}
void PacketLogger::OnAck(unsigned int messageNumber, SystemAddress remoteSystemAddress, RakNetTime time)
{
	char str[256];
	char str1[64], str2[62];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);
	localSystemAddress.ToString(true, str1);
	remoteSystemAddress.ToString(true, str2);
	char localtime[128];
	GetLocalTime(localtime);

	sprintf(str, "%s,Rcv,Ack,%i,,,,%" PRINTF_64_BIT_MODIFIER "u,%s,%s,,,,,,"
					, localtime
					, messageNumber
					, (unsigned long long) time
					, str1
					, str2
					);
	AddToLog(str);
}
void PacketLogger::OnPushBackPacket(const char *data, const BitSize_t bitsUsed, SystemAddress remoteSystemAddress)
{
	char str[256];
	char str1[64], str2[62];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);
	localSystemAddress.ToString(true, str1);
	remoteSystemAddress.ToString(true, str2);
	RakNetTime time = RakNet::GetTime();
	char localtime[128];
	GetLocalTime(localtime);

	sprintf(str, "%s,Lcl,PBP,,,%s,%i,%" PRINTF_64_BIT_MODIFIER "u,%s,%s,,,,,,"
					, localtime
					, BaseIDTOString(data[0])
					, bitsUsed
					, (unsigned long long) time
					, str1
					, str2
					);
	AddToLog(str);
}
void PacketLogger::OnInternalPacket(InternalPacket *internalPacket, unsigned frameNumber, SystemAddress remoteSystemAddress, RakNetTime time, int isSend)
{
	char str[256];
	const char *sendTypes[] =
	{
		"Rcv",
		"Snd",
		"Err1",
		"Err2",
		"Err3",
		"Err4",
		"Err5",
		"Err6",
	};
	const char *sendType = sendTypes[isSend];
	SystemAddress localSystemAddress = rakPeerInterface->GetExternalID(remoteSystemAddress);

	if (internalPacket->data[0]==ID_TIMESTAMP && internalPacket->data[sizeof(unsigned char)+sizeof(RakNetTime)]!=ID_RPC)
	{
		FormatLine(str, sendType, "Tms", internalPacket->reliableMessageNumber, frameNumber, internalPacket->data[1+sizeof(RakNetTime)], internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);
	}
	else if (internalPacket->data[0]==ID_RPC || (internalPacket->dataBitLength>(sizeof(unsigned char)+sizeof(RakNetTime))*8 && internalPacket->data[0]==ID_TIMESTAMP && internalPacket->data[sizeof(unsigned char)+sizeof(RakNetTime)]==ID_RPC))
	{
		const char *uniqueIdentifier = rakPeerInterface->GetRPCString((const char*) internalPacket->data, internalPacket->dataBitLength, isSend==1 ? remoteSystemAddress : UNASSIGNED_SYSTEM_ADDRESS);

		if (internalPacket->data[0]==ID_TIMESTAMP)
			FormatLine(str, sendType, "RpT", internalPacket->reliableMessageNumber, frameNumber, uniqueIdentifier, internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);
		else
			FormatLine(str, sendType, "Rpc", internalPacket->reliableMessageNumber, frameNumber, uniqueIdentifier, internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);
	}
	else
	{
		FormatLine(str, sendType, "Nrm", internalPacket->reliableMessageNumber, frameNumber, internalPacket->data[0], internalPacket->dataBitLength, (unsigned long long)time, localSystemAddress, remoteSystemAddress, internalPacket->splitPacketId, internalPacket->splitPacketIndex, internalPacket->splitPacketCount, internalPacket->orderingIndex);
	}

	AddToLog(str);
}
void PacketLogger::AddToLog(const char *str)
{
	WriteLog(str);
}
void PacketLogger::WriteLog(const char *str)
{
	RAKNET_DEBUG_PRINTF("%s\n", str);
}
void PacketLogger::WriteMiscellaneous(const char *type, const char *msg)
{
	char str[1024];
	char str1[64];
	SystemAddress localSystemAddress = rakPeerInterface->GetInternalID();
	localSystemAddress.ToString(true, str1);
	RakNetTime time = RakNet::GetTime();
	char localtime[128];
	GetLocalTime(localtime);

	sprintf(str, "%s,Lcl,%s,,,,,%" PRINTF_64_BIT_MODIFIER "u,%s,,,,,,,%s"
					, localtime
					, type
					, (unsigned long long) time
					, str1
					, msg
					);

	AddToLog(msg);
}
void PacketLogger::SetPrintID(bool print)
{
	printId=print;
}
void PacketLogger::SetPrintAcks(bool print)
{
	printAcks=print;
}
const char* PacketLogger::BaseIDTOString(unsigned char Id)
{
	if (Id >= ID_USER_PACKET_ENUM)
        return 0;

	const char *IDTable[((int)ID_USER_PACKET_ENUM)+1]=
	{
		"ID_INTERNAL_PING",
		"ID_PING",
		"ID_PING_OPEN_CONNECTIONS",
		"ID_CONNECTED_PONG",
		"ID_CONNECTION_REQUEST",
		"ID_SECURED_CONNECTION_RESPONSE",
		"ID_SECURED_CONNECTION_CONFIRMATION",
		"ID_RPC_MAPPING",
		"ID_DETECT_LOST_CONNECTIONS",
		"ID_OPEN_CONNECTION_REQUEST",
		"ID_OPEN_CONNECTION_REPLY",
		"ID_RPC",
		"ID_RPC_REPLY",
		"ID_OUT_OF_BAND_INTERNAL",
		"ID_CONNECTION_REQUEST_ACCEPTED",
		"ID_CONNECTION_ATTEMPT_FAILED",
		"ID_ALREADY_CONNECTED",
		"ID_NEW_INCOMING_CONNECTION",
		"ID_NO_FREE_INCOMING_CONNECTIONS",
		"ID_DISCONNECTION_NOTIFICATION",
		"ID_CONNECTION_LOST",
		"ID_RSA_PUBLIC_KEY_MISMATCH",
		"ID_CONNECTION_BANNED",
		"ID_INVALID_PASSWORD",
		"ID_INCOMPATIBLE_PROTOCOL_VERSION",
		"ID_IP_RECENTLY_CONNECTED",
		"ID_MODIFIED_PACKET",
		"ID_TIMESTAMP",
		"ID_PONG",
		"ID_ADVERTISE_SYSTEM",
		"ID_REMOTE_DISCONNECTION_NOTIFICATION",
		"ID_REMOTE_CONNECTION_LOST",
		"ID_REMOTE_NEW_INCOMING_CONNECTION",
		"ID_DOWNLOAD_PROGRESS",
		"ID_FILE_LIST_TRANSFER_HEADER",
		"ID_FILE_LIST_TRANSFER_FILE",
		"ID_FILE_LIST_REFERENCE_PUSH_ACK",
		"ID_DDT_DOWNLOAD_REQUEST",
		"ID_TRANSPORT_STRING",
		"ID_REPLICA_MANAGER_CONSTRUCTION",
		"ID_REPLICA_MANAGER_DESTRUCTION",
		"ID_REPLICA_MANAGER_SCOPE_CHANGE",
		"ID_REPLICA_MANAGER_SERIALIZE",
		"ID_REPLICA_MANAGER_DOWNLOAD_STARTED",
		"ID_REPLICA_MANAGER_DOWNLOAD_COMPLETE",
		"ID_CONNECTION_GRAPH_REQUEST",
		"ID_CONNECTION_GRAPH_REPLY",
		"ID_CONNECTION_GRAPH_UPDATE",
		"ID_CONNECTION_GRAPH_NEW_CONNECTION",
		"ID_CONNECTION_GRAPH_CONNECTION_LOST",
		"ID_CONNECTION_GRAPH_DISCONNECTION_NOTIFICATION",
		"ID_ROUTE_AND_MULTICAST",
		"ID_RAKVOICE_OPEN_CHANNEL_REQUEST",
		"ID_RAKVOICE_OPEN_CHANNEL_REPLY",
		"ID_RAKVOICE_CLOSE_CHANNEL",
		"ID_RAKVOICE_DATA",
		"ID_AUTOPATCHER_GET_CHANGELIST_SINCE_DATE",
		"ID_AUTOPATCHER_CREATION_LIST",
		"ID_AUTOPATCHER_DELETION_LIST",
		"ID_AUTOPATCHER_GET_PATCH",
		"ID_AUTOPATCHER_PATCH_LIST",
		"ID_AUTOPATCHER_REPOSITORY_FATAL_ERROR",
		"ID_AUTOPATCHER_FINISHED_INTERNAL",
		"ID_AUTOPATCHER_FINISHED",
		"ID_AUTOPATCHER_RESTART_APPLICATION",
		"ID_NAT_PUNCHTHROUGH_REQUEST",
		"ID_NAT_CONNECT_AT_TIME",
		"ID_NAT_GET_MOST_RECENT_PORT",
		"ID_NAT_CLIENT_READY",
		"ID_NAT_TARGET_NOT_CONNECTED",
		"ID_NAT_TARGET_UNRESPONSIVE",
		"ID_NAT_CONNECTION_TO_TARGET_LOST",
		"ID_NAT_ALREADY_IN_PROGRESS",
		"ID_NAT_PUNCHTHROUGH_FAILED",
		"ID_NAT_PUNCHTHROUGH_SUCCEEDED",
		"ID_DATABASE_QUERY_REQUEST",
		"ID_DATABASE_UPDATE_ROW",
		"ID_DATABASE_REMOVE_ROW",
		"ID_DATABASE_QUERY_REPLY",
		"ID_DATABASE_UNKNOWN_TABLE",
		"ID_DATABASE_INCORRECT_PASSWORD",
		"ID_READY_EVENT_SET",
		"ID_READY_EVENT_UNSET",
		"ID_READY_EVENT_ALL_SET",
		"ID_READY_EVENT_QUERY",
		"ID_LOBBY_GENERAL",
		"ID_AUTO_RPC_CALL",
		"ID_AUTO_RPC_REMOTE_INDEX",
		"ID_AUTO_RPC_UNKNOWN_REMOTE_INDEX",
		"ID_RPC_REMOTE_ERROR",
		"ID_FILE_LIST_REFERENCE_PUSH",
		"ID_READY_EVENT_FORCE_ALL_SET",
		"ID_ROOMS_EXECUTE_FUNC",
		"ID_ROOMS_LOGON_STATUS",
		"ID_ROOMS_HANDLE_CHANGE",
		"ID_LOBBY2_SEND_MESSAGE",
		"ID_LOBBY2_SERVER_ERROR",
		"ID_FCM2_NEW_HOST",
		"ID_FCM2_REQUEST_FCMGUID",
		"ID_FCM2_RESPOND_CONNECTION_COUNT",
		"ID_FCM2_INFORM_FCMGUID",
		"ID_UDP_PROXY_GENERAL",
		"ID_SQLite3_EXEC",
		"ID_SQLite3_UNKNOWN_DB",
		"ID_REPLICA_MANAGER_3_SERIALIZE_CONSTRUCTION_EXISTING",
		"ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_REJECTED",
		"ID_REPLICA_MANAGER_3_LOCAL_CONSTRUCTION_ACCEPTED",
		"ID_NAT_TYPE_DETECTION_REQUEST",
		"ID_NAT_TYPE_DETECTION_RESULT",
		"ID_SQLLITE_LOGGER",
		"ID_ROUTER_2_INTERNAL",
		"ID_ROUTER_2_FORWARDING_NO_PATH",
		"ID_ROUTER_2_REROUTED",
		"ID_TEAM_BALANCER_INTERNAL",
		"ID_TEAM_BALANCER_REQUESTED_TEAM_CHANGE_PENDING",
		"ID_TEAM_BALANCER_TEAMS_LOCKED",
		"ID_TEAM_BALANCER_TEAM_ASSIGNED",
		"ID_LIGHTSPEED_INTEGRATION",
		"ID_RPC_4_PLUGIN",
		"ID_SND_RECEIPT_ACKED",
		"ID_SND_RECEIPT_LOSS",
		"ID_RESERVED_5",
		"ID_RESERVED_6",
		"ID_RESERVED_7",
		"ID_RESERVED_8",
		"ID_RESERVED_9",
		"ID_USER_PACKET_ENUM"
	};

	return (char*)IDTable[Id];
}
const char* PacketLogger::UserIDTOString(unsigned char Id)
{
	// Users should override this
	static char str[256];
	Itoa(Id, str, 10);
	return (const char*) str;
}
const char* PacketLogger::IDTOString(unsigned char Id)
{
	const char *out;
	out=BaseIDTOString(Id);
	if (out)
		return out;
	return UserIDTOString(Id);
}
void PacketLogger::SetPrefix(const char *_prefix)
{
	strncpy(prefix, _prefix, 255);
	prefix[255]=0;
}
void PacketLogger::SetSuffix(const char *_suffix)
{
	strncpy(suffix, _suffix, 255);
	suffix[255]=0;
}
void PacketLogger::GetLocalTime(char buffer[128])
{
#if defined(_WIN32) && !defined(__GNUC__)  && !defined(__GCCXML__)
    time_t rawtime;
	struct timeval tv;
	// If you get an arror about an incomplete type, just delete this file
	struct timezone tz;
	gettimeofday(&tv, &tz);
	// time ( &rawtime );
	rawtime=tv.tv_sec;

	struct tm * timeinfo;
	timeinfo = localtime ( &rawtime );
	strftime (buffer,128,"%x %X",timeinfo);
	char buff[32];
	sprintf(buff, ".%i", tv.tv_usec);
	strcat(buffer,buff);

	// Commented version puts the time first
	/*
	struct tm * timeinfo;
	timeinfo = localtime ( &rawtime );
	strftime (buffer,128,"%X",timeinfo);
	char buff[32];
	sprintf(buff, ".%i ", tv.tv_usec);
	strcat(buffer,buff);
	char buff2[32];
	strftime (buff2,32,"%x",timeinfo);
	strcat(buffer,buff2);
	*/
#else
    buffer[0]=0;
#endif
}
void PacketLogger::SetLogDirectMessages(bool send)
{
	logDirectMessages=send;
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // _RAKNET_SUPPORT_*
