#pragma once

#include <systemd/sd-journal.h>

#include <cstdarg>

namespace phosphor
{
namespace logging
{

/**
 * Implementation that calls into real sd_journal methods.
 */
class SdJournalHandler
{
  public:
    SdJournalHandler() = default;
    virtual ~SdJournalHandler() = default;
    SdJournalHandler(const SdJournalHandler&) = default;
    SdJournalHandler& operator=(const SdJournalHandler&) = default;
    SdJournalHandler(SdJournalHandler&&) = default;
    SdJournalHandler& operator=(SdJournalHandler&&) = default;

    /**
     * Provide a fake method that's called by the real method so we can catch
     * the journal_send call in testing.
     *
     * @param[in] fmt - the format string passed into journal_send.
     * @return an int meant to be intepreted by the journal_send caller during
     * testing.
     */
    virtual int journal_send_call(const char* fmt)
    {
        return 0;
    };

    /**
     * Send the information to sd_journal_send.
     *
     * @param[in] fmt - c string format.
     * @param[in] ... - parameters.
     * @return value from sd_journal_send
     *
     * sentinel default makes sure the last parameter is null.
     */
    virtual int journal_send(const char* fmt, ...)
        __attribute__((format(printf, 2, 0))) __attribute__((sentinel))
    {
        va_list args;
        va_start(args, fmt);

        int rc = ::sd_journal_send(fmt, args, NULL);
        va_end(args);

        return rc;
    }
};

extern SdJournalHandler* sdjournal_ptr;

/**
 * Swap out the sdjournal_ptr used by log<> such that a test
 * won't need to hit the real sd_journal and fail.
 *
 * @param[in] with - pointer to your sdjournal_mock object.
 * @return pointer to the previously stored sdjournal_ptr.
 */
SdJournalHandler* SwapJouralHandler(SdJournalHandler* with);

} // namespace logging
} // namespace phosphor
