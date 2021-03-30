/*
// Copyright (c) 2019 Intel Corporation
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

#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>
#include <set>
#include <string>
#include <vector>

class PSUSubEvent : public std::enable_shared_from_this<PSUSubEvent>
{
  public:
    PSUSubEvent(std::shared_ptr<sdbusplus::asio::dbus_interface> eventInterface,
                const std::string& path,
                std::shared_ptr<sdbusplus::asio::connection>& conn,
                boost::asio::io_service& io, const std::string& groupEventName,
                const std::string& eventName,
                std::shared_ptr<std::set<std::string>> asserts,
                std::shared_ptr<std::set<std::string>> combineEvent,
                std::shared_ptr<bool> state, const std::string& psuName);
    ~PSUSubEvent();

    std::shared_ptr<sdbusplus::asio::dbus_interface> eventInterface;
    std::shared_ptr<std::set<std::string>> asserts;
    std::shared_ptr<std::set<std::string>> combineEvent;
    std::shared_ptr<bool> assertState;
    void setupRead(void);

  private:
    int value = 0;
    int fd;
    size_t errCount;
    std::string path;
    std::string eventName;
    std::string groupEventName;
    boost::asio::deadline_timer waitTimer;
    std::shared_ptr<boost::asio::streambuf> readBuf;
    void handleResponse(const boost::system::error_code& err);
    void updateValue(const int& newValue);
    void beep(const uint8_t& beepPriority);
    static constexpr uint8_t beepPSUFailure = 2;
    boost::asio::posix::stream_descriptor inputDev;
    static constexpr unsigned int eventPollMs = 1000;
    static constexpr size_t warnAfterErrorCount = 10;
    std::string psuName;
    std::string fanName;
    std::string assertMessage;
    std::string deassertMessage;
    std::shared_ptr<sdbusplus::asio::connection> systemBus;
};

class PSUCombineEvent
{
  public:
    PSUCombineEvent(
        sdbusplus::asio::object_server& objectSever,
        std::shared_ptr<sdbusplus::asio::connection>& conn,
        boost::asio::io_service& io, const std::string& psuName,
        boost::container::flat_map<std::string, std::vector<std::string>>&
            eventPathList,
        boost::container::flat_map<
            std::string,
            boost::container::flat_map<std::string, std::vector<std::string>>>&
            groupEventPathList,
        const std::string& combineEventName);
    ~PSUCombineEvent();

    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> eventInterface;
    boost::container::flat_map<std::string,
                               std::vector<std::shared_ptr<PSUSubEvent>>>
        events;
    std::vector<std::shared_ptr<std::set<std::string>>> asserts;
    std::vector<std::shared_ptr<bool>> states;
};
