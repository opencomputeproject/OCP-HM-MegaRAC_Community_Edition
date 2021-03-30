#pragma once

#include "xyz/openbmc_project/Common/TFTP/server.hpp"

#include <sdbusplus/bus.hpp>

#include <string>

namespace phosphor
{
namespace software
{
namespace manager
{

using DownloadInherit = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Common::server::TFTP>;

/** @class Download
 *  @brief OpenBMC download software management implementation.
 *  @details A concrete implementation for xyz.openbmc_project.Common.TFTP
 *  DBus API.
 */
class Download : public DownloadInherit
{
  public:
    /** @brief Constructs Download Software Manager
     *
     * @param[in] bus       - The Dbus bus object
     * @param[in] objPath   - The Dbus object path
     */
    Download(sdbusplus::bus::bus& bus, const std::string& objPath) :
        DownloadInherit(bus, (objPath).c_str()){};

    /**
     * @brief Download the specified image via TFTP
     *
     * @param[in] fileName      - The name of the file to transfer.
     * @param[in] serverAddress - The TFTP Server IP Address.
     **/
    void downloadViaTFTP(std::string fileName,
                         std::string serverAddress) override;
};

} // namespace manager
} // namespace software
} // namespace phosphor
