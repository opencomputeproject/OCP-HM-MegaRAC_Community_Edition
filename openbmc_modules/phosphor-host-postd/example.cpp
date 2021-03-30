/**
 * Copyright 2017 Google Inc.
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

#include "lpcsnoop/snoop_listen.hpp"

#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <memory>

/* Example PostCode handler which simply prints them */
static void printPostcode(uint64_t postcode)
{
    /* Print output to verify the example program is receiving values. */
    std::printf("recv: 0x%" PRIx64 "\n", postcode);
}

/*
 * One can also specify custom handler that operates on
 * sdbusplus::message::message type and pass them to constructor.
 * e.g.
 *
 * static void PrintMessageMap(sdbusplus::message::message& m)
 * {
 *     std::string messageBusName;
 *     std::map<std::string, std::variant<uint64_t>> messageData;
 *
 *     m.read(messageBusName, messageData);
 *
 *     std::cout << "Got message from " << messageBusName << std::endl;
 *     for (const auto& kv : messageData)
 *     {
 *         std::cout << "Key: " << kv.first << std::endl;
 *         std::cout << "Value: " << get<uint64_t>(kv.second) << std::endl;
 *     }
 * }
 *
 * lpcsnoop::SnoopListen snoop(ListenBus, PrintMessageMap);
 */

/*
 * This is the entry point for the application.
 *
 * This application simply creates an object that registers for incoming value
 * updates for the POST code dbus object.
 */
int main()
{
    auto ListenBus = sdbusplus::bus::new_default();
    lpcsnoop::SnoopListen snoop(ListenBus, printPostcode);

    while (true)
    {
        ListenBus.process_discard();
        ListenBus.wait();
    }

    return 0;
}
