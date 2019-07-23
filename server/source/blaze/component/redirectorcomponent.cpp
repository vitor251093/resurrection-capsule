
// Include
#include "redirectorcomponent.h"
#include "../client.h"
#include <iostream>

/*
	Request Packet IDs
		0x01 = ServerInstanceInfo

	Response Packet IDs

	BlazeValues meanings
		PDTL = PersonaDetails
		SESS = SessionInfo
		NTOS = Need Terms Of Service
		PID = Persona ID
		MAIL = Client Email
		UID = Unique ID
		PCTK = Packet Key

	BlazeValues
		AMAP
			DPRT = 0x40
			MASK = 0x38
			SID = 0x38
			SIP = 0x38
			SPRT = 0x40

		NMAP
			DPRT = 0x40
			SID = 0x38
			SIP = 0x24
			SITE = 0x24
			SPRT = 0x40

		ADDR
			HOST = 0x24
			IP = 0x38
			PORT = 0x40

		(Xbox Server Address)
			PORT
			SID
			SITE

		(Xbox Id)
			GTAG
			XUID

		(Server Endpoint Info)
			ADRS
			CCON
			CHAN
			DEC
			ENC
			MCON
			PROT

		(Server Address Info)
			ADDR = 0x14
			TYPE = 0x1C

		(Server Instance)
			CWD
			ENDP
			ID
			LOAD
			NAME
			SVC

		(Server Info Data)
			AMAP
			BTGT
			BTIM
			CVER
			DEPO
			INST
			IVER
			LOCN
			MSTR
			NAME
			NASP
			NMAP
			PLAT
			SNMS
			SVID
			VERS
			XDNS
			XMST
			XSLV

		(Server Instance Error)
			MSGS

		(Xbox Client Address)
			XDDR
			XUID

	Request Packets
		ServerInstanceRequest
			BSDK
			BTIM
			CLNT
			CSKU
			CVER
			DSDK
			ENV
			FPID
			LOC
			NAME
			PLAT
			PROF

	Response Packets
		ServerInstanceInfo
			ADDR = 0x14
			AMAP = 0x58
			MSGS = 0x58
			NMAP = 0x58
			SECU = 0x50
			XDNS = 0x38

		UserSessionExtendedData
			ADDR
			BPS
			CMAP
			CTY
			CVAR
			DMAP
			HWFG
			PSLM
			QDAT
			UATT
			ULST
*/

// Blaze
namespace Blaze {
	// RedirectorComponent
	void RedirectorComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x01:
				ServerInstanceInfo(client, header);
				break;

			default:
				std::cout << "Unknown redirector command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void RedirectorComponent::SendServerInstanceInfo(Client* client, const std::string& host, uint16_t port) {
		TDF::Packet packet;

		auto& addrUnion = packet.CreateUnion(nullptr, "ADDR", NetworkAddressMember::XboxClientAddress);
		{
			auto& valuStruct = packet.CreateStruct(&addrUnion, "VALU");
			packet.PutString(&valuStruct, "HOST", host);
			packet.PutInteger(&valuStruct, "IP", 0); // boost::asio::ip::address_v4::from_string(host).to_ulong()
			packet.PutInteger(&valuStruct, "PORT", port);
		}
		/*
		{
			auto& amapList = packet.CreateList(nullptr, "AMAP", TDF::Type::Struct);
			{
				auto& amapStruct = packet.CreateStruct(&amapList, "");
				packet.PutInteger(&amapStruct, "DPRT", 10000);
				packet.PutInteger(&amapStruct, "MASK", 0);
				packet.PutInteger(&amapStruct, "SID", 0);
				packet.PutInteger(&amapStruct, "SIP", boost::asio::ip::address_v4::from_string(host).to_ulong());
				packet.PutInteger(&amapStruct, "SPRT", 10000);
			}
		} {
			auto& msgsList = packet.CreateList(nullptr, "MSGS", TDF::Type::String);
			packet.PutString(&msgsList, "", "What goes here?");
		} {
			auto& nmapList = packet.CreateList(nullptr, "NMAP", TDF::Type::Struct);
			{
				auto& nmapStruct = packet.CreateStruct(&nmapList, "");
				packet.PutInteger(&nmapStruct, "DPRT", 10000);
				packet.PutInteger(&nmapStruct, "SID", 0);
				packet.PutString(&nmapStruct, "SIP", host);
				packet.PutString(&nmapStruct, "SITE", "");
				packet.PutInteger(&nmapStruct, "SID", 10000);
			}
		}
		*/
		packet.PutInteger(nullptr, "SECU", 1);
		packet.PutInteger(nullptr, "XDNS", 0);

		Header header;
		header.component = Component::Redirector;
		header.command = 0x01;
		header.error_code = 0;

		DataBuffer outBuffer;
		packet.Write(outBuffer);
		
		client->reply(header, outBuffer);
	}

	void RedirectorComponent::SendServerAddressInfo(Client* client, const std::string& host, uint16_t port) {
		TDF::Packet packet;

		auto& addrUnion = packet.CreateUnion(nullptr, "ADDR", NetworkAddressMember::XboxClientAddress);
		{
			auto& valuStruct = packet.CreateStruct(&addrUnion, "VALU");
			packet.PutString(&valuStruct, "HOST", host);
			packet.PutInteger(&valuStruct, "IP", 0);
			packet.PutInteger(&valuStruct, "PORT", port);
		}
		packet.PutInteger(nullptr, "TYPE", AddressInfoType::ExternalIp);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Redirector;
		header.command = 0x01;
		header.error_code = 0;

		client->reply(header, outBuffer);
	}

	void RedirectorComponent::ServerInstanceInfo(Client* client, Header header) {
		SendServerInstanceInfo(client, "127.0.0.1", 10041);
	}
}
