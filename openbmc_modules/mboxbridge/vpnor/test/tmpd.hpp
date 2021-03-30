/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#include "config.h"

extern "C" {
#include "backend.h"
#include "mboxd.h"
}

#include "vpnor/table.hpp"

#include <cassert>
#include <cstring>
#include <experimental/filesystem>
#include <fstream>
#include <vector>

namespace openpower
{
namespace virtual_pnor
{
namespace test
{

namespace fs = std::experimental::filesystem;

class VpnorRoot
{
  public:
    template <std::size_t N>
    VpnorRoot(struct backend* backend, const std::string (&toc)[N],
              size_t blockSize) :
        backend(backend)
    {
        char tmplt[] = "/tmp/vpnor_root.XXXXXX";
        char* tmpdir = mkdtemp(tmplt);
        root = fs::path{tmpdir};

        for (const auto& attr : attributes)
        {
            fs::create_directory(root / attr);
        }

        fs::path tocFilePath = root / "ro" / PARTITION_TOC_FILE;

        for (const std::string& line : toc)
        {
            pnor_partition part;

            openpower::virtual_pnor::parseTocLine(line, blockSize, part);

            /* Populate the partition in the tree */
            std::vector<char> zeroed(part.data.actual, 0);
            fs::path partitionFilePath = root / "ro" / part.data.name;
            std::ofstream(partitionFilePath)
                .write(zeroed.data(), zeroed.size());

            /* Update the ToC if the partition file was created */
            std::ofstream(tocFilePath, std::ofstream::app) << line << "\n";
        }

        vpnor_partition_paths paths{};

        snprintf(paths.ro_loc, PATH_MAX - 1, "%s/ro", root.c_str());
        paths.ro_loc[PATH_MAX - 1] = '\0';
        snprintf(paths.rw_loc, PATH_MAX - 1, "%s/rw", root.c_str());
        paths.rw_loc[PATH_MAX - 1] = '\0';
        snprintf(paths.prsv_loc, PATH_MAX - 1, "%s/prsv", root.c_str());
        paths.prsv_loc[PATH_MAX - 1] = '\0';
        snprintf(paths.patch_loc, PATH_MAX - 1, "%s/patch", root.c_str());
        paths.patch_loc[PATH_MAX - 1] = '\0';

        if (backend_probe_vpnor(backend, &paths))
        {
            throw std::system_error(errno, std::system_category());
        }
    }

    VpnorRoot(const VpnorRoot&) = delete;
    VpnorRoot& operator=(const VpnorRoot&) = delete;
    VpnorRoot(VpnorRoot&&) = delete;
    VpnorRoot& operator=(VpnorRoot&&) = delete;

    ~VpnorRoot()
    {
        backend_free(backend);
        fs::remove_all(root);
    }
    fs::path ro()
    {
        return fs::path{root} / "ro";
    }
    fs::path rw()
    {
        return fs::path{root} / "rw";
    }
    fs::path prsv()
    {
        return fs::path{root} / "prsv";
    }
    fs::path patch()
    {
        return fs::path{root} / "patch";
    }
    size_t write(const std::string& name, const void* data, size_t len);
    size_t patch(const std::string& name, const void* data, size_t len);

  private:
    struct backend* backend;
    fs::path root;
    const std::string attributes[4] = {"ro", "rw", "prsv", "patch"};
};

} // namespace test
} // namespace virtual_pnor
} // namespace openpower
