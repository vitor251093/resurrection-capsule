/// \file
/// \brief Declares RakPeer class.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.


#ifndef __RAK_PEER_H
#define __RAK_PEER_H

#include "ReliabilityLayer.h"
#include "RakPeerInterface.h"
#include "RPCNode.h"
#include "RSACrypt.h"
#include "BitStream.h"
#include "SingleProducerConsumer.h"
#include "RPCMap.h"
#include "SimpleMutex.h"
#include "DS_OrderedList.h"
#include "Export.h"
#include "RakString.h"
#include "RakThread.h"
#include "RakNetSocket.h"
#include "RakNetSmartPtr.h"
#include "DS_ThreadsafeAllocatingQueue.h"
#include "SignaledEvent.h"

class HuffmanEncodingTree;
class PluginInterface2;

// Sucks but this struct has to be outside the class.  Inside and DevCPP won't let you refer to the struct as RakPeer::RemoteSystemIndex while GCC
// forces you to do RakPeer::RemoteSystemIndex
struct RemoteSystemIndex{unsigned index; RemoteSystemIndex *next;};
//int RAK_DLL_EXPORT SystemAddressAndIndexComp( const SystemAddress &key, const RemoteSystemIndex &data ); // GCC requires RakPeer::RemoteSystemIndex or it won't compile

// TODO - RakNet 4 Move plugins to each sample directory respectively, to reduce the lib size

///\brief Main interface for network communications.
/// \details It implements most of RakNet's functionality and is the primary interface for RakNet.
///
/// Inherits RakPeerInterface.
///
/// See the individual functions for what the class can do.
/// 
class RAK_DLL_EXPORT RakPeer : public RakPeerInterface
{
public:
	///Constructor
	RakPeer();

	///Destructor
	virtual ~RakPeer();

	// --------------------------------------------------------------------------------------------Major Low Level Functions - Functions needed by most users--------------------------------------------------------------------------------------------
	/// \brief Starts the network threads and opens the listen port.
	/// \details You must call this before calling Connect().
	/// \note Multiple calls while already active are ignored.  To call this function again with different settings, you must first call Shutdown().
	/// \note Call SetMaximumIncomingConnections if you want to accept incoming connections.
	/// \note Set _RAKNET_THREADSAFE in RakNetDefines.h if you want to call RakNet functions from multiple threads (not recommended, as it is much slower and RakNet is already asynchronous).
	/// \param[in] maxConnections Maximum number of connections between this instance of RakPeer and another instance of RakPeer. Required so that the network can preallocate and for thread safety. A pure client would set this to 1.  A pure server would set it to the number of allowed clients.A hybrid would set it to the sum of both types of connections.
	/// \param[in] localPort Port to listen for connections on.
	/// \param[in] _threadSleepTimer Time in milliseconds for the thread to Sleep in each internal update cycle. With new congestion control, the best results will be obtained by passing 10.
	/// \param[in] socketDescriptors An array of SocketDescriptor structures to force RakNet to listen on a particular IP address or port (or both).  Each SocketDescriptor will represent one unique socket.  Do not pass redundant structures.  To listen on a specific port, you can pass SocketDescriptor(myPort,0); for a server.  For a client, it is usually OK to pass SocketDescriptor();
	/// \param[in] socketDescriptorCount The size of the \a socketDescriptors array.  Pass 1 if you are not sure what to pass.
	/// \param[in] threadPriority Passed to the thread creation routine. Use THREAD_PRIORITY_NORMAL for Windows. For Linux based systems, you MUST pass something reasonable based on the thread priorities for your application.
	/// \return False on failure (can't create socket or thread), true on success.
	bool Startup( unsigned short maxConnections, int _threadSleepTimer, SocketDescriptor *socketDescriptors, unsigned socketDescriptorCount, int threadPriority=-99999 );

	/// \brief Secures connections though a combination of SHA1, AES128, SYN Cookies, and RSA to prevent connection spoofing, replay attacks, data eavesdropping, packet tampering, and MitM attacks.
	/// \details If you accept connections, you must call this for the secure connection to be enabled for incoming connections.
	/// If you are connecting to another system, you can call this with public key values for p,q and e before connecting to prevent MitM.
	/// Define how many bits are used in RakNetDefines.h with RAKNET_RSA_FACTOR_LIMBS.
	/// \note There is a significant amount of processing and a slight amount of bandwidth overhead for this feature.
	/// \pre Must be called before Initialize.
	/// \param[in] pubKeyE A pointer to the public keys from the RSACrypt class.  
	/// \param[in] pubKeyN A pointer to the public keys from the RSACrypt class. 
	/// \param[in] privKeyP Public key generated from the RSACrypt class.  
	/// \param[in] privKeyQ Public key generated from the RSACrypt class.  If the private keys are 0, then a new key will be generated when this function is called@see the Encryption sample
	void InitializeSecurity(const char *pubKeyE, const char *pubKeyN, const char *privKeyP, const char *privKeyQ );

	/// \brief Disables all security.
	/// \note Must be called while offline.
	void DisableSecurity( void );

	/// \brief This is useful if you have a fixed-address internal server behind a LAN.
	///
	///  Secure connections are determined by the recipient of an incoming connection. This has no effect if called on the system attempting to connect.	
	/// \note If secure connections are on, do not use secure connections for a specific IP address.
	/// \param[in] ip IP address to add. * wildcards are supported.
	void AddToSecurityExceptionList(const char *ip);

	/// \brief Remove a specific connection previously added via AddToSecurityExceptionList.
	/// \param[in] ip IP address to remove. Pass 0 to remove all IP addresses. * wildcards are supported.
	void RemoveFromSecurityExceptionList(const char *ip);

	/// \brief Checks to see if a given IP is in the security exception list.
	/// \param[in] IP address to check.
	/// \return True if the IP address is found in security exception list, else returns false.
	bool IsInSecurityExceptionList(const char *ip);

	/// \brief Sets the maximum number of incoming connections allowed.
	/// \details If the number of incoming connections is less than the number of players currently connected,
	/// no more players will be allowed to connect.  If this is greater than the maximum number of peers allowed,
	/// it will be reduced to the maximum number of peers allowed.
	/// 
	/// Defaults to 0, meaning by default, nobody can connect to you
	/// \param[in] numberAllowed Maximum number of incoming connections allowed.
	void SetMaximumIncomingConnections( unsigned short numberAllowed );

	/// \brief Returns the value passed to SetMaximumIncomingConnections().
	/// \return Maximum number of incoming connections, which is always <= maxConnections
	unsigned short GetMaximumIncomingConnections( void ) const;

	/// \brief Returns how many open connections exist at this time.
	/// \return Number of open connections.
	unsigned short NumberOfConnections(void) const;

	/// \brief Sets the password for the incoming connections. 
	/// \details  The password must match in the call to Connect (defaults to none).
	/// Pass 0 to passwordData to specify no password.
	/// This is a way to set a low level password for all incoming connections.  To selectively reject connections, implement your own scheme using CloseConnection() to remove unwanted connections.
	/// \param[in] passwordData A data block that incoming connections must match.  This can be just a password, or can be a stream of data. Specify 0 for no password data
	/// \param[in] passwordDataLength The length in bytes of passwordData
	void SetIncomingPassword( const char* passwordData, int passwordDataLength );

	/// \brief Gets the password passed to SetIncomingPassword
	/// \param[out] passwordData  Should point to a block large enough to hold the password data you passed to SetIncomingPassword()
	/// \param[in,out] passwordDataLength Maximum size of the passwordData array.  Modified to hold the number of bytes actually written.
	void GetIncomingPassword( char* passwordData, int *passwordDataLength  );

	/// \brief Connect to the specified host (ip or domain name) and server port.
	/// \details Calling Connect and not calling SetMaximumIncomingConnections acts as a dedicated client.
	/// Calling both acts as a true peer.
	///	
	/// This is a non-blocking connection.
	///
	/// The connection is successful when IsConnected() returns true or Receive() gets a message with the type identifier ID_CONNECTION_ACCEPTED.
	/// If the connection is not successful, such as a rejected connection or no response then neither of these things will happen.
	/// \pre Requires that you first call Initialize.
	/// \param[in] host Either a dotted IP address or a domain name.
	/// \param[in] remotePort Port to connect to on the remote machine.
	/// \param[in] passwordData A data block that must match the data block on the server passed to SetIncomingPassword().  This can be a string or can be a stream of data.  Use 0 for no password.
	/// \param[in] passwordDataLength The length in bytes of passwordData.
	/// \param[in] connectionSocketIndex Index into the array of socket descriptors passed to socketDescriptors in RakPeer::Startup() to determine the one to send on.
	/// \param[in] sendConnectionAttemptCount Number of datagrams to send to the other system to try to connect.
	/// \param[in] timeBetweenSendConnectionAttemptsMS Time to elapse before a datagram is sent to the other system to try to connect. After sendConnectionAttemptCount number of attempts, ID_CONNECTION_ATTEMPT_FAILED is returned.
	/// \param[in] timeoutTime Time to elapse before dropping the connection if a reliable message could not be sent. 0 to use the default value from SetTimeoutTime(UNASSIGNED_SYSTEM_ADDRESS);
	/// \return True on successful initiation. False if you are already connected to this system, a connection to the system is pending,  the domain name cannot be resolved, incorrect parameters, internal error, or too many existing peers.
	/// \note Returning true does not mean you are connected!
	/// TODO - RakNet 4 - return enum
	bool Connect( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex=0, unsigned sendConnectionAttemptCount=12, unsigned timeBetweenSendConnectionAttemptsMS=500, RakNetTime timeoutTime=0 );

