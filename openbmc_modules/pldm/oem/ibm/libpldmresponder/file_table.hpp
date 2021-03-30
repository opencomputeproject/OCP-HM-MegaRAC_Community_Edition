#pragma once

#include "libpldm/pldm_types.h"

#include <stdint.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <vector>

namespace pldm
{

namespace filetable
{

namespace fs = std::filesystem;
using Handle = uint32_t;
using Json = nlohmann::json;
using Table = std::vector<uint8_t>;

/** @struct FileEntry
 *
 *  Data structure for storing information regarding the files supported by
 *  PLDM File I/O. The file handle is used to uniquely identify the file. The
 *  traits provide information whether the file is Read only, Read/Write and
 *  preserved across firmware upgrades.
 */
struct FileEntry
{
    Handle handle;       //!< File handle
    fs::path fsPath;     //!< File path
    bitfield32_t traits; //!< File traits
};

/** @class FileTable
 *
 *  FileTable class encapsulates the data related to files supported by PLDM
 *  File I/O and provides interfaces to lookup files information based on the
 *  file handle and extract the file attribute table. The file attribute table
 *  comprises of metadata for files. Metadata includes the file handle, file
 *  name, current file size and file traits.
 */
class FileTable
{
  public:
    /** @brief The file table is initialised by parsing the config file
     *         containing information about the files.
     *
     * @param[in] fileTableConfigPath - path to the json file containing
     *                                  information
     */
    FileTable(const std::string& fileTableConfigPath);
    FileTable() = default;
    ~FileTable() = default;
    FileTable(const FileTable&) = default;
    FileTable& operator=(const FileTable&) = default;
    FileTable(FileTable&&) = default;
    FileTable& operator=(FileTable&&) = default;

    /** @brief Get the file attribute table
     *
     * @return Table- contents of the file attribute table
     */
    Table operator()() const;

    /** @brief Get the FileEntry at the file handle
     *
     * @param[in] handle - file handle
     *
     * @return FileEntry - file entry at the handle
     */
    FileEntry at(Handle handle) const
    {
        return tableEntries.at(handle);
    }

    /** @brief Check is file attribute table is empty
     *
     * @return bool - true if file attribute table is empty, false otherwise.
     */
    bool isEmpty() const
    {
        return fileTable.empty();
    }

    /** @brief Clear the file table contents
     *
     */
    void clear()
    {
        tableEntries.clear();
        fileTable.clear();
        padCount = 0;
        checkSum = 0;
    }

  private:
    /** @brief handle to FileEntry mappings for lookups based on file handle */
    std::unordered_map<Handle, FileEntry> tableEntries;

    /** @brief file attribute table including the pad bytes, except the checksum
     */
    std::vector<uint8_t> fileTable;

    /** @brief the pad count of the file attribute table, the number of pad
     * bytes is between 0 and 3 */
    uint8_t padCount = 0;

    /** @brief the checksum of the file attribute table */
    uint32_t checkSum = 0;
};

/** @brief Build the file attribute table if not already built using the
 *         file table config.
 *
 *  @param[in] fileTablePath - path of the file table config
 *
 *  @return FileTable& - Reference to instance of file table
 */

FileTable& buildFileTable(const std::string& fileTablePath);

} // namespace filetable
} // namespace pldm
