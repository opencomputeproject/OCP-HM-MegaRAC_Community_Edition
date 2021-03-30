#pragma once

#include "file_io.hpp"

namespace pldm
{

namespace responder
{

namespace fs = std::filesystem;

/**
 *  @class FileHandler
 *
 *  Base class to handle read/write of all oem file types
 */
class FileHandler
{
  public:
    /** @brief Method to write an oem file type from host memory. Individual
     *  file types need to override this method to do the file specific
     *  processing
     *  @param[in] offset - offset to read/write
     *  @param[in] length - length to be read/write mentioned by Host
     *  @param[in] address - DMA address
     *  @return PLDM status code
     */
    virtual int writeFromMemory(uint32_t offset, uint32_t length,
                                uint64_t address) = 0;

    /** @brief Method to read an oem file type into host memory. Individual
     *  file types need to override this method to do the file specific
     *  processing
     *  @param[in] offset - offset to read
     *  @param[in/out] length - length to be read mentioned by Host
     *  @param[in] address - DMA address
     *  @return PLDM status code
     */
    virtual int readIntoMemory(uint32_t offset, uint32_t& length,
                               uint64_t address) = 0;

    /** @brief Method to read an oem file type's content into the PLDM response.
     *  @param[in] offset - offset to read
     *  @param[in/out] length - length to be read
     *  @param[in] response - PLDM response
     *  @return PLDM status code
     */
    virtual int read(uint32_t offset, uint32_t& length, Response& response) = 0;

    /** @brief Method to write an oem file by type
     *  @param[in] buffer - buffer to be written to file
     *  @param[in] offset - offset to write to
     *  @param[in/out] length - length to be written
     *  @return PLDM status code
     */
    virtual int write(const char* buffer, uint32_t offset,
                      uint32_t& length) = 0;

    virtual int fileAck(uint8_t fileStatus) = 0;

    /** @brief Method to process a new file available notification from the
     *  host. The bmc can chose to do different actions based on the file type.
     *
     *  @param[in] length - size of the file content to be transferred
     *
     *  @return PLDM status code
     */
    virtual int newFileAvailable(uint64_t length) = 0;

    /** @brief Method to read an oem file type's content into the PLDM response.
     *  @param[in] filePath - file to read from
     *  @param[in] offset - offset to read
     *  @param[in/out] length - length to be read
     *  @param[in] response - PLDM response
     *  @return PLDM status code
     */
    virtual int readFile(const std::string& filePath, uint32_t offset,
                         uint32_t& length, Response& response);

    /** @brief Method to do the file content transfer ove DMA between host and
     *  bmc. This method is made virtual to be overridden in test case. And need
     *  not be defined in other child classes
     *
     *  @param[in] path - file system path  where read/write will be done
     *  @param[in] upstream - direction of DMA transfer. "false" means a
     *                        transfer from host to BMC
     *  @param[in] offset - offset to read/write
     *  @param[in/out] length - length to be read/write mentioned by Host
     *  @param[in] address - DMA address
     *
     *  @return PLDM status code
     */
    virtual int transferFileData(const fs::path& path, bool upstream,
                                 uint32_t offset, uint32_t& length,
                                 uint64_t address);

    virtual int transferFileData(int fd, bool upstream, uint32_t offset,
                                 uint32_t& length, uint64_t address);

    /** @brief Constructor to create a FileHandler object
     */
    FileHandler(uint32_t fileHandle) : fileHandle(fileHandle)
    {}

    /** FileHandler destructor
     */
    virtual ~FileHandler()
    {}

  protected:
    uint32_t fileHandle; //!< file handle indicating name of file or invalid
};

/** @brief Method to create individual file handler objects based on file type
 *
 *  @param[in] fileType - type of file
 *  @param[in] fileHandle - file handle
 */

std::unique_ptr<FileHandler> getHandlerByType(uint16_t fileType,
                                              uint32_t fileHandle);

} // namespace responder
} // namespace pldm
