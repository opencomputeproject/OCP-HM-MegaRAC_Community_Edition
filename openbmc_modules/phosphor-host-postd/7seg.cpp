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

#include <cstdint>
#include <cstdio>
#include <experimental/filesystem>
#include <lpcsnoop/snoop_listen.hpp>
#include <string>

static const char* device_node_path;

namespace fs = std::experimental::filesystem;

static void DisplayDbusValue(uint64_t postcode)
{
    // Uses cstdio instead of streams because the device file has
    // very strict requirements about the data format and streaming
    // abstractions tend to muck it up.
    FILE* f = std::fopen(device_node_path, "r+");
    if (f)
    {
        int rc = std::fprintf(f, "%d%02x\n", (postcode > 0xff),
                              static_cast<uint8_t>(postcode & 0xff));
        if (rc < 0)
        {
            std::fprintf(stderr, "failed to write 7seg value: rc=%d\n", rc);
        }
        std::fflush(f);
        std::fclose(f);
    }
}

/*
 * This is the entry point for the application.
 *
 * This application simply creates an object that registers for incoming value
 * updates for the POST code dbus object.
 */
int main(int argc, const char* argv[])
{
    if (argc != 2 || !fs::exists(argv[1]))
    {
        std::fprintf(stderr, "usage: %s <device_node>\n", argv[0]);
        return -1;
    }

    device_node_path = argv[1];

    auto ListenBus = sdbusplus::bus::new_default();
    std::unique_ptr<lpcsnoop::SnoopListen> snoop =
        std::make_unique<lpcsnoop::SnoopListen>(ListenBus, DisplayDbusValue);

    while (true)
    {
        ListenBus.process_discard();
        ListenBus.wait();
    }

    return 0;
}
