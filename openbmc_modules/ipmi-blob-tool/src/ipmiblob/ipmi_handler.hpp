#pragma once

#include "internal/sys.hpp"
#include "ipmi_interface.hpp"

#include <memory>
#include <vector>

namespace ipmiblob
{

class IpmiHandler : public IpmiInterface
{
  public:
    /* Create an IpmiHandler object with default inputs. It is ill-advised to
     * share IpmiHandlers between objects.
     */
    static std::unique_ptr<IpmiInterface> CreateIpmiHandler();

    explicit IpmiHandler(const internal::Sys* sys = &internal::sys_impl) :
        sys(sys){};

    ~IpmiHandler() = default;
    IpmiHandler(const IpmiHandler&) = delete;
    IpmiHandler& operator=(const IpmiHandler&) = delete;
    IpmiHandler(IpmiHandler&&) = default;
    IpmiHandler& operator=(IpmiHandler&&) = default;

    /**
     * Attempt to open the device node.
     *
     * @throws IpmiException on failure.
     */
    void open();

    /**
     * @throws IpmiException on failure.
     */
    std::vector<std::uint8_t>
        sendPacket(std::uint8_t netfn, std::uint8_t cmd,
                   std::vector<std::uint8_t>& data) override;

  private:
    const internal::Sys* sys;
    /** TODO: Use a smart file descriptor when it's ready.  Until then only
     * allow moving this object.
     */
    int fd = -1;
    /* The last IPMI sequence number we used. */
    int sequence = 0;
};

} // namespace ipmiblob
