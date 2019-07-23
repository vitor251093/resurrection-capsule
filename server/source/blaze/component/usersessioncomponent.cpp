
// Include
#include "usersessioncomponent.h"
#include "gamemanagercomponent.h"
#include "../client.h"
#include "../../utils/functions.h"
#include <iostream>

/*
	Packet IDs
		0x03 = FetchExtendedData
		0x05 = UpdateExtendedDataAttribute
		0x08 = UpdateHardwareFlags
		0x0C = LookupUser
		0x0D = LookupUsers
		0x0E = LookupUsersByPrefix
		0x14 = UpdateNetworkInfo
		0x17 = LookupUserGeoIPData
		0x18 = OverrideUserGeoIPData
		0x19 = UpdateUserSessionClientData
		0x1A = SetUserInfoAttribute
		0x1B = ResetUserGeoIPData

	Notification Packet IDs
		0x01 = UserSessionExtendedDataUpdate
		0x02 = UserAdded
		0x03 = UserRemoved
		0x04 = UserSessionDisconnected
		0x05 = UserUpdated

	BlazeValues meanings
		USER = UserIdentification

	BlazeValues
		USER
			AID = 0x38
			ALOC = 0x38
			EXBB = 0x20
			EXID = 0x30
			ID = 0x34
			NAME = 0x24
			
		DATA
			ADDR = 0x14
			BPS = 0x24
			CMAP = 0x54
			CTY = 0x24
			CVAR = 0x10
			DMAP = 0x54
			HWFG = 0x28
			PSLM = 0x58
			QDAT = 0x18
			UATT = 0x30
			ULST = 0x58

		EDAT
			

		QDAT

		CVAR
			GRP = 0x40
			LVL = 0x38
			STAT = 0x1C
			XTRA = 0x40

		(User Status)
			FLGS = 0x28
			ID = 0x34

		(User Data)
			EDAT = 0x18
			FLGS = 0x28
			USER = 0x18

	Request Packets
		UpdateUserSessionClientData
			CVAR

	Response Packets
		
*/

// Blaze
namespace Blaze {
	// UserSessionComponent
	void UserSessionComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x14:
				UpdateNetworkInfo(client, header);
				break;

			case 0x19:
				UpdateUserSessionClientData(client, header);
				break;

			default:
				std::cout << "Unknown usersession command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void UserSessionComponent::NotifyUserSessionExtendedDataUpdate(Client* client, uint64_t userId) {
		std::cout << "UserSession: User extended data" << std::endl;

