#pragma once

#include "status.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

#include <memory>
#include <string>

namespace ipmi_flash
{

class SystemdNoFile : public TriggerableActionInterface
{
  public:
    static std::unique_ptr<TriggerableActionInterface>
        CreateSystemdNoFile(sdbusplus::bus::bus&& bus,
                            const std::string& service,
                            const std::string& mode);

    SystemdNoFile(sdbusplus::bus::bus&& bus, const std::string& service,
                  const std::string& mode) :
        bus(std::move(bus)),
        triggerService(service), mode(mode)
    {}

    SystemdNoFile(const SystemdNoFile&) = delete;
    SystemdNoFile& operator=(const SystemdNoFile&) = delete;
    // sdbusplus match requires us to be pinned
    SystemdNoFile(SystemdNoFile&&) = delete;
    SystemdNoFile& operator=(SystemdNoFile&&) = delete;

    bool trigger() override;
    void abort() override;
    ActionStatus status() override;

    const std::string& getMode() const;

  private:
    sdbusplus::bus::bus bus;
    const std::string triggerService;
    const std::string mode;

    std::optional<sdbusplus::bus::match::match> jobMonitor;
    std::optional<std::string> job;
    ActionStatus currentStatus = ActionStatus::unknown;

    void match(sdbusplus::message::message& m);
};

/**
 * Representation of what is used for triggering an action with systemd and
 * checking the result by reading a file.
 */
class SystemdWithStatusFile : public SystemdNoFile
{
  public:
    /**
     * Create a default SystemdWithStatusFile object that uses systemd to
     * trigger the process.
     *
     * @param[in] bus - an sdbusplus handler for a bus to use.
     * @param[in] path - the path to check for verification status.
     * @param[in] service - the systemd service to start to trigger
     * verification.
     * @param[in] mode - the job-mode when starting the systemd Unit.
     */
    static std::unique_ptr<TriggerableActionInterface>
        CreateSystemdWithStatusFile(sdbusplus::bus::bus&& bus,
                                    const std::string& path,
                                    const std::string& service,
                                    const std::string& mode);

    SystemdWithStatusFile(sdbusplus::bus::bus&& bus, const std::string& path,
                          const std::string& service, const std::string& mode) :
        SystemdNoFile(std::move(bus), service, mode),
        checkPath(path)
    {}

    bool trigger() override;
    ActionStatus status() override;

  private:
    const std::string checkPath;
};

} // namespace ipmi_flash
