#include "extensions.hpp"

namespace phosphor
{
namespace logging
{

StartupFunctions Extensions::startupFunctions{};
CreateFunctions Extensions::createFunctions{};
DeleteFunctions Extensions::deleteFunctions{};
DeleteProhibitedFunctions Extensions::deleteProhibitedFunctions{};
Extensions::DefaultErrorCaps Extensions::defaultErrorCaps =
    Extensions::DefaultErrorCaps::enable;

} // namespace logging
} // namespace phosphor
