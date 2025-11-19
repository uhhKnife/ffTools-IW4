#include "assets.hpp"
#include "assets_common.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

int load_phys_preset(XAssetType type, const std::string& basename, const std::string& path)
{
    auto asset = new_xasset(type, "", path);
    auto pp = new PhysPreset();
    asset->header->physpreset = pp;

    std::string tmp = basename + "/" + path;
    std::string buffer = binary_io::read_file_to_memory(tmp);

    pp->buffer = new char[buffer.length() + 1];
    std::copy(buffer.begin(), buffer.end(), const_cast<char*>(pp->buffer));
    const_cast<char*>(pp->buffer)[buffer.length()] = '\0';
    pp->len = static_cast<int>(buffer.length());
    pp->name = asset->filename.c_str();

    std::cout << "Loaded PhysPreset: " << tmp << " (" << pp->len << " bytes)" << std::endl;

    return 0;
}

int serialize_phys_preset(std::ofstream& fp, const XAssetHeader& asset)
{
    const PhysPreset* pp = asset.physpreset;

    std::uint32_t ptr = 0xFFFFFFFF;
    binary_io::write_be32(fp, ptr);
    binary_io::write_be32(fp, 0);
    binary_io::write_be32(fp, pp->len);
    binary_io::write_be32(fp, ptr);

    binary_io::write_string(fp, pp->name);
    fp.write(pp->buffer, pp->len);
    char null_byte = '\0';
    fp.write(&null_byte, 1);

    return 0;
}

int extract_phys_preset(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    if (pos + 16 > buf_len)
    {
        std::cerr << "Truncated physpreset" << std::endl;
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
        std::cerr << "Failed to read physpreset name" << std::endl;
        return -1;
    }

    // TODO: Determine proper content parsing for physics presets
    // For now, treat as binary blob with fixed length
    if (pos + content_len > buf_len)
    {
        std::cerr << "Truncated physpreset content" << std::endl;
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

    std::cout << "Extracted PhysPreset: " << out_fs_path.string() << " (" << content.size() << " bytes)" << std::endl;

    std::string csv_name = name;
    for (char& c : csv_name)
        if (c == '\\')
            c = '/';
    csvfile << "physpreset," << csv_name << "\n";

    return 0;
}
