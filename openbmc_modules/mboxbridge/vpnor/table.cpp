// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#include "config.h"

extern "C" {
#include "backend.h"
#include "common.h"
#include "mboxd.h"
}

#include "vpnor/table.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <endian.h>
#include <syslog.h>

#include <algorithm>
#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <regex>

namespace openpower
{
namespace virtual_pnor
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

namespace partition
{

Table::Table(const struct backend* be) :
    szBytes(sizeof(pnor_partition_table)), numParts(0),
    blockSize(1 << be->erase_size_shift), pnorSize(be->flash_size)
{
    preparePartitions((const struct vpnor_data*)be->priv);
    prepareHeader();
    hostTbl = endianFixup(tbl);
}

void Table::prepareHeader()
{
    decltype(auto) table = getNativeTable();
    table.data.magic = PARTITION_HEADER_MAGIC;
    table.data.version = PARTITION_VERSION_1;
    table.data.size = blocks();
    table.data.entry_size = sizeof(pnor_partition);
    table.data.entry_count = numParts;
    table.data.block_size = blockSize;
    table.data.block_count = pnorSize / blockSize;
    table.checksum = details::checksum(table.data);
}

inline void Table::allocateMemory(const fs::path& tocFile)
{
    size_t num = 0;
    std::string line;
    std::ifstream file(tocFile.c_str());

    // Find number of lines in partition file - this will help
    // determine the number of partitions and hence also how much
    // memory to allocate for the partitions array.
    // The actual number of partitions may turn out to be lesser than this,
    // in case of errors.
    while (std::getline(file, line))
    {
        // Check if line starts with "partition"
        if (std::string::npos != line.find("partition", 0))
        {
            ++num;
        }
    }

    szBytes = sizeof(pnor_partition_table) + (num * sizeof(pnor_partition));
    tbl.resize(capacity());
}

void Table::preparePartitions(const struct vpnor_data* priv)
{
    const fs::path roDir(priv->paths.ro_loc);
    const fs::path patchDir(priv->paths.patch_loc);
    fs::path tocFile = roDir / PARTITION_TOC_FILE;
    allocateMemory(tocFile);

    std::ifstream file(tocFile.c_str());
    std::string line;
    decltype(auto) table = getNativeTable();

    while (std::getline(file, line))
    {
        pnor_partition& part = table.partitions[numParts];
        fs::path patch;
        fs::path file;

        // The ToC file presented in the vpnor squashfs looks like:
        //
        // version=IBM-witherspoon-ibm-OP9_v1.19_1.135
        // extended_version=op-build-v1.19-571-g04f4690-dirty,buildroot-2017.11-5-g65679be,skiboot-v5.10-rc4,hostboot-4c46d66,linux-4.14.20-openpower1-p4a6b675,petitboot-v1.6.6-pe5aaec2,machine-xml-0fea226,occ-3286b6b,hostboot-binaries-3d1af8f,capp-ucode-p9-dd2-v3,sbe-99e2fe2
        // partition00=part,0x00000000,0x00002000,00,READWRITE
        // partition01=HBEL,0x00008000,0x0002c000,00,ECC,REPROVISION,CLEARECC,READWRITE
        // ...
        //
        // As such we want to skip any lines that don't begin with 'partition'
        if (std::string::npos == line.find("partition", 0))
        {
            continue;
        }

        parseTocLine(line, blockSize, part);

        if (numParts > 0)
        {
            struct pnor_partition& prev = table.partitions[numParts - 1];
            uint32_t prev_end = prev.data.base + prev.data.size;

            if (part.data.id == prev.data.id)
            {
                MSG_ERR("ID for previous partition '%s' at block 0x%" PRIx32
                        "matches current partition '%s' at block 0x%" PRIx32
                        ": %" PRId32 "\n",
                        prev.data.name, prev.data.base, part.data.name,
                        part.data.base, part.data.id);
            }

            if (part.data.base < prev_end)
            {
                std::stringstream err;
                err << "Partition '" << part.data.name << "' start block 0x"
                    << std::hex << part.data.base << "is less than the end "
                    << "block 0x" << std::hex << prev_end << " of '"
                    << prev.data.name << "'";
                throw InvalidTocEntry(err.str());
            }
        }

        file = roDir / part.data.name;
        if (!fs::exists(file))
        {
            std::stringstream err;
            err << "Partition file " << file.native() << " does not exist";
            throw InvalidTocEntry(err.str());
        }

        patch = patchDir / part.data.name;
        if (fs::is_regular_file(patch))
        {
            const size_t size = part.data.size * blockSize;
            part.data.actual =
                std::min(size, static_cast<size_t>(fs::file_size(patch)));
        }

        ++numParts;
    }
}

const pnor_partition& Table::partition(size_t offset) const
{
    const decltype(auto) table = getNativeTable();
    size_t blockOffset = offset / blockSize;

    for (decltype(numParts) i{}; i < numParts; ++i)
    {
        const struct pnor_partition& part = table.partitions[i];
        size_t len = part.data.size;

        if ((blockOffset >= part.data.base) &&
            (blockOffset < (part.data.base + len)))
        {
            return part;
        }

        /* Are we in a hole between partitions? */
        if (blockOffset < part.data.base)
        {
            throw UnmappedOffset(offset, part.data.base * blockSize);
        }
    }

    throw UnmappedOffset(offset, pnorSize);
}

const pnor_partition& Table::partition(const std::string& name) const
{
    const decltype(auto) table = getNativeTable();

    for (decltype(numParts) i{}; i < numParts; ++i)
    {
        if (name == table.partitions[i].data.name)
        {
            return table.partitions[i];
        }
    }

    std::stringstream err;
    err << "Partition " << name << " is not listed in the table of contents";
    throw UnknownPartition(err.str());
}

} // namespace partition

PartitionTable endianFixup(const PartitionTable& in)
{
    PartitionTable out;
    out.resize(in.size());
    auto src = reinterpret_cast<const pnor_partition_table*>(in.data());
    auto dst = reinterpret_cast<pnor_partition_table*>(out.data());

    dst->data.magic = htobe32(src->data.magic);
    dst->data.version = htobe32(src->data.version);
    dst->data.size = htobe32(src->data.size);
    dst->data.entry_size = htobe32(src->data.entry_size);
    dst->data.entry_count = htobe32(src->data.entry_count);
    dst->data.block_size = htobe32(src->data.block_size);
    dst->data.block_count = htobe32(src->data.block_count);
    dst->checksum = details::checksum(dst->data);

    for (decltype(src->data.entry_count) i{}; i < src->data.entry_count; ++i)
    {
        auto psrc = &src->partitions[i];
        auto pdst = &dst->partitions[i];
        strncpy(pdst->data.name, psrc->data.name, PARTITION_NAME_MAX);
        // Just to be safe
        pdst->data.name[PARTITION_NAME_MAX] = '\0';
        pdst->data.base = htobe32(psrc->data.base);
        pdst->data.size = htobe32(psrc->data.size);
        pdst->data.pid = htobe32(psrc->data.pid);
        pdst->data.id = htobe32(psrc->data.id);
        pdst->data.type = htobe32(psrc->data.type);
        pdst->data.flags = htobe32(psrc->data.flags);
        pdst->data.actual = htobe32(psrc->data.actual);
        for (size_t j = 0; j < PARTITION_USER_WORDS; ++j)
        {
            pdst->data.user.data[j] = htobe32(psrc->data.user.data[j]);
        }
        pdst->checksum = details::checksum(pdst->data);
    }

    return out;
}

static inline void writeSizes(pnor_partition& part, size_t start, size_t end,
                              size_t blockSize)
{
    size_t size = end - start;
    part.data.base = align_up(start, blockSize) / blockSize;
    size_t sizeInBlocks = align_up(size, blockSize) / blockSize;
    part.data.size = sizeInBlocks;
    part.data.actual = size;
}

static inline void writeUserdata(pnor_partition& part, uint32_t version,
                                 const std::string& data)
{
    std::istringstream stream(data);
    std::string flag{};
    auto perms = 0;
    auto state = 0;

    MSG_DBG("Parsing ToC flags '%s'\n", data.c_str());
    while (std::getline(stream, flag, ','))
    {
        if (flag == "")
            continue;

        if (flag == "ECC")
        {
            state |= PARTITION_ECC_PROTECTED;
        }
        else if (flag == "READONLY")
        {
            perms |= PARTITION_READONLY;
        }
        else if (flag == "READWRITE")
        {
            perms &= ~PARTITION_READONLY;
        }
        else if (flag == "PRESERVED")
        {
            perms |= PARTITION_PRESERVED;
        }
        else if (flag == "REPROVISION")
        {
            perms |= PARTITION_REPROVISION;
        }
        else if (flag == "VOLATILE")
        {
            perms |= PARTITION_VOLATILE;
        }
        else if (flag == "CLEARECC")
        {
            perms |= PARTITION_CLEARECC;
        }
        else
        {
            MSG_INFO("Found unimplemented partition property: %s\n",
                     flag.c_str());
        }
    }

    // Awful hack: Detect the TOC partition and force it read-only.
    //
    // Note that as it stands in the caller code we populate the critical
    // elements before the user data. These tests make doing so a requirement.
    if (part.data.id == 0 && !part.data.base && part.data.size)
    {
        perms |= PARTITION_READONLY;
    }

    part.data.user.data[0] = state;
    part.data.user.data[1] = perms;
    part.data.user.data[1] |= version;
}

static inline void writeDefaults(pnor_partition& part)
{
    part.data.pid = PARENT_PATITION_ID;
    part.data.type = PARTITION_TYPE_DATA;
    part.data.flags = 0; // flags unused
}

static inline void writeNameAndId(pnor_partition& part, std::string&& name,
                                  const std::string& id)
{
    name.resize(PARTITION_NAME_MAX);
    memcpy(part.data.name, name.c_str(), sizeof(part.data.name));
    part.data.id = std::stoul(id);
}

void parseTocLine(const std::string& line, size_t blockSize,
                  pnor_partition& part)
{
    static constexpr auto ID_MATCH = 1;
    static constexpr auto NAME_MATCH = 2;
    static constexpr auto START_ADDR_MATCH = 4;
    static constexpr auto END_ADDR_MATCH = 6;
    static constexpr auto VERSION_MATCH = 8;
    constexpr auto versionShift = 24;

    // Parse PNOR toc (table of contents) file, which has lines like :
    // partition01=HBB,0x00010000,0x000a0000,0x80,ECC,PRESERVED, to indicate
    // partition information
    std::regex regex{
        "^partition([0-9]+)=([A-Za-z0-9_]+),"
        "(0x)?([0-9a-fA-F]+),(0x)?([0-9a-fA-F]+),(0x)?([A-Fa-f0-9]{2})",
        std::regex::extended};

    std::smatch match;
    if (!std::regex_search(line, match, regex))
    {
        std::stringstream err;
        err << "Malformed partition description: " << line.c_str() << "\n";
        throw MalformedTocEntry(err.str());
    }

    writeNameAndId(part, match[NAME_MATCH].str(), match[ID_MATCH].str());
    writeDefaults(part);

    unsigned long start =
        std::stoul(match[START_ADDR_MATCH].str(), nullptr, 16);
    if (start & (blockSize - 1))
    {
        MSG_ERR("Start offset 0x%lx for partition '%s' is not aligned to block "
                "size 0x%zx\n",
                start, match[NAME_MATCH].str().c_str(), blockSize);
    }

    unsigned long end = std::stoul(match[END_ADDR_MATCH].str(), nullptr, 16);
    if ((end - start) & (blockSize - 1))
    {
        MSG_ERR("Partition '%s' has a size 0x%lx that is not aligned to block "
                "size 0x%zx\n",
                match[NAME_MATCH].str().c_str(), (end - start), blockSize);
    }

    if (start >= end)
    {
        std::stringstream err;
        err << "Partition " << match[NAME_MATCH].str()
            << " has an invalid range: start offset (0x" << std::hex << start
            << " is beyond open end (0x" << std::hex << end << ")\n";
        throw InvalidTocEntry(err.str());
    }
    writeSizes(part, start, end, blockSize);

    // Use the shift to convert "80" to 0x80000000
    unsigned long version = std::stoul(match[VERSION_MATCH].str(), nullptr, 16);
    // Note that we must have written the partition ID and sizes prior to
    // populating the userdata. See the note about awful hacks in
    // writeUserdata()
    writeUserdata(part, version << versionShift, match.suffix().str());
    part.checksum = details::checksum(part.data);
}

} // namespace virtual_pnor
} // namespace openpower
