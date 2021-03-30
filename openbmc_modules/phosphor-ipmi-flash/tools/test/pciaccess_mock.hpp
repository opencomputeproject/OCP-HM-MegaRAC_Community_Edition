#pragma once

#include "pciaccess.hpp"

#include <gmock/gmock.h>

namespace host_tool
{

class PciAccessMock : public PciAccess
{
  public:
    MOCK_CONST_METHOD1(pci_id_match_iterator_create,
                       struct pci_device_iterator*(const struct pci_id_match*));
    MOCK_CONST_METHOD1(pci_iterator_destroy, void(struct pci_device_iterator*));
    MOCK_CONST_METHOD1(pci_device_next,
                       struct pci_device*(struct pci_device_iterator*));
    MOCK_CONST_METHOD1(pci_device_probe, int(struct pci_device*));
    MOCK_CONST_METHOD3(pci_device_cfg_read_u8,
                       int(struct pci_device* dev, std::uint8_t* data,
                           pciaddr_t offset));
    MOCK_CONST_METHOD3(pci_device_cfg_write_u8,
                       int(struct pci_device* dev, std::uint8_t data,
                           pciaddr_t offset));
    MOCK_CONST_METHOD5(pci_device_map_range, int(struct pci_device*, pciaddr_t,
                                                 pciaddr_t, unsigned, void**));
    MOCK_CONST_METHOD3(pci_device_unmap_range,
                       int(struct pci_device*, void*, pciaddr_t));
};

} // namespace host_tool
