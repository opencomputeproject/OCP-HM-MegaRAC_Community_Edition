#pragma once

#include "interface.hpp"
#include "internal/sys.hpp"
#include "progress.hpp"

#include <unistd.h>

#include <ipmiblob/blob_interface.hpp>
#include <stdplus/handle/managed.hpp>

#include <cstdint>
#include <string>

namespace host_tool
{

class NetDataHandler : public DataInterface
{
  public:
    NetDataHandler(ipmiblob::BlobInterface* blob, ProgressInterface* progress,
                   const std::string& host, const std::string& port,
                   const internal::Sys* sys = &internal::sys_impl) :
        blob(blob),
        progress(progress), host(host), port(port), sys(sys){};

    bool sendContents(const std::string& input, std::uint16_t session) override;
    ipmi_flash::FirmwareFlags::UpdateFlags supportedType() const override
    {
        return ipmi_flash::FirmwareFlags::UpdateFlags::net;
    }

  private:
    ipmiblob::BlobInterface* blob;
    ProgressInterface* progress;
    std::string host;
    std::string port;
    const internal::Sys* sys;
};

} // namespace host_tool
