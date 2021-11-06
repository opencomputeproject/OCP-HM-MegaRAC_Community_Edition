#pragma once

#include "xyz/openbmc_project/Software/Preserve/server.hpp"

namespace phosphor
{
namespace software
{
namespace preserve
{

class ConfManager;

using Ifaces = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Software::server::Preserve>;

class PreserveConf : public Ifaces
{
  public:
    PreserveConf() = delete;
    PreserveConf(const PreserveConf&) = delete;
    PreserveConf& operator=(const PreserveConf&) = delete;
    PreserveConf(PreserveConf&&) = delete;
    PreserveConf& operator=(PreserveConf&&) = delete;
    virtual ~PreserveConf() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Path to attach at.
     *  @param[in] parent - Parent D-bus Object.
     */
    PreserveConf(sdbusplus::bus::bus& bus, const char *objPath,
            ConfManager& parent);

    /** @brief Update the IPMI preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool iPMI(bool value) override;

    /** @brief Update the User preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool user(bool value) override;

    /** @brief Update the LDAP preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool lDAP(bool value) override;

    /** @brief Update the Certificates preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool certificates(bool value) override;

    /** @brief Update the Hostname preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool hostname(bool value) override;

    /** @brief Update the SEL preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool sEL(bool value) override;

    /** @brief Update the SDR preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool sDR(bool value) override;

    /** @brief Update the Network preservation setting
     * 
     *  @param[in] value - boolean
     *
     *  @return On success the preservation setting
     */
    bool network(bool value) override;

    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::iPMI;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::user;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::lDAP;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::certificates;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::hostname;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::sEL;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::sDR;
    using sdbusplus::xyz::openbmc_project::Software::server::Preserve::network;

  private:
    ConfManager& parent;
};

} // namespace preserve
} // namespace software
} // namespace phosphor
