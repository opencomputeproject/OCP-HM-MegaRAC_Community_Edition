#include <ipmiblob/test/blob_interface_mock.hpp>
#include <ipmiblob/test/crc_mock.hpp>
#include <ipmiblob/test/ipmi_interface_mock.hpp>

#include <gtest/gtest.h>

namespace ipmiblob
{

TEST(BuildMockObjects, buildAllMocks)
{
    BlobInterfaceMock blobMock;
    CrcMock crcMock;
    IpmiInterfaceMock ipmiMock;
}

} // namespace ipmiblob
