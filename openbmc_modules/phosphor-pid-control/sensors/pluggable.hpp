#pragma once

#include "interfaces.hpp"
#include "sensor.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>

/*
 * A Sensor that can use any reader or writer you provide.
 */
class PluggableSensor : public Sensor
{
  public:
    PluggableSensor(const std::string& name, int64_t timeout,
                    std::unique_ptr<ReadInterface> reader,
                    std::unique_ptr<WriteInterface> writer) :
        Sensor(name, timeout),
        _reader(std::move(reader)), _writer(std::move(writer))
    {
    }

    ReadReturn read(void) override;
    void write(double value) override;
    bool getFailed(void) override;

  private:
    std::unique_ptr<ReadInterface> _reader;
    std::unique_ptr<WriteInterface> _writer;
};
