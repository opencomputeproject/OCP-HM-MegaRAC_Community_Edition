#pragma once

#include <libpldm/pldm.h>

namespace phosphor
{
namespace dump
{
namespace pldm
{

/**
 * PLDMInterface
 *
 * Handles sending the SetNumericEffecterValue PLDM
 * command to the host to start dump offload.
 *
 */

/**
 * @brief Kicks of the SetNumericEffecterValue command to
 *        start offload the dump
 *
 * @param[in] id - The Dump Source ID.
 *
 */

void requestOffload(uint32_t id);

/**
 * @brief Reads the MCTP endpoint ID out of a file
 */
mctp_eid_t readEID();

/**
 * @brief Opens the PLDM file descriptor
 */
int open();

/**
 * @brief Closes the PLDM file descriptor
 */
void closeFD(int fd);

/**
 * @brief Returns the PLDM instance ID to use for PLDM commands
 *
 * @param[in] eid - The PLDM EID
 *
 * @return uint8_t - The instance ID
 **/
uint8_t getPLDMInstanceID(uint8_t eid);

} // namespace pldm
} // namespace dump
} // namespace phosphor
