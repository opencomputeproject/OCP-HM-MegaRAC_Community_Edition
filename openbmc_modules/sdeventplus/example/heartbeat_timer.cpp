/**
 * A simple example of a repeating timer that prints out a message for
 * each timer expiration.
 */

#include <chrono>
#include <cstdio>
#include <functional>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <stdplus/signal.hpp>
#include <string>

using sdeventplus::Clock;
using sdeventplus::ClockId;
using sdeventplus::Event;
using sdeventplus::source::Signal;

constexpr auto clockId = ClockId::RealTime;
using Timer = sdeventplus::utility::Timer<clockId>;

void intCb(Signal& signal, const struct signalfd_siginfo*)
{
    printf("Exiting\n");
    signal.get_event().exit(0);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [seconds]\n", argv[0]);
        return 1;
    }

    unsigned interval = std::stoul(argv[1]);
    fprintf(stderr, "Beating every %u seconds\n", interval);

    auto event = Event::get_default();
    Timer timer(
        event, [](Timer&) { printf("Beat\n"); },
        std::chrono::seconds{interval});
    stdplus::signal::block(SIGINT);
    Signal(event, SIGINT, intCb).set_floating(true);
    return event.loop();
}
