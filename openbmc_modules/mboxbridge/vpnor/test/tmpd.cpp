// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#include "vpnor/test/tmpd.hpp"

namespace openpower
{
namespace virtual_pnor
{
namespace test
{

namespace fs = std::experimental::filesystem;

size_t VpnorRoot::write(const std::string& name, const void* data, size_t len)
{
    // write() is for test environment setup - always write to ro section
    fs::path path = root / "ro" / name;

    if (!fs::exists(path))
        /* It's not in the ToC */
        throw std::invalid_argument(name);

    std::ofstream(path).write((const char*)data, len);

    return len;
}

size_t VpnorRoot::patch(const std::string& name, const void* data, size_t len)
{
    if (!fs::exists(root / "ro" / name))
        /* It's not in the ToC */
        throw std::invalid_argument(name);

    std::ofstream(root / "patch" / name).write((const char*)data, len);

    return len;
}

} // namespace test
} // namespace virtual_pnor
} // namespace openpower
