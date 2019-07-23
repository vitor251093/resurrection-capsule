
#ifndef _BLAZE_TDF_HEADER
#define _BLAZE_TDF_HEADER

// Include
#include "../databuffer.h"
#include "types.h"
#include <rapidjson/document.h>
#include <memory>

// Blaze
namespace Blaze {
	void Log(const rapidjson::Document& document);

	// TDF
	namespace TDF {
		using ValuePtr = std::shared_ptr<class Value>;

		// Type
		enum class Type {
			Integer,
			String,
			Binary,
			Struct,
			List,
			Map,
			Union,
			IntegerList,
			Vector2,
			Vector3,
			Float,
			TimeValue,
			Invalid = 0xFF
		};

		// Header
		struct Header {
			std::string label;
			Type type;
		};

		uint32_t CompressLabel(const std::string& label);
		std::string DecompressLabel(uint32_t label);

		Header ReadHeader(DataBuffer& buffer);
		void WriteHeader(DataBuffer& buffer, const Header& header);
		void WriteHeader(DataBuffer& buffer, const std::string& label, Type type);
		void WriteUnionHeader(DataBuffer& buffer, const std::string& label, NetworkAddressMember activeMember);

		uint64_t ReadInteger(DataBuffer& buffer);
		void WriteInteger(DataBuffer& buffer, const std::string& label, uint64_t value);

		std::string ReadString(DataBuffer& buffer);
		void WriteString(DataBuffer& buffer, const std::string& label, const std::string& value);

		// Parser
		void Parse(DataBuffer& buffer, rapidjson::Document& document);
		void ParseValue(DataBuffer& buffer, rapidjson::Value& parent, rapidjson::Document::AllocatorType& allocator);

		rapidjson::Value ParseInteger(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseString(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseBlob(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseStruct(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseUnion(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseList(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseMap(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseIntegerList(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseVector2(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);
		rapidjson::Value ParseVector3(DataBuffer& buffer, rapidjson::Document::AllocatorType& allocator);

		// Packet
		class Packet {
			public:
				Packet();
				
				void Write(DataBuffer& buffer);

				void PutInteger(rapidjson::Value* parent, const std::string& label, uint64_t value);
				void PutString(rapidjson::Value* parent, const std::string& label, const std::string& value);
				void PutBlob(rapidjson::Value* parent, const std::string& label, const uint8_t* data, uint64_t size);
				void PutVector2(rapidjson::Value* parent, const std::string& label, uint64_t x, uint64_t y);
				void PutVector3(rapidjson::Value* parent, const std::string& label, uint64_t x, uint64_t y, uint64_t z);

				rapidjson::Value& CreateStruct(rapidjson::Value* parent, const std::string& label);
				rapidjson::Value& CreateUnion(rapidjson::Value* parent, const std::string& label, NetworkAddressMember addressMember);
				rapidjson::Value& CreateList(rapidjson::Value* parent, const std::string& label, Type listType, bool isStub = false);
				rapidjson::Value& CreateMap(rapidjson::Value* parent, const std::string& label, Type keyType, Type valueType);

				template<typename T>
				std::enable_if_t<std::is_enum_v<T>, void> PutInteger(rapidjson::Value* parent, const std::string& label, T value) {
					PutInteger(parent, label, static_cast<uint64_t>(value));
				}

			private:
				rapidjson::Value& AddMember(rapidjson::Value* parent, const std::string& name, rapidjson::Value& value);

				void Write(DataBuffer& buffer, rapidjson::Value* name, rapidjson::Value& value);
				void WriteType(DataBuffer& buffer, Type type, rapidjson::Value* name, rapidjson::Value& value);

			private:
				rapidjson::Document mDocument;
				rapidjson::Document::AllocatorType& mAllocator;
		};

		/*
		Blaze::TDF::Header read_tdf_header();
		void write_tdf_header(const std::string& label, Blaze::TDF::Type type);

		uint64_t read_tdf_integer();
		void write_tdf_integer(const std::string& label, uint64_t value);

		std::string read_tdf_string();
		void write_tdf_string(const std::string& label, const std::string& value);

		Blaze::TDF::ValuePtr read_tdf_value();
		void write_tdf_value(const Blaze::TDF::ValuePtr& value);

		// blaze template
		template<enum T>
		auto read_tdf_list() {
			if constexpr (std::is_same_v<T, std::string>) {

			}
		}
		*/
	}
}

#endif
