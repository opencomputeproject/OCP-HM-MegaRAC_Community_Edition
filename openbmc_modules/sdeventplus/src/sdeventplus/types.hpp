#pragma once

namespace sdeventplus
{
namespace internal
{

/* @brief Indicates that the container should not own the underlying
 *        sd_event primative */
struct NoOwn
{
};

} // namespace internal
} // namespace sdeventplus
