/**
 * Copyright © 2019 IBM Corporation
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
#include "repository.hpp"

#include <sys/stat.h>

#include <fstream>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>

namespace openpower
{
namespace pels
{

namespace fs = std::filesystem;
using namespace phosphor::logging;
namespace file_error = sdbusplus::xyz::openbmc_project::Common::File::Error;

constexpr size_t warningPercentage = 95;

/**
 * @brief Returns the amount of space the file uses on disk.
 *
 * This is different than just the regular size of the file.
 *
 * @param[in] file - The file to get the size of
 *
 * @return size_t The disk space the file uses
 */
size_t getFileDiskSize(const std::filesystem::path& file)
{
    constexpr size_t statBlockSize = 512;
    struct stat statData;
    auto rc = stat(file.c_str(), &statData);
    if (rc != 0)
    {
        auto e = errno;
        std::string msg = "call to stat() failed on " + file.native() +
                          " with errno " + std::to_string(e);
        log<level::ERR>(msg.c_str());
        abort();
    }

    return statData.st_blocks * statBlockSize;
}

Repository::Repository(const std::filesystem::path& basePath, size_t repoSize,
                       size_t maxNumPELs) :
    _logPath(basePath / "logs"),
    _maxRepoSize(repoSize), _maxNumPELs(maxNumPELs)
{
    if (!fs::exists(_logPath))
    {
        fs::create_directories(_logPath);
    }

    restore();
}

void Repository::restore()
{
    for (auto& dirEntry : fs::directory_iterator(_logPath))
    {
        try
        {
            if (!fs::is_regular_file(dirEntry.path()))
            {
                continue;
            }

            std::ifstream file{dirEntry.path()};
            std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                                      std::istreambuf_iterator<char>()};
            file.close();

            PEL pel{data};
            if (pel.valid())
            {
                // If the host hasn't acked it, reset the host state so
                // it will get sent up again.
                if (pel.hostTransmissionState() == TransmissionState::sent)
                {
                    pel.setHostTransmissionState(TransmissionState::newPEL);
                    try
                    {
                        write(pel, dirEntry.path());
                    }
                    catch (std::exception& e)
                    {
                        log<level::ERR>(
                            "Failed to save PEL after updating host state",
                            entry("PELID=0x%X", pel.id()));
                    }
                }

                PELAttributes attributes{dirEntry.path(),
                                         getFileDiskSize(dirEntry.path()),
                                         pel.privateHeader().creatorID(),
                                         pel.userHeader().severity(),
                                         pel.userHeader().actionFlags(),
                                         pel.hostTransmissionState(),
                                         pel.hmcTransmissionState()};

                using pelID = LogID::Pel;
                using obmcID = LogID::Obmc;
                _pelAttributes.emplace(
                    LogID(pelID(pel.id()), obmcID(pel.obmcLogID())),
                    attributes);

                updateRepoStats(attributes, true);
            }
            else
            {
                log<level::ERR>(
                    "Found invalid PEL file while restoring.  Removing.",
                    entry("FILENAME=%s", dirEntry.path().c_str()));
                fs::remove(dirEntry.path());
            }
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Hit exception while restoring PEL File",
                            entry("FILENAME=%s", dirEntry.path().c_str()),
                            entry("ERROR=%s", e.what()));
        }
    }
}

std::string Repository::getPELFilename(uint32_t pelID, const BCDTime& time)
{
    char name[50];
    sprintf(name, "%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X_%.8X", time.yearMSB,
            time.yearLSB, time.month, time.day, time.hour, time.minutes,
            time.seconds, time.hundredths, pelID);
    return std::string{name};
}

void Repository::add(std::unique_ptr<PEL>& pel)
{
    pel->setHostTransmissionState(TransmissionState::newPEL);
    pel->setHMCTransmissionState(TransmissionState::newPEL);

    auto path = _logPath / getPELFilename(pel->id(), pel->commitTime());

    write(*(pel.get()), path);

    PELAttributes attributes{path,
                             getFileDiskSize(path),
                             pel->privateHeader().creatorID(),
                             pel->userHeader().severity(),
                             pel->userHeader().actionFlags(),
                             pel->hostTransmissionState(),
                             pel->hmcTransmissionState()};

    using pelID = LogID::Pel;
    using obmcID = LogID::Obmc;
    _pelAttributes.emplace(LogID(pelID(pel->id()), obmcID(pel->obmcLogID())),
                           attributes);

    updateRepoStats(attributes, true);

    processAddCallbacks(*pel);
}

