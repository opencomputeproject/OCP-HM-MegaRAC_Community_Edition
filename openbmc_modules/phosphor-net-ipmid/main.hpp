#pragma once

#include "command/guid.hpp"
#include "sd_event_loop.hpp"
#include "sol/sol_manager.hpp"

#include <command_table.hpp>
#include <cstddef>
#include <sdbusplus/asio/connection.hpp>
#include <sessions_manager.hpp>
#include <tuple>

extern std::tuple<session::Manager&, command::Table&, eventloop::EventLoop&,
                  sol::Manager&>
    singletonPool;

// Select call timeout is set arbitrarily set to 30 sec
static constexpr size_t SELECT_CALL_TIMEOUT = 30;
static const auto IPMI_STD_PORT = 623;

extern sd_bus* bus;

std::shared_ptr<sdbusplus::asio::connection> getSdBus();
