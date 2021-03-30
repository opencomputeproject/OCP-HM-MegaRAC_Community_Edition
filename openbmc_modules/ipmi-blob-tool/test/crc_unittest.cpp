#include <ipmiblob/crc.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace ipmiblob
{

TEST(Crc16Test, VerifyCrcValue)
{
    // Verify the crc16 is producing the value we expect.

    // Origin: security/crypta/ipmi/portable/ipmi_utils_test.cc
    struct CrcTestVector
    {
        std::string input;
        uint16_t output;
    };

    std::string longString =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAA";

    std::vector<CrcTestVector> vectors({{"", 0x1D0F},
                                        {"A", 0x9479},
                                        {"123456789", 0xE5CC},
                                        {longString, 0xE938}});

    for (const CrcTestVector& testVector : vectors)
    {
        std::vector<std::uint8_t> input;
        input.insert(input.begin(), testVector.input.begin(),
                     testVector.input.end());
        EXPECT_EQ(generateCrc(input), testVector.output);
    }
}
} // namespace ipmiblob
