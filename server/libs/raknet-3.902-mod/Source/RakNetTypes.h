/// \file
/// \brief Types used by RakNet, most of which involve user code.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __NETWORK_TYPES_H
#define __NETWORK_TYPES_H

#include "RakNetDefines.h"
#include "NativeTypes.h"
#include "RakNetTime.h"
#include "Export.h"

/// Forward declaration
namespace RakNet
{
	class BitStream;
};

/// Given a number of bits, return how many bytes are needed to represent that.
#define BITS_TO_BYTES(x) (((x)+7)>>3)
#define BYTES_TO_BITS(x) ((x)<<3)

/// \sa NetworkIDObject.h
typedef unsigned char UniqueIDType;
typedef unsigned short SystemIndex;
typedef unsigned char RPCIndex;
const int MAX_RPC_MAP_SIZE=((RPCIndex)-1)-1;
const int UNDEFINED_RPC_INDEX=((RPCIndex)-1);

/// First byte of a network message
typedef unsigned char MessageID;

typedef uint32_t BitSize_t;

#if defined(_MSC_VER) && _MSC_VER > 0
#define PRINTF_64_BIT_MODIFIER "I64"
#else
#define PRINTF_64_BIT_MODIFIER "ll"
#endif

/// Describes the local socket to use for RakPeer::Startup
struct RAK_DLL_EXPORT SocketDescriptor
{
	SocketDescriptor();
	SocketDescriptor(unsigned short _port, const char *_hostAddress);

	/// The local port to bind to.  Pass 0 to have the OS autoassign a port.
	unsigned short port;

	/// The local network card address to bind to, such as "127.0.0.1".  Pass an empty string to use INADDR_ANY.
	char hostAddress[32];

	// Only need to set for the PS3, when using signaling.
	// Connect with the port returned by signaling. Set this to whatever port RakNet was actually started on
	unsigned short remotePortRakNetWasStartedOn_PS3;
};

extern bool NonNumericHostString( const char *host );

/// \brief Network address for a system
/// \details Corresponds to a network address<BR>
/// This is not necessarily a unique identifier. For example, if a system has both LAN and internet connections, the system may be identified by either one, depending on who is communicating<BR>
/// Use RakNetGUID for a unique per-instance of RakPeer to identify systems
struct RAK_DLL_EXPORT SystemAddress
{
	SystemAddress();
	explicit SystemAddress(const char *a, unsigned short b);
	explicit SystemAddress(unsigned int a, unsigned short b);

	///The peer address from inet_addr.
	uint32_t binaryAddress;
	///The port number
	unsigned short port;
	// Used internally for fast lookup. Optional (use -1 to do regular lookup). Don't transmit this.
	SystemIndex systemIndex;
	static int size() {return (int) sizeof(uint32_t)+sizeof(unsigned short);}

	// Return the systemAddress as a string in the format <IP>:<Port>
	// Returns a static string
	// NOT THREADSAFE
	const char *ToString(bool writePort=true) const;

	// Return the systemAddress as a string in the format <IP>:<Port>
	// dest must be large enough to hold the output
	// THREADSAFE
	void ToString(bool writePort, char *dest) const;

	// Sets the binary address part from a string.  Doesn't set the port
	void SetBinaryAddress(const char *str);

	SystemAddress& operator = ( const SystemAddress& input )
	{
		binaryAddress = input.binaryAddress;
		port = input.port;
		systemIndex = input.systemIndex;
		return *this;
	}

	bool operator==( const SystemAddress& right ) const;
	bool operator!=( const SystemAddress& right ) const;
	bool operator > ( const SystemAddress& right ) const;
	bool operator < ( const SystemAddress& right ) const;
};


/// Size of SystemAddress data
#define SystemAddress_Size 6

class RakPeerInterface;

/// All RPC functions have the same parameter list - this structure.
/// \deprecated use RakNet::RPC3 instead
struct RPCParameters
{
	/// The data from the remote system
	unsigned char *input;

	/// How many bits long \a input is
	BitSize_t numberOfBitsOfData;

