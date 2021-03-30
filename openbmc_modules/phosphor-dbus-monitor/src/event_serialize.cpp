#include "config.h"

#include "event_serialize.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>
#include <phosphor-logging/log.hpp>

// Register class version
// From cereal documentation;
// "This macro should be placed at global scope"
CEREAL_CLASS_VERSION(phosphor::events::Entry, CLASS_VERSION);

namespace phosphor
{
namespace events
{

using namespace phosphor::logging;

/** @brief Function required by Cereal to perform serialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] archive - reference to Cereal archive.
 *  @param[in] event - const reference to event entry.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void save(Archive& archive, const Entry& event, const std::uint32_t version)
{
    archive(event.timestamp(), event.message(), event.additionalData());
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] archive - reference to Cereal archive.
 *  @param[in] event - reference to event entry.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void load(Archive& archive, Entry& event, const std::uint32_t version)
{
    using namespace sdbusplus::xyz::openbmc_project::Logging::server;

    uint64_t timestamp{};
    std::string message{};
    std::vector<std::string> additionalData{};

    archive(timestamp, message, additionalData);

    event.timestamp(timestamp);
    event.message(message);
    event.additionalData(additionalData);
}

fs::path serialize(const Entry& event, const std::string& eventName)
{
    fs::path dir(EVENTS_PERSIST_PATH);
    auto path = dir / eventName;
    fs::create_directories(path);
    path /= std::to_string(event.timestamp());
    std::ofstream os(path.string(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(event);
    return path;
}

bool deserialize(const fs::path& path, Entry& event)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::BinaryInputArchive iarchive(is);
            iarchive(event);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        log<level::ERR>(e.what());
        std::error_code ec;
        fs::remove(path, ec);
        return false;
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }
}

} // namespace events
} // namespace phosphor