void Repository::write(const PEL& pel, const fs::path& path)
{
    std::ofstream file{path, std::ios::binary};

    if (!file.good())
    {
        // If this fails, the filesystem is probably full so it isn't like
        // we could successfully create yet another error log here.
        auto e = errno;
        fs::remove(path);
        log<level::ERR>("Unable to open PEL file for writing",
                        entry("ERRNO=%d", e), entry("PATH=%s", path.c_str()));
        throw file_error::Open();
    }

    auto data = pel.data();
    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    if (file.fail())
    {
        // Same note as above about not being able to create an error log
        // for this case even if we wanted.
        auto e = errno;
        file.close();
        fs::remove(path);
        log<level::ERR>("Unable to write PEL file", entry("ERRNO=%d", e),
                        entry("PATH=%s", path.c_str()));
        throw file_error::Write();
    }
}

std::optional<Repository::LogID> Repository::remove(const LogID& id)
{
    std::optional<LogID> actualID;

    auto pel = findPEL(id);
    if (pel != _pelAttributes.end())
    {
        actualID = pel->first;
        updateRepoStats(pel->second, false);

        log<level::DEBUG>("Removing PEL from repository",
                          entry("PEL_ID=0x%X", pel->first.pelID.id),
                          entry("OBMC_LOG_ID=%d", pel->first.obmcID.id));
        fs::remove(pel->second.path);
        _pelAttributes.erase(pel);

        processDeleteCallbacks(pel->first.pelID.id);
    }

    return actualID;
}

std::optional<std::vector<uint8_t>> Repository::getPELData(const LogID& id)
{
    auto pel = findPEL(id);
    if (pel != _pelAttributes.end())
    {
        std::ifstream file{pel->second.path.c_str()};
        if (!file.good())
        {
            auto e = errno;
            log<level::ERR>("Unable to open PEL file", entry("ERRNO=%d", e),
                            entry("PATH=%s", pel->second.path.c_str()));
            throw file_error::Open();
        }

        std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                                  std::istreambuf_iterator<char>()};
        return data;
    }

    return std::nullopt;
}

std::optional<sdbusplus::message::unix_fd> Repository::getPELFD(const LogID& id)
{
    auto pel = findPEL(id);
    if (pel != _pelAttributes.end())
    {
        FILE* fp = fopen(pel->second.path.c_str(), "rb");

        if (fp == nullptr)
        {
            auto e = errno;
            log<level::ERR>("Unable to open PEL File", entry("ERRNO=%d", e),
                            entry("PATH=%s", pel->second.path.c_str()));
            throw file_error::Open();
        }

        // Must leave the file open here.  It will be closed by sdbusplus
        // when it sends it back over D-Bus.

        return fileno(fp);
    }
    return std::nullopt;
}

void Repository::for_each(ForEachFunc func) const
{
    for (const auto& [id, attributes] : _pelAttributes)
    {
        std::ifstream file{attributes.path};

        if (!file.good())
        {
            auto e = errno;
            log<level::ERR>("Repository::for_each: Unable to open PEL file",
                            entry("ERRNO=%d", e),
                            entry("PATH=%s", attributes.path.c_str()));
            continue;
        }

        std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                                  std::istreambuf_iterator<char>()};
        file.close();

        PEL pel{data};

        try
        {
            if (func(pel))
            {
                break;
            }
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Repository::for_each function exception",
                            entry("ERROR=%s", e.what()));
        }
    }
}

void Repository::processAddCallbacks(const PEL& pel) const
{
    for (auto& [name, func] : _addSubscriptions)
    {
        try
        {
            func(pel);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("PEL Repository add callback exception",
                            entry("NAME=%s", name.c_str()),
                            entry("ERROR=%s", e.what()));
        }
    }
}

void Repository::processDeleteCallbacks(uint32_t id) const
{
    for (auto& [name, func] : _deleteSubscriptions)
    {
        try
        {
            func(id);
        }
        catch (std::exception& e)
        {
            log<level::ERR>("PEL Repository delete callback exception",
                            entry("NAME=%s", name.c_str()),
                            entry("ERROR=%s", e.what()));
        }
    }
}

std::optional<std::reference_wrapper<const Repository::PELAttributes>>
    Repository::getPELAttributes(const LogID& id) const
{
    auto pel = findPEL(id);
    if (pel != _pelAttributes.end())
    {
        return pel->second;
    }

    return std::nullopt;
}

void Repository::setPELHostTransState(uint32_t pelID, TransmissionState state)
{
    LogID id{LogID::Pel{pelID}};
    auto attr = std::find_if(_pelAttributes.begin(), _pelAttributes.end(),
                             [&id](const auto& a) { return a.first == id; });

    if ((attr != _pelAttributes.end()) && (attr->second.hostState != state))
    {
        PELUpdateFunc func = [state](PEL& pel) {
            pel.setHostTransmissionState(state);
        };

        try
        {
            updatePEL(attr->second.path, func);

            attr->second.hostState = state;
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Unable to update PEL host transmission state",
                            entry("PATH=%s", attr->second.path.c_str()),
                            entry("ERROR=%s", e.what()));
        }
    }
}

