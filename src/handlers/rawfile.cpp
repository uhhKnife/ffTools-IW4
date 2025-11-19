#include "assets.hpp"
#include "assets.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include "compression.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int load_raw_file(XAssetType type, const std::string& basename, const std::string& path)
{
    auto asset = new_xasset(type, "", path);
    auto rf = new RawFile();
    asset->header->rawfile = rf;

    std::string tmp = basename + "/" + path;
    std::string buffer = binary_io::read_file_to_memory(tmp);

    rf->buffer = new char[buffer.length() + 1];
    std::copy(buffer.begin(), buffer.end(), const_cast<char*>(rf->buffer));
    const_cast<char*>(rf->buffer)[buffer.length()] = '\0';
    rf->len = static_cast<int>(buffer.length());
    rf->name = asset->filename.c_str();

    return 0;
}

int serialize_raw_file(std::ofstream& fp, const XAssetHeader& asset)
{
    const RawFile* rf = asset.rawfile;

    std::uint32_t ptr = 0xFFFFFFFF;
    binary_io::write_be32(fp, ptr);
    binary_io::write_be32(fp, 0);
    binary_io::write_be32(fp, rf->len);
    binary_io::write_be32(fp, ptr);

    binary_io::write_string(fp, rf->name);
    fp.write(rf->buffer, rf->len);
    char null_byte = '\0';
    fp.write(&null_byte, 1);

    return 0;
}

int extract_raw_file(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    if (pos + 16 > buf_len)
    {
        std::cerr << "Truncated rawfile" << std::endl;
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
    (void)ptr2;

    std::string name;
    if (!read_string(buf, buf_len, pos, name))
    {
        std::cerr << "Failed to read filename" << std::endl;
        return -1;
    }

    std::string sanitized_name = sanitize_for_print(name);
    std::string ffname_norm = normalize_basename_for_compare(outdir);
    std::string name_norm = normalize_basename_for_compare(sanitized_name);
    if (!ffname_norm.empty())
    {
        if (name_norm == ffname_norm || name_norm.find(ffname_norm) != std::string::npos || sanitized_name.find(ffname_norm) != std::string::npos)
        {
            std::cout << "Skipping auto-generated file: " << sanitized_name << std::endl;
            return 0;
        }
    }

    std::string content;
    if (compressedLen > 0)
    {
        if (pos + compressedLen > buf_len)
        {
            std::cerr << "Truncated compressed content for " << sanitized_name << std::endl;
            return -1;
        }
        auto decompressed = compression::decompress_data(buf + pos, compressedLen, static_cast<size_t>(content_len));
        if (decompressed.empty())
        {
            if (compressedLen == content_len)
            {
                if (pos + compressedLen > buf_len)
                {
                    std::cerr << "Truncated raw fallback for " << sanitized_name << std::endl;
                    return -1;
                }
                content.assign(reinterpret_cast<const char*>(buf + pos), compressedLen);
                pos += compressedLen;
            }
            else
            {
                std::cerr << "Failed to decompress content for " << sanitized_name << std::endl;
                return -1;
            }
        }
        else
        {
            content.assign(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
            pos += compressedLen;
        }
    }
    else
    {
        if (pos + content_len > buf_len)
        {
            std::cerr << "Truncated raw content for " << sanitized_name << std::endl;
            return -1;
        }
        content.assign(reinterpret_cast<const char*>(buf + pos), content_len);
        pos += content_len;
    }

    if (content.empty())
    {
        std::cout << "Skipping empty auto-generated file: " << sanitized_name << std::endl;
        return 0;
    }

    std::string outpath = outdir + "/" + sanitized_name;
    fs::path out_fs_path(outpath);

    std::string path_str = out_fs_path.string();
    for (char& c : path_str)
        if (c == '/')
            c = '\\';
    out_fs_path = fs::path(path_str);

    ensure_parent_dirs(out_fs_path);

    std::ofstream outf(out_fs_path, std::ios::binary);
    if (!outf.is_open())
    {
        std::cerr << "Failed to open for writing: " << out_fs_path << std::endl;
        return -1;
    }

    outf.write(content.c_str(), content.size());
    outf.close();

    std::cout << "Extracted: " << out_fs_path.string() << " (" << content.size() << " bytes)" << std::endl;

    std::string csv_name = sanitized_name;
    for (char& c : csv_name)
        if (c == '\\')
            c = '/';
    csvfile << "rawfile," << csv_name << "\n";

    return 0;
}
