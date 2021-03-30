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
#include "pldm_interface.hpp"

#include "dump_utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <libpldm/base.h>
#include <libpldm/platform.h>
#include <unistd.h>

#include <fstream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

namespace phosphor
{
namespace dump
{
namespace host
{
/**
 * @brief Initiate offload of the dump with provided id
 *
 * @param[in] id - The Dump Source ID.
 *
 */
void requestOffload(uint32_t id)
{
    pldm::requestOffload(id);
}
} // namespace host

namespace pldm
{

using namespace phosphor::logging;

constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;

using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

void closeFD(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

mctp_eid_t readEID()
{
    mctp_eid_t eid = defaultEIDValue;

    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        log<level::ERR>("Could not open host EID file");
        elog<InternalFailure>();
    }
    else
    {
        std::string eid;
        eidFile >> eid;
        if (!eid.empty())
        {
            eid = strtol(eid.c_str(), nullptr, 10);
        }
        else
        {
            log<level::ERR>("EID file was empty");
            elog<InternalFailure>();
        }
    }

    return eid;
}

int open()
{
    auto fd = pldm_open();
    if (fd < 0)
    {
        auto e = errno;
        log<level::ERR>("pldm_open failed", entry("ERRNO=%d", e),
                        entry("FD=%d\n", fd));
        elog<InternalFailure>();
    }
    return fd;
}

void requestOffload(uint32_t id)
{
    uint16_t effecterId = 0x05; // TODO PhyP temporary Hardcoded value.

    std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(effecterId) + sizeof(id) +
                            sizeof(uint8_t)>
        requestMsg{};
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

    std::array<uint8_t, sizeof(id)> effecterValue{};

    memcpy(effecterValue.data(), &id, sizeof(id));

    mctp_eid_t eid = readEID();

    auto instanceID = getPLDMInstanceID(eid);

    auto rc = encode_set_numeric_effecter_value_req(
        instanceID, effecterId, PLDM_EFFECTER_DATA_SIZE_UINT32,
        effecterValue.data(), request,
        requestMsg.size() - sizeof(pldm_msg_hdr));

    if (rc != PLDM_SUCCESS)
    {
        log<level::ERR>("Message encode failure. ", entry("RC=%d", rc));
        elog<InternalFailure>();
    }

    uint8_t* responseMsg = nullptr;
    size_t responseMsgSize{};

    auto fd = open();

    rc = pldm_send_recv(eid, fd, requestMsg.data(), requestMsg.size(),
                        &responseMsg, &responseMsgSize);
    if (rc < 0)
    {
        closeFD(fd);
        auto e = errno;
        log<level::ERR>("pldm_send failed", entry("RC=%d", rc),
                        entry("ERRNO=%d", e));
        elog<InternalFailure>();
    }
    pldm_msg* response = reinterpret_cast<pldm_msg*>(responseMsg);
    log<level::INFO>(
        "Done. PLDM message",
        entry("RC=%d", static_cast<uint16_t>(response->payload[0])));

    closeFD(fd);
}

uint8_t getPLDMInstanceID(uint8_t eid)
{

    constexpr auto pldmRequester = "xyz.openbmc_project.PLDM.Requester";
    constexpr auto pldm = "/xyz/openbmc_project/pldm";

    auto bus = sdbusplus::bus::new_default();
    auto service = phosphor::dump::getService(bus, pldm, pldmRequester);

    auto method = bus.new_method_call(service.c_str(), pldm, pldmRequester,
                                      "GetInstanceId");
    method.append(eid);
    auto reply = bus.call(method);

    uint8_t instanceID = 0;
    reply.read(instanceID);

    return instanceID;
}
} // namespace pldm
} // namespace dump
} // namespace phosphor
