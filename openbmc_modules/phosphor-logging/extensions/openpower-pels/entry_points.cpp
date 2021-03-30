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
#include "data_interface.hpp"
#include "elog_entry.hpp"
#include "event_logger.hpp"
#include "extensions.hpp"
#include "manager.hpp"
#include "pldm_interface.hpp"

namespace openpower
{
namespace pels
{

using namespace phosphor::logging;

std::unique_ptr<Manager> manager;

DISABLE_LOG_ENTRY_CAPS();

void pelStartup(internal::Manager& logManager)
{
    EventLogger::LogFunction logger = std::bind(
        std::mem_fn(&internal::Manager::create), &logManager,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::unique_ptr<DataInterfaceBase> dataIface =
        std::make_unique<DataInterface>(logManager.getBus());

#ifndef DONT_SEND_PELS_TO_HOST
    std::unique_ptr<HostInterface> hostIface = std::make_unique<PLDMInterface>(
        logManager.getBus().get_event(), *(dataIface.get()));

    manager =
        std::make_unique<Manager>(logManager, std::move(dataIface),
                                  std::move(logger), std::move(hostIface));
#else
    manager = std::make_unique<Manager>(logManager, std::move(dataIface),
                                        std::move(logger));
#endif
}

REGISTER_EXTENSION_FUNCTION(pelStartup);

void pelCreate(const std::string& message, uint32_t id, uint64_t timestamp,
               Entry::Level severity, const AdditionalDataArg& additionalData,
               const AssociationEndpointsArg& assocs, const FFDCArg& ffdc)
{
    manager->create(message, id, timestamp, severity, additionalData, assocs,
                    ffdc);
}

REGISTER_EXTENSION_FUNCTION(pelCreate);

void pelDelete(uint32_t id)
{
    return manager->erase(id);
}

REGISTER_EXTENSION_FUNCTION(pelDelete);

void pelDeleteProhibited(uint32_t id, bool& prohibited)
{
    prohibited = manager->isDeleteProhibited(id);
}

REGISTER_EXTENSION_FUNCTION(pelDeleteProhibited);

} // namespace pels
} // namespace openpower
