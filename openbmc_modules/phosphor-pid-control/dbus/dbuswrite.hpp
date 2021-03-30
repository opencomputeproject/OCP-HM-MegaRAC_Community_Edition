/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include "interfaces.hpp"
#include "util.hpp"

#include <memory>
#include <sdbusplus/bus.hpp>
#include <string>

class DbusWritePercent : public WriteInterface
{
  public:
    static std::unique_ptr<WriteInterface>
        createDbusWrite(const std::string& path, int64_t min, int64_t max,
                        DbusHelperInterface& helper);

    DbusWritePercent(const std::string& path, int64_t min, int64_t max,
                     const std::string& connectionName) :
        WriteInterface(min, max),
        path(path), connectionName(connectionName)
    {
    }

    void write(double value) override;

  private:
    std::string path;
    std::string connectionName;
    int64_t oldValue = -1;
};

class DbusWrite : public WriteInterface
{
  public:
    static std::unique_ptr<WriteInterface>
        createDbusWrite(const std::string& path, int64_t min, int64_t max,
                        DbusHelperInterface& helper);

    DbusWrite(const std::string& path, int64_t min, int64_t max,
              const std::string& connectionName) :
        WriteInterface(min, max),
        path(path), connectionName(connectionName)
    {
    }

    void write(double value) override;

  private:
    std::string path;
    std::string connectionName;
    int64_t oldValue = -1;
};
