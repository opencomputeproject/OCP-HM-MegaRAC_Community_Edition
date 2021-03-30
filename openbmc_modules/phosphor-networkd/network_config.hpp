#include <string>

namespace phosphor
{
namespace network
{

namespace bmc
{
void writeDHCPDefault(const std::string& filename,
                      const std::string& interface);
}

} // namespace network
} // namespace phosphor
