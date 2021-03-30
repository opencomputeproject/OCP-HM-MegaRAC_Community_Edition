#include "config.h"

#include <phosphor-logging/sdjournal.hpp>

namespace phosphor
{
namespace logging
{

SdJournalHandler sdjournal_impl;
SdJournalHandler* sdjournal_ptr = &sdjournal_impl;

SdJournalHandler* SwapJouralHandler(SdJournalHandler* with)
{
    SdJournalHandler* curr = sdjournal_ptr;
    sdjournal_ptr = with;
    return curr;
}

} // namespace logging
} // namespace phosphor
