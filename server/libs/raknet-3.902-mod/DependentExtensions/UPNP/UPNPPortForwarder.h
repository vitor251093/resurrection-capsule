#ifndef __UPNP_PORT_FORWADER
#define __UPNP_PORT_FORWADER

#include "RakString.h"
#include "Export.h"
#include "RakThread.h"
#include "Rand.h"
#include "DS_ThreadsafeAllocatingQueue.h"

namespace RakNet
{

enum UPNPProtocolType {UDP,TCP};

class UPNPPortForwarder;
class UPNPCallbackInterface;

/// Used by UPNPCallbackInterface::OpenPortOnInterface_Result
struct OpenPortResult
{
	// Input parameters copied from UPNPPortForwarder::OpenPortOnInterface
	int internalPort;
	RakNet::RakString interfaceIp;
	int portToOpenOnRouter;
	RakNet::RakString mappingName;
	UPNPProtocolType protocol;

	// Output parameters
	/// interfacesQueried will be equal to interfaceIp if specified. If ALL, then it may be a list
	DataStructures::List<RakNet::RakString> interfacesQueried;
	/// List of interfaces found that support UPNP
	DataStructures::List<RakNet::RakString> interfacesFound;
	/// For each interface in the list of interfacesFound, succeeded will be set
	DataStructures::List<bool> succeeded;

	/// \internal Pointer to calling class
	UPNPPortForwarder *upnpPortForwarder;

	/// \internal  Pointer to callback to be called when the operation is done
	UPNPCallbackInterface *callBack;
};

/// Contains results of QueryUPNPSupport
struct QueryUPNPSupportResult
{
	DataStructures::List<RakNet::RakString> interfacesQueried;
	DataStructures::List<RakNet::RakString> interfacesFound;

	/// \internal  Pointer to callback to be called when the operation is done
	UPNPCallbackInterface *callBack;
};

/// \brief Progress event notifications from UPNPPortForwarder
/// \ingroup UPNP_GROUP
class RAK_DLL_EXPORT UPNPCallbackInterface
{
public:
	/// This is called to receive status updates, don't override if you don't wish to use 
	/// \param[out] stringToPrint The string that is being printed about what is going on.
	virtual void UPNPStatusUpdate(RakNet::RakString stringToPrint){(void) stringToPrint;}

	/// Result from UPNPPortForwarder::QueryUPNPSupport
	/// \param[out] interfacesQueried If ALL was passed to \a interfaceIp in QueryUPNPSupport, then this list may contain more than one element. Otherwise, it contains the one you passed to it.
	/// \param[out] interfacesFound List of length equal to or less than interfacesQueried. May be none.
	virtual void QueryUPNPSupport_Result(QueryUPNPSupportResult *result) {(void)result;}

	/// Result from UPNPPortForwarder::OpenPortOnInterface
	/// \param[out] internalPort The internal port given when the function was called
	/// \param[out] portToOpenOnRouter The port to open on the router,given when the function was called
	virtual void OpenPortOnInterface_Result(OpenPortResult *result) {(void)result;}

};

/// \brief Threaded interface to UPNP class for programmers to use
/// \details Call either QueryUPNPSupport() or OpenPortOnInterface()
/// Pass the callback you want called when the call completes
/// Periodically call CallCallbacks() when you want the callback to be called
/// \ingroup UPNP_GROUP
class RAK_DLL_EXPORT UPNPPortForwarder 
{
public:
	UPNPPortForwarder();
	~UPNPPortForwarder();

	/// Query if the router supports UPNP at all, or if there is no router
	/// Will call back UPNPCallBackInterface::NoRouterFoundOnAnyInterface(), or UPNPCallBackInterface::RouterFound()
	/// \param[in] callBack Pass in a class that has UPNPCallBackInterface as a superclass for the event notifications
	void QueryUPNPSupport(UPNPCallbackInterface *callBack,RakNet::RakString interfaceIp="ALL");

	/// Creates a new thread and queries selected interface if selected, or all interfaces if not selected for a UPNP router and forwards the ports
	/// \param[in] callBack Pass in a class that has UPNPCallBackInterface as a superclass for the event notifications
	/// \param[in] internalPort The port number used on the interfaces that this machine is running on.
	/// \param[in] interfaceIp (Optional) The ip of the interface you wish to query for UPNP routers and open, by default,or if "ALL" is passed it will try all the interfaces. 127.0.0.1 will be skipped.
	/// \param[in] portToOpenOnRouter (Optional) The external port on the router that the outside sees, copies the internalPort by default or if set to -1
	/// \param[in] mappingName (Optional) This is the name that may show up under the router management, defaults to UPNPRAKNET and three random chars
	/// \param[in] protocol (Optional) What type of packets do you want to forward UDP or TCP, defaults to UDP
	void OpenPortOnInterface(UPNPCallbackInterface *callBack,int internalPort,RakNet::RakString interfaceIp="ALL",int portToOpenOnRouter=-1, RakNet::RakString mappingName="", UPNPProtocolType protocol=UDP);

	/// Required that the user call this to get the UPNPCallbackInterface calls
	/// Calls happen in the calling thread
	void CallCallbacks(void);

	/// \internal
	DataStructures::ThreadsafeAllocatingQueue<QueryUPNPSupportResult> foundInterfaces;
	/// \internal
	DataStructures::ThreadsafeAllocatingQueue<OpenPortResult> openedPorts;	


};
}



#endif
