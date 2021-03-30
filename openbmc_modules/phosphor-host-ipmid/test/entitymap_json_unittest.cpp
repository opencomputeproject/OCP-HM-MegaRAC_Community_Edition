#include "entity_map_json.hpp"

#include <ipmid/types.hpp>
#include <nlohmann/json.hpp>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ipmi
{
namespace sensor
{

namespace
{
using ::testing::IsEmpty;

TEST(ValidateJson, FailWithNonArrayReturnsEmpty)
{
    /* The entity map input json is expected to be an array of objects. */
    auto j = R"(
        {
            "id" : 1,
            "containerEntityId" : 2,
            "containerEntityInstance" : 3,
            "isList" : false,
            "isLinked" : false,
            "entities" : [
                {"id" : 1, "instance" : 2},
                {"id" : 1, "instance" : 3},
                {"id" : 1, "instance" : 4},
                {"id" : 1, "instance" : 5}
            ]
        }
    )"_json;

    EXPECT_THAT(buildJsonEntityMap(j), IsEmpty());
}

TEST(ValidateJson, FailWithMissingFieldReturnsEmpty)
{
    /* There are many required fields, let's just validate that if one is
     * missing, it returns the empty map.
     */
    auto j = R"(
        [
            {
                "id" : 1,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4},
                    {"id" : 1, "instance" : 5}
                ]
            }
        ]
    )"_json;

    EXPECT_THAT(buildJsonEntityMap(j), IsEmpty());
}

TEST(ValidateJson, AllValidEntryReturnsExpectedMap)
{
    /* Boring test where we provide completely valid information and expect the
     * resulting map contains that information.
     */
    auto j = R"(
        [
            {
                "id" : 1,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4},
                    {"id" : 1, "instance" : 5}
                ]
            }
        ]
    )"_json;

    auto map = buildJsonEntityMap(j);
    EXPECT_FALSE(map.find(1) == map.end());
    auto entry = map.find(1);
    EXPECT_EQ(entry->first, 1);

    /* TODO: someone could write an equality operator for this object. */
    EXPECT_EQ(entry->second.containerEntityId, 2);
    EXPECT_EQ(entry->second.containerEntityInstance, 3);
    EXPECT_FALSE(entry->second.isList);
    EXPECT_FALSE(entry->second.isLinked);
    ContainedEntitiesArray expected = {
        std::make_pair(1, 2), std::make_pair(1, 3), std::make_pair(1, 4),
        std::make_pair(1, 5)};
    EXPECT_EQ(entry->second.containedEntities, expected);
}

TEST(ValidateJson, EntryHasInsufficientContainerEntryCountReturnsEmpty)
{
    /* The container must have four pairs. (I don't know why, and maybe this
     * restriction will change).
     */
    auto j = R"(
        [
            {
                "id" : 1,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4}
                ]
            }
        ]
    )"_json;

    EXPECT_THAT(buildJsonEntityMap(j), IsEmpty());
}

TEST(ValidateJson, ThereAreTwoEntriesOneInvalidReturnsEmpty)
{
    /* If any entry in the file is corrupt, the file is disregarded. */
    auto j = R"(
        [
            {
                "id" : 1,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4},
                    {"id" : 1, "instance" : 5}
                ]
            },
            {
                "id" : 2,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4}
                ]
            }
        ]
    )"_json;

    EXPECT_THAT(buildJsonEntityMap(j), IsEmpty());
}

TEST(ValidateJson, ThereAreTwoEntriesBothValidReturnsBoth)
{
    /* The map supports more than one entry, just validate this. */
    auto j = R"(
        [
            {
                "id" : 1,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 2},
                    {"id" : 1, "instance" : 3},
                    {"id" : 1, "instance" : 4},
                    {"id" : 1, "instance" : 5}
                ]
            },
            {
                "id" : 2,
                "containerEntityId" : 2,
                "containerEntityInstance" : 3,
                "isList" : false,
                "isLinked" : false,
                "entities" : [
                    {"id" : 1, "instance" : 6},
                    {"id" : 1, "instance" : 7},
                    {"id" : 1, "instance" : 8},
                    {"id" : 1, "instance" : 9}
                ]
            }
        ]
    )"_json;

    auto map = buildJsonEntityMap(j);
    EXPECT_FALSE(map.find(1) == map.end());
    EXPECT_FALSE(map.find(2) == map.end());

    auto entry = map.find(1);
    EXPECT_EQ(entry->first, 1);
    EXPECT_EQ(entry->second.containerEntityId, 2);
    EXPECT_EQ(entry->second.containerEntityInstance, 3);
    EXPECT_FALSE(entry->second.isList);
    EXPECT_FALSE(entry->second.isLinked);
    ContainedEntitiesArray expected = {
        std::make_pair(1, 2), std::make_pair(1, 3), std::make_pair(1, 4),
        std::make_pair(1, 5)};
    EXPECT_EQ(entry->second.containedEntities, expected);

    entry = map.find(2);
    expected = {std::make_pair(1, 6), std::make_pair(1, 7),
                std::make_pair(1, 8), std::make_pair(1, 9)};
    EXPECT_EQ(entry->second.containedEntities, expected);
}

} // namespace
} // namespace sensor
} // namespace ipmi