	/// Which system called this RPC
	SystemAddress sender;

	/// Which instance of RakPeer (or a derived RakPeer or RakPeer) got this call
	RakPeerInterface *recipient;

	RakNetTime remoteTimestamp;

	/// The name of the function that was called.
	char *functionName;

	/// You can return values from RPC calls by writing them to this BitStream.
	/// This is only sent back if the RPC call originally passed a BitStream to receive the reply.
	/// If you do so and your send is reliable, it will block until you get a reply or you get disconnected from the system you are sending to, whichever is first.
	/// If your send is not reliable, it will block for triple the ping time, or until you are disconnected, or you get a reply, whichever is first.
	RakNet::BitStream *replyToSender;
};

/// Uniquely identifies an instance of RakPeer. Use RakPeer::GetGuidFromSystemAddress() and RakPeer::GetSystemAddressFromGuid() to go between SystemAddress and RakNetGUID
/// Use RakPeer::GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS) to get your own GUID
struct RAK_DLL_EXPORT RakNetGUID
{
	RakNetGUID() {systemIndex=(SystemIndex)-1;}
	explicit RakNetGUID(uint64_t _g) {g=_g; systemIndex=(SystemIndex)-1;}
//	uint32_t g[6];
	uint64_t g;

	// Return the GUID as a string
	// Returns a static string
	// NOT THREADSAFE
	const char *ToString(void) const;

	// Return the GUID as a string
	// dest must be large enough to hold the output
	// THREADSAFE
	void ToString(char *dest) const;

	bool FromString(const char *source);

	RakNetGUID& operator = ( const RakNetGUID& input )
	{
		g=input.g;
		systemIndex=input.systemIndex;
		return *this;
	}

	// Used internally for fast lookup. Optional (use -1 to do regular lookup). Don't transmit this.
	SystemIndex systemIndex;
	static const int size() {return (int) sizeof(uint64_t);}

	bool operator==( const RakNetGUID& right ) const;
	bool operator!=( const RakNetGUID& right ) const;
	bool operator > ( const RakNetGUID& right ) const;
	bool operator < ( const RakNetGUID& right ) const;
};

/// Index of an invalid SystemAddress
//const SystemAddress UNASSIGNED_SYSTEM_ADDRESS =
//{
//	0xFFFFFFFF, 0xFFFF
//};
#ifndef SWIG
const SystemAddress UNASSIGNED_SYSTEM_ADDRESS(0xFFFFFFFF, 0xFFFF);
const RakNetGUID UNASSIGNED_RAKNET_GUID((uint64_t)-1);
#endif
//{
//	{0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF}
//	0xFFFFFFFFFFFFFFFF
//};


struct RAK_DLL_EXPORT AddressOrGUID
{
	RakNetGUID rakNetGuid;
	SystemAddress systemAddress;

	SystemIndex GetSystemIndex(void) const {if (rakNetGuid!=UNASSIGNED_RAKNET_GUID) return rakNetGuid.systemIndex; else return systemAddress.systemIndex;}
	bool IsUndefined(void) const {return rakNetGuid==UNASSIGNED_RAKNET_GUID && systemAddress==UNASSIGNED_SYSTEM_ADDRESS;}
	void SetUndefined(void) {rakNetGuid=UNASSIGNED_RAKNET_GUID; systemAddress=UNASSIGNED_SYSTEM_ADDRESS;}

	AddressOrGUID() {}
	AddressOrGUID( const AddressOrGUID& input )
	{
		rakNetGuid=input.rakNetGuid;
		systemAddress=input.systemAddress;
	}
	AddressOrGUID( const SystemAddress& input )
	{
		rakNetGuid=UNASSIGNED_RAKNET_GUID;
		systemAddress=input;
	}
	AddressOrGUID( const RakNetGUID& input )
	{
		rakNetGuid=input;
		systemAddress=UNASSIGNED_SYSTEM_ADDRESS;
	}
	AddressOrGUID& operator = ( const AddressOrGUID& input )
	{
		rakNetGuid=input.rakNetGuid;
		systemAddress=input.systemAddress;
		return *this;
	}

