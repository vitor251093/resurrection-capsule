#include "UPNPNATInternal.h"
#include "xmlParser.h" // Found at DependentExtensions/XML
static bool parseUrl(const char* url,RakNet::RakString& host,unsigned short* port,RakNet::RakString& path)
{
	RakNet::RakString strUrl=url;

	unsigned int pos1,pos2,pos3;
	pos1=(unsigned int)strUrl.Find("://");
	if(pos1==RakNet::RakString::nPos)
	{
		return false;
	}
	pos1=pos1+3;

	pos2=(unsigned int)strUrl.Find(":",pos1);
	if(pos2==RakNet::RakString::nPos)
	{
		*port=80;
		pos3=(unsigned int)strUrl.Find("/",pos1);
		if(pos3==RakNet::RakString::nPos)
		{
			return false;
		}

		host=strUrl.SubStr(pos1,pos3-pos1);
	}
	else
	{
		host=strUrl.SubStr(pos1,pos2-pos1);
		pos3=(unsigned int)strUrl.Find("/",pos1);
		if(pos3==RakNet::RakString::nPos)
		{
			return false;
		}

		RakNet::RakString strPort=strUrl.SubStr(pos2+1,pos3-pos2-1);
		*port=(unsigned short)atoi(strPort.C_String());
	}

	if(pos3+1>=strUrl.GetLength())
	{
		path="/";
	}
	else
	{
		path=strUrl.SubStr((unsigned int)pos3,(unsigned int)strUrl.GetLength());
	}	

	return true;
}

UPNPNATInternal::UPNPNATInternal(RakNet::RakString whichInterface,RakNetTime timeout)
{
	Restart(whichInterface, timeout);
}

UPNPNATInternal::~UPNPNATInternal(){}

UPNPNATInternal::UPNPNATInternal()
{
	timeOut=0;
	status=NAT_INIT;
	strcpy(httpOk , "200 OK");
}

void UPNPNATInternal::Restart(RakNet::RakString whichInterface,RakNetTime timeout)
{
	tcpInterface.Stop();

	timeOut=timeout;
	status=NAT_INIT;
	strcpy(httpOk , "200 OK");
	thisSocketAddress= whichInterface;
#ifdef _WIN32
	failedStart=tcpInterface.Start(0,0,1);
#else
	failedStart=tcpInterface.Start(0,0,1);
#endif
}

bool UPNPNATInternal::TcpConnect(const char * host,unsigned short int port)
{
	tcpInterface.Connect(host,port,true);

	RakNetTime stopWaiting = RakNet::GetTime() + timeOut;
	while (RakNet::GetTime() < stopWaiting)
	{

		RakSleep(50);

		serverAddress=tcpInterface.HasCompletedConnectionAttempt();

		if(serverAddress!=UNASSIGNED_SYSTEM_ADDRESS)
		{
			status=NAT_TCP_CONNECTED;
			return true;
		}
	}

	status=NAT_ERROR;
	char temp[100];
	sprintf(temp,"Fail to connect to %s:%i (using TCP)\n",host,port);
	lastError=temp;

	return false;	
}