void Repository::setPELHMCTransState(uint32_t pelID, TransmissionState state)
{
    LogID id{LogID::Pel{pelID}};
    auto attr = std::find_if(_pelAttributes.begin(), _pelAttributes.end(),
                             [&id](const auto& a) { return a.first == id; });

    if ((attr != _pelAttributes.end()) && (attr->second.hmcState != state))
    {
        PELUpdateFunc func = [state](PEL& pel) {
            pel.setHMCTransmissionState(state);
        };

        try
        {
            updatePEL(attr->second.path, func);

            attr->second.hmcState = state;
        }
        catch (std::exception& e)
        {
            log<level::ERR>("Unable to update PEL HMC transmission state",
                            entry("PATH=%s", attr->second.path.c_str()),
                            entry("ERROR=%s", e.what()));
        }
    }
}

void Repository::updatePEL(const fs::path& path, PELUpdateFunc updateFunc)
{
    std::ifstream file{path};
    std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                              std::istreambuf_iterator<char>()};
    file.close();

    PEL pel{data};

    if (pel.valid())
    {
        updateFunc(pel);

        write(pel, path);
    }
    else
    {
        throw std::runtime_error(
            "Unable to read a valid PEL when trying to update it");
    }
}

bool Repository::isServiceableSev(const PELAttributes& pel)
{
    auto sevType = static_cast<SeverityType>(pel.severity & 0xF0);
    auto sevPVEntry =
        pel_values::findByValue(pel.severity, pel_values::severityValues);
    std::string sevName = std::get<pel_values::registryNamePos>(*sevPVEntry);

    bool check1 = (sevType == SeverityType::predictive) ||
                  (sevType == SeverityType::unrecoverable) ||
                  (sevType == SeverityType::critical);

    bool check2 = ((sevType == SeverityType::recovered) ||
                   (sevName == "symptom_recovered")) &&
                  !pel.actionFlags.test(hiddenFlagBit);

    bool check3 = (sevName == "symptom_predictive") ||
                  (sevName == "symptom_unrecoverable") ||
                  (sevName == "symptom_critical");

    return check1 || check2 || check3;
}

void Repository::updateRepoStats(const PELAttributes& pel, bool pelAdded)
{
    auto isServiceable = Repository::isServiceableSev(pel);
    auto bmcPEL = CreatorID::openBMC == static_cast<CreatorID>(pel.creator);

    auto adjustSize = [pelAdded, &pel](auto& runningSize) {
        if (pelAdded)
        {
            runningSize += pel.sizeOnDisk;
        }
        else
        {
            runningSize = std::max(static_cast<int64_t>(runningSize) -
                                       static_cast<int64_t>(pel.sizeOnDisk),
                                   static_cast<int64_t>(0));
        }
    };

    adjustSize(_sizes.total);

    if (bmcPEL)
    {
        adjustSize(_sizes.bmc);
        if (isServiceable)
        {
            adjustSize(_sizes.bmcServiceable);
        }
        else
        {
            adjustSize(_sizes.bmcInfo);
        }
    }
    else
    {
        adjustSize(_sizes.nonBMC);
        if (isServiceable)
        {
            adjustSize(_sizes.nonBMCServiceable);
        }
        else
        {
            adjustSize(_sizes.nonBMCInfo);
        }
    }
}

bool Repository::sizeWarning() const
{
    return (_sizes.total > (_maxRepoSize * warningPercentage / 100)) ||
           (_pelAttributes.size() > _maxNumPELs);
}

std::vector<Repository::AttributesReference>
    Repository::getAllPELAttributes(SortOrder order) const
{
    std::vector<Repository::AttributesReference> attributes;

    std::for_each(
        _pelAttributes.begin(), _pelAttributes.end(),
        [&attributes](auto& pelEntry) { attributes.push_back(pelEntry); });

    std::sort(attributes.begin(), attributes.end(),
              [order](const auto& left, const auto& right) {
                  if (order == SortOrder::ascending)
                  {
                      return left.get().second.path < right.get().second.path;
                  }
                  return left.get().second.path > right.get().second.path;
              });

    return attributes;
}

