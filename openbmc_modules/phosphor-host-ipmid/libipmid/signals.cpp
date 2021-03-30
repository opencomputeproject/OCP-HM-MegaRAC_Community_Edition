#include <forward_list>
#include <ipmid/api.hpp>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <vector>

using namespace phosphor::logging;

namespace
{

class SignalHandler
{
  public:
    SignalHandler(std::shared_ptr<boost::asio::io_context>& io, int sigNum) :
        signal(std::make_unique<boost::asio::signal_set>(*io, sigNum))
    {
        asyncWait();
    }

    ~SignalHandler()
    {
        // unregister with asio to unmask the signal
        signal->cancel();
        signal->clear();
    }

    void registerHandler(int prio,
                         const std::function<SignalResponse(int)>& handler)
    {
        // check for initial placement
        if (handlers.empty() || std::get<0>(handlers.front()) < prio)
        {
            handlers.emplace_front(std::make_tuple(prio, handler));
            return;
        }
        // walk the list and put it in the right place
        auto j = handlers.begin();
        for (auto i = j; i != handlers.end() && std::get<0>(*i) > prio; i++)
        {
            j = i;
        }
        handlers.emplace_after(j, std::make_tuple(prio, handler));
    }

    void handleSignal(const boost::system::error_code& ec, int sigNum)
    {
        if (ec)
        {
            log<level::ERR>("Error in common signal handler",
                            entry("SIGNAL=%d", sigNum),
                            entry("ERROR=%s", ec.message().c_str()));
            return;
        }
        for (auto h = handlers.begin(); h != handlers.end(); h++)
        {
            std::function<SignalResponse(int)>& handler = std::get<1>(*h);
            if (handler(sigNum) == SignalResponse::breakExecution)
            {
                break;
            }
        }
        // start the wait for the next signal
        asyncWait();
    }

  protected:
    void asyncWait()
    {
        signal->async_wait([this](const boost::system::error_code& ec,
                                  int sigNum) { handleSignal(ec, sigNum); });
    }

    std::forward_list<std::tuple<int, std::function<SignalResponse(int)>>>
        handlers;
    std::unique_ptr<boost::asio::signal_set> signal;
};

// SIGRTMAX is defined as a non-constexpr function call and thus cannot be used
// as an array size. Get around this by making a vector and resizing it the
// first time it is needed
std::vector<std::unique_ptr<SignalHandler>> signals;

} // namespace

void registerSignalHandler(int priority, int signalNumber,
                           const std::function<SignalResponse(int)>& handler)
{
    if (signalNumber >= SIGRTMAX)
    {
        return;
    }

    if (signals.empty())
    {
        signals.resize(SIGRTMAX);
    }

    if (!signals[signalNumber])
    {
        std::shared_ptr<boost::asio::io_context> io = getIoContext();
        signals[signalNumber] =
            std::make_unique<SignalHandler>(io, signalNumber);
    }
    signals[signalNumber]->registerHandler(priority, handler);
}
