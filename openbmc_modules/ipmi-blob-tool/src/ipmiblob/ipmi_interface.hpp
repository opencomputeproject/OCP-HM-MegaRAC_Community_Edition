#pragma once

#include <cstdint>
#include <vector>

namespace ipmiblob
{

class IpmiInterface
{
  public:
    virtual ~IpmiInterface() = default;

    /**
     * Send an IPMI packet to the BMC.
     *
     * @param[in] netfn - the netfn for the IPMI packet.
     * @param[in] cmd - the command.
     * @param[in] data - a vector of the IPMI packet contents.
     * @return the bytes returned.
     * @throws IpmiException on failure.
     */
    virtual std::vector<std::uint8_t>
        sendPacket(std::uint8_t netfn, std::uint8_t cmd,
                   std::vector<std::uint8_t>& data) = 0;
};

} // namespace ipmiblob
