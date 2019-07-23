
// Include
#include "databuffer.h"

// Endianess
#ifdef _MSC_VER
#	define BSWAP16 _byteswap_ushort
#	define BSWAP32 _byteswap_ulong
#	define BSWAP64 _byteswap_uint64
#else
#	define BSWAP16 __builtin_bswap16
#	define BSWAP32 __builtin_bswap32
#	define BSWAP64 __builtin_bswap64
#endif

constexpr static bool is_big_endian() {
	constexpr uint32_t value = 0x12345678;
	return ((const uint8_t&)value) == 0x12;
}

constexpr static bool is_little_endian() {
	return !is_big_endian();
}

// DataBuffer
bool DataBuffer::can_rw(const size_t length) const {
	return (mPosition + length) <= mBuffer.size();
}

void DataBuffer::resize_buffer() {
	if (mBuffer.empty()) {
		reserve(64);
	} else {
		reserve(mBuffer.size() + 1024);
	}
}

size_t DataBuffer::impl_peek(void* variable, const size_t S, const size_t N) {
	size_t length = S * N;
	if (can_rw(length)) {
		std::memcpy(variable, &mBuffer[mPosition], length);
	} else {
		length = 0;
	}
	return length;
}

size_t DataBuffer::impl_read(void* variable, const size_t S, const size_t N) {
	size_t length = S * N;
	if (can_rw(length)) {
		std::memcpy(variable, &mBuffer[mPosition], length);
		mPosition += length;
	} else {
		length = 0;
	}
	return length;
}

size_t DataBuffer::impl_write(const void* variable, const size_t S, const size_t N) {
	size_t length = S * N;
	while (!can_rw(length)) {
		resize_buffer();
	}
	
	std::memcpy(&mBuffer[mPosition], variable, length);
	mPosition += length;
	mSize = std::max<size_t>(mSize, mPosition);

	return length;
}

void DataBuffer::encode_tdf_integer(uint64_t value) {
	if (value < 0x40) {
		write<uint8_t>(value & 0xFF);
	} else {
		write<uint8_t>((value & 0x3F) | 0x80);

		value >>= 6;
		while (value >= 0x80) {
			write<uint8_t>((value & 0x7F) | 0x80);
			value >>= 7;
		}

		write<uint8_t>(static_cast<uint8_t>(value));
	}
}

uint64_t DataBuffer::decode_tdf_integer() {
	uint64_t value = read<uint8_t>();
	if (value >= 0x80) {
		value &= 0x3F;
		for (uint32_t i = 1; i < 8; i++) {
			uint64_t next_value = read<uint8_t>();
			value |= (next_value & 0x7F) << ((i * 7) - 1);

			if (next_value < 0x80) {
				break;
			}
		}
	}
	return value;
}

uint8_t* DataBuffer::data() {
	return mBuffer.data();
}

const uint8_t* DataBuffer::data() const {
	return mBuffer.data();
}

size_t DataBuffer::position() const {
	return mPosition;
}

void DataBuffer::set_position(size_t newPosition) {
	mPosition = std::min<size_t>(mSize, newPosition);
}

size_t DataBuffer::size() const {
	return mSize;
}

void DataBuffer::resize(size_t newSize) {
	mSize = newSize;
	if (mSize > mBuffer.size()) {
		reserve(mSize);
	}
}

size_t DataBuffer::capacity() const {
	return mBuffer.size();
}

void DataBuffer::reserve(size_t newCapacity) {
	mBuffer.resize(newCapacity);
}

void DataBuffer::clear() {
	mBuffer.clear();
	mPosition = 0;
	mSize = 0;
}

bool DataBuffer::eof() const {
	return mPosition >= mSize;
}

void DataBuffer::insert(const DataBuffer& buffer) {
	mBuffer.insert(mBuffer.begin() + mSize, buffer.mBuffer.begin(), buffer.mBuffer.end());
	mSize += buffer.size();
}

std::vector<uint8_t> DataBuffer::read_bytes(const size_t aSize) {
	std::vector<uint8_t> result(aSize);
	read<uint8_t>(&result[0], aSize);
	return result;
}
/*
std::vector<uint64_t> DataBuffer::read_tdf_integer_list() {
	std::vector<uint64_t> value;

	uint8_t length = read<uint8_t>();
	for (uint8_t i = 0; i < length; ++i) {
		value.push_back(decode_tdf_integer());
	}

	return value;
}

void DataBuffer::write_tdf_integer_list(const std::string& label, const std::vector<uint64_t>& value) {
	uint8_t length = static_cast<uint8_t>(value.size());
	write<uint8_t>(length);
	for (uint8_t i = 0; i < length; ++i) {
		encode_tdf_integer(value[i]);
	}
}
*/
// endianess
uint16_t DataBuffer::peek_u16_le() {
	uint16_t variable;
	peek<uint16_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP16(variable);
	}
	return variable;
}

