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
#include "severity.hpp"

#include "pel_types.hpp"

namespace openpower
{
namespace pels
{

using LogSeverity = phosphor::logging::Entry::Level;

uint8_t convertOBMCSeverityToPEL(LogSeverity severity)
{
    uint8_t pelSeverity = static_cast<uint8_t>(SeverityType::unrecoverable);
    switch (severity)
    {
        case (LogSeverity::Notice):
        case (LogSeverity::Informational):
        case (LogSeverity::Debug):
            pelSeverity = static_cast<uint8_t>(SeverityType::nonError);
            break;

        case (LogSeverity::Warning):
            pelSeverity = static_cast<uint8_t>(SeverityType::predictive);
            break;

        case (LogSeverity::Critical):
            pelSeverity = static_cast<uint8_t>(SeverityType::critical);
            break;

        case (LogSeverity::Emergency):
        case (LogSeverity::Alert):
        case (LogSeverity::Error):
            pelSeverity = static_cast<uint8_t>(SeverityType::unrecoverable);
            break;
    }

    return pelSeverity;
}
} // namespace pels
} // namespace openpower
