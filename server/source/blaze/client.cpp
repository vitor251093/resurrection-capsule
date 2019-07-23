
// Include
#include "client.h"

#include "component/authcomponent.h"
#include "component/redirectorcomponent.h"
#include "component/utilcomponent.h"
#include "component/usersessioncomponent.h"
#include "component/gamemanagercomponent.h"
#include "component/associationcomponent.h"
#include "component/roomscomponent.h"
#include "component/messagingcomponent.h"
#include "component/playgroupscomponent.h"

#include <boost/bind.hpp>
#include <iostream>

// Blaze
namespace Blaze {
	// Client
	Client::Client(boost::asio::io_context& io_service, boost::asio::ssl::context& context) :
		Network::Client(io_service), mSocket(io_service, context)
	{
		static uint32_t id = 0;
		mId = ++id;
		
		//
		int timeout = -1;

		auto nativeHandle = get_socket().native_handle();
		setsockopt(nativeHandle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int));
		setsockopt(nativeHandle, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(int));

		mSocket.set_verify_mode(boost::asio::ssl::verify_none);
		mReadBuffer.resize(10240);
	}

	void Client::start() {
		mSocket.async_handshake(boost::asio::ssl::stream_base::server,
			boost::bind(&Client::handle_handshake, this, boost::asio::placeholders::error));
	}

	void Client::send(const Header& header, const DataBuffer* buffer) {
#if 1
		DataBuffer& writeBuffer = mWriteBuffers.emplace_back();

		size_t length;
		if (buffer) {
			length = buffer->size();
		} else {
			length = 0;
		}

		writeBuffer.write_u16_be(static_cast<uint16_t>(length));
		writeBuffer.write_u16_be(static_cast<uint16_t>(header.component));
		writeBuffer.write_u16_be(header.command);
		writeBuffer.write_u16_be(header.error_code);

		uint32_t message = 0;
		message |= static_cast<uint32_t>(header.message_type) << 28;
		// message |= 0 & 0x3FF;
		message |= header.message_id & 0xFFFFF;
		writeBuffer.write_u32_be(message);

		if (length > 0) {
			writeBuffer.insert(*buffer);
		}
#else
		size_t length = buffer.size();

		mWriteBuffer.write_u16_be(static_cast<uint16_t>(length));
		mWriteBuffer.write_u16_be(static_cast<uint16_t>(header.component));
		mWriteBuffer.write_u16_be(header.command);
		mWriteBuffer.write_u16_be(header.error_code);

		if (header.message_type != MessageType::Notification) {
			uint32_t message = 0;
			message |= static_cast<uint32_t>(header.message_type) << 28;
			// message |= 0 & 0x3FF;
			message |= header.message_id & 0xFFFFF;
			mWriteBuffer.write_u32_be(message);
		} else {
			mWriteBuffer.write_u16_le(static_cast<uint16_t>(header.message_type) << 4);
			mWriteBuffer.write_u16_be(header.message_id);
		}

		if (length > 0) {
			mWriteBuffer.insert(buffer);
		}
#endif
	}

	void Client::notify(Header header, const DataBuffer& buffer) {
		header.message_type = MessageType::Notification;
		header.message_id = 0;
		send(header, &buffer);
	}

	void Client::reply(Header header) {
		header.message_type = (header.error_code > 0) ? MessageType::ErrorReply : MessageType::Reply;
		if (header.message_id == 0) {
			header.message_id = mCurrentMessageId;
		}
		send(header, nullptr);
	}

	void Client::reply(Header header, const DataBuffer& buffer) {
		header.message_type = (header.error_code > 0) ? MessageType::ErrorReply : MessageType::Reply;
		if (header.message_id == 0) {
			header.message_id = mCurrentMessageId;
		}
		send(header, &buffer);
	}

	void Client::handle_handshake(const boost::system::error_code& error) {
		if (!error) {
			mSocket.async_read_some(boost::asio::buffer(mReadBuffer.data(), mReadBuffer.capacity()),
				boost::bind(&Client::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		} else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
			std::cout << "Handshake error: Client disconnected." << std::endl;
			delete this;
		} else {
			std::cout << "Handshake error: " << error.message() << std::endl;
			delete this;
		}
	}

	void Client::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
		if (!error) {
			mReadBuffer.resize(bytes_transferred);
			mReadBuffer.set_position(0);

			mWriteBuffers.clear();
			while (mReadBuffer.position() < bytes_transferred) {
				parse_packets();
			}

#if 1
			for (const auto& buffer : mWriteBuffers) {
				boost::asio::write(mSocket, boost::asio::buffer(buffer.data(), buffer.size()));
			}

			mSocket.async_read_some(boost::asio::buffer(mReadBuffer.data(), mReadBuffer.capacity()),
				boost::bind(&Client::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
#else
			boost::asio::async_write(mSocket,
				boost::asio::buffer(mWriteBuffer.data(), mWriteBuffer.size()),
				boost::bind(&Client::handle_write, this, boost::asio::placeholders::error));
#endif
		} else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
			std::cout << "Error: Client disconnected." << std::endl;
			delete this;
		} else {
			std::cout << "Error: " << error.message() << std::endl;
			delete this;
		}
	}

	void Client::handle_write(const boost::system::error_code& error) {
		if (!error) {
			mWriteBuffer.clear();
			mSocket.async_read_some(boost::asio::buffer(mReadBuffer.data(), mReadBuffer.capacity()),
				boost::bind(&Client::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		} else if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset) {
			std::cout << "Error: Client disconnected." << std::endl;
			delete this;
		} else {
			std::cout << "Error: " << error.message() << std::endl;
			delete this;
		}
	}

	void Client::parse_packets() {
		/*
		uint32_t message = mReadBuffer.read_u32_be();
		header.message_type = static_cast<Blaze::MessageType>(message >> 30);
		header.message_id = message & 0x3FFFFFF;
		*/

		Header header;
		header.length = mReadBuffer.read_u16_be();
		header.component = static_cast<Blaze::Component>(mReadBuffer.read_u16_be());
		header.command = mReadBuffer.read_u16_be();
		header.error_code = mReadBuffer.read_u16_be();

		uint32_t message = mReadBuffer.read_u32_be();
		header.message_type = static_cast<Blaze::MessageType>(message >> 28);
		header.message_id = message & 0xFFFFF;
		/*
		if (message & 1) {
			header.length |= mReadBuffer.read_u16_be();
		}
		*/

		auto pos = mReadBuffer.position();

		mRequest = {};
		TDF::Parse(mReadBuffer, mRequest);
		
		if (header.component != Blaze::Component::UserSessions) {
			// Log(mRequest);
			std::cout << "Component: " << static_cast<int>(header.component) <<
				", Command: 0x" << std::hex << header.command << std::dec <<
				", Type: " << (message >> 28) <<
				std::endl;
		}

		mCurrentMessageId = header.message_id;
		switch (header.component) {
			case Blaze::Component::AssociationLists:
				Blaze::AssociationComponent::Parse(this, header);
				break;

			case Blaze::Component::Authentication:
				Blaze::AuthComponent::Parse(this, header);
				break;

			case Blaze::Component::Redirector:
				Blaze::RedirectorComponent::Parse(this, header);
				break;

			case Blaze::Component::Messaging:
				Blaze::MessagingComponent::Parse(this, header);
				break;

			case Blaze::Component::Playgroups:
				Blaze::PlaygroupsComponent::Parse(this, header);
				break;

			case Blaze::Component::Util:
				Blaze::UtilComponent::Parse(this, header);
				break;

			case Blaze::Component::GameManager:
				Blaze::GameManagerComponent::Parse(this, header);
				break;

			case Blaze::Component::Rooms:
				Blaze::RoomsComponent::Parse(this, header);
				break;

			case Blaze::Component::UserSessions:
				// Log(mRequest);
				Blaze::UserSessionComponent::Parse(this, header);
				break;

			default:
				std::cout << static_cast<int>(header.component) << std::endl;
				break;
		}
	}
}

/*
	Encoded names in client
		ADDR - union
		NQOS
		NLMP
		BSDK - string
		BTIM - string
		CLNT - string
		CSKU - string
		CVER - string
		DSDK - string
		ENV - string
		FPID - union
		LOC - integer
		NAME - string
		PLAT - string
		PROF - string
		TIID - unknown
		RPRT - unknown
		PJID - unknown
		OIDS - unknown
		CSIG - unknown
		ADRS - unknown
		XDNS - integer
		SECU - integer
		MSGS - unknown
		NMAP - unknown
		AMAP - unknown
*/
