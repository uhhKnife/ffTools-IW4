#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <memory>

namespace binary_io
{
	inline void write_be32(std::ofstream& file, std::uint32_t val)
	{
		std::uint32_t be_val = ((val & 0x000000FF) << 24) | ((val & 0x0000FF00) << 8) |
		                       ((val & 0x00FF0000) >> 8) | ((val & 0xFF000000) >> 24);
		file.write(reinterpret_cast<const char*>(&be_val), sizeof(be_val));
	}

	inline void write_be16(std::ofstream& file, std::uint16_t val)
	{
		std::uint16_t be_val = ((val & 0x00FF) << 8) | ((val & 0xFF00) >> 8);
		file.write(reinterpret_cast<const char*>(&be_val), sizeof(be_val));
	}

	inline void write_string(std::ofstream& file, const std::string& str)
	{
		file.write(str.c_str(), str.length());
		char null_byte = '\0';
		file.write(&null_byte, 1);
	}

	inline std::string read_file_to_memory(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary);
		if (!file.is_open())
			return "";

		file.seekg(0, std::ios::end);
		size_t filesize = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string buffer(filesize, '\0');
		file.read(&buffer[0], filesize);
		file.close();

		return buffer;
	}
}
