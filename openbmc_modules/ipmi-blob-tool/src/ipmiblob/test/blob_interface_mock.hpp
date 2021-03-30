#include <ipmiblob/blob_interface.hpp>

#include <gmock/gmock.h>

namespace ipmiblob
{

class BlobInterfaceMock : public BlobInterface
{
  public:
    virtual ~BlobInterfaceMock() = default;
    MOCK_METHOD2(commit, void(std::uint16_t, const std::vector<std::uint8_t>&));
    MOCK_METHOD3(writeMeta, void(std::uint16_t, std::uint32_t,
                                 const std::vector<std::uint8_t>&));
    MOCK_METHOD3(writeBytes, void(std::uint16_t, std::uint32_t,
                                  const std::vector<std::uint8_t>&));
    MOCK_METHOD0(getBlobList, std::vector<std::string>());
    MOCK_METHOD1(getStat, StatResponse(const std::string&));
    MOCK_METHOD1(getStat, StatResponse(std::uint16_t));
    MOCK_METHOD2(openBlob, std::uint16_t(const std::string&, std::uint16_t));
    MOCK_METHOD1(closeBlob, void(std::uint16_t));
    MOCK_METHOD1(deleteBlob, void(const std::string&));
    MOCK_METHOD3(readBytes,
                 std::vector<std::uint8_t>(std::uint16_t, std::uint32_t,
                                           std::uint32_t));
};

} // namespace ipmiblob
