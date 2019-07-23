
#ifndef _UTILS_FUNCTIONS_HEADER
#define _UTILS_FUNCTIONS_HEADER

// Include
#include <cstdint>
#include <vector>
#include <string>
#include <pugixml.hpp>

// utils
namespace utils {
	namespace detail {
		// Strings
		template<class StrT, class DelimT, class StrRT = std::remove_cvref_t<StrT>>
		auto explode_string(StrT str, DelimT delim, int32_t limit) {
			std::vector<StrRT> strings;

			size_t start = 0, end = 0;
			while (--limit != -1 && (end = str.find(delim, start)) != StrRT::npos) {
				strings.push_back(str.substr(start, end - start));
				if constexpr (std::is_class_v<std::remove_cvref_t<DelimT>>) {
					start = end + delim.size();
				} else {
					start = end + sizeof(delim);
				}
			}

			strings.push_back(str.substr(start));
			return strings;
		}
	}

	// Time
	uint64_t get_unix_time();

	// Files
	std::string get_file_text(const std::string& path);

	// Strings
	void string_replace(std::string& str, const std::string& old_str, const std::string& new_str);

	std::vector<std::string> explode_string(const std::string& str, char delim, int32_t limit = -1);
	std::vector<std::string> explode_string(const std::string& str, const std::string& delim, int32_t limit = -1);

	std::vector<std::string_view> explode_string(std::string_view str, char delim, int32_t limit = -1);
	std::vector<std::string_view> explode_string(std::string_view str, std::string_view delim, int32_t limit = -1);

	// Numbers
	template<typename T>
	std::enable_if_t<std::is_enum_v<T>, T> to_number(const std::string& str) {
		return static_cast<T>(tonumber<std::underlying_type_t<T>>(str));
	}

	template<typename T>
	std::enable_if_t<std::is_integral_v<T>, T> to_number(const std::string& str, int base = 10) {
		T value;
		try {
			if constexpr (std::is_signed_v<T>) {
				if constexpr (sizeof(T) >= sizeof(int64_t)) {
					value = std::stoll(str, nullptr, base);
				} else {
					value = static_cast<T>(std::stoi(str, nullptr, base));
				}
			} else if constexpr (sizeof(T) >= sizeof(uint64_t)) {
				value = std::stoull(str, nullptr, base);
			} else {
				value = static_cast<T>(std::stoul(str, nullptr, base));
			}
		} catch (...) {
			value = static_cast<T>(0);
		}
		return value;
	}

	template<typename T>
	std::enable_if_t<std::is_floating_point_v<T>, T> to_number(const std::string& str) {
		T value;
		try {
			if constexpr (std::is_same_v<T, double>) {
				value = std::stod(str);
			} else if constexpr (std::is_same_v<T, float>) {
				value = std::stof(str);
			} else {
				value = static_cast<T>(0);
			}
		} catch (...) {
			value = static_cast<T>(0);
		}
		return value;
	}

	// XML
	void xml_add_text_node(pugi::xml_node& node, const std::string& name, const std::string& value);
	std::string xml_get_text_node(const pugi::xml_node& node, const std::string& name);

	template<typename T>
	std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, void> xml_add_text_node(pugi::xml_node& node, const std::string& name, T value) {
		xml_add_text_node(node, name, std::to_string(value));
	}

	template<typename T>
	std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>, T> xml_get_text_node(const pugi::xml_node& node, const std::string& name) {
		return to_number<T>(xml_get_text_node(node, name));
	}
}

#endif
