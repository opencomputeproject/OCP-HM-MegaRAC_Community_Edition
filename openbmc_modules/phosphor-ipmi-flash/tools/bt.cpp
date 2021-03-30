#include "bt.hpp"

#include <ipmiblob/blob_errors.hpp>

#include <cstdint>
#include <vector>

namespace host_tool
{

bool BtDataHandler::sendContents(const std::string& input,
                                 std::uint16_t session)
{
    int inputFd = sys->open(input.c_str(), 0);
    if (inputFd < 0)
    {
        return false;
    }

    std::int64_t fileSize = sys->getSize(input.c_str());
    if (fileSize == 0)
    {
        std::fprintf(stderr, "Zero-length file, or other file access error\n");
        return false;
    }

    progress->start(fileSize);

    try
    {
        static constexpr int btBufferLen = 50;
        std::uint8_t readBuffer[btBufferLen];
        int bytesRead;
        std::uint32_t offset = 0;

        do
        {
            bytesRead = sys->read(inputFd, readBuffer, sizeof(readBuffer));
            if (bytesRead > 0)
            {
                /* minorly awkward repackaging. */
                std::vector<std::uint8_t> buffer(&readBuffer[0],
                                                 &readBuffer[bytesRead]);
                blob->writeBytes(session, offset, buffer);
                offset += bytesRead;
                progress->updateProgress(bytesRead);
            }
        } while (bytesRead > 0);
    }
    catch (const ipmiblob::BlobException& b)
    {
        sys->close(inputFd);
        return false;
    }

    sys->close(inputFd);
    return true;
}

} // namespace host_tool
