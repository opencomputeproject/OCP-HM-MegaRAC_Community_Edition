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
#include "extensions/openpower-pels/severity.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using LogSeverity = phosphor::logging::Entry::Level;

TEST(SeverityTest, SeverityMapTest)
{
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Informational), 0x00);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Notice), 0x00);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Debug), 0x00);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Warning), 0x20);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Critical), 0x50);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Emergency), 0x40);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Alert), 0x40);
    ASSERT_EQ(convertOBMCSeverityToPEL(LogSeverity::Error), 0x40);
}
