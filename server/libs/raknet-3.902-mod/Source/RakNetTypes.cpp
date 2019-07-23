/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "RakNetTypes.h"
#include "RakAssert.h"
#include <string.h>
#include <stdio.h>


#if   defined(_WIN32)
// IP_DONTFRAGMENT is different between winsock 1 and winsock 2.  Therefore, Winsock2.h must be linked againt Ws2_32.lib
// winsock.h must be linked against WSock32.lib.  If these two are mixed up the flag won't work correctly
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h> // strncasecmp
#include "Itoa.h"
#include "SocketLayer.h"
#include <stdlib.h>

bool NonNumericHostString( const char *host )
{
	if ( host[ 0 ] >= '0' && host[ 0 ] <= '9' )
		return false;

	if ( (host[ 0 ] == '-') && ( host[ 1 ] >= '0' && host[ 1 ] <= '9' ) )
		return false;

	return true;
}

SocketDescriptor::SocketDescriptor() {port=0; hostAddress[0]=0; remotePortRakNetWasStartedOn_PS3=0;}
SocketDescriptor::SocketDescriptor(unsigned short _port, const char *_hostAddress)
{
	remotePortRakNetWasStartedOn_PS3=0;
	port=_port;
	if (_hostAddress)
		strcpy(hostAddress, _hostAddress);
	else
		hostAddress[0]=0;
}

// Defaults to not in peer to peer mode for NetworkIDs.  This only sends the localSystemAddress portion in the BitStream class
// This is what you want for client/server, where the server assigns all NetworkIDs and it is unnecessary to transmit the full structure.
// For peer to peer, this will transmit the systemAddress of the system that created the object in addition to localSystemAddress.  This allows
// Any system to create unique ids locally.
// All systems must use the same value for this variable.
//bool RAK_DLL_EXPORT NetworkID::peerToPeerMode=false;

bool SystemAddress::operator==( const SystemAddress& right ) const
{
	return binaryAddress == right.binaryAddress && port == right.port;
}

bool SystemAddress::operator!=( const SystemAddress& right ) const
{
	return binaryAddress != right.binaryAddress || port != right.port;
}

bool SystemAddress::operator>( const SystemAddress& right ) const
{
	return ( ( binaryAddress > right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port > right.port ) ) );
}

bool SystemAddress::operator<( const SystemAddress& right ) const
{
	return ( ( binaryAddress < right.binaryAddress ) || ( ( binaryAddress == right.binaryAddress ) && ( port < right.port ) ) );
}
const char *SystemAddress::ToString(bool writePort) const
{
	static unsigned char strIndex=0;
	static char str[8][30];

	unsigned char lastStrIndex=strIndex;
	strIndex++;
	ToString(writePort, str[lastStrIndex&7]);
	return (char*) str[lastStrIndex&7];
}
void SystemAddress::ToString(bool writePort, char *dest) const
{
	if (*this==UNASSIGNED_SYSTEM_ADDRESS)
	{
		strcpy(dest, "UNASSIGNED_SYSTEM_ADDRESS");
		return;
	}











	
	in_addr in;
	in.s_addr = binaryAddress;
//	cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
//	sys_net_initialize_network();
	const char *ntoaStr = inet_ntoa( in );
	strcpy(dest, ntoaStr);
	if (writePort)
	{
		strcat(dest, ":");
		Itoa(port, dest+strlen(dest), 10);
	}

}
SystemAddress::SystemAddress() {*this=UNASSIGNED_SYSTEM_ADDRESS; systemIndex=(SystemIndex)-1;}
SystemAddress::SystemAddress(const char *a, unsigned short b) {SetBinaryAddress(a); port=b; systemIndex=(SystemIndex)-1;};
SystemAddress::SystemAddress(unsigned int a, unsigned short b) {binaryAddress=a; port=b; systemIndex=(SystemIndex)-1;};
#ifdef _MSC_VER
#pragma warning( disable : 4996 )  // The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _strnicmp. See online help for details.
#endif
void SystemAddress::SetBinaryAddress(const char *str)
{
	if ( NonNumericHostString( str ) )
	{

	#if defined(_WIN32)
		if (_strnicmp(str,"localhost", 9)==0)
	#else
		if (strncasecmp(str,"localhost", 9)==0)
	#endif
		{
			binaryAddress=inet_addr("127.0.0.1");
			if (str[9])
				port=(unsigned short) atoi(str+9);
			return;
		}

		const char *ip = ( char* ) SocketLayer::DomainNameToIP( str );
		if (ip)
		{
			binaryAddress=inet_addr(ip);
		}
	}
	else
	{
		//#ifdef _XBOX
		//	binaryAddress=UNASSIGNED_SYSTEM_ADDRESS.binaryAddress;
		//#else
		// Split the string into the first part, and the : part
		int index, portIndex;
		char IPPart[22];
		char portPart[10];
		// Only write the valid parts, don't change existing if invalid
		//	binaryAddress=UNASSIGNED_SYSTEM_ADDRESS.binaryAddress;
		//	port=UNASSIGNED_SYSTEM_ADDRESS.port;
		for (index=0; str[index] && str[index]!=':' && index<22; index++)
		{
			IPPart[index]=str[index];
		}
		IPPart[index]=0;
		portPart[0]=0;
		if (str[index] && str[index+1])
		{
			index++;
			for (portIndex=0; portIndex<10 && str[index] && index < 22+10; index++, portIndex++)
				portPart[portIndex]=str[index];
			portPart[portIndex]=0;
		}













		if (IPPart[0])
			binaryAddress=inet_addr(IPPart);



		if (portPart[0])
			port=(unsigned short) atoi(portPart);
		//#endif
	}

}

