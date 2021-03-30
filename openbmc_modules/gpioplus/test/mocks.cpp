#include <gpioplus/test/event.hpp>
#include <gpioplus/test/handle.hpp>

namespace gpioplus
{
namespace
{

TEST(Mocks, Compile)
{
    test::EventMock event;
    test::HandleMock handle;
}

} // namespace
} // namespace gpioplus
