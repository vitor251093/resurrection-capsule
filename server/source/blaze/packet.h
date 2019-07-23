
#ifndef _PACKET_HEADER
#define _PACKET_HEADER

// Include
#include "enums.h"
#include "client.h"

// Network
namespace Network {
	static uint32_t CompressLabel(const std::string& label);
	static std::string DecompressLabel(uint32_t label);

	// Header
	struct Header {
		std::string label;
		TDFType type;
	};

	// Packet
	class Packet {
		public:
			Component get_component() const;
			void set_component(Component component);

			uint16_t get_command() const;
			void set_command(uint16_t command);

			uint16_t get_error_code() const;
			void set_error_code(uint16_t error_code);

			MessageType get_message_type() const;
			void set_message_type(MessageType message_type);

			uint16_t get_id() const;
			void set_id(uint16_t id);

			bool eof() const;

			uint32_t read_integer();
			void write_integer(const std::string& label, uint32_t value);

			std::string read_string();
			void write_string(const std::string& label, const std::string& value);

		private:
			Header read_header();
			void write_header(const std::string& label, TDFType type);

			void encode_integer(uint32_t value);
			uint32_t decode_integer();

		private:
			DataBuffer mBuffer;

			uint16_t mLength;
			Component mComponent;
			uint16_t mCommand;
			uint16_t mError;
			MessageType mMessageType;
			uint16_t mId;
			uint16_t mExtLength;
	};
}

#endif
