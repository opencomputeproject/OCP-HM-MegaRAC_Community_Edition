#include "bmc_epoch.hpp"

#include "utils.hpp"

#include <sys/timerfd.h>
#include <unistd.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

// Need to do this since its not exported outside of the kernel.
// Refer : https://gist.github.com/lethean/446cea944b7441228298
#ifndef TFD_TIMER_CANCEL_ON_SET
#define TFD_TIMER_CANCEL_ON_SET (1 << 1)
#endif

// Needed to make sure timerfd does not misfire even though we set CANCEL_ON_SET
#define TIME_T_MAX (time_t)((1UL << ((sizeof(time_t) << 3) - 1)) - 1)

namespace phosphor
{
namespace time
{
namespace server = sdbusplus::xyz::openbmc_project::Time::server;
using namespace phosphor::logging;

BmcEpoch::BmcEpoch(sdbusplus::bus::bus& bus, const char* objPath) :
    EpochBase(bus, objPath), bus(bus)
{
    initialize();
}

void BmcEpoch::initialize()
{
    using InternalFailure =
        sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

    // Subscribe time change event
    // Choose the MAX time that is possible to avoid mis fires.
    constexpr itimerspec maxTime = {
        {0, 0},          // it_interval
        {TIME_T_MAX, 0}, // it_value
    };

    timeFd = timerfd_create(CLOCK_REALTIME, 0);
    if (timeFd == -1)
    {
        log<level::ERR>("Failed to create timerfd", entry("ERRNO=%d", errno),
                        entry("ERR=%s", strerror(errno)));
        elog<InternalFailure>();
    }

    auto r = timerfd_settime(
        timeFd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &maxTime, nullptr);
    if (r != 0)
    {
        log<level::ERR>("Failed to set timerfd", entry("ERRNO=%d", errno),
                        entry("ERR=%s", strerror(errno)));
        elog<InternalFailure>();
    }

    sd_event_source* es;
    r = sd_event_add_io(bus.get_event(), &es, timeFd, EPOLLIN, onTimeChange,
                        this);
    if (r < 0)
    {
        log<level::ERR>("Failed to add event", entry("ERRNO=%d", -r),
                        entry("ERR=%s", strerror(-r)));
        elog<InternalFailure>();
    }
    timeChangeEventSource.reset(es);
}

BmcEpoch::~BmcEpoch()
{
    close(timeFd);
}

uint64_t BmcEpoch::elapsed() const
{
    return getTime().count();
}

uint64_t BmcEpoch::elapsed(uint64_t value)
{
    /*
        Mode  | Set BMC Time
        ----- | -------------
        NTP   | Fail to set
        MANUAL| OK
    */
    auto time = microseconds(value);
    setTime(time);

    server::EpochTime::elapsed(value);
    return value;
}

int BmcEpoch::onTimeChange(sd_event_source* es, int fd, uint32_t /* revents */,
                           void* userdata)
{
    std::array<char, 64> time{};

    // We are not interested in the data here.
    // So read until there is no new data here in the FD
    while (read(fd, time.data(), time.max_size()) > 0)
        ;

    return 0;
}

} // namespace time
} // namespace phosphor
