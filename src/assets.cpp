#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unordered_set>

#include "assets.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include "compression.hpp"

namespace fs = std::filesystem;

bool read_string(const unsigned char* buf, size_t buf_len, size_t& pos, std::string& out)
{
    if (pos >= buf_len)
        return false;

    size_t start = pos;
    while (pos < buf_len && buf[pos] != '\0')
        pos++;

    if (pos >= buf_len)
        return false;

    out = std::string(reinterpret_cast<const char*>(buf + start), pos - start);
    pos++;

    return true;
}

std::unordered_set<std::string> g_emitted_localize_prefixes;

void ensure_parent_dirs(const fs::path& filepath)
{
    fs::path parent = filepath.parent_path();
    if (!parent.empty())
        fs::create_directories(parent);
}

std::string trim_and_lower(std::string s)
{
    auto is_space_char = [](unsigned char c){
        return std::isspace(c) || c == 0xA0;
    };
    size_t start = 0;
    while (start < s.size() && is_space_char(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && is_space_char(static_cast<unsigned char>(s[end - 1]))) end--;
    std::string out = s.substr(start, end - start);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string normalize_basename_for_compare(const std::string& path)
{
    std::string fname = fs::path(path).filename().string();
    std::string stem = fs::path(fname).stem().string();
    return trim_and_lower(stem);
}

std::string sanitize_for_print(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        if (c == 0xA0)
            out.push_back(' ');
        else if (c >= 32 && c < 127)
            out.push_back(static_cast<char>(c));
    }
    return out;
}

std::shared_ptr<Asset> assets;
AssetHandler asset_handlers[static_cast<int>(XAssetType::ASSETLIST)];

std::shared_ptr<Asset> new_xasset(XAssetType type, const std::string& name, const std::string& filename)
{
    auto asset = std::make_shared<Asset>();
    asset->type = type;
    asset->filename = filename;
    asset->name = name;
    asset->header = std::make_unique<XAssetHeader>();
    asset->next = nullptr;

    if (!assets)
    {
        assets = asset;
    }
    else
    {
        auto current = assets;
        while (current->next)
            current = current->next;
        current->next = asset;
    }

    return asset;
}

int asset_count()
{
    int count = 0;
    auto current = assets;
    while (current)
    {
        count++;
        current = current->next;
    }
    return count;
}

extern void init_asset_handlers_impl();

void init_asset_handlers()
{
    init_asset_handlers_impl();
}
