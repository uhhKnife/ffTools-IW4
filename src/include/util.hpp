#pragma once

#include <string>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace util
{
	inline std::string get_basename(const std::string& filepath)
	{
		std::filesystem::path p(filepath);
		std::string name = p.stem().string();
		
		for (char& c : name)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
		
		return name;
	}

	inline void strtoupper(std::string& str)
	{
		for (char& c : str)
			c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	}

	inline std::uint32_t to_big_endian_32(std::uint32_t val)
	{
		return ((val & 0x000000FF) << 24) | ((val & 0x0000FF00) << 8) |
		       ((val & 0x00FF0000) >> 8) | ((val & 0xFF000000) >> 24);
	}

	inline std::uint16_t to_big_endian_16(std::uint16_t val)
	{
		return ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
	}

	inline std::uint32_t read_be32(const unsigned char* buf, size_t pos)
	{
		return (buf[pos] << 24) | (buf[pos + 1] << 16) | (buf[pos + 2] << 8) | buf[pos + 3];
	}
}