uint32_t DataBuffer::peek_u32_le() {
	uint32_t variable;
	peek<uint32_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP32(variable);
	}
	return variable;
}

uint64_t DataBuffer::peek_u64_le() {
	uint64_t variable;
	peek<uint64_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP64(variable);
	}
	return variable;
}

float DataBuffer::peek_float_le() {
	union { float f; uint32_t u; } v;
	v.u = peek_u32_le();
	return v.f;
}

double DataBuffer::peek_double_le() {
	union { double f; uint64_t u; } v;
	v.u = peek_u64_le();
	return v.f;
}

uint16_t DataBuffer::peek_u16_be() {
	uint16_t variable;
	peek<uint16_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP16(variable);
	}
	return variable;
}

uint32_t DataBuffer::peek_u32_be() {
	uint32_t variable;
	peek<uint32_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP32(variable);
	}
	return variable;
}

uint64_t DataBuffer::peek_u64_be() {
	uint64_t variable;
	peek<uint64_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP64(variable);
	}
	return variable;
}

float DataBuffer::peek_float_be() {
	union { float f; uint32_t u; } v;
	v.u = peek_u32_be();
	return v.f;
}

double DataBuffer::peek_double_be() {
	union { double f; uint64_t u; } v;
	v.u = peek_u64_be();
	return v.f;
}

uint16_t DataBuffer::read_u16_le() {
	uint16_t variable;
	read<uint16_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP16(variable);
	}
	return variable;
}

uint32_t DataBuffer::read_u32_le() {
	uint32_t variable;
	read<uint32_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP32(variable);
	}
	return variable;
}

uint64_t DataBuffer::read_u64_le() {
	uint64_t variable;
	read<uint64_t>(&variable);
	if constexpr (is_big_endian()) {
		variable = BSWAP64(variable);
	}
	return variable;
}

float DataBuffer::read_float_le() {
	union { float f; uint32_t u; } v;
	v.u = read_u32_le();
	return v.f;
}

double DataBuffer::read_double_le() {
	union { double f; uint64_t u; } v;
	v.u = read_u64_le();
	return v.f;
}

uint16_t DataBuffer::read_u16_be() {
	uint16_t variable;
	read<uint16_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP16(variable);
	}
	return variable;
}

uint32_t DataBuffer::read_u32_be() {
	uint32_t variable;
	read<uint32_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP32(variable);
	}
	return variable;
}

uint64_t DataBuffer::read_u64_be() {
	uint64_t variable;
	read<uint64_t>(&variable);
	if constexpr (is_little_endian()) {
		variable = BSWAP64(variable);
	}
	return variable;
}

float DataBuffer::read_float_be() {
	union { float f; uint32_t u; } v;
	v.u = read_u32_be();
	return v.f;
}

double DataBuffer::read_double_be() {
	union { double f; uint64_t u; } v;
	v.u = read_u64_be();
	return v.f;
}

void DataBuffer::write_u16_le(uint16_t variable) {
	if constexpr (is_big_endian()) {
		variable = BSWAP16(variable);
	}
	write<uint16_t>(variable);
}

void DataBuffer::write_u32_le(uint32_t variable) {
	if constexpr (is_big_endian()) {
		variable = BSWAP32(variable);
	}
	write<uint32_t>(variable);
}

void DataBuffer::write_u64_le(uint64_t variable) {
	if constexpr (is_big_endian()) {
		variable = BSWAP64(variable);
	}
	write<uint64_t>(variable);
}

void DataBuffer::write_float_le(float variable) {
	union { float f; uint32_t u; } v;
	v.f = variable;
	write_u32_le(v.u);
}

void DataBuffer::write_double_le(double variable) {
	union { double f; uint64_t u; } v;
	v.f = variable;
	write_u64_le(v.u);
}

void DataBuffer::write_u16_be(uint16_t variable) {
	if constexpr (is_little_endian()) {
		variable = BSWAP16(variable);
	}
	write<uint16_t>(variable);
}

void DataBuffer::write_u32_be(uint32_t variable) {
	if constexpr (is_little_endian()) {
		variable = BSWAP32(variable);
	}
	write<uint32_t>(variable);
}

void DataBuffer::write_u64_be(uint64_t variable) {
	if constexpr (is_little_endian()) {
		variable = BSWAP64(variable);
	}
	write<uint64_t>(variable);
}

void DataBuffer::write_float_be(float variable) {
	union { float f; uint32_t u; } v;
	v.f = variable;
	write_u32_be(v.u);
}

void DataBuffer::write_double_be(double variable) {
	union { double f; uint64_t u; } v;
	v.f = variable;
	write_u64_be(v.u);
}
