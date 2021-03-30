#pragma once

#include <system_error>

namespace sdeventplus
{

/** @class SdEventError
 *  @brief Holds information about underlying sd_event
 *         issued errors
 */
class SdEventError final : public std::system_error
{
  public:
    /** @brief Creates a new SdEventError from error data
     *
     * @param[in] r      - The positive errno code
     * @param[in] prefix - The prefix string to display in the what()
     */
    SdEventError(int r, const char* prefix);
};

} // namespace sdeventplus
