/**
 * Copyright © 2017 IBM Corporation
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
#include "names_values.hpp"

#include <gtest/gtest.h>

TEST(NamesValuesTest, TestValues)
{
    witherspoon::power::util::NamesValues nv;

    std::string expected;
    EXPECT_EQ(nv.get(), expected); // empty

    nv.add("name1", 0);
    nv.add("name2", 0xC0FFEE);
    nv.add("name3", 0x12345678abcdef12);
    nv.add("name4", 0x0000000001);

    expected = "name1=0x0|name2=0xc0ffee|name3=0x12345678abcdef12|name4=0x1";

    EXPECT_EQ(nv.get(), expected);
}