bool UPNPNATInternal::Discovery(void)
{
	const RakNet::RakString searchRequestString= "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nST:upnp:rootdevice\r\nMAN:\"ssdp:discover\"\r\nMX:3\r\n\r\n";
	int ret;
	RakNet::RakString sendBuff=searchRequestString;
	RakNet::RakString recvBuff;
	RakNet::RakString broadCastAddress ="";
	u_long hostAddr;
	u_long netMask;
	u_long netAddr;
	u_long dirBcastAddr;

	const int targetPort =1900;

	bool bOptVal = true;
	int bOptLen = sizeof(bool);
	int iOptLen = sizeof(int);
	RakNetSmartPtr<RakNetSocket> socketptr;
	socketptr= new RakNetSocket();
	
	socketptr->s= (unsigned int)SocketLayer::CreateBoundSocket(0,false,thisSocketAddress.C_String(),true);
	SocketLayer::SetNonBlocking(socketptr->s);
	
	SystemAddress connvertSys,convertSys2;

	connvertSys.SetBinaryAddress(thisSocketAddress.C_String());

	RakNet::RakString subNetAddress=SocketLayer::GetSubNetForSocketAndIp(socketptr->s,thisSocketAddress);

	if (subNetAddress=="")
	{
		subNetAddress="255.255.255.0";
	}
	hostAddr = connvertSys.binaryAddress;   
	convertSys2.SetBinaryAddress(subNetAddress.C_String());
	netMask = convertSys2.binaryAddress;  
	netAddr = (hostAddr & netMask);         
	dirBcastAddr = netAddr | (~netMask); 
	char tmpBroadcast[32];

	sprintf(tmpBroadcast,"%d.%d.%d.%d",  ((dirBcastAddr >> (24 - 8 * 3)) & 0xFF),((dirBcastAddr >> (24 - 16)) & 0xFF),((dirBcastAddr >> (24 - 8 )) & 0xFF),((dirBcastAddr >> (24)) & 0xFF));

	broadCastAddress=tmpBroadcast;
	char buff2[maxBuffSize];
	SocketLayer::SendTo(socketptr->s, sendBuff.C_String(), (int) sendBuff.GetLength()+1, broadCastAddress.C_String(), targetPort, 0, __FILE__, __LINE__);

	SystemAddress outAddress;
	RakNetTimeUS timeOut;
	RakNetTime stopWaiting = RakNet::GetTime() + 5000;//Reply times can vary, this value seems to be a good buffer to catch
	//the reply so the discovery doesn't fail, most of the time the reply is almost immediate
	while (RakNet::GetTime() < stopWaiting)
	{

		memset(buff2, 0, sizeof(buff2));

		SocketLayer::RawRecvFromNonBlocking( socketptr->s, false, buff2, &ret, &outAddress, &timeOut);
		if(ret==-1){
			RakSleep(100);
			continue;
		}

		recvBuff=buff2;

		ret=(int) recvBuff.Find(httpOk);

		if(ret==RakNet::RakString::nPos)
			continue;                       //invalid response

		size_t begin=recvBuff.Find("http://");
		if(begin==RakNet::RakString::nPos)
			continue;                       //invalid response
		size_t end=recvBuff.Find("\r",begin);
		if(end==RakNet::RakString::nPos)
			continue;    //invalid response

		describeUrl.Assign(recvBuff,begin,end-begin);
		if(!GetDescription()){
			RakSleep(50);
			continue;
		}

		if(!ParseDescription()){
			RakSleep(50);
			continue;
		}
		status=NAT_FOUND;	//find a router

		return true ;
	}

	status=NAT_ERROR;
	lastError="Fail to find an UPNP NAT.\n";
	//delete currentInstance;
	return false;                               //no router found
}

bool UPNPNATInternal::GetDescription()
{
	RakNet::RakString host,path;
	unsigned short int port;
	int ret=parseUrl(describeUrl.C_String(),host,&port,path);

	if(!ret)
	{
		status=NAT_ERROR;
		lastError="Failed to parseURl: "+describeUrl+"\n";
		return false;
	}

	//connect 

	if(!TcpConnect(host.C_String(),port)){
		return false;
	}

	char request[200];
	sprintf (request,"GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n",path.C_String(),host.C_String(),port); 
	RakNet::RakString httpRequest=request;

	//send request
	tcpInterface.Send(httpRequest.C_String(),(int) httpRequest.GetLength(),UNASSIGNED_SYSTEM_ADDRESS,true);


	RakSleep(1000);
	descriptionInfo=WaitAndReturnResponse(5000);	

	return true;
}

RakNet::RakString UPNPNATInternal::WaitAndReturnResponse(int waitTime)
{
	Packet *packet;
	RakNet::RakString response;	
	RakNetTime stopWaiting = RakNet::GetTime() + waitTime;
	while (RakNet::GetTime() < stopWaiting)
	{
		while ( (packet=tcpInterface.Receive()) >0) 
		{
			response+=packet->data;

			tcpInterface.DeallocatePacket(packet);

		}
	}
	return response;
}

