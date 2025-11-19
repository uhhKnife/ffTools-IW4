#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <memory>
#include <cstdint>

#include "types.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include "compression.hpp"
#include "assets.hpp"

namespace fs = std::filesystem;

struct ff_header
{
	std::uint32_t padding = 0;
	std::uint32_t size_a = 0;
	std::uint32_t size_b = 0;
	std::uint32_t region = 0;
	std::uint32_t bool1 = 0;
};

static size_t find_zlib_header(const unsigned char* data, size_t len)
{
	for (size_t i = 0; i + 1 < len; ++i)
	{
		if (data[i] == 0x78 && (data[i + 1] == 0x01 || data[i + 1] == 0x9C || data[i + 1] == 0xDA))
			return i;
	}
	return SIZE_MAX;
}

static size_t find_zlib_header_from(const unsigned char* data, size_t len, size_t start)
{
	if (start >= len) return SIZE_MAX;
	for (size_t i = start; i + 1 < len; ++i)
	{
		if (data[i] == 0x78 && (data[i + 1] == 0x01 || data[i + 1] == 0x9C || data[i + 1] == 0xDA))
			return i;
	}
	return SIZE_MAX;
}

static bool read_prefix(const unsigned char* data, size_t len, ff_header& out, size_t& prefix_end)
{
	const unsigned char mw2_magic[] = {'I','W','f','f','u','1','0','0'};
	if (len < sizeof(mw2_magic)) return false;
	if (std::memcmp(data, mw2_magic, sizeof(mw2_magic)) != 0) return false;

	(void)out;
	prefix_end = std::min<size_t>(len, 0x1C);
	return true;
}

static const char* XAssetTypeToString(std::uint32_t t)
{
	using X = XAssetType;
	switch (static_cast<X>(t))
	{
	case X::PHYSPRESET: return "PHYSPRESET";
	case X::PHYSCOLLMAP: return "PHYSCOLLMAP";
	case X::MAP_ENTS: return "MAP_ENTS";
	case X::LOCALIZE_ENTRY: return "LOCALIZE_ENTRY";
	case X::RAWFILE: return "RAWFILE";
	case X::STRINGTABLE: return "STRINGTABLE";
	case X::ADDON_MAP_ENTS: return "ADDON_MAP_ENTS";
	default: return "UNKNOWN";
	}
}

bool read_tail(const unsigned char* data, size_t len, ff_header& out, size_t start_pos, size_t& tail_end)
{
	if (start_pos + 3 * 4 > len) return false;
	size_t pos = start_pos;
	out.padding = util::read_be32(data, pos); pos += 4;
	out.size_a = util::read_be32(data, pos); pos += 4;
	out.size_b = util::read_be32(data, pos); pos += 4;
	tail_end = pos;
	return true;
}

