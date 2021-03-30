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

#include "lpc_handler.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace ipmi_flash
{

bool LpcDataHandler::open()
{
    /* For the ASPEED LPC CTRL driver, the ioctl is required to set up the
     * window, with information from writeMeta() below.
     */
    return true;
}

bool LpcDataHandler::close()
{
    mapper->close();

    return setInitializedAndReturn(false);
}

std::vector<std::uint8_t> LpcDataHandler::copyFrom(std::uint32_t length)
{
    /* TODO: implement this -- in an earlier and different version of this that
     * didn't use BLOBs, the region was memory-mapped and the writes to the data
     * were just done directly from the memory-mapped region instead of a
     * copyFrom() first call.  The idea with this change is that we may not be
     * able to get a memory-mapped handle from the driver from which to
     * automatically read data, but rather must perform some ioctl or other
     * access to get the data from the driver.
     */
    if (!initialized)
    {
        /* TODO: Consider designing some exceptions we can catch for when there
         * is an error.
         */
        return {};
    }

    std::vector<std::uint8_t> results(length);
    std::memcpy(results.data(), memory.mapped + mappingResult.windowOffset,
                length);

    return results;
}

bool LpcDataHandler::writeMeta(const std::vector<std::uint8_t>& configuration)
{
    struct LpcRegion lpcRegion;

    if (configuration.size() != sizeof(lpcRegion))
    {
        return false;
    }

    std::memcpy(&lpcRegion, configuration.data(), configuration.size());

    /* TODO: LpcRegion sanity checking. */
    mappingResult = mapper->mapWindow(lpcRegion.address, lpcRegion.length);
    if (mappingResult.response != 0)
    {
        std::fprintf(stderr, "mappingResult.response %u\n",
                     mappingResult.response);
        /* Failed to map region. */
        return false;
    }

    return setInitializedAndReturn(true);
}

std::vector<std::uint8_t> LpcDataHandler::readMeta()
{
    /* Return the MemoryResult structure packed. */
    std::vector<std::uint8_t> output(
        sizeof(std::uint8_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t));

    int index = 0;
    std::memcpy(&output[index], &mappingResult.response,
                sizeof(mappingResult.response));

    index += sizeof(mappingResult.response);
    std::memcpy(&output[index], &mappingResult.windowOffset,
                sizeof(mappingResult.windowOffset));

    index += sizeof(mappingResult.windowOffset);
    std::memcpy(&output[index], &mappingResult.windowSize,
                sizeof(mappingResult.windowSize));

    return output;
}

} // namespace ipmi_flash
