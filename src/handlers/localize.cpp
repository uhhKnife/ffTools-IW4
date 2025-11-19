#include "assets.hpp"
#include "assets_common.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

struct LocalizationEntry
{
    std::string key;
    std::string value;
};

static std::string unescape_string(const std::string& str)
{
    std::string result;
    for (size_t i = 0; i < str.length(); ++i)
    {
        if (str[i] == '\\' && i + 1 < str.length())
        {
            ++i;
            switch (str[i])
            {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                case '0': result += '\0'; break;
                default:
                    result += '\\';
                    result += str[i];
                    break;
            }
        }
        else
        {
            result += str[i];
        }
    }
    return result;
}

static std::string escape_string(const std::string& v)
{
    std::string out;
    for (char c : v)
    {
        switch (c)
        {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static std::vector<LocalizationEntry> parse_loc_file(const std::string& filename)
{
    std::vector<LocalizationEntry> entries;
    std::ifstream file(filename);
    
    if (!file.is_open())
        return entries;

    std::string line;
    std::string current_key;

    while (std::getline(file, line))
    {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
            continue;

        std::string trimmed = line.substr(start);

        if (trimmed.size() >= 9 && trimmed.substr(0, 9) == "REFERENCE")
        {
            size_t key_start = trimmed.find_first_not_of(" \t", 9);
            if (key_start != std::string::npos)
            {
                current_key = trimmed.substr(key_start);
                size_t key_end = current_key.find_last_not_of(" \t\r\n");
                if (key_end != std::string::npos)
                    current_key = current_key.substr(0, key_end + 1);
            }
        }
        else if (trimmed.size() >= 12 && trimmed.substr(0, 12) == "LANG_ENGLISH")
        {
            size_t val_start = trimmed.find('"');
            if (val_start == std::string::npos)
                continue;

            val_start++;
            size_t val_end = trimmed.rfind('"');
            if (val_end == std::string::npos || val_end <= val_start)
                continue;

            std::string raw_value = trimmed.substr(val_start, val_end - val_start);
            std::string value = unescape_string(raw_value);

            entries.push_back({current_key, value});
        }
    }

    file.close();
    return entries;
}

int load_localize_entry(XAssetType type, const std::string& basename, const std::string& path)
{
    std::string tmp = basename + "/english/localizedstrings/" + path + ".str";

    std::string prefix_str = fs::path(tmp).stem().string();
    util::strtoupper(prefix_str);

    std::cout << "Loading localize entry: " << prefix_str << std::endl;

    auto entries = parse_loc_file(tmp);
    if (entries.empty())
    {
        std::cerr << "Failed to parse localization file: " << tmp << std::endl;
        return 1;
    }

    for (auto& entry : entries)
    {
        std::string key = prefix_str + "_" + entry.key;
        std::cout << "  " << key << " = " << entry.value << std::endl;

        auto asset = new_xasset(type, "", path);
        auto loc_entry = new LocalizeEntry();
        asset->header->localize = loc_entry;
        
        loc_entry->name = new char[key.length() + 1];
        std::copy(key.begin(), key.end(), const_cast<char*>(loc_entry->name));
        const_cast<char*>(loc_entry->name)[key.length()] = '\0';
        
        loc_entry->value = new char[entry.value.length() + 1];
        std::copy(entry.value.begin(), entry.value.end(), const_cast<char*>(loc_entry->value));
        const_cast<char*>(loc_entry->value)[entry.value.length()] = '\0';
    }

    return 0;
}

int serialize_localize_entry(std::ofstream& fp, const XAssetHeader& asset)
{
    binary_io::write_be32(fp, 0xFFFFFFFF);
    binary_io::write_be32(fp, 0xFFFFFFFF);

    const LocalizeEntry* loc = asset.localize;
    binary_io::write_string(fp, loc->value);
    binary_io::write_string(fp, loc->name);

    return 0;
}

int extract_localize_entry(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    (void)outdir;
    (void)csvfile;

    if (pos + 8 > buf_len)
        return -1;
    
    pos += 8;

    std::string value, name;
    if (!read_string(buf, buf_len, pos, value)) return -1;
    if (!read_string(buf, buf_len, pos, name)) return -1;

    size_t us = name.find('_');
    std::string prefix;
    std::string key;
    if (us == std::string::npos)
    {
        prefix = "default";
        key = name;
    }
    else
    {
        prefix = name.substr(0, us);
        key = name.substr(us + 1);
    }

    std::string prefix_lower = prefix;
    std::transform(prefix_lower.begin(), prefix_lower.end(), prefix_lower.begin(), [](unsigned char c){ return std::tolower(c); });

    fs::path dir = fs::path(outdir) / "english" / "localizedstrings";
    fs::create_directories(dir);

    fs::path strpath = dir / (prefix_lower + ".str");

    {
        std::ofstream sf(strpath.string(), std::ios::app);
        if (!sf.is_open())
        {
            std::cerr << "Failed to open localize file for writing: " << strpath.string() << std::endl;
            return -1;
        }

    sf << "REFERENCE " << key << "\n";
    sf << "LANG_ENGLISH \"" << escape_string(value) << "\"\n";
        sf.close();
    }

    if (g_emitted_localize_prefixes.find(prefix_lower) == g_emitted_localize_prefixes.end())
    {
        g_emitted_localize_prefixes.insert(prefix_lower);
        std::string csv_name = std::string("english/localizedstrings/") + prefix_lower + ".str";
        for (char& c : csv_name)
            if (c == '\\') c = '/';
        csvfile << "localize," << prefix_lower << "\n";
    }

    std::cout << "Extracted Localize entry: " << prefix_lower << " -> " << key << std::endl;

    return 0;
}