	/// \brief Connect to the specified host (ip or domain name) and server port.
	/// \param[in] host Either a dotted IP address or a domain name.
	/// \param[in] remotePort Which port to connect to on the remote machine.
	/// \param[in] passwordData A data block that must match the data block on the server passed to SetIncomingPassword().  This can be a string or can be a stream of data.  Use 0 for no password.
	/// \param[in] passwordDataLength The length in bytes of passwordData.
	/// \param[in] socket A bound socket returned by another instance of RakPeerInterface.
	/// \param[in] sendConnectionAttemptCount Number of datagrams to send to the other system to try to connect.
	/// \param[in] timeBetweenSendConnectionAttemptsMS Time to elapse before a datagram is sent to the other system to try to connect. After sendConnectionAttemptCount number of attempts, ID_CONNECTION_ATTEMPT_FAILED is returned.
	/// \param[in] timeoutTime Time to elapse before dropping the connection if a reliable message could not be sent. 0 to use the default from SetTimeoutTime(UNASSIGNED_SYSTEM_ADDRESS);
	/// \return True on successful initiation. False on incorrect parameters, internal error, or too many existing peers.  
	/// \note Returning true does not mean you arebconnected!
	virtual bool ConnectWithSocket(const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, RakNetSmartPtr<RakNetSocket> socket, unsigned sendConnectionAttemptCount=12, unsigned timeBetweenSendConnectionAttemptsMS=500, RakNetTime timeoutTime=0);

	/* /// \brief Connect to the specified network ID (Platform specific console function)
	/// \details Does built-in NAT traversal
	/// \param[in] networkServiceId Network ID structure for the online service
	/// \param[in] passwordData A data block that must match the data block on the server passed to SetIncomingPassword().  This can be a string or can be a stream of data.  Use 0 for no password.
	/// \param[in] passwordDataLength The length in bytes of passwordData.
	//bool Console2LobbyConnect( void *networkServiceId, const char *passwordData, int passwordDataLength );*/	

	/// \brief Stops the network threads and closes all connections.
	/// \param[in] blockDuration Wait time(milli seconds) for all remaining messages to go out, including ID_DISCONNECTION_NOTIFICATION.  If 0, it doesn't wait at all.
	/// \param[in] orderingChannel Channel on which ID_DISCONNECTION_NOTIFICATION will be sent, if blockDuration > 0.
	/// \param[in] disconnectionNotificationPriority Priority of sending ID_DISCONNECTION_NOTIFICATION.
	/// If set to 0, the disconnection notification won't be sent.
	void Shutdown( unsigned int blockDuration, unsigned char orderingChannel=0, PacketPriority disconnectionNotificationPriority=LOW_PRIORITY );

	/// \brief Returns true if the network thread is running.
	/// \return True if the network thread is running, False otherwise
	bool IsActive( void ) const;

	/// \brief Fills the array remoteSystems with the SystemAddress of all the systems we are connected to.
	/// \param[out] remoteSystems An array of SystemAddress structures, to be filled with the SystemAddresss of the systems we are connected to. Pass 0 to remoteSystems to get the number of systems we are connected to.
	/// \param[in, out] numberOfSystems As input, the size of remoteSystems array.  As output, the number of elements put into the array. 
	bool GetConnectionList( SystemAddress *remoteSystems, unsigned short *numberOfSystems ) const;

	/// Returns the next uint32_t that Send() will return
	/// \note If using RakPeer from multiple threads, this may not be accurate for your thread. Use IncrementNextSendReceipt() in that case.
	/// \return The next uint32_t that Send() or SendList will return
	virtual uint32_t GetNextSendReceipt(void);

	/// Returns the next uint32_t that Send() will return, and increments the value by one
	/// \note If using RakPeer from multiple threads, pass this to forceReceipt in the send function
	/// \return The next uint32_t that Send() or SendList will return
	virtual uint32_t IncrementNextSendReceipt(void);

	/// \brief Sends a block of data to the specified system that you are connected to.
	/// \note This function only works while the connected.
	/// \note The first byte should be a message identifier starting at ID_USER_PACKET_ENUM.
	/// \param[in] data Block of data to send.
	/// \param[in] length Size in bytes of the data to send.
	/// \param[in] priority Priority level to send on.  See PacketPriority.h
	/// \param[in] reliability How reliably to send this data.  See PacketPriority.h
	/// \param[in] orderingChannel When using ordered or sequenced messages, the channel to order these on. Messages are only ordered relative to other messages on the same stream.
	/// \param[in] systemIdentifier Who to send this packet to, or in the case of broadcasting who not to send it to. Pass either a SystemAddress structure or a RakNetGUID structure. Use UNASSIGNED_SYSTEM_ADDRESS or to specify none
	/// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
	/// \param[in] forceReceipt If 0, will automatically determine the receipt number to return. If non-zero, will return what you give it.
	/// \return 0 on bad input. Otherwise a number that identifies this message. If \a reliability is a type that returns a receipt, on a later call to Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS with bytes 1-4 inclusive containing this number
	uint32_t Send( const char *data, const int length, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt=0 );

	/// \brief "Send" to yourself rather than a remote system.
	/// \details The message will be processed through the plugins and returned to the game as usual.
	/// This function works anytime
	/// \note The first byte should be a message identifier starting at ID_USER_PACKET_ENUM
	/// \param[in] data Block of data to send.
	/// \param[in] length Size in bytes of the data to send.
	void SendLoopback( const char *data, const int length );

	/// \brief Sends a block of data to the specified system that you are connected to.
	/// 
	/// Same as the above version, but takes a BitStream as input.
	/// \param[in] bitStream Bitstream to send
	/// \param[in] priority Priority level to send on.  See PacketPriority.h
	/// \param[in] reliability How reliably to send this data.  See PacketPriority.h
	/// \param[in] orderingChannel Channel to order the messages on, when using ordered or sequenced messages. Messages are only ordered relative to other messages on the same stream.
	/// \param[in] systemIdentifier System Address or RakNetGUID to send this packet to, or in the case of broadcasting, the address not to send it to.  Use UNASSIGNED_SYSTEM_ADDRESS to specify none.
	/// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
	/// \param[in] forceReceipt If 0, will automatically determine the receipt number to return. If non-zero, will return what you give it.
	/// \return 0 on bad input. Otherwise a number that identifies this message. If \a reliability is a type that returns a receipt, on a later call to Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS with bytes 1-4 inclusive containing this number
	/// \note COMMON MISTAKE: When writing the first byte, bitStream->Write((unsigned char) ID_MY_TYPE) be sure it is casted to a byte, and you are not writing a 4 byte enumeration.
	uint32_t Send( const RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt=0 );

	/// \brief Sends multiple blocks of data, concatenating them automatically.
	///
	/// This is equivalent to:
	/// RakNet::BitStream bs;
	/// bs.WriteAlignedBytes(block1, blockLength1);
	/// bs.WriteAlignedBytes(block2, blockLength2);
	/// bs.WriteAlignedBytes(block3, blockLength3);
	/// Send(&bs, ...)
	///
	/// This function only works when connected.
	/// \param[in] data An array of pointers to blocks of data
	/// \param[in] lengths An array of integers indicating the length of each block of data
	/// \param[in] numParameters Length of the arrays data and lengths
	/// \param[in] priority Priority level to send on.  See PacketPriority.h
	/// \param[in] reliability How reliably to send this data.  See PacketPriority.h
	/// \param[in] orderingChannel Channel to order the messages on, when using ordered or sequenced messages. Messages are only ordered relative to other messages on the same stream.
	/// \param[in] systemIdentifier System Address or RakNetGUID to send this packet to, or in the case of broadcasting, the address not to send it to.  Use UNASSIGNED_SYSTEM_ADDRESS to specify none.
	/// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
	/// \param[in] forceReceipt If 0, will automatically determine the receipt number to return. If non-zero, will return what you give it.
	/// \return 0 on bad input. Otherwise a number that identifies this message. If \a reliability is a type that returns a receipt, on a later call to Receive() you will get ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS with bytes 1-4 inclusive containing this number
	/// \note Doesn't support the router plugin.
	uint32_t SendList( const char **data, const int *lengths, const int numParameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, uint32_t forceReceipt=0 );

	/// \brief Gets a message from the incoming message queue.
	/// \details Use DeallocatePacket() to deallocate the message after you are done with it.
	/// User-thread functions, such as RPC calls and the plugin function PluginInterface::Update occur here.
	/// \return 0 if no packets are waiting to be handled, otherwise a pointer to a packet.
	/// \note COMMON MISTAKE: Be sure to call this in a loop, once per game tick, until it returns 0. If you only process one packet per game tick they will buffer up.
	/// \sa RakNetTypes.h contains struct Packet.
	Packet* Receive( void );