	AddressOrGUID& operator = ( const SystemAddress& input )
	{
		rakNetGuid=UNASSIGNED_RAKNET_GUID;
		systemAddress=input;
		return *this;
	}

	AddressOrGUID& operator = ( const RakNetGUID& input )
	{
		rakNetGuid=input;
		systemAddress=UNASSIGNED_SYSTEM_ADDRESS;
		return *this;
	}
};

struct RAK_DLL_EXPORT NetworkID
{
	// This is done because we don't know the global constructor order
	NetworkID()
		:
#if NETWORK_ID_SUPPORTS_PEER_TO_PEER
	guid((uint64_t)-1), systemAddress(0xFFFFFFFF, 0xFFFF),
#endif // NETWORK_ID_SUPPORTS_PEER_TO_PEER
		localSystemAddress(65535)
	{
	}
	~NetworkID() {} 

	/// \deprecated Use NETWORK_ID_SUPPORTS_PEER_TO_PEER in RakNetDefines.h
	// Set this to true to use peer to peer mode for NetworkIDs.
	// Obviously the value of this must match on all systems.
	// True, and this will write the systemAddress portion with network sends.  Takes more bandwidth, but NetworkIDs can be locally generated
	// False, and only localSystemAddress is used.
//	static bool peerToPeerMode;

#if NETWORK_ID_SUPPORTS_PEER_TO_PEER==1

	RakNetGUID guid;

	// deprecated: Use guid instead
	// In peer to peer, we use both systemAddress and localSystemAddress
	// In client / server, we only use localSystemAddress
	SystemAddress systemAddress;
#endif
	unsigned short localSystemAddress;

	NetworkID& operator = ( const NetworkID& input );

	static bool IsPeerToPeerMode(void);
	static void SetPeerToPeerMode(bool isPeerToPeer);
	bool operator==( const NetworkID& right ) const;
	bool operator!=( const NetworkID& right ) const;
	bool operator > ( const NetworkID& right ) const;
	bool operator < ( const NetworkID& right ) const;
};

/// This represents a user message from another system.
struct Packet
{
	// This is now in the systemAddress struct and is used for lookups automatically
	/// Server only - this is the index into the player array that this systemAddress maps to
//	SystemIndex systemIndex;

	/// The system that send this packet.
	SystemAddress systemAddress;

	/// A unique identifier for the system that sent this packet, regardless of IP address (internal / external / remote system)
	/// Only valid once a connection has been established (ID_CONNECTION_REQUEST_ACCEPTED, or ID_NEW_INCOMING_CONNECTION)
	/// Until that time, will be UNASSIGNED_RAKNET_GUID
	RakNetGUID guid;

	/// The length of the data in bytes
	unsigned int length;

	/// The length of the data in bits
	BitSize_t bitSize;

	/// The data from the sender
	unsigned char* data;

	/// @internal
	/// Indicates whether to delete the data, or to simply delete the packet.
	bool deleteData;

	/// @internal
	/// If true, this message is meant for the user, not for the plugins, so do not process it through plugins
	bool bypassPlugins;
};

///  Index of an unassigned player
const SystemIndex UNASSIGNED_PLAYER_INDEX = 65535;

/// Unassigned object ID
const NetworkID UNASSIGNED_NETWORK_ID;

const int PING_TIMES_ARRAY_SIZE = 5;

