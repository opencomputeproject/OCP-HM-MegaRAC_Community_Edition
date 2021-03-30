#include <phosphor-logging/test/sdjournal_mock.hpp>

namespace phosphor
{
namespace logging
{

TEST(LoggingSwapTest, BasicTestToEnsureItCompiles)
{
    SdJournalMock mockInstance;
    auto* old = SwapJouralHandler(&mockInstance);
    SwapJouralHandler(old);
}

} // namespace logging
} // namespace phosphor