NetworkID& NetworkID::operator = ( const NetworkID& input )
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
	systemAddress = input.systemAddress;
	guid = input.guid;
#endif
	localSystemAddress = input.localSystemAddress;
	return *this;
}

bool NetworkID::operator==( const NetworkID& right ) const
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return guid == right.guid && localSystemAddress == right.localSystemAddress;
		else
			return systemAddress == right.systemAddress && localSystemAddress == right.localSystemAddress;
	}
#else
//	else
		return localSystemAddress==right.localSystemAddress;
#endif
}

bool NetworkID::operator!=( const NetworkID& right ) const
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return guid != right.guid || localSystemAddress != right.localSystemAddress;
		else
			return systemAddress != right.systemAddress || localSystemAddress != right.localSystemAddress;
	}
#else
//	else
		return localSystemAddress!=right.localSystemAddress;
#endif
}

bool NetworkID::operator>( const NetworkID& right ) const
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return ( ( guid > right.guid ) || ( ( guid == right.guid ) && ( localSystemAddress > right.localSystemAddress ) ) );
		else
			return ( ( systemAddress > right.systemAddress ) || ( ( systemAddress == right.systemAddress ) && ( localSystemAddress > right.localSystemAddress ) ) );
	}
#else
//	else
		return localSystemAddress>right.localSystemAddress;
#endif
}

bool NetworkID::operator<( const NetworkID& right ) const
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
//	if (NetworkID::peerToPeerMode)
	{
		if (guid!=UNASSIGNED_RAKNET_GUID)
			return ( ( guid < right.guid ) || ( ( guid == right.guid ) && ( localSystemAddress < right.localSystemAddress ) ) );
		else
			return ( ( systemAddress < right.systemAddress ) || ( ( systemAddress == right.systemAddress ) && ( localSystemAddress < right.localSystemAddress ) ) );
	}
#else
	//else
		return localSystemAddress<right.localSystemAddress;
#endif
}
bool NetworkID::IsPeerToPeerMode(void)
{
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1
	return true;
#else
	return false;
#endif
//	return peerToPeerMode;
}
void NetworkID::SetPeerToPeerMode(bool isPeerToPeer)
{
	(void) isPeerToPeer;

	// deprecated, define NETWORK_ID_SUPPORTS_PEER_TO_PEER instead for true, comment out for false
	// This is in RakNetDefines.h
	RakAssert(0);

//	peerToPeerMode=isPeerToPeer;
}
bool RakNetGUID::operator==( const RakNetGUID& right ) const
{
	return g==right.g;
}
bool RakNetGUID::operator!=( const RakNetGUID& right ) const
{
	return g!=right.g;
}
bool RakNetGUID::operator > ( const RakNetGUID& right ) const
{
	return g > right.g;
}
bool RakNetGUID::operator < ( const RakNetGUID& right ) const
{
	return g < right.g;
}
const char *RakNetGUID::ToString(void) const
{
	static unsigned char strIndex=0;
	static char str[8][22];

	unsigned char lastStrIndex=strIndex;
	strIndex++;
	ToString(str[lastStrIndex&7]);
	return (char*) str[lastStrIndex&7];
}
void RakNetGUID::ToString(char *dest) const
{
	if (*this==UNASSIGNED_RAKNET_GUID)
		strcpy(dest, "UNASSIGNED_RAKNET_GUID");

	//sprintf(dest, "%u.%u.%u.%u.%u.%u", g[0], g[1], g[2], g[3], g[4], g[5]);
	sprintf(dest, "%" PRINTF_64_BIT_MODIFIER "u", (long long unsigned int) g);
	// sprintf(dest, "%u.%u.%u.%u.%u.%u", g[0], g[1], g[2], g[3], g[4], g[5]);
}
bool RakNetGUID::FromString(const char *source)
{
	if (source==0)
		return false;



#if   defined(WIN32)
	g=_atoi64(source);


#else
	// Changed from g=strtoull(source,0,10); for android
	g=strtoull(source, (char **)NULL, 10);
#endif
	return true;

}
