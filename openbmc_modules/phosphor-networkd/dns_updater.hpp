#pragma once

#include <filesystem>

namespace phosphor
{
namespace network
{
namespace dns
{
namespace updater
{

namespace fs = std::filesystem;

constexpr auto RESOLV_CONF = "/etc/resolv.conf";

/** @brief Reads DNS entries supplied by DHCP and updates specified file
 *
 *  @param[in] inFile  - File having DNS entries supplied by DHCP
 *  @param[in] outFile - File to write the nameserver entries to
 */
void updateDNSEntries(const fs::path& inFile, const fs::path& outFile);

/** @brief User callback handler invoked by inotify watcher
 *
 *  Needed to enable production and test code so that the right
 *  callback functions could be implemented
 *
 *  @param[in] inFile - File having DNS entries supplied by DHCP
 */
inline void processDNSEntries(const fs::path& inFile)
{
    return updateDNSEntries(inFile, RESOLV_CONF);
}

} // namespace updater
} // namespace dns
} // namespace network
} // namespace phosphor