	/// \brief Call this to deallocate a message returned by Receive() when you are done handling it.
	/// \param[in] packet Message to deallocate.	
	void DeallocatePacket( Packet *packet );

	/// \brief Return the total number of connections we are allowed.
	/// \return Total number of connections allowed.
	unsigned short GetMaximumNumberOfPeers( void ) const;

	// --------------------------------------------------------------------------------------------Remote Procedure Call Functions - Functions to initialize and perform RPC--------------------------------------------------------------------------------------------
	/// \ingroup RAKNET_RPC
	/// \brief Register a C or static member function as available for calling a remote procedure call.
	/// \param[in] uniqueID A null-terminated unique string to identify this procedure.  See RegisterClassMemberRPC() for class member functions.
	/// \param[in] functionPointer(...) The name of the function to be used as a function pointer. This can be called whether active or not, and registered functions stay registered unless unregistered
	/// \deprecated Use RakNet::RPC3
	void RegisterAsRemoteProcedureCall( const char* uniqueID, void ( *functionPointer ) ( RPCParameters *rpcParms ) );

	/// \ingroup RAKNET_RPC
	/// \brief Register a C++ member function as available for calling as a remote procedure call.
	/// \param[in] uniqueID A null terminated string to identify this procedure. It is recommended to use the macro REGISTER_CLASS_MEMBER_RPC to create the string.  Use RegisterAsRemoteProcedureCall() for static functions.
	/// \param[in] functionPointer The name of the function to be used as a function pointer. This can be called whether active or not, and registered functions stay registered unless unregistered with UnregisterAsRemoteProcedureCall.
	/// \sa The sample ObjectMemberRPC.cpp
	/// \deprecated Use RakNet::RPC3
	void RegisterClassMemberRPC( const char* uniqueID, void *functionPointer );

	/// \ingroup RAKNET_RPC
	/// \brief Unregisters a C function as available for calling as a remote procedure call that was formerly registered with RegisterAsRemoteProcedureCall. Only call offline.
	/// \param[in] uniqueID A string of only letters to identify this procedure.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.
		/// \deprecated Use RakNet::RPC3
	void UnregisterAsRemoteProcedureCall( const char* uniqueID );

	/// \ingroup RAKNET_RPC
	/// \brief Used by Object member RPC to lookup objects given that object's ID.
	/// Also used by the ReplicaManager plugin
	/// \param[in] An instance of NetworkIDManager to use for the loookup.
	void SetNetworkIDManager( NetworkIDManager *manager );

	/// \return Returns the value passed to SetNetworkIDManager or 0 if never called.
	NetworkIDManager *GetNetworkIDManager(void) const;

	/// ------------------------------------------- Deprecated -------------------------
	/// \ingroup RAKNET_RPC
	/// Calls a C function on the remote system that was already registered using RegisterAsRemoteProcedureCall().
	/// \pre To use object member RPC (networkID!=UNASSIGNED_OBJECT_ID), The recipient must have called SetNetworkIDManager so the system can handle the object lookups
	/// \param[in] uniqueID A NULL terminated string identifying the function to call.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.
	/// \param[in] data The data to send
	/// \param[in] bitLength The number of bits of \a data
	/// \param[in] priority What priority level to send on. See PacketPriority.h.
	/// \param[in] reliability How reliability to send this data. See PacketPriority.h.
	/// \param[in] orderingChannel When using ordered or sequenced message, what channel to order these on.
	/// \param[in] systemAddress Who to send this message to, or in the case of broadcasting who not to send it to.  Pass either a SystemAddress structure or a RakNetGUID structure. Use UNASSIGNED_SYSTEM_ADDRESS or to specify none
	/// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
	/// \param[in] includedTimestamp Pass a timestamp if you wish, to be adjusted in the usual fashion as per ID_TIMESTAMP.  Pass 0 to not include a timestamp.
	/// \param[in] networkID For static functions, pass UNASSIGNED_NETWORK_ID.  For member functions, you must derive from NetworkIDObject and pass the value returned by NetworkIDObject::GetNetworkID for that object.
	/// \param[in] replyFromTarget If 0, this function is non-blocking.  Otherwise it will block while waiting for a reply from the target procedure, which should be remotely written to RPCParameters::replyToSender and copied to replyFromTarget.  The block will return early on disconnect or if the sent packet is unreliable and more than 3X the ping has elapsed.
	/// \return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	/// \deprecated Use RakNet::RPC3. This only correctly works for C functions. For C++, it only works for Windows. It is less flexible than RPC3
	/// ------------------------------------------- Deprecated -------------------------
	bool RPC( const char* uniqueID, const char *data, BitSize_t bitLength, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RakNetTime *includedTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget );

	/// ------------------------------------------- Deprecated -------------------------
	/// \ingroup RAKNET_RPC
	/// \brief Calls a C function on the remote system that was already registered using RegisterAsRemoteProcedureCall.
	/// \details If you want that function to return data you should call RPC from that system in the same way.Returns true on a successful packet
	/// send (this does not indicate the recipient performed the call), false on failure.
	/// \pre To use object member RPC (networkID!=UNASSIGNED_OBJECT_ID), the recipient must have called SetNetworkIDManager so the system can handle the object lookups.
	/// \param[in] uniqueID A NULL terminated string identifying the function to call.  Recommended you use the macro CLASS_MEMBER_ID for class member functions.
	/*/// \param[in] data The data to send
	/// \param[in] bitLength The number of bits of \a data*/
	///	\param[in] bitStream The bit stream to send.
	/// \param[in] priority What priority level to send on. See PacketPriority.h.
	/// \param[in] reliability How reliably to send this data. See PacketPriority.h.
	/// \param[in] orderingChannel When using ordered or sequenced message, what channel to order these on.
	/// \param[in] systemAddress Who to send this message to, or in the case of broadcasting who not to send it to.  Pass either a SystemAddress structure or a RakNetGUID structure. Use UNASSIGNED_SYSTEM_ADDRESS or to specify none
	/// \param[in] broadcast True to send this packet to all connected systems. If true, then systemAddress specifies who not to send the packet to.
	/// \param[in] includedTimestamp Pass a timestamp if you wish, to be adjusted in the usual fashion as per ID_TIMESTAMP.  Pass 0 to not include a timestamp.
	/// \param[in] networkID For static functions, pass UNASSIGNED_NETWORK_ID.  For member functions, you must derive from NetworkIDObject and pass the value returned by NetworkIDObject::GetNetworkID for that object.
	/// \param[in] replyFromTarget If 0, this function is non-blocking.  Otherwise it will block while waiting for a reply from the target procedure, which should be remotely written to RPCParameters::replyToSender and copied to replyFromTarget.  The block will return early on disconnect or if the sent packet is unreliable and more than 3X the ping has elapsed.
	/// \return True on a successful packet send (this does not indicate the recipient performed the call), false on failure
	/// \deprecated Use RakNet::RPC3. This only correctly works for C functions. For C++, it only works for Windows. It is less flexible than RPC3
	/// ------------------------------------------- Deprecated -------------------------
	bool RPC( const char* uniqueID, const RakNet::BitStream *bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RakNetTime *includedTimestamp, NetworkID networkID, RakNet::BitStream *replyFromTarget );
	
	// -------------------------------------------------------------------------------------------- Connection Management Functions--------------------------------------------------------------------------------------------
	/// \brief Close the connection to another host (if we initiated the connection it will disconnect, if they did it will kick them out).
	/// \details This method closes the connection irrespective of who initiated the connection.
	/// \param[in] target Which system to close the connection to.
	/// \param[in] sendDisconnectionNotification True to send ID_DISCONNECTION_NOTIFICATION to the recipient.  False to close it silently.
	/// \param[in] channel Which ordering channel to send the disconnection notification on, if any
	/// \param[in] disconnectionNotificationPriority Priority to send ID_DISCONNECTION_NOTIFICATION on.
	void CloseConnection( const SystemAddress target, bool sendDisconnectionNotification, unsigned char orderingChannel=0, PacketPriority disconnectionNotificationPriority=LOW_PRIORITY );

	/// \brief Cancel a pending connection attempt.
	/// \details If we are already connected, the connection stays open
	/// \param[in] target Target system to cancel.
	void CancelConnectionAttempt( const SystemAddress target );

	/// Returns if a particular systemAddress has a pending or in progress connection attempt
	/// \param[in] systemAddress The SystemAddress we are referring to
	bool IsConnectionAttemptPending( const SystemAddress systemAddress );

	/// \brief Returns if a particular systemAddress is connected to us.
	///	\note This can also be made to return true if we are in the process of connecting.
	/// \param[in] systemAddress The SystemAddress we are referring to
	/// \param[in] includeInProgress If true, also return true for connections that are in progress but haven't completed
	/// \param[in] includeDisconnecting If true, also return true for connections that are in the process of disconnecting
	/// \note This locks a mutex, do not call too frequently during connection attempts with includeInProgress==true or you may cause the attempt to take longer and fail
	/// \return True if this system is connected and active, false otherwise.
	bool IsConnected(const AddressOrGUID systemIdentifier, bool includeInProgress=false, bool includeDisconnecting=false);

