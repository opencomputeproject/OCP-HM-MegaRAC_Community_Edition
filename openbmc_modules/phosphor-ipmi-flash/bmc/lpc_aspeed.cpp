/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lpc_aspeed.hpp"

#include "mapper_errors.hpp"
#include "window_hw_interface.hpp"

#include <fcntl.h>
#include <linux/aspeed-lpc-ctrl.h>
#include <linux/kernel.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace ipmi_flash
{

const std::string LpcMapperAspeed::lpcControlPath = "/dev/aspeed-lpc-ctrl";

std::unique_ptr<HardwareMapperInterface>
    LpcMapperAspeed::createAspeedMapper(std::uint32_t regionAddress,
                                        std::size_t regionSize)
{
    /* NOTE: considered using a joint factory to create one or the other, for
     * now, separate factories.
     */
    return std::make_unique<LpcMapperAspeed>(regionAddress, regionSize);
}

void LpcMapperAspeed::close()
{
    if (mappedRegion)
    {
        sys->munmap(mappedRegion, regionSize);
        mappedRegion = nullptr;
    }

    if (mappedFd != -1)
    {
        sys->close(mappedFd);
        mappedFd = -1;
    }
}

WindowMapResult LpcMapperAspeed::mapWindow(std::uint32_t address,
                                           std::uint32_t length)
{
    WindowMapResult result = {};
    static const std::uint32_t MASK_64K = 0xFFFFU;
    const std::uint32_t offset = address & MASK_64K;

    if (offset + length > regionSize)
    {
        std::fprintf(stderr,
                     "requested window size %" PRIu32 ", offset %#" PRIx32
                     " is too large for mem region"
                     " of size %zu\n",
                     length, offset, regionSize);

        result.response = EFBIG;
        result.windowSize = regionSize - offset;
        return result;
    }

    struct aspeed_lpc_ctrl_mapping map = {
        .window_type = ASPEED_LPC_CTRL_WINDOW_MEMORY,
        .window_id = 0,
        .flags = 0,
        .addr = address & ~MASK_64K,
        .offset = 0,
        .size = __ALIGN_KERNEL_MASK(offset + length, MASK_64K),
    };

    std::fprintf(stderr,
                 "requesting Aspeed LPC window at %#" PRIx32 " of size %" PRIu32
                 "\n",
                 map.addr, map.size);

    const auto lpcControlFd = sys->open(lpcControlPath.c_str(), O_RDWR);
    if (lpcControlFd == -1)
    {
        std::fprintf(stderr,
                     "cannot open Aspeed LPC kernel control dev \"%s\"\n",
                     lpcControlPath.c_str());

        result.response = EINVAL;
        return result;
    }

    if (sys->ioctl(lpcControlFd, ASPEED_LPC_CTRL_IOCTL_MAP, &map) == -1)
    {
        std::fprintf(stderr, "Failed to ioctl Aspeed LPC map with error %s\n",
                     std::strerror(errno));
        sys->close(lpcControlFd);

        result.response = EINVAL;
        return result;
    }

    sys->close(lpcControlFd);

    result.response = 0;
    result.windowOffset = offset;
    result.windowSize = length;
    return result;
}

MemorySet LpcMapperAspeed::open()
{
    if (mapRegion())
    {
        MemorySet output;
        output.mappedFd = mappedFd;
        output.mapped = mappedRegion;
        return output;
    }

    throw MapperException("Unable to memory-map region");
}

bool LpcMapperAspeed::mapRegion()
{
    /* Open the file to map. */
    mappedFd = sys->open(lpcControlPath.c_str(), O_RDONLY | O_SYNC);
    if (mappedFd == -1)
    {
        std::fprintf(stderr, "ipmiflash: unable to open %s\n",
                     lpcControlPath.c_str());
        return false;
    }

    mappedRegion = reinterpret_cast<uint8_t*>(
        sys->mmap(0, regionSize, PROT_READ, MAP_SHARED, mappedFd, 0));

    if (mappedRegion == MAP_FAILED)
    {
        sys->close(mappedFd);
        mappedFd = -1;
        std::fprintf(stderr, "Mmap failure: '%s'\n", std::strerror(errno));
        return false;
    }

    /* TOOD: There is no close() method here, to close mappedFd, or mappedRegion
     * -- therefore, a good next step will be to evaluate whether or not the
     * other pieces should go here...
     */
    return true;
}

} // namespace ipmi_flash
