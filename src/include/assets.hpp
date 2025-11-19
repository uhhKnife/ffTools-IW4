#pragma once

#include <string>
#include <memory>
#include <fstream>
#include "types.hpp"

struct Asset
{
	std::string filename;
	std::string name;
	XAssetType type = XAssetType::ASSETLIST;
	std::unique_ptr<XAssetHeader> header;
	std::shared_ptr<Asset> next;
};

typedef int(*AssetLoadHandler)(XAssetType type, const std::string& basename, const std::string& path);
typedef int(*AssetSerializeHandler)(std::ofstream& fp, const XAssetHeader& asset);
typedef int(*AssetExtractHandler)(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

struct AssetHandler
{
	AssetLoadHandler load;
	AssetSerializeHandler serialize;
	AssetExtractHandler extract;
};

extern std::shared_ptr<Asset> assets;
extern AssetHandler asset_handlers[static_cast<int>(XAssetType::ASSETLIST)];

void init_asset_handlers();
std::shared_ptr<Asset> new_xasset(XAssetType type, const std::string& name, const std::string& filename);
int asset_count();

int load_localize_entry(XAssetType type, const std::string& basename, const std::string& path);
int serialize_localize_entry(std::ofstream& fp, const XAssetHeader& asset);
int extract_localize_entry(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

int load_raw_file(XAssetType type, const std::string& basename, const std::string& path);
int serialize_raw_file(std::ofstream& fp, const XAssetHeader& asset);
int extract_raw_file(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

int load_phys_preset(XAssetType type, const std::string& basename, const std::string& path);
int serialize_phys_preset(std::ofstream& fp, const XAssetHeader& asset);
int extract_phys_preset(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

int load_map_ents(XAssetType type, const std::string& basename, const std::string& path);
int serialize_map_ents(std::ofstream& fp, const XAssetHeader& asset);
int extract_map_ents(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

int load_string_table(XAssetType type, const std::string& basename, const std::string& path);
int serialize_string_table(std::ofstream& fp, const XAssetHeader& asset);
int extract_string_table(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);

int load_addon_map_ents(XAssetType type, const std::string& basename, const std::string& path);
int serialize_addon_map_ents(std::ofstream& fp, const XAssetHeader& asset);
int extract_addon_map_ents(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile);
