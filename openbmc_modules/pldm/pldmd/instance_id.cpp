#include "instance_id.hpp"

#include <stdexcept>

namespace pldm
{

uint8_t InstanceId::next()
{
    uint8_t idx = 0;
    while (idx < id.size() && id.test(idx))
    {
        ++idx;
    }

    if (idx == id.size())
    {
        throw std::runtime_error("No free instance ids");
    }

    id.set(idx);
    return idx;
}

} // namespace pldm
