#pragma once

#include "file_io_by_type.hpp"

namespace pldm
{
namespace responder
{

/** @class DumpHandler
 *
 *  @brief Inherits and implements FileHandler. This class is used
 *  handle the dump offload/streaming from host to the destination via bmc
 */
class DumpHandler : public FileHandler
{
  public:
    /** @brief DumpHandler constructor
     */
    DumpHandler(uint32_t fileHandle) : FileHandler(fileHandle)
    {}

    virtual int writeFromMemory(uint32_t offset, uint32_t length,
                                uint64_t address);

    virtual int readIntoMemory(uint32_t /*offset*/, uint32_t& /*length*/,
                               uint64_t /*address*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }
    virtual int read(uint32_t /*offset*/, uint32_t& /*length*/,
                     Response& /*response*/)
    {
        return PLDM_ERROR_UNSUPPORTED_PLDM_CMD;
    }

    virtual int write(const char* buffer, uint32_t offset, uint32_t& length);

    virtual int newFileAvailable(uint64_t length);

    virtual int fileAck(uint8_t /*fileStatus*/);

    /** @brief DumpHandler destructor
     */
    ~DumpHandler()
    {}

  private:
    static int fd; //!< fd to manage the dump offload to bmc
};

} // namespace responder
} // namespace pldm
