//Based upon http://www.codeproject.com/KB/IP/port-forwarding.aspx

#ifndef UPNPNAT_H
#define UPNPNAT_H

#include "WindowsIncludes.h"
#include "RakNetTime.h"
#include "Export.h"
#include "RakString.h"
#include "TCPInterface.h"
#include "SocketIncludes.h"
#include "SocketLayer.h"
#include "RakSleep.h"
#include "GetTime.h"


#pragma   warning(disable: 4251) 

/// \defgroup UPNP_GROUP
/// \brief Collection of classes for UPNP

/// \class UPNPNATInternal
/// \brief UPNP class for private RAKNET use, use UPNPPortForwarder instead
/// \details
/// \ingroup UPNP_GROUP
class RAK_DLL_EXPORT UPNPNATInternal
{
public:

	UPNPNATInternal();
	UPNPNATInternal(RakNet::RakString whichInterface, RakNetTime connectToRouterTimeout=1000);
	~UPNPNATInternal(void);

	/// Restart the system
	void Restart(RakNet::RakString whichInterface, RakNetTime connectToRouterTimeout=1000);

	/// Finds a UPNP router. For private RAKNET use, do not use.
	/// \return Success if found, otherwise fails
	bool Discovery(void);

	/// Uses UPNP to open ports, Use the thread wrapper class as it automates much of it and performs checks. For private RAKNET use, do not use.
	/// \param[in] description The name that may show up on router management
	/// \param[in] destinationIp The internal ip of this computer
	/// \param[in] portEx The port to open on the router
	/// \param[in] protocol UDP of TCP
	/// \param[in] destinationPort The port on this machine to forward to
	/// \return Returns true if the router acknowledges the port mapping and returns success, false if the router does not respond or returns failure, this can occur on some routers if the mapping belongs to another machine
	bool AddPortMapping(const char * description, const char * destinationIp, unsigned short int portEx, unsigned short int destinationPort, char * protocol);//add port mapping

	/// During the constructor did anything fail to initialize?
	/// \return Success if started up fine, otherwise fails
	bool DidFailToStart() {return failedStart;}//Did constructor startup successFully?. For private RAKNET use, do not use.
	
	/// Get the last error. For private RAKNET use, do not use.
	/// \return last Error
	const char * GetLastError(){ return lastError.C_String();}

private:
	bool failedStart;
	char httpOk[7];
	int rawSocket;
	SystemAddress serverAddress;
	RakNet::RakString thisSocketAddress;
	static const int maxBuffSize=102400;


		/// Uses UPNP to open ports, Use the thread wrapper class as it automates much of it and performs checks
	/// \param[in] description The name that may show up on router management
	/// \param[in] destinationIp The internal ip of this computer
	/// \param[in] portEx The port to open on the router
	/// \param[in] protocol UDP of TCP
	/// \param[in] destinationPort The port on this machine to forward to
	/// \return
	RakNet::RakString WaitAndReturnResponse(int waitTime);


	TCPInterface tcpInterface;

	/// Gets information from the router for later UPNP communication
	/// \return true on success, false on failure
	bool GetDescription();			//
	
	
	/// Parses information from the router for later UPNP communication
	/// \return true on success, false on failure
	bool ParseDescription();		//
	
	/// The global tcpinterface is connected to the router in this function
	/// \param[in] addr The address to connect to
	/// \param[in] port The internal ip of this computer
	/// \return true on success, false on failure
	bool TcpConnect(const char * addr,unsigned short int port);
	

	typedef enum 
	{
		NAT_INIT=0,
		NAT_FOUND,
		NAT_TCP_CONNECTED,
		NAT_GETDESCRIPTION,
		NAT_GETCONTROL,
		NAT_ADD,
		NAT_DEL,
		NAT_GET,
		NAT_ERROR
	} NAT_STAT;
	NAT_STAT status;
	RakNetTime timeOut;
	RakNet::RakString serviceType;
	RakNet::RakString describeUrl;
	RakNet::RakString controlUrl;
	RakNet::RakString baseUrl;
	RakNet::RakString serviceDescribeUrl;
	RakNet::RakString descriptionInfo;
	RakNet::RakString lastError;
	RakNet::RakString mappingInfo;
};

#endif

