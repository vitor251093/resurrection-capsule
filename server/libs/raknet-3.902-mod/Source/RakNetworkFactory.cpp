/// \file
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#include "LogCommandParser.h"
#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "RakPeer.h"
#include "ConsoleServer.h"
#include "PacketLogger.h"
#include "RakNetCommandParser.h"
#include "ReplicaManager.h"
#include "RakNetTransport.h"
#include "TelnetTransport.h"
#include "PacketConsoleLogger.h"
#include "PacketFileLogger.h"
#include "Router.h"
#include "ConnectionGraph.h"


RakPeerInterface* RakNetworkFactory::GetRakPeerInterface( void ) {return RakNet::OP_NEW<RakPeer>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyRakPeerInterface( RakPeerInterface* i ) {RakNet::OP_DELETE(( RakPeer* ) i, __FILE__, __LINE__);}

#if _RAKNET_SUPPORT_ConsoleServer==1
ConsoleServer* RakNetworkFactory::GetConsoleServer( void ) {return RakNet::OP_NEW<ConsoleServer>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyConsoleServer( ConsoleServer* i) {RakNet::OP_DELETE(( ConsoleServer* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_ReplicaManager==1
ReplicaManager* RakNetworkFactory::GetReplicaManager( void ) {return RakNet::OP_NEW<ReplicaManager>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyReplicaManager( ReplicaManager* i) {RakNet::OP_DELETE(( ReplicaManager* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_LogCommandParser==1
LogCommandParser* RakNetworkFactory::GetLogCommandParser( void ) {return RakNet::OP_NEW<LogCommandParser>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyLogCommandParser( LogCommandParser* i) {RakNet::OP_DELETE(( LogCommandParser* ) i, __FILE__, __LINE__);}
	#if _RAKNET_SUPPORT_PacketLogger==1
	PacketConsoleLogger* RakNetworkFactory::GetPacketConsoleLogger( void ) {return RakNet::OP_NEW<PacketConsoleLogger>( __FILE__, __LINE__ );}
	void RakNetworkFactory::DestroyPacketConsoleLogger(  PacketConsoleLogger* i ) {RakNet::OP_DELETE(( PacketConsoleLogger* ) i, __FILE__, __LINE__);}
	#endif
#endif

#if _RAKNET_SUPPORT_PacketLogger==1
PacketLogger* RakNetworkFactory::GetPacketLogger( void ) {return RakNet::OP_NEW<PacketLogger>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyPacketLogger( PacketLogger* i) {RakNet::OP_DELETE(( PacketLogger* ) i, __FILE__, __LINE__);}
PacketFileLogger* RakNetworkFactory::GetPacketFileLogger( void ) {return RakNet::OP_NEW<PacketFileLogger>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyPacketFileLogger(  PacketFileLogger* i ) {RakNet::OP_DELETE(( PacketFileLogger* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_RakNetCommandParser==1
RakNetCommandParser* RakNetworkFactory::GetRakNetCommandParser( void ) {return RakNet::OP_NEW<RakNetCommandParser>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyRakNetCommandParser(  RakNetCommandParser* i ) {RakNet::OP_DELETE(( RakNetCommandParser* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_RakNetTransport==1
RakNetTransport* RakNetworkFactory::GetRakNetTransport( void ) {return RakNet::OP_NEW<RakNetTransport>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyRakNetTransport(  RakNetTransport* i ) {RakNet::OP_DELETE(( RakNetTransport* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_TelnetTransport==1
TelnetTransport* RakNetworkFactory::GetTelnetTransport( void ) {return RakNet::OP_NEW<TelnetTransport>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyTelnetTransport(  TelnetTransport* i ) {RakNet::OP_DELETE(( TelnetTransport* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_Router==1
Router* RakNetworkFactory::GetRouter( void ) {return RakNet::OP_NEW<Router>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyRouter(  Router* i ) {RakNet::OP_DELETE(( Router* ) i, __FILE__, __LINE__);}
#endif

#if _RAKNET_SUPPORT_ConnectionGraph==1
ConnectionGraph* RakNetworkFactory::GetConnectionGraph( void ) {return RakNet::OP_NEW<ConnectionGraph>( __FILE__, __LINE__ );}
void RakNetworkFactory::DestroyConnectionGraph(  ConnectionGraph* i ) {RakNet::OP_DELETE(( ConnectionGraph* ) i, __FILE__, __LINE__);}
#endif
