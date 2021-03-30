#include <signal.h>
#include <stdplus/signal.hpp>
#include <stdplus/util/cexec.hpp>
#include <system_error>

namespace stdplus
{
namespace signal
{

using util::callCheckErrno;

void block(int signum)
{
    sigset_t set;
    callCheckErrno("sigprocmask get", sigprocmask, SIG_BLOCK, nullptr, &set);
    callCheckErrno("sigaddset", sigaddset, &set, signum);
    callCheckErrno("sigprocmask set", sigprocmask, SIG_BLOCK, &set, nullptr);
}

} // namespace signal
} // namespace stdplus
