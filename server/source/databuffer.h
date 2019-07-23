

#ifndef _DATA_BUFFER_HEADER
#define _DATA_BUFFER_HEADER

// Include
#include <vector>
#include <algorithm>

// DataBuffer
class DataBuffer {
	private:
		bool can_rw(const size_t length) const;
		void resize_buffer();

		size_t impl_peek(void* variable, const size_t S, const size_t N);
		size_t impl_read(void* variable, const size_t S, const size_t N);
		size_t impl_write(const void* variable, const size_t S, const size_t N);

	public:
		uint8_t* data();
		const uint8_t* data() const;

		size_t position() const;
		void set_position(size_t newPosition);

		size_t size() const;
		void resize(size_t newSize);

		size_t capacity() const;
		void reserve(size_t newCapacity);

		void clear();
		bool eof() const;

		void insert(const DataBuffer& buffer);

		std::vector<uint8_t> read_bytes(const size_t count);

		// blaze
		void encode_tdf_integer(uint64_t value);
		uint64_t decode_tdf_integer();

		// endianess
		uint16_t peek_u16_le();
		uint32_t peek_u32_le();
		uint64_t peek_u64_le();
		float peek_float_le();
		double peek_double_le();

		uint16_t peek_u16_be();
		uint32_t peek_u32_be();
		uint64_t peek_u64_be();
		float peek_float_be();
		double peek_double_be();

		uint16_t read_u16_le();
		uint32_t read_u32_le();
		uint64_t read_u64_le();
		float read_float_le();
		double read_double_le();

		uint16_t read_u16_be();
		uint32_t read_u32_be();
		uint64_t read_u64_be();
		float read_float_be();
		double read_double_be();

		void write_u16_le(uint16_t variable);
		void write_u32_le(uint32_t variable);
		void write_u64_le(uint64_t variable);
		void write_float_le(float variable);
		void write_double_le(double variable);

		void write_u16_be(uint16_t variable);
		void write_u32_be(uint32_t variable);
		void write_u64_be(uint64_t variable);
		void write_float_be(float variable);
		void write_double_be(double variable);

		// template functions
		template<typename T>
		inline size_t peek(void* variable, const size_t N = 1) {
			return impl_peek(variable, sizeof(T), N);
		}

		template<typename T>
		inline size_t read(void* variable, const size_t N = 1) {
			return impl_read(variable, sizeof(T), N);
		}

		template<typename T>
		inline size_t write(const void* variable, const size_t N) {
			return impl_write(variable, sizeof(T), N);
		}

		template<typename T>
		inline T peek() {
			T variable;
			(void)peek<T>(&variable, 1);
			return variable;
		}

		template<typename T>
		inline T read() {
			T variable;
			(void)read<T>(&variable, 1);
			return variable;
		}

		template<typename T>
		inline size_t write(const T& variable) {
			return write<T>(&variable, 1);
		}

		template<typename T>
		inline void skip(const size_t N = 1) {
			mPosition += sizeof(T) * N;
		}

	private:
		std::vector<uint8_t> mBuffer;
		size_t mSize;
		size_t mPosition;
};

#endif
