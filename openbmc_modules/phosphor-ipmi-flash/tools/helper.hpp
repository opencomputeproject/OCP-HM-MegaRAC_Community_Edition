#pragma once

#include <ipmiblob/blob_interface.hpp>

#include <cstdint>

namespace host_tool
{

/**
 * Poll an open verification session.
 *
 * @param[in] session - the open verification session
 * @param[in] blob - pointer to blob interface implementation object.
 * @return true if the verification was successul.
 */
bool pollStatus(std::uint16_t session, ipmiblob::BlobInterface* blob);

} // namespace host_tool
