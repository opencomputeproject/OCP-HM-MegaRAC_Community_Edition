#pragma once

#include "interfaces.hpp"
#include "util.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>

/*
 * This ReadInterface will actively reach out over dbus upon calling read to
 * get the value from whomever owns the associated dbus path.
 */
class DbusActiveRead : public ReadInterface
{
  public:
    DbusActiveRead(sdbusplus::bus::bus& bus, const std::string& path,
                   const std::string& service, DbusHelperInterface* helper) :
        ReadInterface(),
        _bus(bus), _path(path), _service(service), _helper(helper)
    {
    }

    ReadReturn read(void) override;

  private:
    sdbusplus::bus::bus& _bus;
    const std::string _path;
    const std::string _service; // the sensor service.
    DbusHelperInterface* _helper;
};
