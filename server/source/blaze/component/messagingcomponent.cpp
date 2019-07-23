
// Include
#include "messagingcomponent.h"
#include "../client.h"
#include "../../utils/functions.h"
#include <iostream>

/*
	Packet IDs
		0x01 = SendMessage
		0x02 = FetchMessages
		0x03 = PurgeMessages
		0x04 = TouchMessages
		0x05 = GetMessages

	Notification Packet IDs
		0x01 = NotifyMessage

	Blaze fields
		ServerMessage (NotifyMessage)
			FLAG = 0x28
			MGID = 0x38
			NAME = 0x24
			PYLD = 0x18
			SRCE = 0x08
			TIME = 0x38
*/

// Blaze
namespace Blaze {
	// MessagingComponent
	void MessagingComponent::Parse(Client* client, const Header& header) {
		switch (header.command) {
			case 0x01:
				OnSendMessage(client, header);
				break;

			case 0x02:
				OnFetchMessages(client, header);
				break;

			case 0x03:
				OnPurgeMessages(client, header);
				break;

			case 0x04:
				OnTouchMessages(client, header);
				break;

			case 0x05:
				OnGetMessages(client, header);
				break;

			default:
				std::cout << "Unknown messaging command: 0x" << std::hex << header.command << std::dec << std::endl;
				break;
		}
	}

	void MessagingComponent::OnSendMessageResponse(Client* client) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "MGID", 0);
		{
			auto& midsList = packet.CreateList(nullptr, "MIDS", TDF::Type::Integer);
			packet.PutInteger(&midsList, "", 0);
		}

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Messaging;
		header.command = 0x01;
		header.error_code = 0;

		client->reply(std::move(header), outBuffer);
	}

	void MessagingComponent::NotifyMessage(Client* client) {
		TDF::Packet packet;
		packet.PutInteger(nullptr, "FLAG", 0);
		packet.PutInteger(nullptr, "MGID", 0);
		packet.PutString(nullptr, "NAME", "");
		{
			auto& pyldStruct = packet.CreateStruct(nullptr, "PYLD");
		}
		packet.PutVector3(nullptr, "SRCE", 0, 0, 0);
		packet.PutInteger(nullptr, "TIME", utils::get_unix_time());

		DataBuffer outBuffer;
		packet.Write(outBuffer);

		Header header;
		header.component = Component::Messaging;
		header.command = 0x01;
		header.error_code = 0;

		client->notify(std::move(header), outBuffer);
	}

	void MessagingComponent::OnSendMessage(Client* client, Header header) {
		std::cout << "OnSendMessage" << std::endl;
		OnSendMessageResponse(client);
	}

	void MessagingComponent::OnFetchMessages(Client* client, Header header) {
		std::cout << "OnFetchMessages" << std::endl;
	}

	void MessagingComponent::OnPurgeMessages(Client* client, Header header) {
		std::cout << "OnPurgeMessages" << std::endl;
	}

	void MessagingComponent::OnTouchMessages(Client* client, Header header) {
		std::cout << "OnTouchMessages" << std::endl;
	}

	void MessagingComponent::OnGetMessages(Client* client, Header header) {
		std::cout << "OnGetMessages" << std::endl;
	}
}
