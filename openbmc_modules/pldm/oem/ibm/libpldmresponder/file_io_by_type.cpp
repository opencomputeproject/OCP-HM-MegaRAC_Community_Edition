#include "config.h"

#include "file_io_by_type.hpp"

#include "libpldm/base.h"
#include "oem/ibm/libpldm/file_io.h"

#include "common/utils.hpp"
#include "file_io_type_cert.hpp"
#include "file_io_type_dump.hpp"
#include "file_io_type_lid.hpp"
#include "file_io_type_pel.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <stdint.h>
#include <unistd.h>

#include <xyz/openbmc_project/Logging/Entry/server.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace pldm
{
namespace responder
{

using namespace sdbusplus::xyz::openbmc_project::Common::Error;

int FileHandler::transferFileData(int32_t fd, bool upstream, uint32_t offset,
                                  uint32_t& length, uint64_t address)
{
    dma::DMA xdmaInterface;
    while (length > dma::maxSize)
    {
        auto rc = xdmaInterface.transferDataHost(fd, offset, dma::maxSize,
                                                 address, upstream);
        if (rc < 0)
        {
            return PLDM_ERROR;
        }
        offset += dma::maxSize;
        length -= dma::maxSize;
        address += dma::maxSize;
    }
    auto rc =
        xdmaInterface.transferDataHost(fd, offset, length, address, upstream);
    return rc < 0 ? PLDM_ERROR : PLDM_SUCCESS;
}

int FileHandler::transferFileData(const fs::path& path, bool upstream,
                                  uint32_t offset, uint32_t& length,
                                  uint64_t address)
{
    bool fileExists = false;
    if (upstream)
    {
        fileExists = fs::exists(path);
        if (!fileExists)
        {
            std::cerr << "File does not exist. PATH=" << path.c_str() << "\n";
            return PLDM_INVALID_FILE_HANDLE;
        }

        size_t fileSize = fs::file_size(path);
        if (offset >= fileSize)
        {
            std::cerr << "Offset exceeds file size, OFFSET=" << offset
                      << " FILE_SIZE=" << fileSize << "\n";
            return PLDM_DATA_OUT_OF_RANGE;
        }
        if (offset + length > fileSize)
        {
            length = fileSize - offset;
        }
    }

    int flags{};
    if (upstream)
    {
        flags = O_RDONLY;
    }
    else if (fileExists)
    {
        flags = O_RDWR;
    }
    else
    {
        flags = O_WRONLY;
    }
    int file = open(path.string().c_str(), flags);
    if (file == -1)
    {
        std::cerr << "File does not exist, PATH = " << path.string() << "\n";
        ;
        return PLDM_ERROR;
    }
    utils::CustomFD fd(file);

    return transferFileData(fd(), upstream, offset, length, address);
}

std::unique_ptr<FileHandler> getHandlerByType(uint16_t fileType,
                                              uint32_t fileHandle)
{
    switch (fileType)
    {
        case PLDM_FILE_TYPE_PEL:
        {
            return std::make_unique<PelHandler>(fileHandle);
            break;
        }
        case PLDM_FILE_TYPE_LID_PERM:
        {
            return std::make_unique<LidHandler>(fileHandle, true);
            break;
        }
        case PLDM_FILE_TYPE_LID_TEMP:
        {
            return std::make_unique<LidHandler>(fileHandle, false);
            break;
        }
        case PLDM_FILE_TYPE_DUMP:
        {
            return std::make_unique<DumpHandler>(fileHandle);
            break;
        }
        case PLDM_FILE_TYPE_CERT_SIGNING_REQUEST:
        case PLDM_FILE_TYPE_SIGNED_CERT:
        case PLDM_FILE_TYPE_ROOT_CERT:
        {
            return std::make_unique<CertHandler>(fileHandle, fileType);
            break;
        }
        default:
        {
            throw InternalFailure();
            break;
        }
    }
    return nullptr;
}

int FileHandler::readFile(const std::string& filePath, uint32_t offset,
                          uint32_t& length, Response& response)
{
    if (!fs::exists(filePath))
    {
        std::cerr << "File does not exist, HANDLE=" << fileHandle
                  << " PATH=" << filePath.c_str() << "\n";
        return PLDM_INVALID_FILE_HANDLE;
    }

    size_t fileSize = fs::file_size(filePath);
    if (offset >= fileSize)
    {
        std::cerr << "Offset exceeds file size, OFFSET=" << offset
                  << " FILE_SIZE=" << fileSize << "\n";
        return PLDM_DATA_OUT_OF_RANGE;
    }

    if (offset + length > fileSize)
    {
        length = fileSize - offset;
    }

    size_t currSize = response.size();
    response.resize(currSize + length);
    auto filePos = reinterpret_cast<char*>(response.data());
    filePos += currSize;
    std::ifstream stream(filePath, std::ios::in | std::ios::binary);
    if (stream)
    {
        stream.seekg(offset);
        stream.read(filePos, length);
        return PLDM_SUCCESS;
    }
    std::cerr << "Unable to read file, FILE=" << filePath.c_str() << "\n";
    return PLDM_ERROR;
}

} // namespace responder
} // namespace pldm
