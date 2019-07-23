#include "UPNPPortForwarder.h"
#include "UPNPNATInternal.h"

using namespace RakNet;

RAK_THREAD_DECLARATION(QueryUPNPSupportThread);
RAK_THREAD_DECLARATION(OpenPortOnInterfaceThread);

/*
#if defined(_XBOX) || defined(X360)
#elif defined(_WIN32)
#include "WSAStartupSingleton.h"
#include <ws2tcpip.h> // 'IP_DONTFRAGMENT' 'IP_TTL'
// GetGateways
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")
#endif
*/

void AddToIpList(char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ], const char *in)
{
	int i;
	for (i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
	{
		if (strcmp(ipList[i], in)==0)
			return;
	}

	for (i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
	{
		if (ipList[i][0]==0)
			break;
	}

	if (i < MAXIMUM_NUMBER_OF_INTERNAL_IDS)
		strcpy(ipList[i],in);
}

/*
void GetGateways(char ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ][ 16 ])
{
#if defined(_XBOX) || defined(X360)
#elif defined(_WIN32)
	for (int i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
		ipList[i][0]=0;

	// Microsoft recommends using GetAdaptersAddresses instead of GetAdaptersInfo, but GetAdaptersAddresses does not return Gateways!
	/*
	ULONG outBufLen;
	PIP_ADAPTER_ADDRESSES pAddresses;
	PIP_ADAPTER_ADDRESSES pCurrAddresses;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
	PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
	PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;

	DWORD dwRetVal = 0;

	// Allocate a 15 KB buffer to start with.
	outBufLen = 15000;
	pAddresses = (IP_ADAPTER_ADDRESSES *) rakMalloc_Ex(outBufLen,__FILE__,__LINE__);

	dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, pAddresses, &outBufLen);

	if (dwRetVal == NO_ERROR)
	{
		pCurrAddresses = pAddresses;
		while (pCurrAddresses)
		{
			pUnicast = pCurrAddresses->FirstUnicastAddress;
			while (pUnicast != NULL)
			{
// 				inet_ntop(pUnicast->Address.lpSockaddr->sa_family, pUnicast->Address.lpSockaddr->sa_data,
// 					ipList[ipListIndex++], 16);

				struct hostent *phe = gethostbyname( pUnicast->Address.lpSockaddr->sa_data );
				int idx;
				for ( idx = 0; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
				{
					if (phe->h_addr_list[ idx ] == 0)
						break;

					struct in_addr addr;
					memcpy( &addr, phe->h_addr_list[ idx ], sizeof( struct in_addr ) );
					AddToIpList( ipList, inet_ntoa( addr ) );
				}


				pUnicast = pUnicast->Next;
			}

			pAnycast = pCurrAddresses->FirstAnycastAddress;
			while (pAnycast != NULL)
			{
				pAnycast = pAnycast->Next;
			}

			pMulticast = pCurrAddresses->FirstMulticastAddress;
			while (pMulticast != NULL)
			{
				struct hostent *phe = gethostbyname( pMulticast->Address.lpSockaddr->sa_data );
				int idx;
				for ( idx = 0; idx < MAXIMUM_NUMBER_OF_INTERNAL_IDS; ++idx )
				{
					if (phe->h_addr_list[ idx ] == 0)
						break;

					struct in_addr addr;
					memcpy( &addr, phe->h_addr_list[ idx ], sizeof( struct in_addr ) );
					AddToIpList( ipList, inet_ntoa( addr ) );
				}

				pMulticast = pMulticast->Next;
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
	}

	rakFree_Ex(pAddresses,__FILE__,__LINE__);
	*/
/*
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
//	UINT i;
	// variables used to print DHCP time info
//	struct tm newtime;
//	char buffer[32];
//	errno_t error;


	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *) rakMalloc_Ex(sizeof (IP_ADAPTER_INFO),__FILE__,__LINE__);
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		rakFree_Ex(pAdapterInfo,__FILE__,__LINE__);
		pAdapterInfo = (IP_ADAPTER_INFO *) rakMalloc_Ex(ulOutBufLen,__FILE__,__LINE__);
		if (pAdapterInfo == NULL) {
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return;
		}
	}

	// http://msdn.microsoft.com/en-us/library/aa365917%28VS.85%29.aspx
	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			
		//	printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
			AddToIpList( ipList, pAdapter->GatewayList.IpAddress.String );
			
			pAdapter = pAdapter->Next;
	//		printf("\n");
		}
	} else {
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);

	}
	if (pAdapterInfo)
		rakFree_Ex(pAdapterInfo,__FILE__,__LINE__);


#else
	unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
	SocketLayer::GetMyIP( ipList, unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS] )
#endif
}
*/


#if defined(_XBOX) || defined(X360)
static const int THREAD_PRIORITY=0;
#elif defined(_WIN32)
static const int THREAD_PRIORITY=0;
#elif defined(_PS3) || defined(__PS3__) || defined(SN_TARGET_PS3)
static const int THREAD_PRIORITY=1000;
#else
static const int THREAD_PRIORITY=1000;
#endif

/// Used by UPNPCallbackInterface::QueryUPNPSupport_Result
struct QueryUPNPSupportInput
{
	// Input parameters copied from UPNPPortForwarder::QueryUPNPSupport
	RakNet::RakString interfaceQueried;

	// Pointer to calling class
	UPNPPortForwarder *upnpPortForwarder;

	// Pointer to callback to be called when the operation is done
	UPNPCallbackInterface *callBack;
};

UPNPPortForwarder::UPNPPortForwarder(void)
{
}

UPNPPortForwarder::~UPNPPortForwarder(void)
{
}

void UPNPPortForwarder::QueryUPNPSupport(UPNPCallbackInterface *callBack,RakNet::RakString interfaceIp)
{
	QueryUPNPSupportInput *fupnpi = OP_NEW<QueryUPNPSupportInput>(__FILE__,__LINE__);
	fupnpi->interfaceQueried=interfaceIp;
	fupnpi->upnpPortForwarder=this;
	fupnpi->callBack=callBack;
	RakNet::RakThread::Create(QueryUPNPSupportThread,fupnpi,THREAD_PRIORITY);
}

void UPNPPortForwarder::OpenPortOnInterface(UPNPCallbackInterface *callBack,int internalPort,RakNet::RakString interfaceIp,int portToOpenOnRouter, RakNet::RakString mappingName, UPNPProtocolType protocol)
{
	OpenPortResult *opr = openedPorts.Allocate(__FILE__,__LINE__);
	opr->internalPort=internalPort;
	opr->interfaceIp=interfaceIp;
	if (portToOpenOnRouter==-1)
		opr->portToOpenOnRouter=internalPort;
	else
		opr->portToOpenOnRouter=portToOpenOnRouter;
	seedMT((unsigned int)RakNet::GetTime()+internalPort+opr->portToOpenOnRouter);
	if (mappingName=="")
	{
		mappingName="UPNPRAKNET";
		for (int i=0;i<3;i++)
			mappingName+=(char)(randomMT()%26+65);
	}
	opr->mappingName=mappingName;
	opr->protocol=protocol;
	opr->upnpPortForwarder=this;
	opr->callBack=callBack;
	RakNet::RakThread::Create(OpenPortOnInterfaceThread,opr,THREAD_PRIORITY);
}

void UPNPPortForwarder::CallCallbacks(void)
{
	QueryUPNPSupportResult *r1;
	while ((r1 = foundInterfaces.Pop())!=0)
	{
		r1->callBack->QueryUPNPSupport_Result(r1);
		foundInterfaces.Deallocate(r1,__FILE__,__LINE__);
	}

	OpenPortResult *r2;
	while ((r2 = openedPorts.Pop())!=0)
	{
		r2->callBack->OpenPortOnInterface_Result(r2);
		openedPorts.Deallocate(r2,__FILE__,__LINE__);
	}
}

RAK_THREAD_DECLARATION(QueryUPNPSupportThread)
{
	UPNPNATInternal upnp;

	QueryUPNPSupportInput *fupnpi = ( QueryUPNPSupportInput * ) arguments;
	QueryUPNPSupportResult *result = fupnpi->upnpPortForwarder->foundInterfaces.Allocate(__FILE__,__LINE__);
	result->callBack=fupnpi->callBack;

	char ipList[MAXIMUM_NUMBER_OF_INTERNAL_IDS][16];
	if (fupnpi->interfaceQueried=="ALL")
	{
		unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
		SocketLayer::GetMyIP(ipList, binaryAddresses);
	}
	else
	{
		strcpy(ipList[0],fupnpi->interfaceQueried.C_String());
		ipList[1][0]=0;
	}

	for (unsigned int i=0;ipList[i][0]!=0;i++)
	{
		if (strcmp(ipList[i],"127.0.0.1")!=0 &&
			strcmp(ipList[i],"0.0.0.0")!=0
			)//Skip localhost
			result->interfacesQueried.Push(ipList[i],__FILE__,__LINE__);
	}

	if (result->interfacesQueried.Size()==0)
	{
		result->callBack->UPNPStatusUpdate("No interfaces to query\n");
	}

	for (unsigned int i=0;i < result->interfacesQueried.Size();i++)
	{
		fupnpi->callBack->UPNPStatusUpdate(RakNet::RakString ("QueryUPNPSupportThread %s... ",result->interfacesQueried[i].C_String()));
		upnp.Restart(result->interfacesQueried[i].C_String(), 250);

		if (upnp.Discovery())
		{
			fupnpi->callBack->UPNPStatusUpdate("Found.\n");
			result->interfacesFound.Push(result->interfacesQueried[i],__FILE__,__LINE__);
		}
		else
		{
			fupnpi->callBack->UPNPStatusUpdate(upnp.GetLastError());
		}
	}

	fupnpi->upnpPortForwarder->foundInterfaces.Push(result);

	RakNet::OP_DELETE(fupnpi,__FILE__,__LINE__);

	return 0;
}

RAK_THREAD_DECLARATION(OpenPortOnInterfaceThread)
{
	UPNPNATInternal upnp;
	OpenPortResult *opr = ( OpenPortResult * ) arguments;

	char ipList[MAXIMUM_NUMBER_OF_INTERNAL_IDS][16];
	if (opr->interfaceIp=="ALL")
	{
		unsigned int binaryAddresses[MAXIMUM_NUMBER_OF_INTERNAL_IDS];
		SocketLayer::GetMyIP(ipList, binaryAddresses);
	}
	else
	{
		strcpy(ipList[0],opr->interfaceIp.C_String());
		ipList[1][0]=0;
	}

	for (unsigned int i=0;ipList[i][0]!=0;i++)
	{
		if (strcmp(ipList[i],"127.0.0.1")!=0 &&
			strcmp(ipList[i],"0.0.0.0")!=0
			)//Skip localhost
			opr->interfacesQueried.Push(ipList[i],__FILE__,__LINE__);
	}

	if (opr->interfacesQueried.Size()==0)
	{
		opr->callBack->UPNPStatusUpdate("No interfaces to open\n");
	}


	for (unsigned int i=0;i < opr->interfacesQueried.Size();i++)
	{
		opr->callBack->UPNPStatusUpdate(RakNet::RakString ("OpenPortOnInterfaceThread %s intPort=%i routerPort=%i... ",opr->interfacesQueried[i].C_String(),opr->internalPort,opr->portToOpenOnRouter));
		upnp.Restart(opr->interfacesQueried[i].C_String(), 1000);
		if (upnp.Discovery())
		{
			opr->interfacesFound.Push(opr->interfacesQueried[i],__FILE__,__LINE__);
			if (upnp.AddPortMapping(opr->mappingName.C_String(),opr->interfacesQueried[i].C_String(),opr->portToOpenOnRouter,opr->internalPort,(char*)(opr->protocol==UDP ? "UDP" : "TCP")))
			{
				opr->callBack->UPNPStatusUpdate("Opened.\n");
				opr->succeeded.Push(true,__FILE__,__LINE__);
			}
			else
			{
				opr->callBack->UPNPStatusUpdate("Not opened.\n");
				opr->succeeded.Push(false,__FILE__,__LINE__);
			}
		}
		else
		{
			opr->callBack->UPNPStatusUpdate(RakNet::RakString("Failed discovery on %s.\n", opr->interfacesQueried[i].C_String()));
		}
	}

	opr->upnpPortForwarder->openedPorts.Push(opr);

	return 0;
}
