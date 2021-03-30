#pragma once

#include <cstdint>

namespace host_tool
{

class ProgressInterface
{
  public:
    virtual ~ProgressInterface() = default;

    /** Update the progress by X bytes.  This will inform any listening
     * interfaces (just write to stdout mostly), and tick off as time passed.
     */
    virtual void updateProgress(std::int64_t bytes) = 0;
    virtual void start(std::int64_t bytes) = 0;
};

/**
 * @brief A progress indicator that writes to stdout.  It deliberately
 * overwrites the same line when it's used, so it's advised to not interject
 * other non-error messages.
 */
class ProgressStdoutIndicator : public ProgressInterface
{
  public:
    ProgressStdoutIndicator() = default;

    void updateProgress(std::int64_t bytes) override;
    void start(std::int64_t bytes) override;

  private:
    std::int64_t totalBytes = 0;
    std::int64_t currentBytes = 0;
};

} // namespace host_tool
