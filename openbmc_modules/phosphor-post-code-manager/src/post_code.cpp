/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include "post_code.hpp"

#include "iomanip"

void PostCode::deleteAll()
{
    auto dir = fs::path(PostCodeListPath);
    std::uintmax_t n = fs::remove_all(dir);
    std::cerr << "clearPostCodes deleted " << n << " files in "
              << PostCodeListPath << std::endl;
    fs::create_directories(dir);
    postCodes.clear();
    currentBootCycleIndex = 1;
    currentBootCycleCount(1);
}

std::vector<uint64_t> PostCode::getPostCodes(uint16_t index)
{
    std::vector<uint64_t> codesVec;
    if (1 == index && !postCodes.empty())
    {
        for (auto& code : postCodes)
            codesVec.push_back(code.second);
    }
    else
    {
        uint16_t bootNum = getBootNum(index);

        decltype(postCodes) codes;
        deserializePostCodes(
            fs::path(strPostCodeListPath + std::to_string(bootNum)), codes);
        for (std::pair<uint64_t, uint64_t> code : codes)
            codesVec.push_back(code.second);
    }
    return codesVec;
}

std::map<uint64_t, uint64_t> PostCode::getPostCodesWithTimeStamp(uint16_t index)
{
    if (1 == index && !postCodes.empty())
    {
        return postCodes;
    }

    uint16_t bootNum = getBootNum(index);
    decltype(postCodes) codes;
    deserializePostCodes(
        fs::path(strPostCodeListPath + std::to_string(bootNum)), codes);
    return codes;
}

void PostCode::savePostCodes(uint64_t code)
{
    // steady_clock is a monotonic clock that is guaranteed to never be adjusted
    auto postCodeTimeSteady = std::chrono::steady_clock::now();
    uint64_t tsUS = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();

    if (postCodes.empty())
    {
        firstPostCodeTimeSteady = postCodeTimeSteady;
        firstPostCodeUsSinceEpoch = tsUS; // uS since epoch for 1st post code
        incrBootCycle();
    }
    else
    {
        // calculating tsUS so it is monotonic within the same boot
        tsUS = firstPostCodeUsSinceEpoch +
               std::chrono::duration_cast<std::chrono::microseconds>(
                   postCodeTimeSteady - firstPostCodeTimeSteady)
                   .count();
    }

    postCodes.insert(std::make_pair(tsUS, code));
    serialize(fs::path(PostCodeListPath));

    return;
}

fs::path PostCode::serialize(const std::string& path)
{
    try
    {
        fs::path idxPath(path + strCurrentBootCycleIndexName);
        std::ofstream osIdx(idxPath.c_str(), std::ios::binary);
        cereal::JSONOutputArchive idxArchive(osIdx);
        idxArchive(currentBootCycleIndex);

        uint16_t count = currentBootCycleCount();
        fs::path cntPath(path + strCurrentBootCycleCountName);
        std::ofstream osCnt(cntPath.c_str(), std::ios::binary);
        cereal::JSONOutputArchive cntArchive(osCnt);
        cntArchive(count);

        std::ofstream osPostCodes(
            (path + std::to_string(currentBootCycleIndex)));
        cereal::JSONOutputArchive oarchivePostCodes(osPostCodes);
        oarchivePostCodes(postCodes);
    }
    catch (cereal::Exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return "";
    }
    catch (const fs::filesystem_error& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return "";
    }
    return path;
}

bool PostCode::deserialize(const fs::path& path, uint16_t& index)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(index);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return false;
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }

    return false;
}

bool PostCode::deserializePostCodes(const fs::path& path,
                                    std::map<uint64_t, uint64_t>& codes)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::JSONInputArchive iarchive(is);
            iarchive(codes);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(e.what());
        return false;
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }
    return false;
}

void PostCode::incrBootCycle()
{
    if (currentBootCycleIndex >= maxBootCycleNum())
    {
        currentBootCycleIndex = 1;
    }
    else
    {
        currentBootCycleIndex++;
    }
    currentBootCycleCount(std::min(
        maxBootCycleNum(), static_cast<uint16_t>(currentBootCycleCount() + 1)));
}

uint16_t PostCode::getBootNum(const uint16_t index) const
{
    // bootNum assumes the oldest archive is boot number 1
    // and the current boot number equals bootCycleCount
    // map bootNum back to bootIndex that was used to archive postcode
    uint16_t bootNum = currentBootCycleIndex;
    if (index > bootNum) // need to wrap around
    {
        bootNum = (maxBootCycleNum() + currentBootCycleIndex) - index + 1;
    }
    else
    {
        bootNum = currentBootCycleIndex - index + 1;
    }
    return bootNum;
}