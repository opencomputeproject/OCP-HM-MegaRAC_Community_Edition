#include "handler.hpp"
#include "parse_config.hpp"
#include "sys_file_impl.hpp"

#include <blobs-ipmid/blobs.hpp>
#include <exception>
#include <fstream>
#include <memory>
#include <phosphor-logging/elog.hpp>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is required by the blob manager.
 * TODO: move the declaration to blobs.hpp since all handlers need it
 */
std::unique_ptr<blobs::GenericBlobInterface> createHandler();

#ifdef __cplusplus
}
#endif

/* Configuration file path */
constexpr auto blobConfigPath = "/usr/share/binaryblob/config.json";

std::unique_ptr<blobs::GenericBlobInterface> createHandler()
{
    using namespace phosphor::logging;
    using nlohmann::json;

    std::ifstream input(blobConfigPath);
    json j;

    try
    {
        input >> j;
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Failed to parse config into json",
                        entry("ERR=%s", e.what()));
        return nullptr;
    }

    // Construct binary blobs from config and add to handler
    auto handler = std::make_unique<blobs::BinaryStoreBlobHandler>();

    for (const auto& element : j)
    {
        conf::BinaryBlobConfig config;
        try
        {
            conf::parseFromConfigFile(element, config);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>("Encountered error when parsing config file",
                            entry("ERR=%s", e.what()));
            return nullptr;
        }

        log<level::INFO>("Loading from config with",
                         entry("BASE_ID=%s", config.blobBaseId.c_str()),
                         entry("FILE=%s", config.sysFilePath.c_str()),
                         entry("MAX_SIZE=%llx", static_cast<unsigned long long>(
                                                    config.maxSizeBytes)));

        auto file = std::make_unique<binstore::SysFileImpl>(config.sysFilePath,
                                                            config.offsetBytes);

        handler->addNewBinaryStore(binstore::BinaryStore::createFromConfig(
            config.blobBaseId, std::move(file)));
    }

    return std::move(handler);
}
