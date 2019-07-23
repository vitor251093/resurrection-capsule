
// Include
#include "playgroupscomponent.h"
#include "../client.h"
#include "../../utils/functions.h"
#include <iostream>

/*
	Packet IDs
		0x01 = CreatePlaygroup
		0x02 = DestroyPlaygroup
		0x03 = JoinPlaygroup
		0x04 = LeavePlaygroup
		0x05 = SetPlaygroupAttributes
		0x06 = SetMemberAttributes
		0x07 = KickPlaygroupMember
		0x08 = SetPlaygroupJoinControls
		0x09 = FinalizePlaygroupCreation
		0x0A = LookupPlaygroupInfo
		0x0B = ResetPlaygroupSession

	Notification Packet IDs
		0x32 = NotifyDestroyPlaygroup
		0x33 = NotifyJoinPlaygroup
		0x34 = NotifyMemberJoinedPlaygroup
		0x35 = NotifyMemberRemovedFromPlaygroup
		0x36 = NotifyPlaygroupAttributesSet
		0x45 = NotifyMemberAttributesSet
		0x4F = NotifyLeaderChange
		0x50 = NotifyMemberPermissionsChange
		0x55 = NotifyJoinControlsChange
		0x56 = NotifyXboxSessionInfo
		0x57 = NotifyXboxSessionChange

	Blaze fields
		

	Notify Packets
		NotifyDestroyPlaygroup
			PGID = 0x38
			REAS = 0x1C

		NotifyJoinPlaygroup
			INFO = 0x18
			MLST = 0x58
			USER = 0x34

		NotifyMemberJoinedPlaygroup
			MEMB = 0x18
			PGID = 0x38

		NotifyMemberRemovedFromPlaygroup
			MLST = 0x34
			PGID = 0x38
			REAS = 0x1C

		NotifyPlaygroupAttributesSet
			ATTR = 0x54
			PGID = 0x38

		NotifyMemberAttributesSet
			ATTR = 0x54
			EID = 0x34
			PGID = 0x38

		NotifyLeaderChange
			HSID = 0x48
			LID = 0x34
			PGID = 0x38

		NotifyMemberPermissionsChange
			LID = 0x34
			PERM = 0x28
			PGID = 0x38

		NotifyJoinControlsChange
			OPEN = 0x1C
			PGID = 0x38

		NotifyXboxSessionInfo
			PGID = 0x38
			PRES = 0x50
			XNNC = 0x20
			XSES = 0x20
*/

// Blaze
namespace Blaze {
	// PlaygroupsComponent
	void PlaygroupsComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x01:
				CreatePlaygroup(client, header);
				break;

			case 0x02:
				DestroyPlaygroup(client, header);
				break;

			case 0x03:
				JoinPlaygroup(client, header);
				break;

			case 0x04:
				LeavePlaygroup(client, header);
				break;

			case 0x05:
				SetPlaygroupAttributes(client, header);
				break;

			case 0x06:
				SetMemberAttributes(client, header);
				break;

			case 0x07:
				KickPlaygroupMember(client, header);
				break;

			case 0x08:
				SetPlaygroupJoinControls(client, header);
				break;

			case 0x09:
				FinalizePlaygroupCreation(client, header);
				break;

			case 0x0A:
				LookupPlaygroupInfo(client, header);
				break;

			case 0x0B:
				ResetPlaygroupSession(client, header);
				break;

			default:
				std::cout << "Unknown playgroups command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void PlaygroupsComponent::NotifyDestroyPlaygroup(Client* client, uint32_t playgroupId, uint32_t reason) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "PGID", playgroupId);
		packet.PutInteger(nullptr, "REAS", reason);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x32;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyJoinPlaygroup(Client* client) {
		TDF::Packet packet;
		{
			auto& infoStruct = packet.CreateStruct(nullptr, "INFO");
		} {
			auto& mlstList = packet.CreateList(nullptr, "MLST", TDF::Type::Struct);
		}
		packet.PutInteger(nullptr, "USER", 0); // user id?

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x33;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyMemberJoinedPlaygroup(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		{
			auto& membStruct = packet.CreateStruct(nullptr, "MEMB");
		}
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x34;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyMemberRemovedFromPlaygroup(Client* client, uint32_t playgroupId, uint32_t reason) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MLST", 0);
		packet.PutInteger(nullptr, "PGID", playgroupId);
		packet.PutInteger(nullptr, "REAS", reason);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x35;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyPlaygroupAttributesSet(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		{
			auto& attrMap = packet.CreateMap(nullptr, "ATTR", TDF::Type::String, TDF::Type::String);
		}
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x36;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyMemberAttributesSet(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		{
			auto& attrMap = packet.CreateMap(nullptr, "ATTR", TDF::Type::String, TDF::Type::String);
		}
		packet.PutInteger(nullptr, "EID", 0);
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x45;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyLeaderChange(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "HSID", 0);
		packet.PutInteger(nullptr, "LID", 0); // Leader ID?
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x4F;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyMemberPermissionsChange(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "LID", 0);
		packet.PutInteger(nullptr, "PERM", 0);
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x50;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyJoinControlsChange(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "OPEN", 0);
		packet.PutInteger(nullptr, "PGID", playgroupId);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x55;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyXboxSessionInfo(Client* client, uint32_t playgroupId) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "PGID", playgroupId);
		packet.PutInteger(nullptr, "PRES", 1);
		packet.PutBlob(nullptr, "XNNC", nullptr, 0);
		packet.PutBlob(nullptr, "XSES", nullptr, 0);

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Playgroups;
		header.command = 0x56;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void PlaygroupsComponent::NotifyXboxSessionChange(Client* client, uint32_t playgroupId) {
		NotifyXboxSessionInfo(client, playgroupId);
	}

	void PlaygroupsComponent::CreatePlaygroup(Client* client, Header header) {
		std::cout << "CreatePlaygroup" << std::endl;
	}

	void PlaygroupsComponent::DestroyPlaygroup(Client* client, Header header) {
		std::cout << "DestroyPlaygroup" << std::endl;
	}

	void PlaygroupsComponent::JoinPlaygroup(Client* client, Header header) {
		std::cout << "JoinPlaygroup" << std::endl;
	}

	void PlaygroupsComponent::LeavePlaygroup(Client* client, Header header) {
		std::cout << "LeavePlaygroup" << std::endl;
	}

	void PlaygroupsComponent::SetPlaygroupAttributes(Client* client, Header header) {
		std::cout << "SetPlaygroupAttributes" << std::endl;
	}

	void PlaygroupsComponent::SetMemberAttributes(Client* client, Header header) {
		std::cout << "SetMemberAttributes" << std::endl;
	}

	void PlaygroupsComponent::KickPlaygroupMember(Client* client, Header header) {
		std::cout << "KickPlaygroupMember" << std::endl;
	}

	void PlaygroupsComponent::SetPlaygroupJoinControls(Client* client, Header header) {
		std::cout << "SetPlaygroupJoinControls" << std::endl;
	}

	void PlaygroupsComponent::FinalizePlaygroupCreation(Client* client, Header header) {
		std::cout << "FinalizePlaygroupCreation" << std::endl;
	}

	void PlaygroupsComponent::LookupPlaygroupInfo(Client* client, Header header) {
		std::cout << "LookupPlaygroupInfo" << std::endl;
	}

	void PlaygroupsComponent::ResetPlaygroupSession(Client* client, Header header) {
		std::cout << "ResetPlaygroupSession" << std::endl;
	}
}
