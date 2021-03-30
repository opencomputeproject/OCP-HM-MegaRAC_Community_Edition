/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */
#pragma once

extern "C" {
#include "mbox.h"
};

#include "mboxd_pnor_partition_table.h"
#include "pnor_partition_table.hpp"
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <experimental/filesystem>

namespace openpower
{
namespace virtual_pnor
{

namespace fs = std::experimental::filesystem;

class Request
{
  public:
    /** @brief Construct a flash access request
     *
     *  @param[in] ctx - The mbox context used to process the request
     *  @param[in] offset - The absolute offset into the flash device as
     *                      provided by the mbox message associated with the
     *                      request
     *
     *  The class does not take ownership of the ctx pointer. The lifetime of
     *  the ctx pointer must strictly exceed the lifetime of the class
     *  instance.
     */
    Request(struct mbox_context* ctx, size_t offset) :
        ctx(ctx), partition(ctx->vpnor->table->partition(offset)),
        base(partition.data.base << ctx->block_size_shift),
        offset(offset - base)
    {
    }
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;
    Request(Request&&) = default;
    Request& operator=(Request&&) = default;
    ~Request() = default;

    ssize_t read(void* dst, size_t len)
    {
        len = clamp(len);
        constexpr auto flags = O_RDONLY;
        fs::path path = getPartitionFilePath(flags);
        return fulfil(path, flags, dst, len);
    }

    ssize_t write(void* dst, size_t len)
    {
        if (len != clamp(len))
        {
            std::stringstream err;
            err << "Request size 0x" << std::hex << len << " from offset 0x"
                << std::hex << offset << " exceeds the partition size 0x"
                << std::hex << (partition.data.size << ctx->block_size_shift);
            throw OutOfBoundsOffset(err.str());
        }
        constexpr auto flags = O_RDWR;
        /* Ensure file is at least the size of the maximum access */
        fs::path path = getPartitionFilePath(flags);
        resize(path, len);
        return fulfil(path, flags, dst, len);
    }

  private:
    /** @brief Clamp the access length to the maximum supported by the ToC */
    size_t clamp(size_t len);

    /** @brief Ensure the backing file is sized appropriately for the access
     *
     *  We need to ensure the file is big enough to satisfy the request so that
     *  mmap() will succeed for the required size.
     *
     *  @return The valid access length
     *
     *  Throws: std::system_error
     */
    void resize(const std::experimental::filesystem::path& path, size_t len);

    /** @brief Returns the partition file path associated with the offset.
     *
     *  The search strategy for the partition file depends on the value of the
     *  flags parameter.
     *
     *  For the O_RDONLY case:
     *
     *  1.  Depending on the partition type,tries to open the file
     *      from the associated partition(RW/PRSV/RO).
     *  1a. if file not found in the corresponding
     *      partition(RW/PRSV/RO) then tries to read the file from
     *      the read only partition.
     *  1b. if the file not found in the read only partition then
     *      throw exception.
     *
     * For the O_RDWR case:
     *
     *  1.  Depending on the partition type tries to open the file
     *      from the associated partition.
     *  1a. if file not found in the corresponding partition(RW/PRSV)
     *      then copy the file from the read only partition to the (RW/PRSV)
     *      partition depending on the partition type.
     *  1b. if the file not found in the read only partition then throw
     *      exception.
     *
     *  @param[in] flags - The flags that will be used to open the file. Must
     *                     be one of O_RDONLY or O_RDWR.
     *
     *  Post-condition: The file described by the returned path exists
     *
     *  Throws: std::filesystem_error, std::bad_alloc
     */
    std::experimental::filesystem::path getPartitionFilePath(int flags);

    /** @brief Fill dst with the content of the partition relative to offset.
     *
     *  @param[in] offset - The pnor offset(bytes).
     *  @param[out] dst - The buffer to fill with partition data
     *  @param[in] len - The length of the destination buffer
     */
    size_t fulfil(const std::experimental::filesystem::path& path, int flags,
                  void* dst, size_t len);

    struct mbox_context* ctx;
    const pnor_partition& partition;
    size_t base;
    size_t offset;
};

} // namespace virtual_pnor
} // namespace openpower