/// \brief RPC Function Implementation
/// \Deprecated Use RPC3
/// \details The Remote Procedure Call Subsystem provide the RPC paradigm to
/// RakNet user. It consists in providing remote function call over the
/// network.  A call to a remote function require you to prepare the
/// data for each parameter (using BitStream) for example.
///
/// Use the following C function prototype for your callbacks
/// @code
/// void functionName(RPCParameters *rpcParms);
/// @endcode
/// If you pass input data, you can parse the input data in two ways.
/// 1.
/// Cast input to a struct (such as if you sent a struct)
/// i.e. MyStruct *s = (MyStruct*) input;
/// Make sure that the sizeof(MyStruct) is equal to the number of bytes passed!
/// 2.
/// Create a BitStream instance with input as data and the number of bytes
/// i.e. BitStream myBitStream(input, (numberOfBitsOfData-1)/8+1)
/// (numberOfBitsOfData-1)/8+1 is how convert from bits to bytes
/// Full example:
/// @code
/// void MyFunc(RPCParameters *rpcParms) {}
/// RakPeer *rakClient;
/// REGISTER_AS_REMOTE_PROCEDURE_CALL(rakClient, MyFunc);
/// This would allow MyFunc to be called from the server using  (for example)
/// rakServer->RPC("MyFunc", 0, clientID, false);
/// @endcode


/// \def REGISTER_STATIC_RPC
/// \ingroup RAKNET_RPC
/// Register a C function as a Remote procedure.
/// \param[in] networkObject Your instance of RakPeer, RakPeer, or RakPeer
/// \param[in] functionName The name of the C function to call
/// \attention 12/01/05 REGISTER_AS_REMOTE_PROCEDURE_CALL renamed to REGISTER_STATIC_RPC.  Delete the old name sometime in the future
//#pragma deprecated(REGISTER_AS_REMOTE_PROCEDURE_CALL)
//#define REGISTER_AS_REMOTE_PROCEDURE_CALL(networkObject, functionName) REGISTER_STATIC_RPC(networkObject, functionName)
/// \deprecated Use RakNet::RPC3 instead
#define REGISTER_STATIC_RPC(networkObject, functionName) (networkObject)->RegisterAsRemoteProcedureCall((#functionName),(functionName))

/// \def CLASS_MEMBER_ID
/// \ingroup RAKNET_RPC
/// \brief Concatenate two strings

/// \def REGISTER_CLASS_MEMBER_RPC
/// \ingroup RAKNET_RPC
/// \brief Register a member function of an instantiated object as a Remote procedure call.
/// \details RPC member Functions MUST be marked __cdecl!
/// \sa ObjectMemberRPC.cpp
/// \b CLASS_MEMBER_ID is a utility macro to generate a unique signature for a class and function pair and can be used for the Raknet functions RegisterClassMemberRPC(...) and RPC(...)
/// \b REGISTER_CLASS_MEMBER_RPC is a utility macro to more easily call RegisterClassMemberRPC
/// \param[in] networkObject Your instance of RakPeer, RakPeer, or RakPeer
/// \param[in] className The class containing the function
/// \param[in] functionName The name of the function (not in quotes, just the name)
/// \deprecated Use RakNet::RPC3 instead
#define CLASS_MEMBER_ID(className, functionName) #className "_" #functionName
/// \deprecated Use RakNet::RPC3 instead
#define REGISTER_CLASS_MEMBER_RPC(networkObject, className, functionName) {union {void (__cdecl className::*cFunc)( RPCParameters *rpcParms ); void* voidFunc;}; cFunc=&className::functionName; networkObject->RegisterClassMemberRPC(CLASS_MEMBER_ID(className, functionName),voidFunc);}

/// \def UNREGISTER_AS_REMOTE_PROCEDURE_CALL
/// \brief Only calls UNREGISTER_STATIC_RPC

/// \def UNREGISTER_STATIC_RPC
/// \ingroup RAKNET_RPC
/// Unregisters a remote procedure call
/// RPC member Functions MUST be marked __cdecl!  See the ObjectMemberRPC example.
/// \param[in] networkObject The object that manages the function
/// \param[in] functionName The function name
// 12/01/05 UNREGISTER_AS_REMOTE_PROCEDURE_CALL Renamed to UNREGISTER_STATIC_RPC.  Delete the old name sometime in the future
//#pragma deprecated(UNREGISTER_AS_REMOTE_PROCEDURE_CALL)
//#define UNREGISTER_AS_REMOTE_PROCEDURE_CALL(networkObject,functionName) UNREGISTER_STATIC_RPC(networkObject,functionName)
/// \deprecated Use RakNet::RPC3 instead
#define UNREGISTER_STATIC_RPC(networkObject,functionName) (networkObject)->UnregisterAsRemoteProcedureCall((#functionName))

