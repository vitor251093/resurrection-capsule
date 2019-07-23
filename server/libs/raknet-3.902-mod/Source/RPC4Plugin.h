/// \file
/// \brief Remote procedure call, supporting C functions only. No external dependencies required.
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#include "NativeFeatureIncludes.h"
#if _RAKNET_SUPPORT_RPC4Plugin==1


#ifndef __RPC_4_PLUGIN_H
#define __RPC_4_PLUGIN_H

class RakPeerInterface;
class NetworkIDManager;
#include "PluginInterface2.h"
#include "PacketPriority.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "RakString.h"
#include "NetworkIDObject.h"
#include "DS_StringKeyedHash.h"

#ifdef _MSC_VER
#pragma warning( push )
#endif

/// \defgroup RPC_PLUGIN_GROUP RPC
/// \brief Remote procedure calls, without external dependencies.
/// \details This should not be used at the same time as RPC3. This is a less functional version of RPC3, and is here for users that do not want the Boost dependency of RPC3.
/// \ingroup PLUGINS_GROUP

namespace RakNet
{
	/// \brief Error codes returned by a remote system as to why an RPC function call cannot execute
	/// \details Error code follows packet ID ID_RPC_REMOTE_ERROR, that is packet->data[1]<BR>
	/// Name of the function will be appended starting at packet->data[2]
	/// \ingroup RPC_PLUGIN_GROUP
	enum RPCErrorCodes
	{
		/// Named function was not registered with RegisterFunction(). Check your spelling.
		RPC_ERROR_FUNCTION_NOT_REGISTERED,
	};

	/// \brief The RPC4Plugin plugin is just an association between a C function pointer and a string.
	/// \details It is for users that want to use RPC, but do not want to use boost.
	/// You do not have the automatic serialization or other features of RPC3, and C++ member calls are not supported.
	/// \ingroup RPC_PLUGIN_GROUP
	class RPC4Plugin : public PluginInterface2
	{
	public:
		// Constructor
		RPC4Plugin();

		// Destructor
		virtual ~RPC4Plugin();

		/// \brief Register a function pointer to be callable from a remote system
		/// \details The hash of the function name will be stored as an association with the function pointer
		/// When a call is made to call this function from the \a Call() or CallLoopback() function, the function pointer will be invoked with the passed bitStream to Call() and the actual Packet that RakNet got.
		/// \param[in] uniqueID Identifier to be associated with \a functionPointer. If this identifier is already in use, the call will return false.
		/// \param[in] functionPointer C function pointer to be called
		/// \return True if the hash of uniqueID is not in use, false otherwise.
		bool RegisterFunction(const char* uniqueID, void ( *functionPointer ) ( RakNet::BitStream *userData, Packet *packet ));

		/// \brief Unregister a function pointer previously registered with RegisterFunction()
		/// \param[in] Identifier originally passed to RegisterFunction()
		/// \return True if the hash of uniqueID was in use, and hence removed. false otherwise.
		bool UnregisterFunction(const char* uniqueID);

		/// Send to the attached instance of RakPeer. See RakPeerInterface::SendLoopback()
		/// \param[in] Identifier originally passed to RegisterFunction() on the local system
		/// \param[in] bitStream bitStream encoded data to send to the function callback
		void CallLoopback( const char* uniqueID, RakNet::BitStream * bitStream );

		/// Send to the specified remote instance of RakPeer.
		/// \param[in] Identifier originally passed to RegisterFunction() on the remote system(s)
		/// \param[in] bitStream bitStream encoded data to send to the function callback
		/// \param[in] priority See RakPeerInterface::Send()
		/// \param[in] reliability See RakPeerInterface::Send()
		/// \param[in] orderingChannel See RakPeerInterface::Send()
		/// \param[in] systemIdentifier See RakPeerInterface::Send()
		/// \param[in] broadcast See RakPeerInterface::Send()
		void Call( const char* uniqueID, RakNet::BitStream * bitStream, PacketPriority priority, PacketReliability reliability, char orderingChannel, const AddressOrGUID systemIdentifier, bool broadcast );

	protected:

		// --------------------------------------------------------------------------------------------
		// Packet handling functions
		// --------------------------------------------------------------------------------------------
		virtual PluginReceiveResult OnReceive(Packet *packet);

		DataStructures::StringKeyedHash<void ( * ) ( RakNet::BitStream *, Packet * ),64> registeredFunctions;

	};

} // End namespace

#endif

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // _RAKNET_SUPPORT_*
