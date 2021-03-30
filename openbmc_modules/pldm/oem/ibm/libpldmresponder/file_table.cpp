#include "file_table.hpp"

#include "libpldm/utils.h"

#include <fstream>
#include <iostream>

namespace pldm
{

namespace filetable
{

FileTable::FileTable(const std::string& fileTableConfigPath)
{
    std::ifstream jsonFile(fileTableConfigPath);
    if (!jsonFile.is_open())
    {
        std::cerr << "File table config file does not exist, FILE="
                  << fileTableConfigPath.c_str() << "\n";
        return;
    }

    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        std::cerr << "Parsing config file failed"
                  << "\n";
        return;
    }

    uint16_t fileNameLength = 0;
    uint32_t fileSize = 0;
    uint32_t traits = 0;
    size_t tableSize = 0;
    Handle handle = 0;
    auto iter = fileTable.begin();

    // Iterate through each JSON object in the config file
    for (const auto& record : data)
    {
        constexpr auto path = "path";
        constexpr auto fileTraits = "file_traits";

        std::string filepath = record.value(path, "");
        traits = static_cast<uint32_t>(record.value(fileTraits, 0));

        fs::path fsPath(filepath);
        if (!fs::is_regular_file(fsPath))
        {
            continue;
        }

        fileNameLength =
            static_cast<uint16_t>(fsPath.filename().string().size());
        fileSize = static_cast<uint32_t>(fs::file_size(fsPath));
        tableSize = fileTable.size();

        fileTable.resize(tableSize + sizeof(handle) + sizeof(fileNameLength) +
                         fileNameLength + sizeof(fileSize) + sizeof(traits));
        iter = fileTable.begin() + tableSize;

        // Populate the file table with the contents of the JSON entry
        std::copy_n(reinterpret_cast<uint8_t*>(&handle), sizeof(handle), iter);
        std::advance(iter, sizeof(handle));

        std::copy_n(reinterpret_cast<uint8_t*>(&fileNameLength),
                    sizeof(fileNameLength), iter);
        std::advance(iter, sizeof(fileNameLength));

        std::copy_n(reinterpret_cast<const uint8_t*>(fsPath.filename().c_str()),
                    fileNameLength, iter);
        std::advance(iter, fileNameLength);

        std::copy_n(reinterpret_cast<uint8_t*>(&fileSize), sizeof(fileSize),
                    iter);
        std::advance(iter, sizeof(fileSize));

        std::copy_n(reinterpret_cast<uint8_t*>(&traits), sizeof(traits), iter);
        std::advance(iter, sizeof(traits));

        // Create the file entry for the JSON entry
        FileEntry entry{};
        entry.handle = handle;
        entry.fsPath = std::move(fsPath);
        entry.traits.value = traits;

        // Insert the file entries in the map
        tableEntries.emplace(handle, std::move(entry));
        handle++;
    }

    constexpr uint8_t padWidth = 4;
    tableSize = fileTable.size();
    // Add pad bytes
    if ((tableSize % padWidth) != 0)
    {
        padCount = padWidth - (tableSize % padWidth);
        fileTable.resize(tableSize + padCount, 0);
    }

    // Calculate the checksum
    checkSum = crc32(fileTable.data(), fileTable.size());
}

Table FileTable::operator()() const
{
    Table table(fileTable);
    table.resize(fileTable.size() + sizeof(checkSum));
    auto iter = table.begin() + fileTable.size();
    std::copy_n(reinterpret_cast<const uint8_t*>(&checkSum), sizeof(checkSum),
                iter);
    return table;
}

FileTable& buildFileTable(const std::string& fileTablePath)
{
    static FileTable table;
    if (table.isEmpty())
    {
        table = std::move(FileTable(fileTablePath));
    }
    return table;
}

} // namespace filetable
} // namespace pldm
