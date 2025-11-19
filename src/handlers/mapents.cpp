#include "assets.hpp"
#include "assets_common.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int load_map_ents(XAssetType type, const std::string& basename, const std::string& path)
{
    auto asset = new_xasset(type, "", path);
    auto me = new MapEnts();
    asset->header->mapents = me;

    std::string tmp = basename + "/" + path;
    std::string buffer = binary_io::read_file_to_memory(tmp);

    me->buffer = new char[buffer.length() + 1];
    std::copy(buffer.begin(), buffer.end(), const_cast<char*>(me->buffer));
    const_cast<char*>(me->buffer)[buffer.length()] = '\0';
    me->len = static_cast<int>(buffer.length());
    me->name = asset->filename.c_str();

    std::cout << "Loaded MapEnts: " << tmp << " (" << me->len << " bytes)" << std::endl;

    return 0;
}

int serialize_map_ents(std::ofstream& fp, const XAssetHeader& asset)
{
    const MapEnts* me = asset.mapents;

    std::uint32_t ptr = 0xFFFFFFFF;
    binary_io::write_be32(fp, ptr);
    binary_io::write_be32(fp, 0);
    binary_io::write_be32(fp, me->len);
    binary_io::write_be32(fp, ptr);

    binary_io::write_string(fp, me->name);
    fp.write(me->buffer, me->len);
    char null_byte = '\0';
    fp.write(&null_byte, 1);

    return 0;
}

int extract_map_ents(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    if (pos + 16 > buf_len)
    {
        std::cerr << "Truncated map_ents" << std::endl;
        return -1;
    }

    std::uint32_t ptr1 = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t compressedLen = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t content_len = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t ptr2 = util::read_be32(buf, pos);
    pos += 4;

    (void)ptr1;
    (void)compressedLen;
    (void)ptr2;

    std::string name;
    if (!read_string(buf, buf_len, pos, name))
    {
        std::cerr << "Failed to read map_ents name" << std::endl;
        return -1;
    }

    if (pos + content_len > buf_len)
    {
        std::cerr << "Truncated map_ents content" << std::endl;
        return -1;
    }

    std::string content(reinterpret_cast<const char*>(buf + pos), content_len);
    pos += content_len;

    std::string outpath = outdir + "/" + name;
    fs::path out_fs_path(outpath);

    ensure_parent_dirs(out_fs_path);

    std::ofstream outf(out_fs_path, std::ios::binary);
    if (!outf.is_open())
    {
        std::cerr << "Failed to open for writing: " << out_fs_path << std::endl;
        return -1;
    }

    outf.write(content.c_str(), content.size());
    outf.close();

    std::cout << "Extracted MapEnts: " << out_fs_path.string() << " (" << content.size() << " bytes)" << std::endl;

    std::string csv_name = name;
    for (char& c : csv_name)
        if (c == '\\')
            c = '/';
    csvfile << "map_ents," << csv_name << "\n";

    return 0;
}

// AddonMapEnts functions
int load_addon_map_ents(XAssetType type, const std::string& basename, const std::string& path)
{
    auto asset = new_xasset(type, "", path);
    auto ame = new AddonMapEnts();
    asset->header->addonmapents = ame;

    std::string tmp = basename + "/" + path;
    std::string buffer = binary_io::read_file_to_memory(tmp);

    ame->buffer = new char[buffer.length() + 1];
    std::copy(buffer.begin(), buffer.end(), const_cast<char*>(ame->buffer));
    const_cast<char*>(ame->buffer)[buffer.length()] = '\0';
    ame->len = static_cast<int>(buffer.length());
    ame->name = asset->filename.c_str();

    std::cout << "Loaded AddonMapEnts: " << tmp << " (" << ame->len << " bytes)" << std::endl;

    return 0;
}

int serialize_addon_map_ents(std::ofstream& fp, const XAssetHeader& asset)
{
    const AddonMapEnts* ame = asset.addonmapents;

    std::uint32_t ptr = 0xFFFFFFFF;
    binary_io::write_be32(fp, ptr);
    binary_io::write_be32(fp, 0);
    binary_io::write_be32(fp, ame->len);
    binary_io::write_be32(fp, ptr);

    binary_io::write_string(fp, ame->name);
    fp.write(ame->buffer, ame->len);
    char null_byte = '\0';
    fp.write(&null_byte, 1);

    return 0;
}

int extract_addon_map_ents(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    if (pos + 16 > buf_len)
    {
        std::cerr << "Truncated addon_map_ents" << std::endl;
        return -1;
    }

    std::uint32_t ptr1 = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t compressedLen = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t content_len = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t ptr2 = util::read_be32(buf, pos);
    pos += 4;

    (void)ptr1;
    (void)compressedLen;
    (void)ptr2;

    std::string name;
    if (!read_string(buf, buf_len, pos, name))
    {
        std::cerr << "Failed to read addon_map_ents name" << std::endl;
        return -1;
    }

    if (pos + content_len > buf_len)
    {
        std::cerr << "Truncated addon_map_ents content" << std::endl;
        return -1;
    }

    std::string content(reinterpret_cast<const char*>(buf + pos), content_len);
    pos += content_len;

    std::string outpath = outdir + "/" + name;
    fs::path out_fs_path(outpath);

    ensure_parent_dirs(out_fs_path);

    std::ofstream outf(out_fs_path, std::ios::binary);
    if (!outf.is_open())
    {
        std::cerr << "Failed to open for writing: " << out_fs_path << std::endl;
        return -1;
    }

    outf.write(content.c_str(), content.size());
    outf.close();

    std::cout << "Extracted AddonMapEnts: " << out_fs_path.string() << " (" << content.size() << " bytes)" << std::endl;

    std::string csv_name = name;
    for (char& c : csv_name)
        if (c == '\\')
            c = '/';
    csvfile << "addon_map_ents," << csv_name << "\n";

    return 0;
}
