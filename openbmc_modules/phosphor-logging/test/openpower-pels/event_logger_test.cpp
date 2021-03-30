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
#include "extensions/openpower-pels/event_logger.hpp"
#include "log_manager.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;
using namespace phosphor::logging;

class CreateHelper
{
  public:
    void create(const std::string& name, Entry::Level level,
                const EventLogger::ADMap& ad)
    {
        _createCount++;
        _prevName = name;
        _prevLevel = level;
        _prevAD = ad;

        // Try to create another event from within the creation
        // function.  Should never work or else we could get stuck
        // infinitely creating events.
        if (_eventLogger)
        {
            AdditionalData d;
            _eventLogger->log(name, level, d);
        }
    }

    size_t _createCount = 0;
    std::string _prevName;
    Entry::Level _prevLevel;
    EventLogger::ADMap _prevAD;
    EventLogger* _eventLogger = nullptr;
};

void runEvents(sd_event* event, size_t numEvents)
{
    sdeventplus::Event e{event};

    for (size_t i = 0; i < numEvents; i++)
    {
        e.run(std::chrono::milliseconds(1));
    }
}

TEST(EventLoggerTest, TestCreateEvents)
{
    sd_event* sdEvent = nullptr;
    auto r = sd_event_default(&sdEvent);
    ASSERT_TRUE(r >= 0);

    CreateHelper ch;

    EventLogger eventLogger(std::bind(
        std::mem_fn(&CreateHelper::create), &ch, std::placeholders::_1,
        std::placeholders::_2, std::placeholders::_3));

    ch._eventLogger = &eventLogger;

    AdditionalData ad;
    ad.add("key1", "value1");

    eventLogger.log("one", Entry::Level::Error, ad);
    EXPECT_EQ(eventLogger.queueSize(), 1);

    runEvents(sdEvent, 1);

    // Verify 1 event was created
    EXPECT_EQ(eventLogger.queueSize(), 0);
    EXPECT_EQ(ch._prevName, "one");
    EXPECT_EQ(ch._prevLevel, Entry::Level::Error);
    EXPECT_EQ(ch._prevAD, ad.getData());
    EXPECT_EQ(ch._createCount, 1);

    // Create 2 more, and run 1 event loop at a time and check the results
    eventLogger.log("two", Entry::Level::Error, ad);
    eventLogger.log("three", Entry::Level::Error, ad);

    EXPECT_EQ(eventLogger.queueSize(), 2);

    runEvents(sdEvent, 1);

    EXPECT_EQ(ch._createCount, 2);
    EXPECT_EQ(ch._prevName, "two");
    EXPECT_EQ(eventLogger.queueSize(), 1);

    runEvents(sdEvent, 1);
    EXPECT_EQ(ch._createCount, 3);
    EXPECT_EQ(ch._prevName, "three");
    EXPECT_EQ(eventLogger.queueSize(), 0);

    // Add them all again and run them all at once
    eventLogger.log("three", Entry::Level::Error, ad);
    eventLogger.log("two", Entry::Level::Error, ad);
    eventLogger.log("one", Entry::Level::Error, ad);
    runEvents(sdEvent, 3);

    EXPECT_EQ(ch._createCount, 6);
    EXPECT_EQ(ch._prevName, "one");
    EXPECT_EQ(eventLogger.queueSize(), 0);

    // Run extra events - doesn't do anything
    runEvents(sdEvent, 1);
    EXPECT_EQ(ch._createCount, 6);
    EXPECT_EQ(ch._prevName, "one");
    EXPECT_EQ(eventLogger.queueSize(), 0);

    sd_event_unref(sdEvent);
}
