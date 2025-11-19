#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <filesystem>

#include "types.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include "compression.hpp"
#include "assets.hpp"

namespace fs = std::filesystem;

#define APP_VERSION "0.1.0"

XAssetType asset_type_for_string(const std::string& type_str)
{
	if (type_str == "localize")
		return XAssetType::LOCALIZE_ENTRY;
	if (type_str == "rawfile")
		return XAssetType::RAWFILE;
	if (type_str == "stringtable")
		return XAssetType::STRINGTABLE;
	return static_cast<XAssetType>(-1);
}
void init_asset_handlers_impl()
{
	asset_handlers[static_cast<int>(XAssetType::LOCALIZE_ENTRY)] = {load_localize_entry, serialize_localize_entry, extract_localize_entry};
	asset_handlers[static_cast<int>(XAssetType::RAWFILE)] = {load_raw_file, serialize_raw_file, extract_raw_file};
	asset_handlers[static_cast<int>(XAssetType::STRINGTABLE)] = {load_string_table, serialize_string_table, extract_string_table};
}

void write_zone_memory_header(std::ofstream& fp, const XZoneMemory& mem)
{
	binary_io::write_be32(fp, mem.size);
	binary_io::write_be32(fp, mem.externalsize);
	for (int i = 0; i < MAX_XFILE_COUNT; i++)
		binary_io::write_be32(fp, mem.streams[i]);
}

void write_xassetlist(std::ofstream& fp, const XAssetList& list)
{
	const size_t static_size = 5000000;
	XZoneMemory zone_memory = {static_size, 0, {static_size, 0, 0, static_size, 0, 0}};
	write_zone_memory_header(fp, zone_memory);

	binary_io::write_be32(fp, list.scriptStringCount);
	binary_io::write_be32(fp, list.scriptStrings);
	binary_io::write_be32(fp, list.assetCount);
	binary_io::write_be32(fp, list.assets);

	for (std::uint32_t i = 0; i < list.scriptStringCount; ++i)
		binary_io::write_be32(fp, 0xFFFFFFFF);

	for (std::uint32_t i = 0; i < list.scriptStringCount; ++i)
		binary_io::write_string(fp, "");

	auto it = assets;
	while (it)
	{
		binary_io::write_be32(fp, static_cast<std::uint32_t>(it->type));
		binary_io::write_be32(fp, 0xFFFFFFFF);
		it = it->next;
	}
}

int write_fastfile_raw(const std::string& output_filename)
{
	std::ofstream fp(output_filename, std::ios::binary);
	if (!fp.is_open())
	{
		std::cerr << "Failed to open output file: " << output_filename << std::endl;
		return 1;
	}

	int numassets = 0;
	auto it = assets;
	while (it)
	{
		numassets++;
		it = it->next;
	}

	std::string basename = fs::path(output_filename).stem().string();

	XAssetList list = {};
	list.scriptStringCount = 0;
	list.scriptStrings = 0;
	list.assetCount = numassets;
	list.assets = 0xFFFFFFFF;

	write_xassetlist(fp, list);

	it = assets;
	while (it)
	{
		asset_handlers[static_cast<int>(it->type)].serialize(fp, *it->header);
		it = it->next;
	}

	fp.close();
	return 0;
}

int write_fastfile(const std::string& input_filename, const std::string& output_filename)
{
	std::ifstream fin(input_filename, std::ios::binary);
	if (!fin.is_open())
	{
		std::cerr << "Failed to open raw file: " << input_filename << std::endl;
		return 1;
	}

	fin.seekg(0, std::ios::end);
	size_t filesize = fin.tellg();
	fin.seekg(0, std::ios::beg);

	std::vector<unsigned char> input_data(filesize);
	fin.read(reinterpret_cast<char*>(input_data.data()), filesize);
	fin.close();

	auto compressed_data = compression::compress_data(input_data.data(), filesize);
	if (compressed_data.empty())
	{
		std::cerr << "Compression failed" << std::endl;
		return 1;
	}

	std::ofstream fout(output_filename, std::ios::binary);
	if (!fout.is_open())
	{
		std::cerr << "Failed to open output file: " << output_filename << std::endl;
		return 1;
	}

	// write magic
	const char iw4_magic[8] = {'I','W','f','f','u','1','0','0'};
	fout.write(iw4_magic, sizeof(iw4_magic));

	// version
	binary_io::write_be32(fout, 0x000000FD);

	// timestamps / region
	binary_io::write_be32(fout, 0x00000000);
	binary_io::write_be32(fout, 0x00000000);
	binary_io::write_be32(fout, 0x00000000);

	// not setting this make the dlc language mismatch?
	binary_io::write_be32(fout, 0x01000000);

	// unknown
	binary_io::write_be32(fout, 0x000000F5);
	binary_io::write_be32(fout, 0xEF0000F5);

	// 0xEF cuz idk?
	unsigned char stray = 0xEF;
	fout.write(reinterpret_cast<const char*>(&stray), 1);

	fout.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_data.size());
	fout.close();

	return 0;
}