bool UPNPNATInternal::ParseDescription()
{
	const char deviceType1[]=	"urn:schemas-upnp-org:device:InternetGatewayDevice:1";
	const char deviceType2[]=	"urn:schemas-upnp-org:device:WANDevice:1";
	const char deviceType3[]=	"urn:schemas-upnp-org:device:WANConnectionDevice:1";

	const char serviceWanIp[]=	"urn:schemas-upnp-org:service:WANIPConnection:1";
	const char serviceWanPPP[]= "urn:schemas-upnp-org:service:WANPPPConnection:1" ;

	int xmlStart=descriptionInfo.Find("<",0);
	descriptionInfo=descriptionInfo.SubStr(xmlStart,descriptionInfo.GetLength());

	XMLNode node=XMLNode::parseString(descriptionInfo.C_String(),"root");
	if(node.isEmpty())
	{
		status=NAT_ERROR;
		lastError="The device descripe XML file is not a valid XML file. Cann't find root element.\n";
		return false;
	}

	XMLNode baseURLNode=node.getChildNode("URLBase",0);
	if(!baseURLNode.getText())
	{
		size_t index=describeUrl.Find("/",7);
		if(index==RakNet::RakString::nPos )
		{
			status=NAT_ERROR;
			lastError="Fail to get baseURL from XMLNode \"URLBase\" or describeUrl.\n";
			return false;
		}
		baseUrl.Assign(describeUrl,0,index);
	}
	else
		baseUrl=baseURLNode.getText();

	int num,i;
	XMLNode deviceNode,deviceListNode,deviceTypeNode;
	num=node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		deviceNode=node.getChildNode("device",i);
		if(deviceNode.isEmpty())
			break;
		deviceTypeNode=deviceNode.getChildNode("deviceType",0);
		if(strcmp(deviceTypeNode.getText(),deviceType1)==0)
			break;
	}

	if(deviceNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError="Fail to find device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}	

	deviceListNode=deviceNode.getChildNode("deviceList",0);
	if(deviceListNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANDevice:1 and it's devicelist 
	num=deviceListNode.nChildNode("device");
	for(i=0;i<num;i++)
	{
		deviceNode=deviceListNode.getChildNode("device",i);
		if(deviceNode.isEmpty())
			break;
		deviceTypeNode=deviceNode.getChildNode("deviceType",0);
		if(strcmp(deviceTypeNode.getText(),deviceType2)==0)
			break;
	}

	if(deviceNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError="Fail to find device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	deviceListNode=deviceNode.getChildNode("deviceList",0);
	if(deviceListNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANConnectionDevice:1 and it's servicelist 
	num=deviceListNode.nChildNode("device");
	for(i=0;i<num;i++)
	{
		deviceNode=deviceListNode.getChildNode("device",i);
		if(deviceNode.isEmpty())
			break;
		deviceTypeNode=deviceNode.getChildNode("deviceType",0);
		if(strcmp(deviceTypeNode.getText(),deviceType3)==0)
			break;
	}
	if(deviceNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError="Fail to find device \"urn:schemas-upnp-org:device:WANConnectionDevice:1\"\n";
		return false;
	}	

	XMLNode serviceListNode,serviceNode,serviceTypeNode;
	serviceListNode=deviceNode.getChildNode("serviceList",0);
	if(serviceListNode.isEmpty())
	{
		status=NAT_ERROR;
		lastError=" Fail to find serviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	num=serviceListNode.nChildNode("service");
	const char * serviceType;
	bool isFound=false;
	for(i=0;i<num;i++)
	{
		serviceNode=serviceListNode.getChildNode("service",i);
		if(serviceNode.isEmpty())
			break;
		serviceTypeNode=serviceNode.getChildNode("serviceType");
		if(serviceTypeNode.isEmpty())
			continue;
		serviceType=serviceTypeNode.getText();
		if(strcmp(serviceType,serviceWanIp)==0||strcmp(serviceType,serviceWanPPP)==0)
		{
			isFound=true;
			break;
		}
	}

	if(!isFound)
	{
		status=NAT_ERROR;
		lastError="can't find  \" SERVICE_WANIP \" or \" SERVICE_WANPPP \" service.\n";
		return false;
	}

	this->serviceType=serviceType;

	XMLNode controlURLNode=serviceNode.getChildNode("controlURL");
	controlUrl=controlURLNode.getText();
	if(controlUrl.Find("http://")==RakNet::RakString::nPos&&controlUrl.Find("HTTP://")==RakNet::RakString::nPos)
		controlUrl=baseUrl+controlUrl;
	if(serviceDescribeUrl.Find("http://")==RakNet::RakString::nPos&&serviceDescribeUrl.Find("HTTP://")==RakNet::RakString::nPos)
		serviceDescribeUrl=baseUrl+serviceDescribeUrl;

	tcpInterface.CloseConnection(serverAddress);

	status=NAT_GETCONTROL;
	return true;	
}
bool UPNPNATInternal::AddPortMapping(const char * description, const char * destinationIp, unsigned short int portEx, unsigned short int port_in, char * protocol)
{
	const char httpHeaderAction[]= "POST %s HTTP/1.1\r\nHOST: %s:%u\r\nSOAPACTION:\"%s#%s\"\r\nCONTENT-TYPE: text/xml ; charset=\"utf-8\"\r\nContent-Length: %d \r\n\r\n";

	const char soapAction[]=   "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n<s:Body>\r\n<u:%s xmlns:u=\"%s\">\r\n%s</u:%s>\r\n</s:Body>\r\n</s:Envelope>\r\n";
	const char portMappingLeaseTime[]=  "0";
	const char actionAdd[]= 	 "AddPortMapping";

	const char addPortMappingParams[]= "<NewRemoteHost></NewRemoteHost>\r\n<NewExternalPort>%u</NewExternalPort>\r\n<NewProtocol>%s</NewProtocol>\r\n<NewInternalPort>%u</NewInternalPort>\r\n<NewInternalClient>%s</NewInternalClient>\r\n<NewEnabled>1</NewEnabled>\r\n<NewPortMappingDescription>%s</NewPortMappingDescription>\r\n<NewLeaseDuration>0</NewLeaseDuration>\r\n";

	int ret;

	RakNet::RakString host,path;
	unsigned short int port;
	ret=parseUrl(controlUrl.C_String(),host,&port,path);
	if(!ret)
	{
		status=NAT_ERROR;
		lastError="Fail to parseURl: "+describeUrl+"\n";
		return false;
	}

	if(!TcpConnect(host.C_String(),port)){
		return false;
	}

	char buff[maxBuffSize+1];
	sprintf(buff,addPortMappingParams,portEx,protocol,port_in,destinationIp,description);
	RakNet::RakString action_params=buff;

	sprintf(buff,soapAction,actionAdd,serviceType.C_String(),action_params.C_String(),actionAdd);
	RakNet::RakString soap_message=buff;

	sprintf(buff,httpHeaderAction,path.C_String(),host.C_String(),port,serviceType.C_String(),actionAdd,soap_message.GetLength());
	RakNet::RakString action_message=buff;

	RakNet::RakString httpRequest=action_message+soap_message;
	tcpInterface.Send(httpRequest.C_String(),(int) httpRequest.GetLength(),UNASSIGNED_SYSTEM_ADDRESS,true);

	RakSleep(1000);
	RakNet::RakString response=WaitAndReturnResponse(5000);

	if(response.Find(httpOk)==RakNet::RakString::nPos)
	{
		status=NAT_ERROR;
		char temp[100];
		sprintf(temp,"Fail to add port mapping (%s/%s)\n",description,protocol);
		lastError=temp;

		return false;
	}
	tcpInterface.CloseConnection(serverAddress);
	status=NAT_ADD;
	return true;	
}

