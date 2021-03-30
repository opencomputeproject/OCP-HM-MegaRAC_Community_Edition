#include "ipmi.hpp"
#include "manager_mock.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

TEST(IpmiValidateTest, VerifyCommandMinimumLengths)
{

    struct TestCase
    {
        BlobOEMCommands cmd;
        size_t len;
        bool expect;
    };

    std::vector<TestCase> tests = {
        {BlobOEMCommands::bmcBlobClose, sizeof(struct BmcBlobCloseTx) - 1,
         false},
        {BlobOEMCommands::bmcBlobCommit, sizeof(struct BmcBlobCommitTx) - 1,
         false},
        {BlobOEMCommands::bmcBlobDelete, sizeof(struct BmcBlobDeleteTx) + 1,
         false},
        {BlobOEMCommands::bmcBlobEnumerate,
         sizeof(struct BmcBlobEnumerateTx) - 1, false},
        {BlobOEMCommands::bmcBlobOpen, sizeof(struct BmcBlobOpenTx) + 1, false},
        {BlobOEMCommands::bmcBlobRead, sizeof(struct BmcBlobReadTx) - 1, false},
        {BlobOEMCommands::bmcBlobSessionStat,
         sizeof(struct BmcBlobSessionStatTx) - 1, false},
        {BlobOEMCommands::bmcBlobStat, sizeof(struct BmcBlobStatTx) + 1, false},
        {BlobOEMCommands::bmcBlobWrite, sizeof(struct BmcBlobWriteTx), false},
    };

    for (const auto& test : tests)
    {
        bool result = validateRequestLength(test.cmd, test.len);
        EXPECT_EQ(result, test.expect);
    }
}
} // namespace blobs
