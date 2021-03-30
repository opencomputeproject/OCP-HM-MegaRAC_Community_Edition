/**
 * Reads stdin looking for a string, and coalesces that buffer until stdin
 * is calm for the passed in number of seconds.
 */

#include <array>
#include <chrono>
#include <cstdio>
#include <sdeventplus/clock.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/io.hpp>
#include <sdeventplus/utility/timer.hpp>
#include <string>
#include <unistd.h>
#include <utility>

using sdeventplus::Clock;
using sdeventplus::ClockId;
using sdeventplus::Event;
using sdeventplus::source::IO;

constexpr auto clockId = ClockId::RealTime;
using Timer = sdeventplus::utility::Timer<clockId>;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [seconds]\n", argv[0]);
        return 1;
    }

    std::chrono::seconds delay(std::stoul(argv[1]));

    auto event = Event::get_default();

    std::string content;
    auto timerCb = [&](Timer&) {
        printf("%s", content.c_str());
        content.clear();
    };
    Timer timer(event, std::move(timerCb));

    auto ioCb = [&](IO&, int fd, uint32_t) {
        std::array<char, 4096> buffer;
        ssize_t bytes = read(fd, buffer.data(), buffer.size());
        if (bytes <= 0)
        {
            printf("%s", content.c_str());
            event.exit(bytes < 0);
            return;
        }
        content.append(buffer.data(), bytes);
        timer.restartOnce(delay);
    };
    IO ioSource(event, STDIN_FILENO, EPOLLIN, std::move(ioCb));

    return event.loop();
}
