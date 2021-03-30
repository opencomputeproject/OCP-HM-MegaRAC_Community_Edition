#include "elog_entry.hpp"
#include "elog_serialize.hpp"
#include "serialization_tests.hpp"

namespace phosphor
{
namespace logging
{
namespace test
{

TEST_F(TestSerialization, testPath)
{
    auto id = 99;
    auto e = std::make_unique<Entry>(
        bus, std::string(OBJ_ENTRY) + '/' + std::to_string(id), id, manager);
    auto path = serialize(*e, TestSerialization::dir);
    EXPECT_EQ(path.c_str(), TestSerialization::dir / std::to_string(id));
}

} // namespace test
} // namespace logging
} // namespace phosphor
