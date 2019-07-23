
// Include
#include "roomscomponent.h"
#include "../client.h"
#include "../../utils/functions.h"
#include <iostream>

/*
	Packet IDs
		0x0A = SelectViewUpdates
		0x0B = SelectCategoryUpdates
		0x14 = JoinRoom
		0x15 = LeaveRoom
		0x1F = KickUser
		0x28 = TransferRoomHost
		0x64 = CreateRoomCategory
		0x65 = RemoveRoomCategory
		0x66 = CreateRoom
		0x67 = RemoveRoom
		0x68 = ClearBannedUsers
		0x69 = UnbanUser
		0x6D = GetViews
		0x6E = CreateScheduledCategory
		0x6F = DeleteScheduledCategory
		0x70 = GetSchedulesCategories
		0x78 = LookupRoomData
		0x7A = ListBannedUsers
		0x82 = SetRoomAttributes
		0x8C = CheckEntryCriteria
		0x96 = ToggleJoinedRoomNotifications
		0xA0 = SelectPseudoRoomUpdates

	Notification Packet IDs
		0x0A = NotifyRoomViewUpdated
		0x0B = NotifyRoomViewAdded
		0x0C = NotifyRoomViewRemoved
		0x14 = NotifyRoomCategoryUpdated
		0x15 = NotifyRoomCategoryAdded
		0x16 = NotifyRoomCategoryRemoved
		0x1E = NotifyRoomUpdated
		0x1F = NotifyRoomAdded
		0x20 = NotifyRoomRemoved
		0x28 = NotifyRoomPopulationUpdated
		0x32 = NotifyRoomMemberJoined
		0x33 = NotifyRoomMemberLeft
		0x34 = NotifyRoomMemberUpdated
		0x3C = NotifyRoomKick
		0x46 = NotifyRoomHostTransfer
		0x50 = NotifyRoomAttributesSet

	Blaze fields
		RoomReplicationContext
			SEID = 0x38
			UPRE = 0x1C
			USID = 0x34
			VWID = 0x38

		RoomViewData
			DISP = 0x24
			GMET = 0x54
			META = 0x54
			MXRM = 0x38
			NAME = 0x24
			USRM = 0x38
			VWID = 0x38
*/

// Blaze
namespace Blaze {
	// RoomsComponent
	void RoomsComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x0A:
				SelectViewUpdates(client, header);
				break;

			case 0x0B:
				SelectCategoryUpdates(client, header);
				break;

			default:
				std::cout << "Unknown rooms command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void RoomsComponent::SendSelectCategoryUpdates(Client* client, uint32_t viewId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "VWID", viewId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x0B;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomViewUpdated(Client* client, uint32_t viewId) {
		TDF::Packet packet;
		packet.PutString(nullptr, "DISP", "");
		{
			auto& gmetMap = packet.CreateMap(nullptr, "GMET", TDF::Type::String, TDF::Type::Struct);
		} {
			auto& metaMap = packet.CreateMap(nullptr, "META", TDF::Type::String, TDF::Type::Struct);
		}
		packet.PutInteger(nullptr, "MXRM", viewId);
		packet.PutString(nullptr, "NAME", "Dalkon's Room");
		packet.PutInteger(nullptr, "USRM", viewId);
		packet.PutInteger(nullptr, "VWID", viewId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x0A;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}
	
	void RoomsComponent::NotifyRoomViewAdded(Client* client, uint32_t viewId) {
		TDF::Packet packet;
		packet.PutString(nullptr, "DISP", "");
		{
			auto& gmetMap = packet.CreateMap(nullptr, "GMET", TDF::Type::String, TDF::Type::Struct);
		} {
			auto& metaMap = packet.CreateMap(nullptr, "META", TDF::Type::String, TDF::Type::Struct);
		}
		packet.PutInteger(nullptr, "MXRM", viewId);
		packet.PutString(nullptr, "NAME", "Dalkon's Room");
		packet.PutInteger(nullptr, "USRM", viewId);
		packet.PutInteger(nullptr, "VWID", viewId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x0B;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomViewRemoved(Client* client, uint32_t viewId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "VWID", viewId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x0C;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomCategoryUpdated(Client* client) {

	}

	void RoomsComponent::NotifyRoomCategoryAdded(Client* client) {

	}

	void RoomsComponent::NotifyRoomCategoryRemoved(Client* client, uint32_t categoryId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "CTID", categoryId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x16;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomUpdated(Client* client) {

	}

	void RoomsComponent::NotifyRoomAdded(Client* client) {

	}

	void RoomsComponent::NotifyRoomRemoved(Client* client, uint32_t roomId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "RMID", roomId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x20;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomPopulationUpdated(Client* client) {
		TDF::Packet packet;
		{
			auto& popaMap = packet.CreateMap(nullptr, "POPA", TDF::Type::String, TDF::Type::String);
		} {
			auto& popmMap = packet.CreateMap(nullptr, "POPM", TDF::Type::String, TDF::Type::String);
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x28;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomMemberJoined(Client* client) {

	}

	void RoomsComponent::NotifyRoomMemberLeft(Client* client, uint32_t roomId, uint32_t memberId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MBID", memberId);
		packet.PutInteger(nullptr, "RMID", roomId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x33;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomMemberUpdated(Client* client) {
		
	}

	void RoomsComponent::NotifyRoomKick(Client* client, uint32_t roomId, uint32_t memberId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MBID", memberId);
		packet.PutInteger(nullptr, "RMID", roomId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x3C;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomHostTransfer(Client* client, uint32_t roomId, uint32_t memberId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MBID", memberId);
		packet.PutInteger(nullptr, "RMID", roomId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x46;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::NotifyRoomAttributesSet(Client* client, uint32_t roomId) {
		// TODO: add a map<string, string> for attributes
		TDF::Packet packet;
		{
			auto& attrMap = packet.CreateMap(nullptr, "ATTR", TDF::Type::String, TDF::Type::String);
		}
		packet.PutInteger(nullptr, "RMID", roomId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Rooms;
		header.command = 0x50;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void RoomsComponent::SelectViewUpdates(Client* client, Header header) {
		auto& request = client->get_request();

		bool update = request["UPDT"].GetUint() != 0;
		if (update) {
			uint32_t viewId = 0;

			// What do we send here?
			TDF::Packet packet;
			packet.PutInteger(nullptr, "SEID", 0);
			packet.PutInteger(nullptr, "UPRE", 1);
			packet.PutInteger(nullptr, "USID", 1);
			packet.PutInteger(nullptr, "VWID", viewId);

			DataBuffer outBuffer;
			packet.Write(outBuffer);

			header.component = Component::Rooms;
			header.command = 0x0A;
			header.error_code = 0;

			client->reply(std::move(header), outBuffer);
			
			NotifyRoomViewUpdated(client, viewId);
		}
	}

	void RoomsComponent::SelectCategoryUpdates(Client* client, Header header) {
		auto& request = client->get_request();

		uint32_t viewId = request["VWID"].GetUint();
		SendSelectCategoryUpdates(client, viewId);

		NotifyRoomCategoryUpdated(client);
	}
}