size_t get_zlib_offset(const unsigned char* data, size_t len, bool &is_mw2)
{
	const unsigned char mw2_magic[] = {'I','W','f','f','u','1','0','0'};
	if (len >= sizeof(mw2_magic) && std::memcmp(data, mw2_magic, sizeof(mw2_magic)) == 0)
	{
		is_mw2 = true;
		size_t start = 0x18;
		size_t end = std::min<size_t>(0x40, len - 1);
		for (size_t i = start; i + 1 <= end; ++i)
		{
			if (data[i] == 0x78 && (data[i + 1] == 0x01 || data[i + 1] == 0x9C || data[i + 1] == 0xDA))
			{
				return i;
			}
		}
		return find_zlib_header(data, len);
	}
	is_mw2 = false;
	return find_zlib_header(data, len);
}
int unlink_fastfile(const std::string& infile, const std::string& outdir)
{
	std::ifstream fin(infile, std::ios::binary);
	if (!fin.is_open())
	{
		std::cerr << "Failed to open input file: " << infile << std::endl;
		return 1;
	}

	fin.seekg(0, std::ios::end);
	size_t flen = fin.tellg();
	fin.seekg(0, std::ios::beg);

	if (flen < 38)
	{
		std::cerr << "File too small" << std::endl;
		return 1;
	}

	auto data = std::make_unique<unsigned char[]>(flen);
	if (!fin.read(reinterpret_cast<char*>(data.get()), flen))
	{
		std::cerr << "Failed to read file" << std::endl;
		return 1;
	}
	fin.close();

	ff_header header = {};
	size_t prefix_end = 0;
	bool is_mw2 = read_prefix(data.get(), flen, header, prefix_end);

	size_t offset = SIZE_MAX;
	bool tail_read = false;

	if (is_mw2)
	{
		size_t window_start = 0x18;
		size_t window_end = std::min<size_t>(0x40, flen - 1);
		for (size_t i = window_start; i + 1 <= window_end; ++i)
		{
			if (data.get()[i] == 0x78 && (data.get()[i + 1] == 0x01 || data.get()[i + 1] == 0x9C || data.get()[i + 1] == 0xDA))
			{
				offset = i;
				break;
			}
		}

		if (offset == SIZE_MAX)
		{
			size_t tail_end = 0;
			if (read_tail(data.get(), flen, header, prefix_end, tail_end))
			{
				tail_read = true;
				offset = find_zlib_header_from(data.get(), flen, tail_end);
			}
			else
			{
				offset = find_zlib_header(data.get(), flen);
			}
		}
	}
	else
	{
		offset = find_zlib_header(data.get(), flen);
	}
	if (offset == SIZE_MAX)
	{
		std::cerr << "No zlib stream found\n";
		return 1;
	}

	const unsigned char* comp_ptr = data.get() + offset;
	size_t comp_len = flen - offset;

	auto decompressed = compression::decompress_data(comp_ptr, comp_len, static_cast<size_t>(50 * 1024) * 1024);
	if (decompressed.empty())
	{
		std::cerr << "Decompression failed" << std::endl;
		return 1;
	}

	auto dest = decompressed.data();
	size_t dest_len = decompressed.size();

	size_t pos = 0;

	pos += 4 + 4 + (MAX_XFILE_COUNT * 4);

	if (pos + 16 > dest_len)
	{
		std::cerr << "Truncated asset list" << std::endl;
		return 1;
	}

	std::uint32_t scriptStringCount = util::read_be32(dest, pos);
	pos += 4;
	pos += 4;
	std::uint32_t assetCount = util::read_be32(dest, pos);
	pos += 4;
	pos += 4;

	pos += scriptStringCount * 4;

	for (std::uint32_t i = 0; i < scriptStringCount; i++)
	{
		while (pos < dest_len && dest[pos] != '\0')
			pos++;
		if (pos >= dest_len)
		{
			std::cerr << "Malformed script strings" << std::endl;
			return 1;
		}
		pos++;
	}

	auto asset_types = std::make_unique<std::uint32_t[]>(assetCount);
	auto asset_ptrs = std::make_unique<std::uint32_t[]>(assetCount);

	for (std::uint32_t i = 0; i < assetCount; i++)
	{
		if (pos + 8 > dest_len)
		{
			std::cerr << "Truncated asset headers" << std::endl;
			return 1;
		}
		asset_types[i] = util::read_be32(dest, pos);
		pos += 4;
		asset_ptrs[i] = util::read_be32(dest, pos);
		pos += 4;
	}

	fs::create_directories(outdir);
	fs::create_directories(outdir + "/zone_source");

	std::string csvpath = outdir + "/zone_source/" + outdir + ".csv";
	std::ofstream csvfile(csvpath);
	if (!csvfile.is_open())
	{
		std::cerr << "Failed to create CSV: " << csvpath << std::endl;
		return 1;
	}

	for (std::uint32_t i = 0; i < assetCount; i++)
	{
		std::uint32_t type = asset_types[i];

		const std::uint32_t PTR_PLACEHOLDER = 0xFFFFFFFFu;
		if (asset_ptrs[i] == PTR_PLACEHOLDER)
		{
		}
		else if (asset_ptrs[i] >= dest_len)
		{
			std::cerr << "Invalid asset ptr for index " << i << ": " << asset_ptrs[i] << std::endl;
			return 1;
		}
		else
		{
			pos = asset_ptrs[i];
		}

		(void)type; (void)asset_ptrs;

		switch (type)
		{
			// DISABLED: Not fully implemented yet
			// case static_cast<std::uint32_t>(XAssetType::PHYSPRESET):
			// 	if (extract_phys_preset(dest, dest_len, pos, outdir, csvfile) < 0)
			// 		return -1;
			// 	break;
			// case static_cast<std::uint32_t>(XAssetType::MAP_ENTS):
			// 	if (extract_map_ents(dest, dest_len, pos, outdir, csvfile) < 0)
			// 		return -1;
			// 	break;
			case static_cast<std::uint32_t>(XAssetType::LOCALIZE_ENTRY):
				if (extract_localize_entry(dest, dest_len, pos, outdir, csvfile) < 0)
					return -1;
				break;
			case static_cast<std::uint32_t>(XAssetType::RAWFILE):
				if (extract_raw_file(dest, dest_len, pos, outdir, csvfile) < 0)
					return -1;
				break;
			case static_cast<std::uint32_t>(XAssetType::STRINGTABLE):
				if (extract_string_table(dest, dest_len, pos, outdir, csvfile) < 0)
					return -1;
				break;
			// case static_cast<std::uint32_t>(XAssetType::ADDON_MAP_ENTS):
			// 	if (extract_addon_map_ents(dest, dest_len, pos, outdir, csvfile) < 0)
			// 		return -1;
			// 	break;
			default:
				std::cout << "Skipping unknown asset type: " << XAssetTypeToString(type) << " (0x" << std::hex << type << std::dec << ")" << std::endl;
				break;
		}
	}

	csvfile.close();

	std::cout << "Extraction complete. Files: " << outdir << "/, CSV: " << csvpath << std::endl;

	std::string infile_stem = fs::path(infile).stem().string();
	std::string out_root = fs::path(outdir).filename().string();
	return 0;
}

void init_asset_handlers_impl()
{
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <file.ff|file.ffm> [outdir]" << std::endl;
		return 1;
	}
	std::string infile;
	std::string outdir;

	for (int i = 1; i < argc; ++i)
	{
		if (infile.empty())
			infile = argv[i];
		else if (outdir.empty())
			outdir = argv[i];
		else
		{
			std::cerr << "Unexpected argument: " << argv[i] << std::endl;
			return 1;
		}
	}

	if (infile.empty())
	{
		std::cerr << "Usage: " << argv[0] << "<file.ff|file.ffm> [outdir]" << std::endl;
		return 1;
	}

	if (outdir.empty())
	{
		fs::path p(infile);
		outdir = p.stem().string();
	}

	return unlink_fastfile(infile, outdir);
}