	/// \brief Given \a systemAddress, returns its index into remoteSystemList.
	/// \details Values range from 0 to the maximum number of players allowed - 1.
	/// This includes systems which were formerly connected, but are now not connected.
	/// \param[in] systemAddress The SystemAddress we are referring to
	/// \return The index of this SystemAddress or -1 on system not found.
	int GetIndexFromSystemAddress( const SystemAddress systemAddress ) const;

	/// \brief Given \a index into remoteSystemList, will return a SystemAddress.
	/// This function is only useful for looping through all systems.
	/// 
	/// \param[in] index Index should range between 0 and the maximum number of players allowed - 1.
	/// \return The SystemAddress structure corresponding to \a index in remoteSystemList.
	SystemAddress GetSystemAddressFromIndex( int index );

	/// \brief Same as GetSystemAddressFromIndex but returns RakNetGUID
	/// \param[in] index Index should range between 0 and the maximum number of players allowed - 1.
	/// \return The RakNetGUID
	RakNetGUID GetGUIDFromIndex( int index );

	/// \brief Same as calling GetSystemAddressFromIndex and GetGUIDFromIndex for all systems, but more efficient
	/// Indices match each other, so \a addresses[0] and \a guids[0] refer to the same system
	/// \param[out] addresses All system addresses. Size of the list is the number of connections. Size of the \a addresses list will match the size of the \a guids list.
	/// \param[out] guids All guids. Size of the list is the number of connections. Size of the list will match the size of the \a addresses list.
	void GetSystemList(DataStructures::List<SystemAddress> &addresses, DataStructures::List<RakNetGUID> &guids);

	/// \brief Bans an IP from connecting.
	/// \details Banned IPs persist between connections but are not saved on shutdown nor loaded on startup.
	/// \param[in] IP Dotted IP address. You can use * for a wildcard address, such as 128.0.0. * will ban all IP addresses starting with 128.0.0.
	/// \param[in] milliseconds Gives time in milli seconds for a temporary ban of the IP address.  Use 0 for a permanent ban.
	void AddToBanList( const char *IP, RakNetTime milliseconds=0 );

	/// \brief Allows a previously banned IP to connect. 
	/// param[in] Dotted IP address. You can use * as a wildcard. An IP such as 128.0.0.* will ban all IP addresses starting with 128.0.0.
	void RemoveFromBanList( const char *IP );

	/// \brief Allows all previously banned IPs to connect.
	void ClearBanList( void );

	/// \brief Returns true or false indicating if a particular IP is banned.
	/// \param[in] IP Dotted IP address.
	/// \return True if IP matches any IPs in the ban list, accounting for any wildcards. False otherwise.
	bool IsBanned( const char *IP );

	/// \brief Enable or disable allowing frequent connections from the same IP adderss
	/// \details This is a security measure which is disabled by default, but can be set to true to prevent attackers from using up all connection slots.
	/// \param[in] b True to limit connections from the same ip to at most 1 per 100 milliseconds.
	void SetLimitIPConnectionFrequency(bool b);
	
	// --------------------------------------------------------------------------------------------Pinging Functions - Functions dealing with the automatic ping mechanism--------------------------------------------------------------------------------------------
	/// Send a ping to the specified connected system.
	/// \pre The sender and recipient must already be started via a successful call to Startup()
	/// \param[in] target Which system to ping
	void Ping( const SystemAddress target );

	/// \brief Send a ping to the specified unconnected system. 
	/// \details The remote system, if it is Initialized, will respond with ID_PONG followed by sizeof(RakNetTime) containing the system time the ping was sent. Default is 4 bytes - See __GET_TIME_64BIT in RakNetTypes.h
	/// System should reply with ID_PONG if it is active
	/// \param[in] host Either a dotted IP address or a domain name.  Can be 255.255.255.255 for LAN broadcast.
	/// \param[in] remotePort Which port to connect to on the remote machine.
	/// \param[in] onlyReplyOnAcceptingConnections Only request a reply if the remote system is accepting connections
	/// \param[in] connectionSocketIndex Index into the array of socket descriptors passed to socketDescriptors in RakPeer::Startup() to send on.
	/// \return true on success, false on failure (unknown hostname)
	bool Ping( const char* host, unsigned short remotePort, bool onlyReplyOnAcceptingConnections, unsigned connectionSocketIndex=0 );

	/// \brief Returns the average of all ping times read for the specific system or -1 if none read yet
	/// \param[in] systemAddress Which system we are referring to
	/// \return The ping time for this system, or -1
	int GetAveragePing( const AddressOrGUID systemIdentifier );

	/// \brief Returns the last ping time read for the specific system or -1 if none read yet.
	/// \param[in] systemAddress Which system we are referring to
	/// \return The last ping time for this system, or -1.
	int GetLastPing( const AddressOrGUID systemIdentifier ) const;

	/// \brief Returns the lowest ping time read or -1 if none read yet.
	/// \param[in] systemAddress Which system we are referring to
	/// \return The lowest ping time for this system, or -1.
	int GetLowestPing( const AddressOrGUID systemIdentifier ) const;

	/// Ping the remote systems every so often, or not. Can be called anytime.
	/// By default this is true if GET_TIME_SPIKE_LIMIT is non-zero from RakNetDefines, false otherwise
	/// It would be true by default to prevent timestamp drift, since in the event of a clock spike, the timestamp deltas would no longer be accurate
	/// \param[in] doPing True to start occasional pings.  False to stop them.
	void SetOccasionalPing( bool doPing );
	
	// --------------------------------------------------------------------------------------------Static Data Functions - Functions dealing with API defined synchronized memory--------------------------------------------------------------------------------------------
	/// \brief Sets the data to send along with a LAN server discovery or offline ping reply.
	/// \param[in] data Block of data to send, or 0 for none
	/// \param[in] length Length of the data in bytes, or 0 for none
	/// \note \a length should be under 400 bytes, as a security measure against flood attacks
	/// \sa Ping.cpp
	void SetOfflinePingResponse( const char *data, const unsigned int length );

	/// \brief Returns pointers to a copy of the \a data passed to SetOfflinePingResponse.
	/// \param[out] data A pointer to a copy of the data passed to SetOfflinePingResponse()
	/// \param[out] length A pointer filled in with the length parameter passed to SetOfflinePingResponse()
	/// \sa SetOfflinePingResponse
	void GetOfflinePingResponse( char **data, unsigned int *length );
	
	//--------------------------------------------------------------------------------------------Network Functions - Functions dealing with the network in general--------------------------------------------------------------------------------------------
	/// \brief Returns the unique address identifier that represents you or another system on the the network and is based on your local IP / port.
	/// \param[in] systemAddress Use UNASSIGNED_SYSTEM_ADDRESS to get your behind-LAN address. Use a connected system to get their behind-LAN address
	/// \param[in] index When you have multiple internal IDs, which index to return? Currently limited to MAXIMUM_NUMBER_OF_INTERNAL_IDS (so the maximum value of this variable is MAXIMUM_NUMBER_OF_INTERNAL_IDS-1)
	/// \return Identifier of your system internally, which may not be how other systems see if you if you are behind a NAT or proxy
	SystemAddress GetInternalID( const SystemAddress systemAddress=UNASSIGNED_SYSTEM_ADDRESS, const int index=0 ) const;

	/// \brief Returns the unique address identifier that represents the target on the the network and is based on the target's external IP / port.
	/// \param[in] target The SystemAddress of the remote system. Usually the same for all systems, unless you have two or more network cards.
	SystemAddress GetExternalID( const SystemAddress target ) const;

	/// \brief  Given a connected system address, this method gives the unique GUID representing that instance of RakPeer.
	/// This will be the same on all systems connected to that instance of RakPeer, even if the external system addresses are different.
	/// Complexity is O(log2(n)).
	/// If \a input is UNASSIGNED_SYSTEM_ADDRESS, will return your own GUID
	/// \pre Call Startup() first, or the function will return UNASSIGNED_RAKNET_GUID
	/// \param[in] input The system address of the target system we are connected to.
	const RakNetGUID& GetGuidFromSystemAddress( const SystemAddress input ) const;

	/// \brief Gives the system address of a connected system, given its GUID.
	/// The GUID will be the same on all systems connected to that instance of RakPeer, even if the external system addresses are different.
	/// Currently O(log(n)), but this may be improved in the future
	/// If \a input is UNASSIGNED_RAKNET_GUID, UNASSIGNED_SYSTEM_ADDRESS is returned.
	/// \param[in] input The RakNetGUID of the target system.
	SystemAddress GetSystemAddressFromGuid( const RakNetGUID input ) const;

	/// \brief Set the time, in MS, to use before considering ourselves disconnected after not being able to deliver a reliable message.
	/// \details Default time is 10,000 or 10 seconds in release and 30,000 or 30 seconds in debug.
    /// \param[in] timeMS Time, in MS
	/// \param[in] target SystemAddress structure of the target system. Pass UNASSIGNED_SYSTEM_ADDRESS for all systems.
	void SetTimeoutTime( RakNetTime timeMS, const SystemAddress target );
	
