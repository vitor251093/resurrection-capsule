#ifndef __RAKNET_DEFINES_H
#define __RAKNET_DEFINES_H

// If you want to change these defines, put them in RakNetDefinesOverrides so your changes are not lost when updating RakNet
// The user should not edit this file
#include "RakNetDefinesOverrides.h"

/// Define __GET_TIME_64BIT to have RakNetTime use a 64, rather than 32 bit value.  A 32 bit value will overflow after about 5 weeks.
/// However, this doubles the bandwidth use for sending times, so don't do it unless you have a reason to.
/// Comment out if you are using the iPod Touch TG. See http://www.jenkinssoftware.com/forum/index.php?topic=2717.0
#ifndef __GET_TIME_64BIT
#define __GET_TIME_64BIT 1
#endif

/// Makes RakNet threadsafe
/// Do not undefine!
#define _RAKNET_THREADSAFE

/// Define __BITSTREAM_NATIVE_END to NOT support endian swapping in the BitStream class.  This is faster and is what you should use
/// unless you actually plan to have different endianness systems connect to each other
/// Enabled by default.
// #define __BITSTREAM_NATIVE_END

/// Maximum (stack) size to use with _alloca before using new and delete instead.
#ifndef MAX_ALLOCA_STACK_ALLOCATION
#define MAX_ALLOCA_STACK_ALLOCATION 1048576
#endif

// Use WaitForSingleObject instead of sleep.
// Defining it plays nicer with other systems, and uses less CPU, but gives worse RakNet performance
// Undefining it uses more CPU time, but is more responsive and faster.
#ifndef _WIN32_WCE
#define USE_WAIT_FOR_MULTIPLE_EVENTS
#endif

/// Uncomment to use RakMemoryOverride for custom memory tracking
/// See RakMemoryOverride.h. 
#ifndef _USE_RAK_MEMORY_OVERRIDE
#define _USE_RAK_MEMORY_OVERRIDE 0
#endif

/// If defined, OpenSSL is enabled for the class TCPInterface
/// This is necessary to use the SendEmail class with Google POP servers
/// Note that OpenSSL carries its own license restrictions that you should be aware of. If you don't agree, don't enable this define
/// This also requires that you enable header search paths to DependentExtensions\openssl-0.9.8g
// #define OPEN_SSL_CLIENT_SUPPORT
#ifndef OPEN_SSL_CLIENT_SUPPORT
#define OPEN_SSL_CLIENT_SUPPORT 0
#endif

/// Threshold at which to do a malloc / free rather than pushing data onto a fixed stack for the bitstream class
/// Arbitrary size, just picking something likely to be larger than most packets
#ifndef BITSTREAM_STACK_ALLOCATION_SIZE
#define BITSTREAM_STACK_ALLOCATION_SIZE 256
#endif

// Redefine if you want to disable or change the target for debug RAKNET_DEBUG_PRINTF
#ifndef RAKNET_DEBUG_PRINTF
#define RAKNET_DEBUG_PRINTF printf
#endif

// 16 * 4 * 8 = 512 bit. Used for InitializeSecurity()
#ifndef RAKNET_RSA_FACTOR_LIMBS
#define RAKNET_RSA_FACTOR_LIMBS 16
#endif

// Enable to support peer to peer with NetworkIDs. Disable to save memory if doing client/server only
#ifndef NETWORK_ID_SUPPORTS_PEER_TO_PEER
#define NETWORK_ID_SUPPORTS_PEER_TO_PEER 1
#endif

// O(1) instead of O(log2n) but takes more memory if less than 1/3 of the mappings are used.
// Only supported if NETWORK_ID_SUPPORTS_PEER_TO_PEER is commented out
// #define NETWORK_ID_USE_PTR_TABLE

// Maximum number of local IP addresses supported
#ifndef MAXIMUM_NUMBER_OF_INTERNAL_IDS
#define MAXIMUM_NUMBER_OF_INTERNAL_IDS 10
#endif

#ifndef RakAssert



#if defined(_DEBUG)
#define RakAssert(x) assert(x);
#else
#define RakAssert(x) 
#endif

#endif

/// This controls the amount of memory used per connection. If more than this many datagrams are sent without an ack, then the ack has no effect
#ifndef DATAGRAM_MESSAGE_ID_ARRAY_LENGTH
#define DATAGRAM_MESSAGE_ID_ARRAY_LENGTH 512
#endif

/// This is the maximum number of reliable user messages that can be on the wire at a time
#ifndef RESEND_BUFFER_ARRAY_LENGTH
#define RESEND_BUFFER_ARRAY_LENGTH 512
#define RESEND_BUFFER_ARRAY_MASK 511
#endif

/// Uncomment if you want to link in the DLMalloc library to use with RakMemoryOverride
// #define _LINK_DL_MALLOC

#ifndef GET_TIME_SPIKE_LIMIT
/// Workaround for http://support.microsoft.com/kb/274323
/// If two calls between RakNet::GetTime() happen farther apart than this time in microseconds, this delta will be returned instead
/// Note: This will cause ID_TIMESTAMP to be temporarily inaccurate if you set a breakpoint that pauses the UpdateNetworkLoop() thread in RakPeer
/// Define in RakNetDefinesOverrides.h to enable (non-zero) or disable (0)
#define GET_TIME_SPIKE_LIMIT 0
#endif

// Use sliding window congestion control instead of ping based congestion control
#ifndef USE_SLIDING_WINDOW_CONGESTION_CONTROL
#define USE_SLIDING_WINDOW_CONGESTION_CONTROL 1
#endif

// When a large message is arriving, preallocate the memory for the entire block
// This results in large messages not taking up time to reassembly with memcpy, but is vulnerable to attackers causing the host to run out of memory
#ifndef PREALLOCATE_LARGE_MESSAGES
#define PREALLOCATE_LARGE_MESSAGES 0
#endif

#endif // __RAKNET_DEFINES_H
