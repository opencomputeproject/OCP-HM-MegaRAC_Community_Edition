#include <cstdint>
#include <stdexcept>

namespace phosphor
{
namespace dump
{
namespace host
{
void requestOffload(uint32_t id)
{
    throw std::runtime_error("Hostdump offload method not specified");
}

} // namespace host
} // namespace dump
} // namespace phosphor