/// \def UNREGISTER_CLASS_INST_RPC
/// \ingroup RAKNET_RPC
/// \deprecated Use the AutoRPC plugin instead
/// \brief Unregisters a member function of an instantiated object as a Remote procedure call.
/// \param[in] networkObject The object that manages the function
/// \param[in] className The className that was originally passed to REGISTER_AS_REMOTE_PROCEDURE_CALL
/// \param[in] functionName The function name
/// \deprecated Use RakNet::RPC3 instead
#define UNREGISTER_CLASS_MEMBER_RPC(networkObject, className, functionName) (networkObject)->UnregisterAsRemoteProcedureCall((#className "_" #functionName))

struct RAK_DLL_EXPORT uint24_t
{
	uint32_t val;

	uint24_t() {}
	inline operator uint32_t() { return val; }
	inline operator uint32_t() const { return val; }

	inline uint24_t(const uint24_t& a) {val=a.val;}
	inline uint24_t operator++() {++val; val&=0x00FFFFFF; return *this;}
	inline uint24_t operator--() {--val; val&=0x00FFFFFF; return *this;}
	inline uint24_t operator++(int) {uint24_t temp(val); ++val; val&=0x00FFFFFF; return temp;}
	inline uint24_t operator--(int) {uint24_t temp(val); --val; val&=0x00FFFFFF; return temp;}
	inline uint24_t operator&(const uint24_t& a) {return uint24_t(val&a.val);}
	inline uint24_t& operator=(const uint24_t& a) { val=a.val; return *this; }
	inline uint24_t& operator+=(const uint24_t& a) { val+=a.val; val&=0x00FFFFFF; return *this; }
	inline uint24_t& operator-=(const uint24_t& a) { val-=a.val; val&=0x00FFFFFF; return *this; }
	inline bool operator==( const uint24_t& right ) const {return val==right.val;}
	inline bool operator!=( const uint24_t& right ) const {return val!=right.val;}
	inline bool operator > ( const uint24_t& right ) const {return val>right.val;}
	inline bool operator < ( const uint24_t& right ) const {return val<right.val;}
	inline const uint24_t operator+( const uint24_t &other ) const { return uint24_t(val+other.val); }
	inline const uint24_t operator-( const uint24_t &other ) const { return uint24_t(val-other.val); }
	inline const uint24_t operator/( const uint24_t &other ) const { return uint24_t(val/other.val); }
	inline const uint24_t operator*( const uint24_t &other ) const { return uint24_t(val*other.val); }

	inline uint24_t(const uint32_t& a) {val=a; val&=0x00FFFFFF;}
	inline uint24_t operator&(const uint32_t& a) {return uint24_t(val&a);}
	inline uint24_t& operator=(const uint32_t& a) { val=a; val&=0x00FFFFFF; return *this; }
	inline uint24_t& operator+=(const uint32_t& a) { val+=a; val&=0x00FFFFFF; return *this; }
	inline uint24_t& operator-=(const uint32_t& a) { val-=a; val&=0x00FFFFFF; return *this; }
	inline bool operator==( const uint32_t& right ) const {return val==(right&0x00FFFFFF);}
	inline bool operator!=( const uint32_t& right ) const {return val!=(right&0x00FFFFFF);}
	inline bool operator > ( const uint32_t& right ) const {return val>(right&0x00FFFFFF);}
	inline bool operator < ( const uint32_t& right ) const {return val<(right&0x00FFFFFF);}
	inline const uint24_t operator+( const uint32_t &other ) const { return uint24_t(val+other); }
	inline const uint24_t operator-( const uint32_t &other ) const { return uint24_t(val-other); }
	inline const uint24_t operator/( const uint32_t &other ) const { return uint24_t(val/other); }
	inline const uint24_t operator*( const uint32_t &other ) const { return uint24_t(val*other); }
};

#endif
