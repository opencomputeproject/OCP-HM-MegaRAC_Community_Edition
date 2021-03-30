#pragma once

#include "internal/sys.hpp"

#include <cstdint>
#include <string>

namespace host_tool
{

class HostIoInterface
{
  public:
    virtual ~HostIoInterface() = default;

    /**
     * Attempt to read bytes from offset to the destination from the host
     * memory device.
     *
     * @param[in] offset - offset into the host memory device.
     * @param[in] length - the number of bytes to copy from source.
     * @param[in] destination - where to write the bytes.
     * @return true on success, false on failure (such as unable to initialize
     * device).
     */
    virtual bool read(const std::size_t offset, const std::size_t length,
                      void* const destination) = 0;

    /**
     * Attempt to write bytes from source to offset into the host memory device.
     *
     * @param[in] offset - offset into the host memory device.
     * @param[in] length - the number of bytes to copy from source.
     * @param[in] source - the souce of the bytes to copy to the memory device.
     * @return true on success, false on failure (such as unable to initialize
     * device).
     */
    virtual bool write(const std::size_t offset, const std::size_t length,
                       const void* const source) = 0;
};

class DevMemDevice : public HostIoInterface
{
  public:
    explicit DevMemDevice(const internal::Sys* sys = &internal::sys_impl) :
        sys(sys)
    {}

    ~DevMemDevice() = default;

    /* Don't allow copying, assignment or move assignment, only moving. */
    DevMemDevice(const DevMemDevice&) = delete;
    DevMemDevice& operator=(const DevMemDevice&) = delete;
    DevMemDevice(DevMemDevice&&) = default;
    DevMemDevice& operator=(DevMemDevice&&) = delete;

    bool read(const std::size_t offset, const std::size_t length,
              void* const destination) override;

    bool write(const std::size_t offset, const std::size_t length,
               const void* const source) override;

  private:
    static const std::string devMemPath;
    int devMemFd = -1;
    void* devMemMapped = nullptr;
    const internal::Sys* sys;
};

class PpcMemDevice : public HostIoInterface
{
  public:
    explicit PpcMemDevice(const std::string& ppcMemPath,
                          const internal::Sys* sys = &internal::sys_impl) :
        ppcMemPath(ppcMemPath),
        sys(sys)
    {}

    ~PpcMemDevice() override;

    /* Don't allow copying or assignment, only moving. */
    PpcMemDevice(const PpcMemDevice&) = delete;
    PpcMemDevice& operator=(const PpcMemDevice&) = delete;
    PpcMemDevice(PpcMemDevice&&) = default;
    PpcMemDevice& operator=(PpcMemDevice&&) = default;

    bool read(const std::size_t offset, const std::size_t length,
              void* const destination) override;

    bool write(const std::size_t offset, const std::size_t length,
               const void* const source) override;

  private:
    int ppcMemFd = -1;
    const std::string ppcMemPath;
    const internal::Sys* sys;
};

} // namespace host_tool
