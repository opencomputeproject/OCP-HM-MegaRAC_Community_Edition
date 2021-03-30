#include "config.h"

#include "utils.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(Utils, GetPSUInventoryPath)
{
    sdbusplus::SdBusMock sdbusMock;
    auto bus = sdbusplus::get_mocked_new(&sdbusMock);

    EXPECT_CALL(sdbusMock, sd_bus_message_new_method_call(
                               _, _, _, _, _, StrEq("GetSubTreePaths")));

    EXPECT_CALL(sdbusMock, sd_bus_message_ref(IsNull()))
        .WillOnce(Return(nullptr));
    sdbusplus::message::message msg(nullptr, &sdbusMock);

    const char* path0 = "/com/example/chassis/powersupply0";
    const char* path1 = "/com/example/chassis/powersupply1";

    // std::vector
    EXPECT_CALL(sdbusMock,
                sd_bus_message_enter_container(IsNull(), 'a', StrEq("s")))
        .WillOnce(Return(1));

    // while !at_end()
    EXPECT_CALL(sdbusMock, sd_bus_message_at_end(IsNull(), 0))
        .WillOnce(Return(0))
        .WillOnce(Return(0))
        .WillOnce(Return(1)); // So it exits the loop after reading two strings.

    // std::string
    EXPECT_CALL(sdbusMock, sd_bus_message_read_basic(IsNull(), 's', _))
        .WillOnce(Invoke([&](sd_bus_message*, char, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = path0;
            return 0;
        }))
        .WillOnce(Invoke([&](sd_bus_message*, char, void* p) {
            const char** s = static_cast<const char**>(p);
            // Read the first parameter, the string.
            *s = path1;
            return 0;
        }));

    EXPECT_CALL(sdbusMock, sd_bus_message_exit_container(IsNull()))
        .WillOnce(Return(0)); /* end of std::vector */

    auto ret = utils::getPSUInventoryPath(bus);
    EXPECT_EQ(2u, ret.size());
    EXPECT_EQ(path0, ret[0]);
    EXPECT_EQ(path1, ret[1]);
}

TEST(Utils, GetVersionID)
{
    auto ret = utils::getVersionId("");
    EXPECT_EQ("", ret);

    ret = utils::getVersionId("some version");
    EXPECT_EQ(8u, ret.size());
}

TEST(Utils, IsAssociated)
{
    std::string path = "/com/example/chassis/powersupply0";
    utils::AssociationList assocs = {{ACTIVATION_FWD_ASSOCIATION,
                                      ACTIVATION_REV_ASSOCIATION,
                                      "a-different-path"}};

    EXPECT_FALSE(utils::isAssociated(path, assocs));

    assocs.emplace_back(ACTIVATION_FWD_ASSOCIATION, ACTIVATION_REV_ASSOCIATION,
                        path);
    EXPECT_TRUE(utils::isAssociated(path, assocs));
}
