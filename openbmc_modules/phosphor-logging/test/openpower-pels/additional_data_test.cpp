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
#include "extensions/openpower-pels/additional_data.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(AdditionalDataTest, GetKeywords)
{
    std::vector<std::string> data{"KEY1=VALUE1", "KEY2=VALUE2",
                                  "KEY3=", "HELLOWORLD", "=VALUE5"};
    AdditionalData ad{data};

    EXPECT_TRUE(ad.getValue("KEY1"));
    EXPECT_EQ(*(ad.getValue("KEY1")), "VALUE1");

    EXPECT_TRUE(ad.getValue("KEY2"));
    EXPECT_EQ(*(ad.getValue("KEY2")), "VALUE2");

    EXPECT_FALSE(ad.getValue("x"));

    auto value3 = ad.getValue("KEY3");
    EXPECT_TRUE(value3);
    EXPECT_TRUE((*value3).empty());

    EXPECT_FALSE(ad.getValue("HELLOWORLD"));
    EXPECT_FALSE(ad.getValue("VALUE5"));

    auto json = ad.toJSON();
    std::string expected = R"({"KEY1":"VALUE1","KEY2":"VALUE2","KEY3":""})";
    EXPECT_EQ(json.dump(), expected);

    ad.remove("KEY1");
    EXPECT_FALSE(ad.getValue("KEY1"));
}

TEST(AdditionalDataTest, AddData)
{
    AdditionalData ad;

    ad.add("KEY1", "VALUE1");
    EXPECT_EQ(*(ad.getValue("KEY1")), "VALUE1");

    ad.add("KEY2", "VALUE2");
    EXPECT_EQ(*(ad.getValue("KEY2")), "VALUE2");

    std::map<std::string, std::string> expected{{"KEY1", "VALUE1"},
                                                {"KEY2", "VALUE2"}};

    EXPECT_EQ(expected, ad.getData());
}