int parse_csv(const std::string& basename, const std::string& csv)
{
	std::ifstream fp(csv);
	if (!fp.is_open())
	{
		std::cerr << "Failed to open CSV: " << csv << std::endl;
		return 1;
	}

	std::string line;
	while (std::getline(fp, line))
	{
		line.erase(remove_if(line.begin(), line.end(), [](unsigned char c) { return std::isspace(c); }),
		           line.end());

		if (line.empty())
			continue;

		size_t comma_pos = line.find(',');
		if (comma_pos == std::string::npos)
			continue;

		std::string asset_type_str = line.substr(0, comma_pos);
		std::string asset_path = line.substr(comma_pos + 1);

		std::cout << asset_type_str << ", " << asset_path << std::endl;

		XAssetType type = asset_type_for_string(asset_type_str);
		if (type == static_cast<XAssetType>(-1))
		{
			std::cerr << "Unsupported asset type: " << asset_type_str << std::endl;
			return 1;
		}

		int handler_idx = static_cast<int>(type);
		if (handler_idx < 0 || handler_idx >= static_cast<int>(XAssetType::ASSETLIST))
		{
			std::cerr << "Invalid asset type: " << asset_type_str << std::endl;
			return 1;
		}

		if (asset_handlers[handler_idx].load(type, basename, asset_path) > 0)
		{
			std::cerr << "Error loading asset: " << asset_path << std::endl;
			return 1;
		}
	}

	fp.close();
	return 0;
}

void print_banner()
{
	std::cout << "fastfile - compiler / linker v" << APP_VERSION << " for MW2" << std::endl;
}

int main(int argc, char** argv)
{
	print_banner();
	init_asset_handlers();
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " [-m] [-k] <modname>    (-m: produce .ffm; default: .ff; -k: keep .ffraw)" << std::endl;
		return 1;
	}

	bool make_ffm = false;
	bool keep_raw = false;
	std::string name;

	for (int i = 1; i < argc; ++i)
	{
		std::string a(argv[i]);
		if (a == "-m")
		{
			make_ffm = true;
		}
		else if (a == "-k")
		{
			keep_raw = true;
		}
		else if (!a.empty() && a[0] == '-')
		{
			std::cerr << "Unknown option: " << a << std::endl;
			std::cerr << "Usage: " << argv[0] << " [-m] [-k] <modname>    (-m: produce .ffm; default: .ff; -k: keep .ffraw)" << std::endl;
			return 1;
		}
		else
		{
			if (name.empty())
				name = fs::path(a).stem().string();
			else
			{
				std::cerr << "Unexpected argument: " << a << std::endl;
				std::cerr << "Usage: " << argv[0] << " [-m] [-k] <modname>    (-m: produce .ffm; default: .ff; -k: keep .ffraw)" << std::endl;
				return 1;
			}
		}
	}

	if (name.empty())
	{
		std::cerr << "Usage: " << argv[0] << " [-m] [-k] <modname>    (-m: produce .ffm; default: .ff; -k: keep .ffraw)" << std::endl;
		return 1;
	}

	fs::path csv_path = fs::path(name) / "zone_source" / (name + ".csv");
	std::string csv = csv_path.string();

	std::string basename = name;

	std::cout << "Loading CSV: " << csv << std::endl;

	if (parse_csv(basename, csv) > 0)
	{
		std::cerr << "Failed to read CSV: " << csv << std::endl;
		return 1;
	}

	std::string ffraw = basename + ".ffraw";
	std::cout << "Writing raw file: " << ffraw << std::endl;

	if (write_fastfile_raw(ffraw) > 0)
	{
		std::cerr << "Failed to write raw file" << std::endl;
		return 1;
	}

	std::string out_ext = make_ffm ? ".ffm" : ".ff";
	std::string ff_out = basename + out_ext;
	std::cout << "Compressing and writing: " << ff_out << std::endl;

	if (write_fastfile(ffraw, ff_out) > 0)
	{
		std::cerr << "Failed to write fastfile" << std::endl;
		return 1;
	}
	std::cout << "Successfully wrote: " << ff_out << std::endl;

	if (!keep_raw)
	{
		std::error_code ec;
		if (fs::remove(ffraw, ec))
		{
			std::cout << "Removed temporary file: " << ffraw << std::endl;
		}
		else if (ec)
		{
			std::cerr << "Warning: failed to remove temporary file " << ffraw << ": " << ec.message() << std::endl;
		}
	}
	return 0;
}
