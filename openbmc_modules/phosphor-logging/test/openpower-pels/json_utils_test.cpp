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
#include "extensions/openpower-pels/json_utils.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

TEST(JsonUtilsTest, TrimEndTest)
{
    std::string testStr("Test string 1");
    EXPECT_EQ(trimEnd(testStr), "Test string 1");
    testStr = "Test string 2 ";
    EXPECT_EQ(trimEnd(testStr), "Test string 2");
    testStr = " Test string 3  ";
    EXPECT_EQ(trimEnd(testStr), " Test string 3");
}

TEST(JsonUtilsTest, NumberToStringTest)
{
    size_t number = 123;
    EXPECT_EQ(getNumberString("%d", number), "123");
    EXPECT_EQ(getNumberString("%03X", number), "07B");
    EXPECT_EQ(getNumberString("0x%X", number), "0x7B");
    ASSERT_EXIT((getNumberString("%123", number), exit(0)),
                ::testing::KilledBySignal(SIGSEGV), ".*");
}

TEST(JsonUtilsTest, JsonInsertTest)
{
    std::string json;
    jsonInsert(json, "Key", "Value1", 1);
    EXPECT_EQ(json, "    \"Key\":                      \"Value1\",\n");
    jsonInsert(json, "Keyxxxxxxxxxxxxxxxxxxxxxxxxxx", "Value2", 2);
    EXPECT_EQ(json, "    \"Key\":                      \"Value1\",\n"
                    "        \"Keyxxxxxxxxxxxxxxxxxxxxxxxxxx\": \"Value2\",\n");
}
