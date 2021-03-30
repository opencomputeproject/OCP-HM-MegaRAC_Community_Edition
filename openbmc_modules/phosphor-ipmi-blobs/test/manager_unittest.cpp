#include "blob_mock.hpp"
#include "manager.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace blobs
{

using ::testing::Return;

TEST(BlobsTest, RegisterNullPointerFails)
{
    // The only invalid pointer really is a null one.

    BlobManager mgr;
    EXPECT_FALSE(mgr.registerHandler(nullptr));
}

TEST(BlobsTest, RegisterNonNullPointerPasses)
{
    // Test that the valid pointer is boringly registered.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));
}

TEST(BlobsTest, GetCountNoBlobsRegistered)
{
    // Request the Blob Count when there are no blobs.

    BlobManager mgr;
    EXPECT_EQ(0, mgr.buildBlobList());
}

TEST(BlobsTest, GetCountBlobRegisteredReturnsOne)
{
    // Request the blob count and verify the list is of length one.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    std::vector<std::string> v = {"item"};

    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));

    // We expect it to ask for the list.
    EXPECT_CALL(*m1ptr, getBlobIds()).WillOnce(Return(v));

    EXPECT_EQ(1, mgr.buildBlobList());
}

TEST(BlobsTest, GetCountBlobsRegisteredEachReturnsOne)
{
    // Request the blob count and verify the list is of length two.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    std::unique_ptr<BlobMock> m2 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    auto m2ptr = m2.get();
    std::vector<std::string> v1, v2;

    v1.push_back("asdf");
    v2.push_back("ghjk");

    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));
    EXPECT_TRUE(mgr.registerHandler(std::move(m2)));

    // We expect it to ask for the list.
    EXPECT_CALL(*m1ptr, getBlobIds()).WillOnce(Return(v1));
    EXPECT_CALL(*m2ptr, getBlobIds()).WillOnce(Return(v2));

    EXPECT_EQ(2, mgr.buildBlobList());
}

TEST(BlobsTest, EnumerateBlobZerothEntry)
{
    // Validate that you can read back the 0th blobId.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    std::unique_ptr<BlobMock> m2 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    auto m2ptr = m2.get();
    std::vector<std::string> v1, v2;

    v1.push_back("asdf");
    v2.push_back("ghjk");

    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));
    EXPECT_TRUE(mgr.registerHandler(std::move(m2)));

    // We expect it to ask for the list.
    EXPECT_CALL(*m1ptr, getBlobIds()).WillOnce(Return(v1));
    EXPECT_CALL(*m2ptr, getBlobIds()).WillOnce(Return(v2));

    EXPECT_EQ(2, mgr.buildBlobList());

    std::string result = mgr.getBlobId(0);
    // The exact order the blobIds is returned is not guaranteed to never
    // change.
    EXPECT_TRUE("asdf" == result || "ghjk" == result);
}

TEST(BlobsTest, EnumerateBlobFirstEntry)
{
    // Validate you can read back the two real entries.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    std::unique_ptr<BlobMock> m2 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    auto m2ptr = m2.get();
    std::vector<std::string> v1, v2;

    v1.push_back("asdf");
    v2.push_back("ghjk");

    // Presently the list of blobs is read and appended in a specific order,
    // but I don't want to rely on that.
    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));
    EXPECT_TRUE(mgr.registerHandler(std::move(m2)));

    // We expect it to ask for the list.
    EXPECT_CALL(*m1ptr, getBlobIds()).WillOnce(Return(v1));
    EXPECT_CALL(*m2ptr, getBlobIds()).WillOnce(Return(v2));

    EXPECT_EQ(2, mgr.buildBlobList());

    // Try to grab the two blobIds and verify they're in the list.
    std::vector<std::string> results;
    results.push_back(mgr.getBlobId(0));
    results.push_back(mgr.getBlobId(1));
    EXPECT_EQ(2, results.size());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "asdf") !=
                results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), "ghjk") !=
                results.end());
}

TEST(BlobTest, EnumerateBlobInvalidEntry)
{
    // Validate trying to read an invalid entry fails expectedly.

    BlobManager mgr;
    std::unique_ptr<BlobMock> m1 = std::make_unique<BlobMock>();
    std::unique_ptr<BlobMock> m2 = std::make_unique<BlobMock>();
    auto m1ptr = m1.get();
    auto m2ptr = m2.get();
    std::vector<std::string> v1, v2;

    v1.push_back("asdf");
    v2.push_back("ghjk");

    EXPECT_TRUE(mgr.registerHandler(std::move(m1)));
    EXPECT_TRUE(mgr.registerHandler(std::move(m2)));

    // We expect it to ask for the list.
    EXPECT_CALL(*m1ptr, getBlobIds()).WillOnce(Return(v1));
    EXPECT_CALL(*m2ptr, getBlobIds()).WillOnce(Return(v2));

    EXPECT_EQ(2, mgr.buildBlobList());

    // Grabs the third entry which isn't valid.
    EXPECT_STREQ("", mgr.getBlobId(2).c_str());
}
} // namespace blobs
