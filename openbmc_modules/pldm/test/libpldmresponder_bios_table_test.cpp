#include "libpldmresponder/bios_table.hpp"

#include <stdlib.h>

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

using namespace pldm::responder::bios;

class TestBIOSTable : public testing::Test
{
  public:
    void SetUp() override
    {
        char tmpdir[] = "/tmp/pldm_bios_table.XXXXXX";
        dir = fs::path(mkdtemp(tmpdir));
    }

    void TearDown() override
    {
        fs::remove_all(dir);
    }

    fs::path dir;
};

TEST_F(TestBIOSTable, testStoreLoad)
{
    std::vector<uint8_t> table{10, 34, 56, 100, 44, 55, 69, 21, 48, 2, 7, 82};
    fs::path file(dir / "t1");
    BIOSTable t(file.string().c_str());
    std::vector<uint8_t> out{};

    ASSERT_THROW(t.load(out), fs::filesystem_error);

    ASSERT_EQ(true, t.isEmpty());

    t.store(table);
    t.load(out);
    ASSERT_EQ(true, std::equal(table.begin(), table.end(), out.begin()));
}

TEST_F(TestBIOSTable, testLoadOntoExisting)
{
    std::vector<uint8_t> table{10, 34, 56, 100, 44, 55, 69, 21, 48, 2, 7, 82};
    fs::path file(dir / "t1");
    BIOSTable t(file.string().c_str());
    std::vector<uint8_t> out{99, 99};

    ASSERT_THROW(t.load(out), fs::filesystem_error);

    ASSERT_EQ(true, t.isEmpty());

    t.store(table);
    t.load(out);
    ASSERT_EQ(true, std::equal(table.begin(), table.end(), out.begin() + 2));
    ASSERT_EQ(out[0], 99);
    ASSERT_EQ(out[1], 99);
}
