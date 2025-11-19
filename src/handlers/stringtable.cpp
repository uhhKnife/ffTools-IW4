#include "assets.hpp"
#include "assets.hpp"
#include "util.hpp"
#include "binary_io.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

static std::vector<std::string> split_csv_line(const std::string& line, char delimiter = ',')
{
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.length(); i++)
    {
        char c = line[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == delimiter && !inQuotes)
        {
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }
    result.push_back(current);
    return result;
}

static int stringtable_hash(const char* string)
{
    int hash = 0;
    const char* data = string;

    while (*data != 0)
    {
        hash = tolower(*data) + (31 * hash);
        data++;
    }

    return hash;
}

int load_string_table(XAssetType type, const std::string& basename, const std::string& path)
{
    auto asset = new_xasset(type, "", path);
    auto st = new StringTable();
    asset->header->stringtable = st;

    std::string tmp = basename + "/" + path;
    std::ifstream file(tmp);
    if (!file.is_open())
    {
        std::cerr << "Failed to open stringtable file: " << tmp << std::endl;
        return 1;
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    int maxColumns = 0;

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        auto cells = split_csv_line(line, ',');
        rows.push_back(cells);
        
        if (static_cast<int>(cells.size()) > maxColumns)
            maxColumns = static_cast<int>(cells.size());
    }
    file.close();

    st->name = asset->filename.c_str();
    st->rowCount = static_cast<int>(rows.size());
    st->columnCount = maxColumns;

    int totalCells = st->rowCount * st->columnCount;
    st->values = new StringTableCell[totalCells];

    for (int row = 0; row < st->rowCount; row++)
    {
        for (int col = 0; col < st->columnCount; col++)
        {
            int cellIndex = (row * st->columnCount) + col;
            
            std::string cellValue;
            if (col < static_cast<int>(rows[row].size()))
                cellValue = rows[row][col];
            else
                cellValue = "";

            char* cellStr = new char[cellValue.length() + 1];
            std::copy(cellValue.begin(), cellValue.end(), cellStr);
            cellStr[cellValue.length()] = '\0';

            st->values[cellIndex].string = cellStr;
            st->values[cellIndex].hash = stringtable_hash(cellStr);
        }
    }

    std::cout << "Loaded StringTable: " << tmp << " (" << st->rowCount << " rows, " << st->columnCount << " columns)" << std::endl;

    return 0;
}

int serialize_string_table(std::ofstream& fp, const XAssetHeader& asset)
{
    const StringTable* st = asset.stringtable;

    std::uint32_t ptr = 0xFFFFFFFF;
    binary_io::write_be32(fp, ptr);
    binary_io::write_be32(fp, st->columnCount);
    binary_io::write_be32(fp, st->rowCount);
    binary_io::write_be32(fp, ptr);

    binary_io::write_string(fp, st->name);

    int totalCells = st->rowCount * st->columnCount;
    for (int i = 0; i < totalCells; i++)
    {
        binary_io::write_be32(fp, ptr);
        binary_io::write_be32(fp, st->values[i].hash);
    }

    for (int i = 0; i < totalCells; i++)
    {
        binary_io::write_string(fp, st->values[i].string);
    }

    return 0;
}

int extract_string_table(const unsigned char* buf, size_t buf_len, size_t& pos, const std::string& outdir, std::ofstream& csvfile)
{
    if (pos + 16 > buf_len)
    {
        std::cerr << "Truncated stringtable header" << std::endl;
        return -1;
    }

    std::uint32_t name_ptr = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t columnCount = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t rowCount = util::read_be32(buf, pos);
    pos += 4;
    std::uint32_t values_ptr = util::read_be32(buf, pos);
    pos += 4;

    (void)name_ptr;
    (void)values_ptr;

    std::string name;
    if (!read_string(buf, buf_len, pos, name))
    {
        std::cerr << "Failed to read stringtable name" << std::endl;
        return -1;
    }

    int totalCells = rowCount * columnCount;
    
    if (pos + (totalCells * 8) > buf_len)
    {
        std::cerr << "Truncated stringtable cells" << std::endl;
        return -1;
    }

    std::vector<std::uint32_t> string_ptrs(totalCells);
    std::vector<std::uint32_t> hashes(totalCells);

    for (int i = 0; i < totalCells; i++)
    {
        string_ptrs[i] = util::read_be32(buf, pos);
        pos += 4;
        hashes[i] = util::read_be32(buf, pos);
        pos += 4;
    }

    std::vector<std::string> cellStrings(totalCells);
    for (int i = 0; i < totalCells; i++)
    {
        if (!read_string(buf, buf_len, pos, cellStrings[i]))
        {
            std::cerr << "Failed to read stringtable cell string" << std::endl;
            return -1;
        }
    }

    std::string outpath = outdir + "/" + name;
    fs::path out_fs_path(outpath);

    std::string path_str = out_fs_path.string();
    for (char& c : path_str)
        if (c == '/')
            c = '\\';
    out_fs_path = fs::path(path_str);

    ensure_parent_dirs(out_fs_path);

    std::ofstream outf(out_fs_path);
    if (!outf.is_open())
    {
        std::cerr << "Failed to open for writing: " << out_fs_path << std::endl;
        return -1;
    }

    for (std::uint32_t row = 0; row < rowCount; row++)
    {
        for (std::uint32_t col = 0; col < columnCount; col++)
        {
            int cellIndex = (row * columnCount) + col;
            outf << cellStrings[cellIndex];
            
            if (col < columnCount - 1)
                outf << ",";
        }
        outf << "\n";
    }

    outf.close();

    std::cout << "Extracted StringTable: " << out_fs_path.string() << " (" << rowCount << " rows, " << columnCount << " columns)" << std::endl;

    std::string csv_name = name;
    for (char& c : csv_name)
        if (c == '\\')
            c = '/';
    csvfile << "stringtable," << csv_name << "\n";

    return 0;
}