		TDF::Packet packet;
		{
			auto& dataStruct = packet.CreateStruct(nullptr, "DATA");
			packet.CreateUnion(&dataStruct, "ADDR", NetworkAddressMember::Unset);
			packet.PutString(&dataStruct, "BPS", "");
			packet.PutString(&dataStruct, "CTY", "");
			{
				auto& dmapMap = packet.CreateMap(&dataStruct, "DMAP", TDF::Type::Integer, TDF::Type::Integer);
				packet.PutInteger(&dmapMap, "0x70001", 55);
				packet.PutInteger(&dmapMap, "0x70002", 707);
			}
			packet.PutInteger(&dataStruct, "HWFG", 0);
			{
				auto& qdatStruct = packet.CreateStruct(&dataStruct, "QDAT");
				packet.PutInteger(&qdatStruct, "DBPS", 0);
				packet.PutInteger(&qdatStruct, "NATT", NatType::Open);
				packet.PutInteger(&qdatStruct, "UBPS", 0);
			}
			packet.PutInteger(&dataStruct, "UATT", 0);
		}
		packet.PutInteger(nullptr, "USID", userId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::UserSessions;
		header.command = 0x01;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void UserSessionComponent::NotifyUserAdded(Client* client, uint64_t userId, const std::string& userName) {
		const auto& user = client->get_user();
		if (!user) {
			return;
		}

		std::cout << "UserSession: Add user" << std::endl;

		auto& request = client->get_request();

		TDF::Packet packet;
		{
			auto& dataStruct = packet.CreateStruct(nullptr, "DATA");
			packet.CreateUnion(&dataStruct, "ADDR", NetworkAddressMember::Unset);
			packet.PutString(&dataStruct, "BPS", "");
			packet.PutString(&dataStruct, "CTY", "");
			{
				auto& dmapMap = packet.CreateMap(&dataStruct, "DMAP", TDF::Type::Integer, TDF::Type::Integer);
				packet.PutInteger(&dmapMap, "0x70001", 55);
				packet.PutInteger(&dmapMap, "0x70002", 707);
			}
			packet.PutInteger(&dataStruct, "HWFG", 0);
			{
				auto& qdatStruct = packet.CreateStruct(&dataStruct, "QDAT");
				packet.PutInteger(&qdatStruct, "DBPS", 1);
				packet.PutInteger(&qdatStruct, "NATT", NatType::Open);
				packet.PutInteger(&qdatStruct, "UBPS", 1);
			}
			packet.PutInteger(&dataStruct, "UATT", 0);
		} {
			auto& userStruct = packet.CreateStruct(nullptr, "USER");
			packet.PutInteger(&userStruct, "AID", userId);
			packet.PutInteger(&userStruct, "ALOC", client->localization());
			packet.PutBlob(&userStruct, "EXBB", nullptr, 0);
			packet.PutInteger(&userStruct, "EXID", 0);
			packet.PutInteger(&userStruct, "ID", userId);
			packet.PutString(&userStruct, "NAME", userName);
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::UserSessions;
		header.command = 0x02;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void UserSessionComponent::NotifyUserUpdated(Client* client, uint64_t userId) {
		auto& request = client->get_request();

		TDF::Packet packet;
		packet.PutInteger(nullptr, "FLGS", SessionState::Authenticated);
		packet.PutInteger(nullptr, "ID", userId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::UserSessions;
		header.command = 0x05;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void UserSessionComponent::UpdateNetworkInfo(Client* client, Header header) {
		std::cout << "Update network info" << std::endl;

		auto& request = client->get_request();

		const auto& addrData = request["ADDR"]["VALU"];
		/*
		"ADDR": {
			"_Type": 6,
				"_AddressMember" : 2,
				"VALU" : {
				"_Type": 3,
					"EXIP" : {
					"_Type": 3,
						"IP" : 0,
						"PORT" : 0
				},
					"INIP" : {
						"_Type": 3,
							"IP" : 3232235954,
							"PORT" : 3659
					}
			}
		},
		*/
		header.error_code = 0;
		client->reply(std::move(header));

		/*
		Log.Info(string.Format("Client {0} updating network info", request.Client.ID));

        var addr = (TdfUnion)request.Data["ADDR"];
        var valu = (TdfStruct)addr.Data.Find(tdf => tdf.Label == "VALU");

        var inip = (TdfStruct)valu.Data.Find(tdf => tdf.Label == "INIP");
        var ip = (TdfInteger)inip.Data.Find(tdf => tdf.Label == "IP");
        var port = (TdfInteger)inip.Data.Find(tdf => tdf.Label == "PORT");

        request.Client.InternalIP = ip.Value;
        request.Client.InternalPort = (ushort)port.Value;

        request.Client.ExternalIP = ip.Value;
        request.Client.ExternalPort = (ushort)port.Value;

        request.Reply();
		*/
	}

	void UserSessionComponent::UpdateUserSessionClientData(Client* client, Header header) {
		header.error_code = 0;
		client->reply(std::move(header));

		// NotifyUserSessionExtendedDataUpdate(client, 1);
		// Log(request);
		/*
		TDF::Packet packet;
		{
			auto& edatStruct = packet.CreateStruct(nullptr, "EDAT");
			packet.PutInteger(&edatStruct, "AID", userId);
			packet.PutInteger(&edatStruct, "ALOC", request["CINF"]["LOC"].GetUint64());
			// packet.PutBlob(&edatStruct, "EXBB", nullptr, 0);
			// packet.PutInteger(&edatStruct, "EXID", 0);
			packet.PutInteger(&edatStruct, "ID", userId);
			packet.PutString(&edatStruct, "NAME", userName);
		}
		packet.PutInteger(nullptr, "FLGS", static_cast<uint32_t>(SessionState::Authenticated));
		{
			auto& userStruct = packet.CreateStruct(nullptr, "USER");
			packet.PutInteger(&userStruct, "AID", 1);
			packet.PutInteger(&userStruct, "ALOC", request["CINF"]["LOC"].GetUint64());
			packet.PutBlob(&userStruct, "EXBB", nullptr, 0);
			packet.PutInteger(&userStruct, "EXID", 0);
			packet.PutInteger(&userStruct, "ID", 1);
			packet.PutString(&userStruct, "NAME", "Dalkon");
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::UserSessions;
		header.command = 0x19;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
		*/
	}
}
