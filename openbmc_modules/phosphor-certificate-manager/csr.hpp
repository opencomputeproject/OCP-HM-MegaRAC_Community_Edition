#pragma once
#include <xyz/openbmc_project/Certs/CSR/server.hpp>

namespace phosphor
{
namespace certs
{
using CSRRead = sdbusplus::xyz::openbmc_project::Certs::server::CSR;
using CSRIface = sdbusplus::server::object::object<CSRRead>;

enum class Status
{
    SUCCESS,
    FAILURE,
};

using CertInstallPath = std::string;

/** @class CSR
 *  @brief To read CSR certificate
 */
class CSR : public CSRIface
{
  public:
    CSR() = delete;
    ~CSR() = default;
    CSR(const CSR&) = delete;
    CSR& operator=(const CSR&) = delete;
    CSR(CSR&&) = default;
    CSR& operator=(CSR&&) = default;

    /** @brief Constructor to put object onto bus at a D-Bus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - The D-Bus object path to attach at.
     *  @param[in] installPath - Certificate installation path.
     *  @param[in] status - Status of Generate CSR request
     */
    CSR(sdbusplus::bus::bus& bus, const char* path,
        CertInstallPath&& installPath, const Status& status);
    /** @brief Return CSR
     */
    std::string cSR() override;

  private:
    /** @brief sdbusplus handler */
    sdbusplus::bus::bus& bus;

    /** @brief object path */
    std::string objectPath;

    /** @brief Certificate file installation path **/
    CertInstallPath certInstallPath;

    /** @brief Status of GenerateCSR request */
    Status csrStatus;
};
} // namespace certs
} // namespace phosphor