	/// \brief Returns the Timeout time for the given system.
	/// \param[in] target Target system to get the TimeoutTime for. Pass UNASSIGNED_SYSTEM_ADDRESS to get the default value.
	/// \return Timeout time for a given system.
	RakNetTime GetTimeoutTime( const SystemAddress target );

	/*/// \deprecated 8/12/09 
	/// \brief MTU automatically calculated during connection process 
	/// \details Set the MTU per datagram.  It's important to set this correctly - otherwise packets will be needlessly split, decreasing performance and throughput.
	/// Maximum allowed size is MAXIMUM_MTU_SIZE.
	/// Too high of a value will cause packets not to arrive at worst and be fragmented at best.
	/// Too low of a value will split packets unnecessarily.
	/// sa MTUSize.h
	/// \param[in] size The MTU size
	/// \param[in] target Which system to set this for.  UNASSIGNED_SYSTEM_ADDRESS to set the default, for new systems
	/// \pre Can only be called when not connected.
	/// \return false on failure (we are connected), else true
	/// bool SetMTUSize( int size, const SystemAddress target );*/

	/// \brief Returns the current MTU size
	/// \param[in] target Which system to get MTU for.  UNASSIGNED_SYSTEM_ADDRESS to get the default
	/// \return The current MTU size of the target system.
	int GetMTUSize( const SystemAddress target ) const;

	/// \brief Returns the number of IP addresses this system has internally.
	/// \details Get the actual addresses from GetLocalIP()
	unsigned GetNumberOfAddresses( void );

	/// Returns an IP address at index 0 to GetNumberOfAddresses-1 in ipList array.
	/// \param[in] index index into the list of IP addresses
	/// \return The local IP address at this index
	const char* GetLocalIP( unsigned int index );

	/// Is this a local IP?
	/// Checks if this ip is in the ipList array.
	/// \param[in] An IP address to check, excluding the port.
	/// \return True if this is one of the IP addresses returned by GetLocalIP
	bool IsLocalIP( const char *ip );

	/// \brief Allow or disallow connection responses from any IP. 
	/// \details Normally this should be false, but may be necessary when connecting to servers with multiple IP addresses.
	/// \param[in] allow - True to allow this behavior, false to not allow. Defaults to false. Value persists between connections.
	void AllowConnectionResponseIPMigration( bool allow );

	/// \brief Sends a one byte message ID_ADVERTISE_SYSTEM to the remote unconnected system.
	/// This will send our external IP outside the LAN along with some user data to the remote system.
	/// \pre The sender and recipient must already be started via a successful call to Initialize
	/// \param[in] host Either a dotted IP address or a domain name
	/// \param[in] remotePort Which port to connect to on the remote machine.
	/// \param[in] data Optional data to append to the packet.
	/// \param[in] dataLength Length of data in bytes.  Use 0 if no data.
	/// \param[in] connectionSocketIndex Index into the array of socket descriptors passed to socketDescriptors in RakPeer::Startup() to send on.
	/// \return False if IsActive()==false or the host is unresolvable. True otherwise.
	bool AdvertiseSystem( const char *host, unsigned short remotePort, const char *data, int dataLength, unsigned connectionSocketIndex=0 );

	/// \brief Controls how often to return ID_DOWNLOAD_PROGRESS for large message downloads.
	/// \details ID_DOWNLOAD_PROGRESS is returned to indicate a new partial message chunk, roughly the MTU size, has arrived.
	/// As it can be slow or cumbersome to get this notification for every chunk, you can set the interval at which it is returned.
	/// Defaults to 0 (never return this notification).
	/// \param[in] interval How many messages to use as an interval before a download progress notification is returned.
	void SetSplitMessageProgressInterval(int interval);

	/// \brief Returns what was passed to SetSplitMessageProgressInterval().
	/// \return Number of messages to be recieved before a download progress notification is returned. Default to 0.
	int GetSplitMessageProgressInterval(void) const;

	/// \brief Set how long to wait before giving up on sending an unreliable message.
	/// Useful if the network is clogged up.
	/// Set to 0 or less to never timeout.  Defaults to 0.
	/// \param[in] timeoutMS How many ms to wait before simply not sending an unreliable message.
	void SetUnreliableTimeout(RakNetTime timeoutMS);

	/// \brief Send a message to a host, with the IP socket option TTL set to 3.
	/// \details This message will not reach the host, but will open the router.
	/// \param[in] host The address of the remote host in dotted notation.
	/// \param[in] remotePort The port number to send to.
	/// \param[in] ttl Max hops of datagram, set to 3
	/// \param[in] connectionSocketIndex userConnectionSocketIndex.
	/// \remarks Used for NAT-Punchthrough
	void SendTTL( const char* host, unsigned short remotePort, int ttl, unsigned connectionSocketIndex=0 );

	// --------------------------------------------------------------------------------------------Compression Functions - Functions related to the compression layer--------------------------------------------------------------------------------------------
	/// \brief Enables or disables frequency table tracking.
	/// \details This is required to get a frequency table, which is used in GenerateCompressionLayer(). 
	/// This value persists between connect calls and defaults to false (no frequency tracking).
	/// \pre You can call this at any time - however you SHOULD only call it when disconnected.  Otherwise you will only trackpart of the values sent over the network.
	/// \param[in] doCompile True to enable tracking 
	void SetCompileFrequencyTable( bool doCompile );

	/// \brief Returns the frequency of outgoing bytes into outputFrequencyTable
	/// The purpose is to save to file as either a master frequency table from a sample game session for passing to
	/// GenerateCompressionLayer() 
	/// \pre You should only call this when disconnected. Requires that you first enable data frequency tracking by calling SetCompileFrequencyTable(true)
	/// \param[out] outputFrequencyTable  The frequency of each corresponding byte
	/// \return False (failure) if connected or if frequency table tracking is not enabled. Otherwise true (success)
	bool GetOutgoingFrequencyTable( unsigned int outputFrequencyTable[ 256 ] );

	/// \brief This is an optional function to generate the compression layer based on the input frequency table.
	/// \details If you want to use it you should call this twice - once with inputLayer as true and once as false.
	/// The frequency table passed here with inputLayer=true should match the frequency table on the recipient with inputLayer=false.
	/// Likewise, the frequency table passed here with inputLayer=false should match the frequency table on the recipient with inputLayer=true.
	/// Calling this function when there is an existing layer will overwrite the old layer.
	/// \pre You should only call this when disconnected.
	/// \param[in] inputFrequencyTable A frequency table for your data returned from GetOutgoingFrequencyTable()
	/// \param[in] inputLayer Whether inputFrequencyTable represents incoming data from other systems (true) or outgoing data from this system (false).
	/// \return false (failure) if connected.  Otherwise true (success)
	/// \sa Compression.cpp
	bool GenerateCompressionLayer( unsigned int inputFrequencyTable[ 256 ], bool inputLayer );

	/// \brief Delete the output or input layer as specified.
	/// 
	/// This is not necessary to call and is only useful for freeing memory.
	/// \pre You should only call this when disconnected.
	/// \param[in] inputLayer True to mean the inputLayer, false to mean the output layer.
	/// \return False (failure) if connected.  Otherwise True (success).
	bool DeleteCompressionLayer( bool inputLayer );

	/// \brief Returns the compression ratio. 
	/// \details A low compression ratio is good. Compression is defined for outgoing data.
	/// \return The compression ratio
	float GetCompressionRatio( void ) const;

	/// \brief Returns the decompression ratio.
	/// \details A high decompression ratio is good. Decompression ratio is defined for incoming data.
	/// \return The decompression ratio
	float GetDecompressionRatio( void ) const;

	// -------------------------------------------------------------------------------------------- Plugin Functions--------------------------------------------------------------------------------------------
	/// \brief Attatches a Plugin interface to an instance of the base class (RakPeer or PacketizedTCP) to run code automatically on message receipt in the Receive call.
	/// \note If plugins have dependencies on each other then the order does matter - for example the router plugin should go first because it might route messages for other plugins.
	/// \param[in] messageHandler Pointer to the plugin to attach.
	void AttachPlugin( PluginInterface2 *plugin );

	/// \brief Detaches a Plugin interface from the instance of the base class (RakPeer or PacketizedTCP) it is attached to.
	///	\details This method disables the plugin code from running automatically on base class's updates or message receipt.
	/// \param[in] messageHandler Pointer to a plugin to detach.
	void DetachPlugin( PluginInterface2 *messageHandler );

	// --------------------------------------------------------------------------------------------Miscellaneous Functions--------------------------------------------------------------------------------------------
	/// \brief Puts a message back in the receive queue in case you don't want to deal with it immediately.
	/// \param[in] packet The pointer to the packet you want to push back.
	/// \param[in] pushAtHead True to push the packet at the start of the queue so that the next receive call returns it.  False to push it at the end of the queue.
	/// \note Setting pushAtHead to false end makes the packets out of order.
	void PushBackPacket( Packet *packet, bool pushAtHead );

	/// ------------------------------------------- Deprecated -------------------------
	/// \Internal
	/// \deprecated This was added without considering proper architecture
	// \param[in] routerInterface The router to use to route messages to systems not directly connected to this system.
	/// ------------------------------------------- Deprecated -------------------------
	void SetRouterInterface( RouterInterface *routerInterface );

