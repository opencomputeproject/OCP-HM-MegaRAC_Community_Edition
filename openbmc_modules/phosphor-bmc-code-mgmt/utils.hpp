#include "config.h"

#include <sdbusplus/server.hpp>

#include <map>
#include <string>

namespace utils
{

/**
 * @brief Get the bus service
 *
 * @return the bus service as a string
 **/
std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface);

} // namespace utils
