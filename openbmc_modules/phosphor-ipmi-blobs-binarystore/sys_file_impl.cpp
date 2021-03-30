#include "sys_file_impl.hpp"

#include <system_error>

using namespace std::string_literals;

static constexpr size_t rwBlockSize = 8192;

namespace binstore
{

namespace
{

std::system_error errnoException(const std::string& message)
{
    return std::system_error(errno, std::generic_category(), message);
}

} // namespace

SysFileImpl::SysFileImpl(const std::string& path, size_t offset,
                         const internal::Sys* sys) :
    sys(sys)
{
    fd_ = sys->open(path.c_str(), O_RDWR);
    offset_ = offset;

    if (fd_ < 0)
    {
        throw errnoException("Error opening file "s + path);
    }
}

SysFileImpl::~SysFileImpl()
{
    sys->close(fd_);
}

void SysFileImpl::lseek(size_t pos) const
{
    if (sys->lseek(fd_, offset_ + pos, SEEK_SET) < 0)
    {
        throw errnoException("Cannot lseek to pos "s + std::to_string(pos));
    }
}

size_t SysFileImpl::readToBuf(size_t pos, size_t count, char* buf) const
{

    lseek(pos);

    size_t bytesRead = 0;

    do
    {
        auto ret = sys->read(fd_, &buf[bytesRead], count - bytesRead);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw errnoException("Error reading from file"s);
        }
        else if (ret > 0)
        {
            bytesRead += ret;
        }
        else // ret == 0
        {
            break;
        }
    } while (bytesRead < count);

    return bytesRead;
}

std::string SysFileImpl::readAsStr(size_t pos, size_t count) const
{
    std::string result;

    /* If count is invalid, return an empty string. */
    if (count == 0 || count > result.max_size())
    {
        return result;
    }

    result.resize(count);
    size_t bytesRead = readToBuf(pos, count, result.data());
    result.resize(bytesRead);
    return result;
}

std::string SysFileImpl::readRemainingAsStr(size_t pos) const
{
    std::string result;
    size_t bytesRead, size = 0;

    /* Since we don't know how much to read, read 'rwBlockSize' at a time
     * until there is nothing to read anymore. */
    do
    {
        result.resize(size + rwBlockSize);
        bytesRead = readToBuf(pos + size, rwBlockSize, result.data() + size);
        size += bytesRead;
    } while (bytesRead == rwBlockSize);

    result.resize(size);
    return result;
}

void SysFileImpl::writeStr(const std::string& data, size_t pos)
{
    lseek(pos);
    ssize_t ret;
    ret = sys->write(fd_, data.data(), data.size());
    if (ret < 0)
    {
        throw errnoException("Error writing to file"s);
    }
    if (static_cast<size_t>(ret) != data.size())
    {
        throw std::runtime_error(
            "Tried to send data size "s + std::to_string(data.size()) +
            " but could only send "s + std::to_string(ret));
    }
}

} // namespace binstore
