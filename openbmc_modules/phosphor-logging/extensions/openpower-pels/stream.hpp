#pragma once

#include <arpa/inet.h>
#include <byteswap.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace openpower
{
namespace pels
{

namespace detail
{
/**
 * @brief A host-to-network implementation for uint64_t
 *
 * @param[in] value - the value to convert to
 * @return uint64_t - the byteswapped value
 */
inline uint64_t htonll(uint64_t value)
{
    return bswap_64(value);
}

/**
 * @brief A network-to-host implementation for uint64_t
 *
 * @param[in] value - the value to convert to
 * @return uint64_t - the byteswapped value
 */
inline uint64_t ntohll(uint64_t value)
{
    return bswap_64(value);
}
} // namespace detail

/**
 * @class Stream
 *
 * This class is used for getting data types into and out of a vector<uint8_t>
 * that contains data in network byte (big endian) ordering.
 */
class Stream
{
  public:
    Stream() = delete;
    ~Stream() = default;
    Stream(const Stream&) = default;
    Stream& operator=(const Stream&) = default;
    Stream(Stream&&) = default;
    Stream& operator=(Stream&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] data - the vector of data
     */
    explicit Stream(std::vector<uint8_t>& data) : _data(data), _offset(0)
    {
    }

    /**
     * @brief Constructor
     *
     * @param[in] data - the vector of data
     * @param[in] offset - the starting offset
     */
    Stream(std::vector<uint8_t>& data, std::size_t offset) :
        _data(data), _offset(offset)
    {
        if (_offset >= _data.size())
        {
            throw std::out_of_range("Offset out of range");
        }
    }

    /**
     * @brief Extraction operator for a uint8_t
     *
     * @param[out] value - filled in with the value
     * @return Stream&
     */
    Stream& operator>>(uint8_t& value)
    {
        read(&value, 1);
        return *this;
    }

    /**
     * @brief Extraction operator for a char
     *
     * @param[out] value -filled in with the value
     * @return Stream&
     */
    Stream& operator>>(char& value)
    {
        read(&value, 1);
        return *this;
    }

    /**
     * @brief Extraction operator for a uint16_t
     *
     * @param[out] value -filled in with the value
     * @return Stream&
     */
    Stream& operator>>(uint16_t& value)
    {
        read(&value, 2);
        value = htons(value);
        return *this;
    }

    /**
     * @brief Extraction operator for a uint32_t
     *
     * @param[out] value -filled in with the value
     * @return Stream&
     */
    Stream& operator>>(uint32_t& value)
    {
        read(&value, 4);
        value = htonl(value);
        return *this;
    }

    /**
     * @brief Extraction operator for a uint64_t
     *
     * @param[out] value -filled in with the value
     * @return Stream&
     */
    Stream& operator>>(uint64_t& value)
    {
        read(&value, 8);
        value = detail::htonll(value);
        return *this;
    }

    /**
     * @brief Extraction operator for a std::vector<uint8_t>
     *
     * The vector's size is the amount extracted.
     *
     * @param[out] value - filled in with the value
     * @return Stream&
     */
    Stream& operator>>(std::vector<uint8_t>& value)
    {
        if (!value.empty())
        {
            read(value.data(), value.size());
        }
        return *this;
    }

    /**
     * @brief Extraction operator for a std::vector<char>
     *
     * The vector's size is the amount extracted.
     *
     * @param[out] value - filled in with the value
     * @return Stream&
     */
    Stream& operator>>(std::vector<char>& value)
    {
        if (!value.empty())
        {
            read(value.data(), value.size());
        }
        return *this;
    }

    /**
     * @brief Insert operator for a uint8_t
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(uint8_t value)
    {
        write(&value, 1);
        return *this;
    }

    /**
     * @brief Insert operator for a char
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(char value)
    {
        write(&value, 1);
        return *this;
    }

    /**
     * @brief Insert operator for a uint16_t
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(uint16_t value)
    {
        uint16_t data = ntohs(value);
        write(&data, 2);
        return *this;
    }

    /**
     * @brief Insert operator for a uint32_t
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(uint32_t value)
    {
        uint32_t data = ntohl(value);
        write(&data, 4);
        return *this;
    }

    /**
     * @brief Insert operator for a uint64_t
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(uint64_t value)
    {
        uint64_t data = detail::ntohll(value);
        write(&data, 8);
        return *this;
    }

    /**
     * @brief Insert operator for a std::vector<uint8_t>
     *
     * The full vector is written to the stream.
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(const std::vector<uint8_t>& value)
    {
        if (!value.empty())
        {
            write(value.data(), value.size());
        }
        return *this;
    }

    /**
     * @brief Insert operator for a std::vector<char>
     *
     * The full vector is written to the stream.
     *
     * @param[in] value - the value to write to the stream
     * @return Stream&
     */
    Stream& operator<<(const std::vector<char>& value)
    {
        if (!value.empty())
        {
            write(value.data(), value.size());
        }
        return *this;
    }

    /**
     * @brief Sets the offset of the stream
     *
     * @param[in] newOffset - the new offset
     */
    void offset(std::size_t newOffset)
    {
        if (newOffset >= _data.size())
        {
            throw std::out_of_range("new offset out of range");
        }

        _offset = newOffset;
    }

    /**
     * @brief Returns the current offset of the stream
     *
     * @return size_t - the offset
     */
    std::size_t offset() const
    {
        return _offset;
    }

    /**
     * @brief Returns the remaining bytes left between the current offset
     *        and the data size.
     *
     * @return size_t - the remaining size
     */
    std::size_t remaining() const
    {
        assert(_data.size() >= _offset);
        return _data.size() - _offset;
    }

    /**
     * @brief Reads a specified number of bytes out of a stream
     *
     * @param[out] out - filled in with the data
     * @param[in] size - the size to read
     */
    void read(void* out, std::size_t size)
    {
        rangeCheck(size);
        memcpy(out, &_data[_offset], size);
        _offset += size;
    }

    /**
     * @brief Writes a specified number of bytes into the stream
     *
     * @param[in] in - the data to write
     * @param[in] size - the size to write
     */
    void write(const void* in, std::size_t size)
    {
        size_t newSize = _offset + size;
        if (newSize > _data.size())
        {
            _data.resize(newSize, 0);
        }
        memcpy(&_data[_offset], in, size);
        _offset += size;
    }

  private:
    /**
     * @brief Throws an exception if the size passed in plus the current
     *        offset is bigger than the current data size.
     * @param[in] size - the size to check
     */
    void rangeCheck(std::size_t size)
    {
        if (_offset + size > _data.size())
        {
            std::string msg{"Attempted stream overflow: offset "};
            msg += std::to_string(_offset) + " buffer size " +
                   std::to_string(_data.size()) + " op size " +
                   std::to_string(size);
            throw std::out_of_range(msg.c_str());
        }
    }

    /**
     * @brief The data that the stream accesses.
     */
    std::vector<uint8_t>& _data;

    /**
     * @brief The current offset of the stream.
     */
    std::size_t _offset;
};

} // namespace pels
} // namespace openpower
