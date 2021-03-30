#include <chrono>
#include <cstdio>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/signal.hpp>
#include <sdeventplus/source/time.hpp>
#include <stdplus/signal.hpp>
#include <string>
#include <utility>

using sdeventplus::Event;
using sdeventplus::source::Enabled;
using sdeventplus::source::Signal;

constexpr auto clockId = sdeventplus::ClockId::RealTime;
using Clock = sdeventplus::Clock<clockId>;
using Time = sdeventplus::source::Time<clockId>;

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
    auto hbFunc = [interval](Time& source, Time::TimePoint time) {
        printf("Beat\n");

        // Time sources are oneshot and are based on an absolute time
        // we need to reconfigure the time source to go off again after the
        // configured interval and re-enable it.
        source.set_time(time + std::chrono::seconds{interval});
        source.set_enabled(Enabled::OneShot);
    };
    Time time(event, Clock(event).now(), std::chrono::seconds{1},
              std::move(hbFunc));
    stdplus::signal::block(SIGINT);
    Signal(event, SIGINT, intCb).set_floating(true);
    return event.loop();
}