std::vector<uint32_t> Repository::prune()
{
    std::vector<uint32_t> obmcLogIDs;
    std::string msg = "Pruning PEL repository that takes up " +
                      std::to_string(_sizes.total) + " bytes and has " +
                      std::to_string(_pelAttributes.size()) + " PELs";
    log<level::INFO>(msg.c_str());

    // Set up the 5 functions to check if the PEL category
    // is still over its limits.

    // BMC informational PELs should only take up 15%
    IsOverLimitFunc overBMCInfoLimit = [this]() {
        return _sizes.bmcInfo > _maxRepoSize * 15 / 100;
    };

    // BMC non informational PELs should only take up 30%
    IsOverLimitFunc overBMCNonInfoLimit = [this]() {
        return _sizes.bmcServiceable > _maxRepoSize * 30 / 100;
    };

    // Non BMC informational PELs should only take up 15%
    IsOverLimitFunc overNonBMCInfoLimit = [this]() {
        return _sizes.nonBMCInfo > _maxRepoSize * 15 / 100;
    };

    // Non BMC non informational PELs should only take up 15%
    IsOverLimitFunc overNonBMCNonInfoLimit = [this]() {
        return _sizes.nonBMCServiceable > _maxRepoSize * 30 / 100;
    };

    // Bring the total number of PELs down to 80% of the max
    IsOverLimitFunc tooManyPELsLimit = [this]() {
        return _pelAttributes.size() > _maxNumPELs * 80 / 100;
    };

    // Set up the functions to determine which category a PEL is in.
    // TODO: Return false in these functions if a PEL caused a guard record.

    // A BMC informational PEL
    IsPELTypeFunc isBMCInfo = [](const PELAttributes& pel) {
        return (CreatorID::openBMC == static_cast<CreatorID>(pel.creator)) &&
               !Repository::isServiceableSev(pel);
    };

    // A BMC non informational PEL
    IsPELTypeFunc isBMCNonInfo = [](const PELAttributes& pel) {
        return (CreatorID::openBMC == static_cast<CreatorID>(pel.creator)) &&
               Repository::isServiceableSev(pel);
    };

    // A non BMC informational PEL
    IsPELTypeFunc isNonBMCInfo = [](const PELAttributes& pel) {
        return (CreatorID::openBMC != static_cast<CreatorID>(pel.creator)) &&
               !Repository::isServiceableSev(pel);
    };

    // A non BMC non informational PEL
    IsPELTypeFunc isNonBMCNonInfo = [](const PELAttributes& pel) {
        return (CreatorID::openBMC != static_cast<CreatorID>(pel.creator)) &&
               Repository::isServiceableSev(pel);
    };

    // When counting PELs, count every PEL
    IsPELTypeFunc isAnyPEL = [](const PELAttributes& pel) { return true; };

    // Check all 4 categories, which will result in at most 90%
    // usage (15 + 30 + 15 + 30).
    removePELs(overBMCInfoLimit, isBMCInfo, obmcLogIDs);
    removePELs(overBMCNonInfoLimit, isBMCNonInfo, obmcLogIDs);
    removePELs(overNonBMCInfoLimit, isNonBMCInfo, obmcLogIDs);
    removePELs(overNonBMCNonInfoLimit, isNonBMCNonInfo, obmcLogIDs);

    // After the above pruning check if there are still too many PELs,
    // which can happen depending on PEL sizes.
    if (_pelAttributes.size() > _maxNumPELs)
    {
        removePELs(tooManyPELsLimit, isAnyPEL, obmcLogIDs);
    }

    if (!obmcLogIDs.empty())
    {
        std::string msg = "Number of PELs removed to save space: " +
                          std::to_string(obmcLogIDs.size());
        log<level::INFO>(msg.c_str());
    }

    return obmcLogIDs;
}

void Repository::removePELs(IsOverLimitFunc& isOverLimit,
                            IsPELTypeFunc& isPELType,
                            std::vector<uint32_t>& removedBMCLogIDs)
{
    if (!isOverLimit())
    {
        return;
    }

    auto attributes = getAllPELAttributes(SortOrder::ascending);

    // Make 4 passes on the PELs, stopping as soon as isOverLimit
    // returns false.
    //   Pass 1: only delete HMC acked PELs
    //   Pass 2: only delete OS acked PELs
    //   Pass 3: only delete PHYP sent PELs
    //   Pass 4: delete all PELs
    static const std::vector<std::function<bool(const PELAttributes& pel)>>
        stateChecks{[](const auto& pel) {
                        return pel.hmcState == TransmissionState::acked;
                    },

                    [](const auto& pel) {
                        return pel.hostState == TransmissionState::acked;
                    },

                    [](const auto& pel) {
                        return pel.hostState == TransmissionState::sent;
                    },

                    [](const auto& pel) { return true; }};

    for (const auto& stateCheck : stateChecks)
    {
        for (auto it = attributes.begin(); it != attributes.end();)
        {
            const auto& pel = it->get();
            if (isPELType(pel.second) && stateCheck(pel.second))
            {
                auto removedID = pel.first.obmcID.id;
                remove(pel.first);

                removedBMCLogIDs.push_back(removedID);

                attributes.erase(it);

                if (!isOverLimit())
                {
                    break;
                }
            }
            else
            {
                ++it;
            }
        }

        if (!isOverLimit())
        {
            break;
        }
    }
}

} // namespace pels
} // namespace openpower
