
// Include
#include "packet.h"

// Network
namespace Network {
	uint32_t CompressLabel(const std::string& label) {
		uint32_t ret = 0;
		for (int i = 0; i < 4; ++i) {
			ret |= (0x20 | (label[i] & 0x1F)) << ((3 - i) * 6);
		}
		return _byteswap_ulong(ret) >> 8;
	}

	std::string DecompressLabel(uint32_t label) {
		label = _byteswap_ulong(label) >> 8;

		std::string ret;
		for (uint32_t i = 0; i < 4; ++i) {
			uint32_t val = (label >> ((3 - i) * 6)) & 0x3F;
			if (val > 0) {
				ret.push_back(0x40 | (val & 0x1F));
			}
		}

		return ret;
	}

	// Packet
	Packet::Packet(Client* client) : mClient(client) {
		assert(client != nullptr);

		auto& buffer = mClient->get_read_buffer();
		mLength = buffer.read_u16_le();
		mComponent = buffer.read_u16_le();
		mCommand = buffer.read_u16_le();
		mError = buffer.read_u16_le();
		mQType = buffer.read_u16_le();
		mId = buffer.read_u16_le();
		if (mQType & 0x10) {
			mExtLength = buffer.read_u16_le();
		} else {
			mExtLength = 0;
		}
	}

	uint16_t Packet::get_component() const {
		return mComponent;
	}

	uint16_t Packet::get_command() const {
		return mCommand;
	}

	bool Packet::eof() const {
		return mClient->get_read_buffer().eof();
	}

	Header Packet::read_header() {
		Header header;

		uint32_t value = mStream.read<uint32_t>();
		header.type = static_cast<Type>(value >> 24);
		header.label = DecompressLabel(value & 0x00FFFFFF);

		return header;
	}

	void Packet::write_header(const std::string& label, Type type) {
		mStream.write<uint32_t>(CompressLabel(label) | (type << 24));
	}

	uint32_t Packet::read_integer() {
		return decode_integer();
	}

	void Packet::write_integer(const std::string& label, uint32_t value) {
		write_header(label, Type::Integer1);
		encode_integer(value);
	}

	std::string Packet::read_string() {
		std::string value;

		uint32_t length = decode_integer() - 1;
		if (length > 0) {
			value.resize(length);
			mStream.read<char>(&value[0], length);
		}

		mStream.skip<char>();
		return value;
	}

	void Packet::write_string(const std::string& label, const std::string& value) {
		write_header(label, Type::String);

		uint32_t length = value.length();
		encode_integer(length + 1);

		mStream.write<char>(&value[0], length);
		mStream.write<char>(0);
	}

	void Packet::encode_integer(uint32_t value) {
		if (value < 0x40) {
			mStream.write<uint8_t>(value & 0xFF);
		} else {
			mStream.write<uint8_t>((value & 0x3F) | 0x80);

			value >>= 6;
			while (value >= 0x80) {
				mStream.write<uint8_t>((value & 0x7F) | 0x80);
				value >>= 7;
			}

			mStream.write<uint8_t>(value);
		}
	}

	uint32_t Packet::decode_integer() {
		uint32_t value = mStream.read<uint8_t>();
		if (value >= 0x80) {
			value &= 0x3F;
			for (uint32_t i = 1; i < 8; i++) {
				uint32_t next_value = mStream.read<uint8_t>();
				value |= (next_value & 0x7F) << ((i * 7) - 1);

				if (next_value < 0x80) {
					break;
				}
			}
		}
		return value;
	}

	/*
	std::unique_ptr<Packet> Packet::From(MemoryStream& stream) {
		std::unique_ptr<Packet> packet;
		if (!stream.eof()) {
			uint32_t header = stream.read<uint32_t>();

			Type type = static_cast<Type>(header >> 24);
			switch (type) {
				case Type::Integer1:
					packet = std::make_unique<IntegerPacket>(header, stream);
					break;
			}
		}
		return packet;
	}
	*/
}
/*
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

#include "Tdf.h"

Tdf::Tdf()
{

}

Tdf::~Tdf()
{

}

Tdf* Tdf::fromPacket(BlazeInStream* stream)
{
	TdfHeader* Header = (TdfHeader *)stream->ReadP(0); //HACK

	switch (Header->Type)
	{
	case TDF_INTEGER_1:
	case TDF_INTEGER_2:
	case TDF_INTEGER_3:
		return TdfInteger::fromPacket(stream);
	case TDF_STRING:
		return TdfString::fromPacket(stream);
	case TDF_STRUCT:
		return TdfStruct::fromPacket(stream);
	case TDF_LIST:
		return TdfList::fromPacket(stream);
	case TDF_DOUBLE_LIST:
		return TdfDoubleList::fromPacket(stream);
	case TDF_INTEGER_LIST:
		return TdfIntegerList::fromPacket(stream);
	case TDF_VECTOR2D:
		return TdfVector2D::fromPacket(stream);
	case TDF_VECTOR3D:
		return TdfVector3D::fromPacket(stream);
	default:
		printf("Unsupported type: %i\n", Header->Type);
	}

	return nullptr;
}

DWORD Tdf::CompressLabel(DWORD Label)
{
	DWORD ret = 0;

	for (int i = 0; i < 4; ++i)
		ret |= (0x20 | ((((BYTE *)&Label)[i]) & 0x1F)) << ((3 - i) * 6);

	return _byteswap_ulong(ret) >> 8;
}

DWORD Tdf::DecompressLabel(DWORD Label)
{
	Label = _byteswap_ulong(Label) >> 8;

	DWORD ret = 0;

	for (int i = 0; i < 4; ++i)
	{
		DWORD j = (Label >> (3 - i) * 6) & 0x3F;
		if (j > 0)
			((BYTE *)&ret)[i] = (BYTE)(0x40 | (j & 0x1F));
	}

	return ret;
}
*/
/*DWORD Tdf::CompressInteger(DWORD integer)
{
	if (integer < 0x40)
		return integer;

	DWORD result = 0x80 | (integer & 0x3F);

	for (int i = 1; i < 4; i++)
	{
		if (integer >> (i * 7 - 1) == 0)
			break;

		((BYTE *)&result)[i] = 0x80 | ((integer >> (i * 7 - 1)) & 0x7F);
	}

	return result;
}

DWORD Tdf::DecompressInteger(void* data, DWORD * offset)
{
	BYTE* buff = (BYTE *)data + *offset;

	DWORD res = buff[0] & 0x3F; ++*offset;

	if (buff[0] < 0x80)
		return res;

	for (int i = 1; i < 8; i++)
	{
		res |= (DWORD)(buff[i] & 0x7F) << (i * 7 - 1); ++*offset;
		
		if (buff[i] < 0x80)
			break;
	}

	return res;
}*/