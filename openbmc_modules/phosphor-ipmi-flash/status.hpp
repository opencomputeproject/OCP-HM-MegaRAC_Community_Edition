#pragma once

#include <cstdint>

namespace ipmi_flash
{

/** The status of the update mechanism or the verification mechanism */
enum class ActionStatus : std::uint8_t
{
    running = 0,
    success = 1,
    failed = 2,
    unknown = 3,
};

class TriggerableActionInterface
{
  public:
    virtual ~TriggerableActionInterface() = default;

    /**
     * Trigger action.
     *
     * @return true if successfully started, false otherwise.
     */
    virtual bool trigger() = 0;

    /** Abort the action if possible. */
    virtual void abort() = 0;

    /** Check the current state of the action. */
    virtual ActionStatus status() = 0;
};

} // namespace ipmi_flash
