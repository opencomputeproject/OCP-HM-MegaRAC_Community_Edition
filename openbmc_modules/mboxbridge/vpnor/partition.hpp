/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */
#pragma once

extern "C" {
#include "backend.h"
#include "vpnor/backend.h"
};

#include "vpnor/table.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <experimental/filesystem>
#include <string>

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
     *  @param[in] backend - The backend context used to process the request
     *  @param[in] offset - The absolute offset into the flash device as
     *                      provided by the mbox message associated with the
     *                      request
     *
     *  The class does not take ownership of the ctx pointer. The lifetime of
     *  the ctx pointer must strictly exceed the lifetime of the class
     *  instance.
     */
    Request(struct backend* backend, size_t offset) :
        backend(backend), partition(((struct vpnor_data*)backend->priv)
                                        ->vpnor->table->partition(offset)),
        base(partition.data.base << backend->block_size_shift),
        offset(offset - base)
    {
    }
    Request(const Request&) = delete;
    Request& operator=(const Request&) = delete;
    Request(Request&&) = default;
    Request& operator=(Request&&) = default;
    ~Request() = default;

    ssize_t read(void* dst, size_t len);
    ssize_t write(void* dst, size_t len);

  private:
    /** @brief Clamp the access length to the maximum supported by the ToC */
    size_t clamp(size_t len);

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

    struct backend* backend;
    const pnor_partition& partition;
    size_t base;
    size_t offset;
};

} // namespace virtual_pnor
} // namespace openpower