	/// \Internal
	/// \deprecated This was added without considering proper architecture
	// \param[in] routerInterface The router to use to route messages to systems not directly connected to this system.
	void RemoveRouterInterface( RouterInterface *routerInterface );

	/// \internal
	/// \brief For a given system identified by \a guid, change the SystemAddress to send to.
	/// \param[in] guid The connection we are referring to
	/// \param[in] systemAddress The new address to send to
	void ChangeSystemAddress(RakNetGUID guid, SystemAddress systemAddress);

	/// \brief Returns a packet for you to write to if you want to create a Packet for some reason.
	/// You can add it to the receive buffer with PushBackPacket
	/// \param[in] dataSize How many bytes to allocate for the buffer
	/// \return A packet.
	Packet* AllocatePacket(unsigned dataSize);

	/// \brief Get the socket used with a particular active connection.
	/// The smart pointer reference counts the RakNetSocket object, so the socket will remain active as long as the smart pointer does, even if RakNet were to shutdown or close the connection.
	/// \note This sends a query to the thread and blocks on the return value for up to one second. In practice it should only take a millisecond or so.
	/// \param[in] target Which system.
	/// \return A smart pointer object containing the socket information about the target. Be sure to check IsNull() which is returned if the update thread is unresponsive, shutting down, or if this system is not connected.
	virtual RakNetSmartPtr<RakNetSocket> GetSocket( const SystemAddress target );

	/// \brief Gets all sockets in use.
	/// \note This sends a query to the thread and blocks on the return value for up to one second. In practice it should only take a millisecond or so.
	/// \param[out] sockets List of RakNetSocket structures in use. Sockets will not be closed until \a sockets goes out of scope
	virtual void GetSockets( DataStructures::List<RakNetSmartPtr<RakNetSocket> > &sockets );

	/// \internal
	virtual void WriteOutOfBandHeader(RakNet::BitStream *bitStream);

	/// If you need code to run in the same thread as RakNet's update thread, this function can be used for that
	/// \param[in] _userUpdateThreadPtr C callback function
	/// \param[in] _userUpdateThreadData Passed to C callback function
	virtual void SetUserUpdateThread(void (*_userUpdateThreadPtr)(RakPeerInterface *, void *), void *_userUpdateThreadData);

	// --------------------------------------------------------------------------------------------Network Simulator Functions--------------------------------------------------------------------------------------------
	/// \brief Adds simulated ping and packet loss to the outgoing data flow.
	/// \details To simulate bi-directional ping and packet loss, you should call this on both the sender and the recipient, with half the total ping and maxSendBPS values on each.
	/// \deprecated Use http://www.jenkinssoftware.com/raknet/forum/index.php?topic=1671.0 instead.
	/// \note You can exclude network simulator code with the  #define _RELEASE to decrease code size.
	/// \note Doesn't work past version 3.6201
	/// \param[in] packetloss Chance to lose a packet. Ranges from 0 to 1.
	/// \param[in] minExtraPing The minimum time to delay sends.
	/// \param[in] extraPingVariance The additional random time to delay sends.
    void ApplyNetworkSimulator( float packetloss, unsigned short minExtraPing, unsigned short extraPingVariance);

	/// \brief Limits how much outgoing bandwidth can be used per-connection.
	/// This limit does not apply to the sum of all connections!
	/// Exceeding the limit queues up outgoing traffic.
	/// \param[in] maxBitsPerSecond Maximum bits per second to send.  Use 0 for unlimited (default). Once set, it takes effect immedately and persists until called again.
	void SetPerConnectionOutgoingBandwidthLimit( unsigned maxBitsPerSecond );

	/// Returns true if you previously called ApplyNetworkSimulator.
	/// \return Ture if you previously called ApplyNetworkSimulator. False otherwise.
	bool IsNetworkSimulatorActive( void );

	// --------------------------------------------------------------------------------------------Statistical Functions - Functions dealing with API performance--------------------------------------------------------------------------------------------

	/// \brief Returns a structure containing a large set of network statistics for the specified system.
	/// You can map this data to a string using the C style StatisticsToString() function
	/// \param[in] systemAddress Which connected system to get statistics for.
	/// \param[in] rns If you supply this structure,the network statistics will be written to it. Otherwise the method uses a static struct to write the data, which is not threadsafe.
	/// \return 0 if the specified system can't be found. Otherwise a pointer to the struct containing the specified system's network statistics.
	/// \sa RakNetStatistics.h
	RakNetStatistics * const GetStatistics( const SystemAddress systemAddress, RakNetStatistics *rns=0 );
	/// \brief Returns the network statistics of the system at the given index in the remoteSystemList.
	///	\return True if the index is less than the maximum number of peers allowed and the system is active. False otherwise.
	bool GetStatistics( const int index, RakNetStatistics *rns );

	/// \Returns how many messages are waiting when you call Receive()
	virtual unsigned int GetReceiveBufferSize(void);

	// --------------------------------------------------------------------------------------------EVERYTHING AFTER THIS COMMENT IS FOR INTERNAL USE ONLY--------------------------------------------------------------------------------------------
	/// \internal
	char *GetRPCString( const char *data, const BitSize_t bitSize, const SystemAddress systemAddress);

	/// \internal
	bool SendOutOfBand(const char *host, unsigned short remotePort, const char *data, BitSize_t dataLength, unsigned connectionSocketIndex=0 );

	// static Packet *AllocPacket(unsigned dataSize, const char *file, unsigned int line);

	/// \internal
	/// \brief Holds the clock differences between systems, along with the ping
	struct PingAndClockDifferential
	{
		unsigned short pingTime;
		RakNetTime clockDifferential;
	};

	/// \internal
	/// \brief All the information representing a connected system system
	struct RemoteSystemStruct
	{
		bool isActive; // Is this structure in use?
		SystemAddress systemAddress;  /// Their external IP on the internet
		SystemAddress myExternalSystemAddress;  /// Your external IP on the internet, from their perspective
		SystemAddress theirInternalSystemAddress[MAXIMUM_NUMBER_OF_INTERNAL_IDS];  /// Their internal IP, behind the LAN
		ReliabilityLayer reliabilityLayer;  /// The reliability layer associated with this player
		bool weInitiatedTheConnection; /// True if we started this connection via Connect.  False if someone else connected to us.
		PingAndClockDifferential pingAndClockDifferential[ PING_TIMES_ARRAY_SIZE ];  /// last x ping times and calculated clock differentials with it
		int pingAndClockDifferentialWriteIndex;  /// The index we are writing into the pingAndClockDifferential circular buffer
		unsigned short lowestPing; ///The lowest ping value encountered
		RakNetTime nextPingTime;  /// When to next ping this player
		RakNetTime lastReliableSend; /// When did the last reliable send occur.  Reliable sends must occur at least once every timeoutTime/2 units to notice disconnects
		RakNetTime connectionTime; /// connection time, if active.
		unsigned char AESKey[ 16 ]; /// Security key.
		bool setAESKey; /// true if security is enabled.
//		int connectionSocketIndex; // index into connectionSockets to send back on.
		RPCMap rpcMap; /// Mapping of RPC calls to single byte integers to save transmission bandwidth.
		RakNetGUID guid;
		int MTUSize;
		// Reference counted socket to send back on
		RakNetSmartPtr<RakNetSocket> rakNetSocket;

		enum ConnectMode {NO_ACTION, DISCONNECT_ASAP, DISCONNECT_ASAP_SILENTLY, DISCONNECT_ON_NO_ACK, REQUESTED_CONNECTION, HANDLING_CONNECTION_REQUEST, UNVERIFIED_SENDER, SET_ENCRYPTION_ON_MULTIPLE_16_BYTE_PACKET, CONNECTED} connectMode;
	};

protected:

	friend RAK_THREAD_DECLARATION(UpdateNetworkLoop);
	friend RAK_THREAD_DECLARATION(RecvFromLoop);
	friend RAK_THREAD_DECLARATION(UDTConnect);

	/*
#ifdef _WIN32
	// friend unsigned __stdcall RecvFromNetworkLoop(LPVOID arguments);
	friend void __stdcall ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
	friend void __stdcall ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer, unsigned connectionSocketIndex );
	friend unsigned __stdcall UpdateNetworkLoop( LPVOID arguments );
#else
	// friend void*  RecvFromNetworkLoop( void*  arguments );
	friend void ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
	friend void ProcessNetworkPacket( const unsigned int binaryAddress, const unsigned short port, const char *data, const int length, RakPeer *rakPeer, unsigned connectionSocketIndex );
	friend void* UpdateNetworkLoop( void* arguments );
#endif
	*/

