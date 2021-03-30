#include "elog_entry.hpp"
#include "elog_serialize.hpp"
#include "serialization_tests.hpp"

namespace phosphor
{
namespace logging
{
namespace test
{

TEST_F(TestSerialization, testProperties)
{
    auto id = 99;
    phosphor::logging::AssociationList assocations{};
    std::vector<std::string> testData{"additional", "data"};
    uint64_t timestamp{100};
    std::string message{"test error"};
    std::string fwLevel{"level42"};
    auto input = std::make_unique<Entry>(
        bus, std::string(OBJ_ENTRY) + '/' + std::to_string(id), id, timestamp,
        Entry::Level::Informational, std::move(message), std::move(testData),
        std::move(assocations), fwLevel, manager);
    auto path = serialize(*input, TestSerialization::dir);

    auto idStr = path.filename().c_str();
    id = std::stol(idStr);
    auto output = std::make_unique<Entry>(
        bus, std::string(OBJ_ENTRY) + '/' + idStr, id, manager);
    deserialize(path, *output);

    EXPECT_EQ(input->id(), output->id());
    EXPECT_EQ(input->severity(), output->severity());
    EXPECT_EQ(input->timestamp(), output->timestamp());
    EXPECT_EQ(input->message(), output->message());
    EXPECT_EQ(input->additionalData(), output->additionalData());
    EXPECT_EQ(input->resolved(), output->resolved());
    EXPECT_EQ(input->associations(), output->associations());
    EXPECT_EQ(input->version(), output->version());
    EXPECT_EQ(input->purpose(), output->purpose());
    EXPECT_EQ(input->updateTimestamp(), output->updateTimestamp());
}

} // namespace test
} // namespace logging
} // namespace phosphor
