#include "file_handler.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

namespace ipmi_flash
{

static constexpr auto TESTPATH = "test.output";

class FileHandlerOpenTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        (void)std::remove(TESTPATH);
    }
};

TEST_F(FileHandlerOpenTest, VerifyItIsHappy)
{
    /* Opening a fail may create it? */

    FileHandler handler(TESTPATH);
    EXPECT_TRUE(handler.open(""));

    /* Calling open twice fails the second time. */
    EXPECT_FALSE(handler.open(""));
}

TEST_F(FileHandlerOpenTest, VerifyWriteDataWrites)
{
    /* Verify writing bytes writes them... flushing data can be an issue here,
     * so we close first.
     */
    FileHandler handler(TESTPATH);
    EXPECT_TRUE(handler.open(""));

    std::vector<std::uint8_t> bytes = {0x01, 0x02};
    std::uint32_t offset = 0;

    EXPECT_TRUE(handler.write(offset, bytes));
    handler.close();

    std::ifstream data;
    data.open(TESTPATH, std::ios::binary);
    char expectedBytes[2];
    data.read(&expectedBytes[0], sizeof(expectedBytes));
    EXPECT_EQ(expectedBytes[0], bytes[0]);
    EXPECT_EQ(expectedBytes[1], bytes[1]);
    /* annoyingly the memcmp was failing... but it's the same data. */
}

} // namespace ipmi_flash
