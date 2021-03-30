#include "config.h"

#include "utils.hpp"

#include <openssl/sha.h>

#include <algorithm>
#include <fstream>
#include <phosphor-logging/log.hpp>
#include <sstream>

using namespace phosphor::logging;

namespace utils
{

namespace // anonymous
{
constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
} // namespace

namespace internal
{
template <typename... Ts>
std::string concat_string(Ts const&... ts)
{
    std::stringstream s;
    ((s << ts << " "), ...) << std::endl;
    return s.str();
}

// Helper function to run command
// Returns return code and the stdout
template <typename... Ts>
std::pair<int, std::string> exec(Ts const&... ts)
{
    std::array<char, 512> buffer;
    std::string cmd = concat_string(ts...);
    std::stringstream result;
    int rc;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
    {
        result << buffer.data();
    }
    rc = pclose(pipe);
    return {rc, result.str()};
}

} // namespace internal
const UtilsInterface& getUtils()
{
    static Utils utils;
    return utils;
}

std::vector<std::string>
    Utils::getPSUInventoryPath(sdbusplus::bus::bus& bus) const
{
    std::vector<std::string> paths;
    auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetSubTreePaths");
    method.append(PSU_INVENTORY_PATH_BASE);
    method.append(0); // Depth 0 to search all
    method.append(std::vector<std::string>({PSU_INVENTORY_IFACE}));
    auto reply = bus.call(method);

    reply.read(paths);
    return paths;
}

std::string Utils::getService(sdbusplus::bus::bus& bus, const char* path,
                              const char* interface) const
{
    auto services = getServices(bus, path, interface);
    if (services.empty())
    {
        return {};
    }
    return services[0];
}

std::vector<std::string> Utils::getServices(sdbusplus::bus::bus& bus,
                                            const char* path,
                                            const char* interface) const
{
    auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetObject");

    mapper.append(path, std::vector<std::string>({interface}));
    try
    {
        auto mapperResponseMsg = bus.call(mapper);

        std::vector<std::pair<std::string, std::vector<std::string>>>
            mapperResponse;
        mapperResponseMsg.read(mapperResponse);
        if (mapperResponse.empty())
        {
            log<level::ERR>("Error reading mapper response");
            throw std::runtime_error("Error reading mapper response");
        }
        std::vector<std::string> ret;
        for (const auto& i : mapperResponse)
        {
            ret.emplace_back(i.first);
        }
        return ret;
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("GetObject call failed", entry("PATH=%s", path),
                        entry("INTERFACE=%s", interface));
        throw std::runtime_error("GetObject call failed");
    }
}

std::string Utils::getVersionId(const std::string& version) const
{
    if (version.empty())
    {
        log<level::ERR>("Error version is empty");
        return {};
    }

    unsigned char digest[SHA512_DIGEST_LENGTH];
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, version.c_str(), strlen(version.c_str()));
    SHA512_Final(digest, &ctx);
    char mdString[SHA512_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        snprintf(&mdString[i * 2], 3, "%02x", (unsigned int)digest[i]);
    }

    // Only need 8 hex digits.
    std::string hexId = std::string(mdString);
    return (hexId.substr(0, 8));
}

std::string Utils::getVersion(const std::string& inventoryPath) const
{
    // Invoke vendor-specify tool to get the version string, e.g.
    //   psutils get-version
    //   /xyz/openbmc_project/inventory/system/chassis/motherboard/powersupply0
    auto [rc, r] = internal::exec(PSU_VERSION_UTIL, inventoryPath);
    return (rc == 0) ? r : "";
}

std::string Utils::getLatestVersion(const std::set<std::string>& versions) const
{
    if (versions.empty())
    {
        return {};
    }
    std::stringstream args;
    for (const auto& s : versions)
    {
        args << s << " ";
    }
    auto [rc, r] = internal::exec(PSU_VERSION_COMPARE_UTIL, args.str());
    return (rc == 0) ? r : "";
}

bool Utils::isAssociated(const std::string& psuInventoryPath,
                         const AssociationList& assocs) const
{
    return std::find_if(assocs.begin(), assocs.end(),
                        [&psuInventoryPath](const auto& assoc) {
                            return psuInventoryPath == std::get<2>(assoc);
                        }) != assocs.end();
}

any Utils::getPropertyImpl(sdbusplus::bus::bus& bus, const char* service,
                           const char* path, const char* interface,
                           const char* propertyName) const
{
    auto method = bus.new_method_call(service, path,
                                      "org.freedesktop.DBus.Properties", "Get");
    method.append(interface, propertyName);
    try
    {
        PropertyType value{};
        auto reply = bus.call(method);
        reply.read(value);
        return any(value);
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        log<level::ERR>("GetProperty call failed", entry("PATH=%s", path),
                        entry("INTERFACE=%s", interface),
                        entry("PROPERTY=%s", propertyName));
        throw std::runtime_error("GetProperty call failed");
    }
}

} // namespace utils
