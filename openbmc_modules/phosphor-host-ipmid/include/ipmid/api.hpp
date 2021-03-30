/*
 * Copyright © 2018 Intel Corporation
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
 *
 */
#pragma once

#define ALLOW_DEPRECATED_API 1
// make it possible to ONLY include api.hpp during the transition
#ifdef ALLOW_DEPRECATED_API
#include <ipmid/api.h>
#endif

#include <ipmid/api-types.hpp>
#include <ipmid/filter.hpp>
#include <ipmid/handler.hpp>
#include <ipmid/message/types.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

// any client can interact with the main asio context
std::shared_ptr<boost::asio::io_context> getIoContext();

// any client can interact with the main sdbus
std::shared_ptr<sdbusplus::asio::connection> getSdBus();

/**
 * @brief post some work to the async exection queue
 *
 * The IPMI daemon runs an async exection queue; this allows any function to
 * pass in work to be executed in that context
 *
 * @tparam WorkFn - a function of type void(void)
 * @param work - the callback function to be executed
 */
template <typename WorkFn>
static inline void post_work(WorkFn work)
{
    boost::asio::post(*getIoContext(), std::forward<WorkFn>(work));
}

enum class SignalResponse : int
{
    breakExecution,
    continueExecution,
};

/**
 * @brief add a signal handler
 *
 * This registers a handler to be called asynchronously via the execution
 * queue when the specified signal is received.
 *
 * Priority allows a signal handler to specify what order in the handler
 * chain it gets called. Lower priority numbers will cause the handler to
 * be executed later in the chain, while the highest priority numbers will cause
 * the handler to be executed first.
 *
 * In order to facilitate a chain of handlers, each handler in the chain will be
 * able to return breakExecution or continueExecution. Returning breakExecution
 * will break the chain and no further handlers will execute for that signal.
 * Returning continueExecution will allow lower-priority handlers to execute.
 *
 * By default, the main asio execution loop will register a low priority
 * (prioOpenBmcBase) handler for SIGINT and SIGTERM to cause the process to stop
 * on either of those signals. To prevent one of those signals from causing the
 * process to stop, simply register a higher priority handler that returns
 * breakExecution.
 *
 * @param int - priority of handler
 * @param int - signal number to wait for
 * @param handler - the callback function to be executed
 */
void registerSignalHandler(int priority, int signalNumber,
                           const std::function<SignalResponse(int)>& handler);
