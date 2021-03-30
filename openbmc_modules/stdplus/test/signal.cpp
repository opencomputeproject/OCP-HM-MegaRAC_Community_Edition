#include <catch2/catch.hpp>
#include <cstring>
#include <signal.h>
#include <stdplus/signal.hpp>

namespace stdplus
{
namespace signal
{
namespace
{

TEST_CASE("Signals are blocked", "[signal]")
{
    constexpr int s = SIGINT;
    constexpr int otherS = SIGTERM;
    constexpr int notBlocked = SIGPROF;

    sigset_t expectedSet;
    REQUIRE(0 == sigprocmask(SIG_BLOCK, nullptr, &expectedSet));
    REQUIRE(0 == sigaddset(&expectedSet, otherS));
    REQUIRE(0 == sigprocmask(SIG_BLOCK, &expectedSet, nullptr));
    REQUIRE(0 == sigismember(&expectedSet, notBlocked));
    REQUIRE(0 == sigismember(&expectedSet, s));
    REQUIRE(0 == sigaddset(&expectedSet, s));

    block(s);

    sigset_t newSet;
    REQUIRE(0 == sigprocmask(SIG_BLOCK, nullptr, &newSet));
    REQUIRE(sigismember(&expectedSet, s) == sigismember(&newSet, s));
    REQUIRE(sigismember(&expectedSet, otherS) == sigismember(&newSet, otherS));
    REQUIRE(sigismember(&expectedSet, notBlocked) ==
            sigismember(&newSet, notBlocked));
}

TEST_CASE("Signals stay blocked if already blocked", "[signal]")
{
    constexpr int s = SIGINT;
    constexpr int otherS = SIGTERM;
    constexpr int notBlocked = SIGPROF;

    sigset_t expectedSet;
    REQUIRE(0 == sigprocmask(SIG_BLOCK, nullptr, &expectedSet));
    REQUIRE(0 == sigaddset(&expectedSet, s));
    REQUIRE(0 == sigaddset(&expectedSet, otherS));
    REQUIRE(0 == sigismember(&expectedSet, notBlocked));
    REQUIRE(0 == sigprocmask(SIG_BLOCK, &expectedSet, nullptr));

    block(s);

    sigset_t newSet;
    REQUIRE(0 == sigprocmask(SIG_BLOCK, nullptr, &newSet));
    REQUIRE(sigismember(&expectedSet, s) == sigismember(&newSet, s));
    REQUIRE(sigismember(&expectedSet, otherS) == sigismember(&newSet, otherS));
    REQUIRE(sigismember(&expectedSet, notBlocked) ==
            sigismember(&newSet, notBlocked));
}

} // namespace
} // namespace signal
} // namespace stdplus