	friend void ProcessPortUnreachable( const unsigned int binaryAddress, const unsigned short port, RakPeer *rakPeer );
	friend bool ProcessOfflineNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetSmartPtr<RakNetSocket> rakNetSocket, bool *isOfflineMessage, RakNetTimeUS timeRead );
	friend void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetSmartPtr<RakNetSocket> rakNetSocket, RakNetTimeUS timeRead );
	friend void ProcessNetworkPacket( const SystemAddress systemAddress, const char *data, const int length, RakPeer *rakPeer, RakNetTimeUS timeRead );

	// This is done to provide custom RPC handling when in a blocking RPC
	Packet* ReceiveIgnoreRPC( void );

	int GetIndexFromSystemAddress( const SystemAddress systemAddress, bool calledFromNetworkThread ) const;
	int GetIndexFromGuid( const RakNetGUID guid );

	//void RemoveFromRequestedConnectionsList( const SystemAddress systemAddress );
	// Two versions needed because some buggy compilers strip the last parameter if unused, and crashes
	bool SendConnectionRequest( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex, unsigned int extraData, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime, RakNetSmartPtr<RakNetSocket> socket );
	bool SendConnectionRequest( const char* host, unsigned short remotePort, const char *passwordData, int passwordDataLength, unsigned connectionSocketIndex, unsigned int extraData, unsigned sendConnectionAttemptCount, unsigned timeBetweenSendConnectionAttemptsMS, RakNetTime timeoutTime );
	///Get the reliability layer associated with a systemAddress.  
	/// \param[in] systemAddress The player identifier 
	/// \return 0 if none
	RemoteSystemStruct *GetRemoteSystemFromSystemAddress( const SystemAddress systemAddress, bool calledFromNetworkThread, bool onlyActive ) const;
	RakPeer::RemoteSystemStruct *GetRemoteSystem( const AddressOrGUID systemIdentifier, bool calledFromNetworkThread, bool onlyActive ) const;
	void ValidateRemoteSystemLookup(void) const;
	RemoteSystemStruct *GetRemoteSystemFromGUID( const RakNetGUID guid, bool onlyActive ) const;
	///Parse out a connection request packet
	void ParseConnectionRequestPacket( RakPeer::RemoteSystemStruct *remoteSystem, SystemAddress systemAddress, const char *data, int byteSize);
	///When we get a connection request from an ip / port, accept it unless full
	void OnConnectionRequest( RakPeer::RemoteSystemStruct *remoteSystem, unsigned char *AESKey, bool setAESKey, RakNetTime incomingTimestamp );
	void SendConnectionRequestAccepted(RakPeer::RemoteSystemStruct *remoteSystem, RakNetTime incomingTimestamp);
	///Send a reliable disconnect packet to this player and disconnect them when it is delivered
	void NotifyAndFlagForShutdown( const SystemAddress systemAddress, bool performImmediate, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority );
	///Returns how many remote systems initiated a connection to us
	unsigned short GetNumberOfRemoteInitiatedConnections( void ) const;
	///	\brief Get a free remote system from the list and assign our systemAddress to it.
	/// \note Should only be called from the update thread - not the user thread.
	/// \param[in] systemAddress	systemAddress to be assigned
	/// \param[in] connectionMode	connection mode of the RemoteSystem.
	/// \param[in] rakNetSocket 
	/// \param[in] thisIPConnectedRecently	Is this IP connected recently? set to False;
	/// \param[in] bindingAddress	Address to be binded with the remote system
	/// \param[in] incomingMTU	MTU for the remote system
	RemoteSystemStruct * AssignSystemAddressToRemoteSystemList( const SystemAddress systemAddress, RemoteSystemStruct::ConnectMode connectionMode, RakNetSmartPtr<RakNetSocket> incomingRakNetSocket, bool *thisIPConnectedRecently, SystemAddress bindingAddress, int incomingMTU, RakNetGUID guid );
	///	\brief Adjust the timestamp of the incoming packet to be relative to this system.
	/// \param[in] data	Data in the incoming packet.
	/// \param[in] systemAddress Sender of the incoming packet.
	void ShiftIncomingTimestamp( unsigned char *data, SystemAddress systemAddress ) const;
	/// Get the most accurate clock differential for a certain player.
	/// \param[in] systemAddress The player with whose clock the time difference is calculated.
	/// \returns The clock differential for a certain player.
	RakNetTime GetBestClockDifferential( const SystemAddress systemAddress ) const;

	//void PushPortRefused( const SystemAddress target );
	///  \brief Handles an RPC packet.  This packet has an RPC request
	/// \param[in] data A packet returned from Receive with the ID ID_RPC
	/// \param[in] length The size of the packet data 
	/// \param[in] systemAddress The sender of the packet 
	/// \return True on success, false on a bad packet or an unregistered function
	bool HandleRPCPacket( const char *data, int length, SystemAddress systemAddress );

	///	\brief Handles an RPC reply packet.  The reply packet has data returned from an RPC call
	/// \param[in] data A packet returned from Receive with the ID ID_RPC
	/// \param[in] length The size of the packet data 
	/// \param[in] systemAddress The sender of the packet 
	void HandleRPCReplyPacket( const char *data, int length, SystemAddress systemAddress );

	bool IsLoopbackAddress(const AddressOrGUID &systemIdentifier, bool matchPort) const;
	SystemAddress GetLoopbackAddress(void) const;

	///Set this to true to terminate the Peer thread execution 
	volatile bool endThreads;
	///true if the peer thread is active. 
	volatile bool isMainLoopThreadActive,isRecvFromLoopThreadActive;
	bool occasionalPing;  /// Do we occasionally ping the other systems?*/
	///Store the maximum number of peers allowed to connect
	unsigned short maximumNumberOfPeers;
	//05/02/06 Just using maximumNumberOfPeers instead
	///Store the maximum number of peers able to connect, including reserved connection slots for pings, etc.
	//unsigned short remoteSystemListSize;
	///Store the maximum incoming connection allowed 
	unsigned short maximumIncomingConnections;
	RakNet::BitStream offlinePingResponse;
	///Local Player ID
	SystemAddress mySystemAddress[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
	char incomingPassword[256];
	unsigned char incomingPasswordLength;

	/// This is an array of pointers to RemoteSystemStruct
	/// This allows us to preallocate the list when starting, so we don't have to allocate or delete at runtime.
	/// Another benefit is that is lets us add and remove active players simply by setting systemAddress
	/// and moving elements in the list by copying pointers variables without affecting running threads, even if they are in the reliability layer
	RemoteSystemStruct* remoteSystemList;

	// Use a hash, with binaryAddress plus port mod length as the index
	RemoteSystemIndex **remoteSystemLookup;
	unsigned int RemoteSystemLookupHashIndex(SystemAddress sa) const;
	void ReferenceRemoteSystem(SystemAddress sa, unsigned int remoteSystemListIndex);
	void DereferenceRemoteSystem(SystemAddress sa);
	RemoteSystemStruct* GetRemoteSystem(SystemAddress sa) const;
	unsigned int GetRemoteSystemIndex(SystemAddress sa) const;
	void ClearRemoteSystemLookup(void);
	DataStructures::MemoryPool<RemoteSystemIndex> remoteSystemIndexPool;

//	unsigned int LookupIndexUsingHashIndex(SystemAddress sa) const;
//	unsigned int RemoteSystemListIndexUsingHashIndex(SystemAddress sa) const;
//	unsigned int FirstFreeRemoteSystemLookupIndex(SystemAddress sa) const;
	
	enum
	{
		// Only put these mutexes in user thread functions!
#ifdef _RAKNET_THREADSAFE
		requestedConnectionList_Mutex,
#endif
		offlinePingResponse_Mutex,
		NUMBER_OF_RAKPEER_MUTEXES
	};
	SimpleMutex rakPeerMutexes[ NUMBER_OF_RAKPEER_MUTEXES ];
	///RunUpdateCycle is not thread safe but we don't need to mutex calls. Just skip calls if it is running already

	bool updateCycleIsRunning;
	///The list of people we have tried to connect to recently

	//DataStructures::Queue<RequestedConnectionStruct*> requestedConnectionsList;
	///Data that both the client and the server needs

	unsigned int bytesSentPerSecond, bytesReceivedPerSecond;
	// bool isSocketLayerBlocking;
	// bool continualPing,isRecvfromThreadActive,isMainLoopThreadActive, endThreads, isSocketLayerBlocking;
	unsigned int validationInteger;
	SimpleMutex incomingQueueMutex, banListMutex; //,synchronizedMemoryQueueMutex, automaticVariableSynchronizationMutex;
	//DataStructures::Queue<Packet *> incomingpacketSingleProducerConsumer; //, synchronizedMemorypacketSingleProducerConsumer;
	// BitStream enumerationData;

	struct BanStruct
	{
		char *IP;
		RakNetTime timeout; // 0 for none
	};

	struct RequestedConnectionStruct
	{
		SystemAddress systemAddress;
		RakNetTime nextRequestTime;
		unsigned char requestsMade;
		char *data;
		unsigned short dataLength;
		char outgoingPassword[256];
		unsigned char outgoingPasswordLength;
		unsigned socketIndex;
		unsigned int extraData;
		unsigned sendConnectionAttemptCount;
		unsigned timeBetweenSendConnectionAttemptsMS;
		RakNetTime timeoutTime;
		RakNetSmartPtr<RakNetSocket> socket;
		enum {CONNECT=1, /*PING=2, PING_OPEN_CONNECTIONS=4,*/ /*ADVERTISE_SYSTEM=2*/} actionToTake;
	};

	//DataStructures::List<DataStructures::List<MemoryBlock>* > automaticVariableSynchronizationList;
	DataStructures::List<BanStruct*> banList;
	DataStructures::List<PluginInterface2*> messageHandlerList;

	DataStructures::Queue<RequestedConnectionStruct*> requestedConnectionQueue;
	SimpleMutex requestedConnectionQueueMutex;

	/// Compression stuff
	unsigned int frequencyTable[ 256 ];
	HuffmanEncodingTree *inputTree, *outputTree;
	unsigned int rawBytesSent, rawBytesReceived, compressedBytesSent, compressedBytesReceived;
	// void DecompressInput(RakNet::BitStream *bitStream);
	// void UpdateOutgoingFrequencyTable(RakNet::BitStream * bitStream);
	void GenerateSYNCookieRandomNumber( void );
	void SecuredConnectionResponse( const SystemAddress systemAddress );
	void SecuredConnectionConfirmation( RakPeer::RemoteSystemStruct * remoteSystem, char* data );
	bool RunUpdateCycle( RakNetTimeUS timeNS, RakNetTime timeMS );
	// void RunMutexedUpdateCycle(void);

	struct BufferedCommandStruct
	{
		BitSize_t numberOfBitsToSend;
		PacketPriority priority;
		PacketReliability reliability;
		char orderingChannel;
		AddressOrGUID systemIdentifier;
		bool broadcast;
		RemoteSystemStruct::ConnectMode connectionMode;
		NetworkID networkID;
		bool blockingCommand; // Only used for RPC
		char *data;
		bool haveRakNetCloseSocket;
		unsigned connectionSocketIndex;
		unsigned short remotePortRakNetWasStartedOn_PS3;
		SOCKET socket;
		unsigned short port;
		uint32_t receipt;
		enum {BCS_SEND, BCS_CLOSE_CONNECTION, BCS_GET_SOCKET, BCS_CHANGE_SYSTEM_ADDRESS,/* BCS_USE_USER_SOCKET, BCS_REBIND_SOCKET_ADDRESS, BCS_RPC, BCS_RPC_SHIFT,*/BCS_SEND_OUT_OF_BAND, BCS_DO_NOTHING} command;
	};

	// Single producer single consumer queue using a linked list
	//BufferedCommandStruct* bufferedCommandReadIndex, bufferedCommandWriteIndex;

#ifndef _RAKNET_THREADSAFE
	DataStructures::SingleProducerConsumer<BufferedCommandStruct> bufferedCommands;
#else
	DataStructures::ThreadsafeAllocatingQueue<BufferedCommandStruct> bufferedCommands;
#endif

	// Constructor not called!
	struct RecvFromStruct
	{



		char data[MAXIMUM_MTU_SIZE];

		int bytesRead;
		SystemAddress systemAddress;
		RakNetTimeUS timeRead;
		SOCKET s;
		unsigned short remotePortRakNetWasStartedOn_PS3;
	};

#ifndef _RAKNET_THREADSAFE
	DataStructures::SingleProducerConsumer<RecvFromStruct> bufferedPackets;
#else
	DataStructures::ThreadsafeAllocatingQueue<RecvFromStruct> bufferedPackets;
#endif

	struct SocketQueryOutput
	{
		SocketQueryOutput() {}
		~SocketQueryOutput() {}
		DataStructures::List<RakNetSmartPtr<RakNetSocket> > sockets;
	};
#ifndef _RAKNET_THREADSAFE
	DataStructures::SingleProducerConsumer< SocketQueryOutput > socketQueryOutput;
#else
	DataStructures::ThreadsafeAllocatingQueue<SocketQueryOutput> socketQueryOutput;
#endif

	bool AllowIncomingConnections(void) const;

	void PingInternal( const SystemAddress target, bool performImmediate, PacketReliability reliability );
	bool ValidSendTarget(SystemAddress systemAddress, bool broadcast);
	// This stores the user send calls to be handled by the update thread.  This way we don't have thread contention over systemAddresss
	void CloseConnectionInternal( const AddressOrGUID& systemIdentifier, bool sendDisconnectionNotification, bool performImmediate, unsigned char orderingChannel, PacketPriority disconnectionNotificationPriority );
	void SendBuffered( const char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode, uint32_t receipt );
	void SendBufferedList( const char **data, const int *lengths, const int numParameters, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, RemoteSystemStruct::ConnectMode connectionMode, uint32_t receipt );
	bool SendImmediate( char *data, BitSize_t numberOfBitsToSend, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast, bool useCallerDataAllocation, RakNetTimeUS currentTime, uint32_t receipt );
	//bool HandleBufferedRPC(BufferedCommandStruct *bcs, RakNetTime time);
	void ClearBufferedCommands(void);
	void ClearBufferedPackets(void);
	void ClearSocketQueryOutput(void);
	void ClearRequestedConnectionList(void);
	void AddPacketToProducer(Packet *p);
	unsigned int GenerateSeedFromGuid(void);
	SimpleMutex securityExceptionMutex;

	//DataStructures::AVLBalancedBinarySearchTree<RPCNode> rpcTree;
	RPCMap rpcMap; // Can't use StrPtrHash because runtime insertions will screw up the indices
	int defaultMTUSize;
	bool trackFrequencyTable;
	int threadSleepTimer;

	// Smart pointer so I can return the object to the user
	DataStructures::List<RakNetSmartPtr<RakNetSocket> > socketList;
	void DerefAllSockets(void);
	unsigned int GetRakNetSocketFromUserConnectionSocketIndex(unsigned int userIndex) const;
	// Used for RPC replies
	RakNet::BitStream *replyFromTargetBS;
	SystemAddress replyFromTargetPlayer;
	bool replyFromTargetBroadcast;

	RakNetTime defaultTimeoutTime;

	// Problem:
	// Waiting in function A:
	// Wait function gets RPC B:
	// 
	bool blockOnRPCReply;

	// For redirecting sends through the router plugin.  Unfortunate I have to use this architecture.
	RouterInterface *router;

	// Generate and store a unique GUID
	void GenerateGUID(void);
	unsigned int GetSystemIndexFromGuid( const RakNetGUID input ) const;
	RakNetGUID myGuid;

	unsigned maxOutgoingBPS;

	// Nobody would use the internet simulator in a final build.
#ifdef _DEBUG
	double _packetloss;
	unsigned short _minExtraPing, _extraPingVariance;
#endif

#if   !defined(_WIN32_WCE) 
	/// Encryption and security
	RSACrypt rsacrypt;
	uint32_t publicKeyE;
	uint32_t publicKeyN[RAKNET_RSA_FACTOR_LIMBS];
	bool keysLocallyGenerated, usingSecurity;
	RakNetTime randomNumberExpirationTime;
	unsigned char newRandomNumber[ 20 ], oldRandomNumber[ 20 ];


	/*
	big::RSACrypt<RSA_BIT_SIZE> rsacrypt;
	big::u32 publicKeyE;
	RSA_BIT_SIZE publicKeyN;
	bool keysLocallyGenerated, usingSecurity;
	RakNetTime randomNumberExpirationTime;
	unsigned char newRandomNumber[ 20 ], oldRandomNumber[ 20 ];
	*/
#endif
    
	///How long it has been since things were updated by a call to receiveUpdate thread uses this to determine how long to sleep for
	//unsigned int lastUserUpdateCycle;
	/// True to allow connection accepted packets from anyone.  False to only allow these packets from servers we requested a connection to.
	bool allowConnectionResponseIPMigration;

	SystemAddress firstExternalID;
	int splitMessageProgressInterval;
	RakNetTime unreliableTimeout;


	// Used for object lookup for RPC (actually deprecated, since RPC is deprecated)
	NetworkIDManager *networkIDManager;
	// Systems in this list will not go through the secure connection process, even when secure connections are turned on. Wildcards are accepted.
	DataStructures::List<RakNet::RakString> securityExceptionList;

	char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ];
	unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];

	bool allowInternalRouting;

	void (*userUpdateThreadPtr)(RakPeerInterface *, void *);
	void *userUpdateThreadData;


	SignaledEvent quitAndDataEvents;
	bool limitConnectionFrequencyFromTheSameIP;

	SimpleMutex packetAllocationPoolMutex;
	DataStructures::MemoryPool<Packet> packetAllocationPool;

	SimpleMutex packetReturnMutex;
	DataStructures::Queue<Packet*> packetReturnQueue;
	Packet *AllocPacket(unsigned dataSize, const char *file, unsigned int line);
	Packet *AllocPacket(unsigned dataSize, unsigned char *data, const char *file, unsigned int line);

	/// This is used to return a number to the user when they call Send identifying the message
	/// This number will be returned back with ID_SND_RECEIPT_ACKED or ID_SND_RECEIPT_LOSS and is only returned
	/// with the reliability types that contain RECEIPT in the name
	SimpleMutex sendReceiptSerialMutex;
	uint32_t sendReceiptSerial;
	void ResetSendReceipt(void);

	void OnConnectedPong(RakNetTime sendPingTime, RakNetTime sendPongTime, RemoteSystemStruct *remoteSystem);
};

#endif
