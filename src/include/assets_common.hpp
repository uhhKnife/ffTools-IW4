#pragma once

#include "assets.hpp"
#include <string>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

// Utility helpers used across handlers
bool read_string(const unsigned char* buf, size_t buf_len, size_t& pos, std::string& out);
extern std::unordered_set<std::string> g_emitted_localize_prefixes;
void ensure_parent_dirs(const fs::path& filepath);
std::string trim_and_lower(std::string s);
std::string normalize_basename_for_compare(const std::string& path);
std::string sanitize_for_print(const std::string& s);

// Asset globals
extern std::shared_ptr<Asset> assets;
extern AssetHandler asset_handlers[static_cast<int>(XAssetType::ASSETLIST)];

std::shared_ptr<Asset> new_xasset(XAssetType type, const std::string& name, const std::string& filename);
int asset_count();

void init_asset_handlers();
