#pragma once

#include "interface.hpp"
#include "internal/sys.hpp"
#include "progress.hpp"

#include <ipmiblob/blob_interface.hpp>

namespace host_tool
{

class BtDataHandler : public DataInterface
{
  public:
    BtDataHandler(ipmiblob::BlobInterface* blob, ProgressInterface* progress,
                  const internal::Sys* sys = &internal::sys_impl) :
        blob(blob),
        progress(progress), sys(sys){};

    bool sendContents(const std::string& input, std::uint16_t session) override;
    ipmi_flash::FirmwareFlags::UpdateFlags supportedType() const override
    {
        return ipmi_flash::FirmwareFlags::UpdateFlags::ipmi;
    }

  private:
    ipmiblob::BlobInterface* blob;
    ProgressInterface* progress;
    const internal::Sys* sys;
};

} // namespace host_tool
