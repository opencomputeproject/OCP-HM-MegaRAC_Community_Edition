#pragma once

namespace stdplus
{
namespace signal
{

/** @brief Blocks the signal from being handled by the designated
 *         sigaction. If the signal is already blocked this does nothing.
 *
 *  @param[in] signum - The int representing the signal to block
 *  @throws std::system_error if any underlying error occurs.
 */
void block(int signum);

} // namespace signal
} // namespace stdplus
