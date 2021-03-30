#pragma once

#include "data_handler.hpp"

#include <unistd.h>

#include <stdplus/handle/managed.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace ipmi_flash
{

/**
 * Data Handler for receiving the image over a network port
 */
class NetDataHandler : public DataInterface
{
  public:
    NetDataHandler() : listenFd(std::nullopt), connFd(std::nullopt)
    {}

    bool open() override;
    bool close() override;
    std::vector<std::uint8_t> copyFrom(std::uint32_t length) override;
    bool writeMeta(const std::vector<std::uint8_t>& configuration) override;
    std::vector<std::uint8_t> readMeta() override;

    static constexpr std::uint16_t listenPort = 623;
    static constexpr int timeoutS = 5;

  private:
    static void closefd(int&& fd)
    {
        ::close(fd);
    }
    using Fd = stdplus::Managed<int>::Handle<closefd>;

    Fd listenFd;
    Fd connFd;
};

} // namespace ipmi_flash
