/*
 * Copyright 2019 Google Inc.
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

#include "helper.hpp"

#include "status.hpp"
#include "tool_errors.hpp"

#include <ipmiblob/blob_errors.hpp>

#include <chrono>
#include <thread>

namespace host_tool
{

/* Poll an open verification session.  Handling closing the session is not yet
 * owned by this method.
 */
bool pollStatus(std::uint16_t session, ipmiblob::BlobInterface* blob)
{
    using namespace std::chrono_literals;

    static constexpr auto verificationSleep = 5s;
    ipmi_flash::ActionStatus result = ipmi_flash::ActionStatus::unknown;

    try
    {
        /* sleep for 5 seconds and check 360 times, for a timeout of: 1800
         * seconds (30 minutes).
         * TODO: make this command line configurable and provide smaller
         * default value.
         */
        static constexpr int commandAttempts = 360;
        int attempts = 0;
        bool exitLoop = false;

        /* Reach back the current status from the verification service output.
         */
        while (attempts++ < commandAttempts)
        {
            ipmiblob::StatResponse resp = blob->getStat(session);

            if (resp.metadata.size() != sizeof(std::uint8_t))
            {
                /* TODO: How do we want to handle the verification failures,
                 * because closing the session to the verify blob has a special
                 * as-of-yet not fully defined behavior.
                 */
                std::fprintf(stderr, "Received invalid metadata response!!!\n");
            }

            result = static_cast<ipmi_flash::ActionStatus>(resp.metadata[0]);

            switch (result)
            {
                case ipmi_flash::ActionStatus::failed:
                    std::fprintf(stderr, "failed\n");
                    exitLoop = true;
                    break;
                case ipmi_flash::ActionStatus::unknown:
                    std::fprintf(stderr, "other\n");
                    break;
                case ipmi_flash::ActionStatus::running:
                    std::fprintf(stderr, "running\n");
                    break;
                case ipmi_flash::ActionStatus::success:
                    std::fprintf(stderr, "success\n");
                    exitLoop = true;
                    break;
                default:
                    std::fprintf(stderr, "wat\n");
            }

            if (exitLoop)
            {
                break;
            }
            std::this_thread::sleep_for(verificationSleep);
        }
    }
    catch (const ipmiblob::BlobException& b)
    {
        throw ToolException("blob exception received: " +
                            std::string(b.what()));
    }

    /* TODO: If this is reached and it's not success, it may be worth just
     * throwing a ToolException with a timeout message specifying the final
     * read's value.
     *
     * TODO: Given that excepting from certain points leaves the BMC update
     * state machine in an inconsistent state, we need to carefully evaluate
     * which exceptions from the lower layers allow one to try and delete the
     * blobs to rollback the state and progress.
     */
    return (result == ipmi_flash::ActionStatus::success);
}

} // namespace host_tool
