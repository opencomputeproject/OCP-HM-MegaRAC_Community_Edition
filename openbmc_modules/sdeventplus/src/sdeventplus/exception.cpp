#include <sdeventplus/exception.hpp>
#include <system_error>

namespace sdeventplus
{

SdEventError::SdEventError(int r, const char* prefix) :
    std::system_error(r, std::generic_category(), prefix)
{
}

} // namespace sdeventplus
