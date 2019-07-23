/// \file
/// \brief Factory class for RakNet objects
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RAK_NETWORK_FACTORY_H
#define __RAK_NETWORK_FACTORY_H

#include "Export.h"

class RakPeerInterface;
class ConsoleServer;
class ReplicaManager;
class LogCommandParser;
class PacketLogger;
class RakNetCommandParser;
class RakNetTransport;
class TelnetTransport;
class PacketConsoleLogger;
class PacketFileLogger;
class Router;
class ConnectionGraph;

// TODO RakNet 4 - move these to the classes themselves
class RAK_DLL_EXPORT RakNetworkFactory
{
public:
	// For DLL's, these are user classes that you might want to new and delete.
	// You can't instantiate exported classes directly in your program.  The instantiation
	// has to take place inside the DLL.  So these functions will do the news and deletes for you.
	// if you're using the source or static library you don't need these functions, but can use them if you want.
	static RakPeerInterface* GetRakPeerInterface( void );
	static void DestroyRakPeerInterface( RakPeerInterface* i );

#if _RAKNET_SUPPORT_ConsoleServer==1
	static ConsoleServer* GetConsoleServer( void );
	static void DestroyConsoleServer( ConsoleServer* i);
#endif

#if _RAKNET_SUPPORT_ReplicaManager==1
	static ReplicaManager* GetReplicaManager( void );
	static void DestroyReplicaManager( ReplicaManager* i);
#endif

#if _RAKNET_SUPPORT_LogCommandParser==1
	static LogCommandParser* GetLogCommandParser( void );
	static void DestroyLogCommandParser( LogCommandParser* i);
	#if _RAKNET_SUPPORT_PacketLogger==1
	static PacketConsoleLogger* GetPacketConsoleLogger( void );
	static void DestroyPacketConsoleLogger(  PacketConsoleLogger* i );
	#endif
#endif

#if _RAKNET_SUPPORT_PacketLogger==1
	static PacketLogger* GetPacketLogger( void );
	static void DestroyPacketLogger( PacketLogger* i);
	static PacketFileLogger* GetPacketFileLogger( void );
	static void DestroyPacketFileLogger(  PacketFileLogger* i );
#endif

#if _RAKNET_SUPPORT_RakNetCommandParser==1
	static RakNetCommandParser* GetRakNetCommandParser( void );
	static void DestroyRakNetCommandParser(  RakNetCommandParser* i );
#endif

#if _RAKNET_SUPPORT_RakNetTransport==1
	static RakNetTransport* GetRakNetTransport( void );
	static void DestroyRakNetTransport(  RakNetTransport* i );
#endif

#if _RAKNET_SUPPORT_TelnetTransport==1
	static TelnetTransport* GetTelnetTransport( void );
	static void DestroyTelnetTransport(  TelnetTransport* i );
#endif

#if _RAKNET_SUPPORT_Router==1
	static Router* GetRouter( void );
	static void DestroyRouter(  Router* i );
#endif

#if _RAKNET_SUPPORT_ConnectionGraph==1
	static ConnectionGraph* GetConnectionGraph( void );
	static void DestroyConnectionGraph(  ConnectionGraph* i );
#endif
};

#endif
