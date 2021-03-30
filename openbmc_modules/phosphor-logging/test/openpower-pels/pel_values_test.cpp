/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "extensions/openpower-pels/pel_values.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace openpower::pels::pel_values;

TEST(PELFieldsTest, TestFindFields)
{
    auto s = findByValue(0x5D, subsystemValues);
    ASSERT_NE(s, subsystemValues.end());
    EXPECT_EQ(0x5D, std::get<fieldValuePos>(*s));
    EXPECT_STREQ("cec_service_network", std::get<registryNamePos>(*s));

    s = findByName("cec_clocks", subsystemValues);
    ASSERT_NE(s, subsystemValues.end());
    EXPECT_EQ(0x58, std::get<fieldValuePos>(*s));
    EXPECT_STREQ("cec_clocks", std::get<registryNamePos>(*s));
    EXPECT_STREQ("CEC Hardware: Clock", std::get<descriptionPos>(*s));

    s = findByValue(0xFF, subsystemValues);
    EXPECT_EQ(s, subsystemValues.end());

    s = findByName("foo", subsystemValues);
    EXPECT_EQ(s, subsystemValues.end());
}
