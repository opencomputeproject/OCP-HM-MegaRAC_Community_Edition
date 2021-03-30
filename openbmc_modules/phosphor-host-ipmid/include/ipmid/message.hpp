/**
 * Copyright © 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <algorithm>
#include <boost/asio/spawn.hpp>
#include <cstdint>
#include <exception>
#include <ipmid/api-types.hpp>
#include <ipmid/message/types.hpp>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <tuple>
#include <utility>
#include <vector>

namespace ipmi
{

struct Context
{
    using ptr = std::shared_ptr<Context>;

    Context() = delete;
    Context(const Context&) = default;
    Context& operator=(const Context&) = default;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    Context(std::shared_ptr<sdbusplus::asio::connection> bus, NetFn netFn,
            Cmd cmd, int channel, int userId, uint32_t sessionId,
            Privilege priv, int rqSA, boost::asio::yield_context& yield) :
        bus(bus),
        netFn(netFn), cmd(cmd), channel(channel), userId(userId),
        sessionId(sessionId), priv(priv), rqSA(rqSA), yield(yield)
    {
    }

    std::shared_ptr<sdbusplus::asio::connection> bus;
    // normal IPMI context (what call is this, from whence it came...)
    NetFn netFn;
    Cmd cmd;
    int channel;
    int userId;
    uint32_t sessionId;
    Privilege priv;
    // srcAddr is only set on IPMB requests because
    // Platform Event Message needs it to determine the incoming format
    int rqSA;
    boost::asio::yield_context yield;
};

namespace message
{

namespace details
{

template <typename A>
struct UnpackSingle;

template <typename T>
using UnpackSingle_t = UnpackSingle<utility::TypeIdDowncast_t<T>>;

template <typename A>
struct PackSingle;

template <typename T>
using PackSingle_t = PackSingle<utility::TypeIdDowncast_t<T>>;

// size to hold 64 bits plus one (possibly-)partial byte
static constexpr size_t bitStreamSize = ((sizeof(uint64_t) + 1) * CHAR_BIT);

} // namespace details

/**
 * @brief a payload class that provides a mechanism to pack and unpack data
 *
 * When a new request is being executed, the Payload class is responsible for
 * attempting to unpack all the required arguments from the incoming blob. For
 * variable-length functions, it is possible to have function signature have a
 * Payload object, which will then allow the remaining data to be extracted as
 * needed.
 *
 * When creating a response, the parameters returned from the callback use a
 * newly created payload object to pack all the parameters into a buffer that is
 * then returned to the requester.
 *
 * These interfaces make calls into the message/pack.hpp and message/unpack.hpp
 * functions.
 */
struct Payload
{
    Payload() = default;
    Payload(const Payload&) = default;
    Payload& operator=(const Payload&) = default;
    Payload(Payload&&) = default;
    Payload& operator=(Payload&&) = default;

    explicit Payload(std::vector<uint8_t>&& data) : raw(std::move(data))
    {
    }

    ~Payload()
    {
        using namespace phosphor::logging;
        if (raw.size() != 0 && std::uncaught_exceptions() == 0 && !trailingOk &&
            !unpackCheck && !unpackError)
        {
            log<level::ERR>("Failed to check request for full unpack");
        }
    }

    /******************************************************************
     * raw vector access
     *****************************************************************/
    /**
     * @brief return the size of the underlying raw buffer
     */
    size_t size() const
    {
        return raw.size();
    }
    /**
     * @brief resize the underlying raw buffer to a new size
     *
     * @param sz - new size for the buffer
     */
    void resize(size_t sz)
    {
        raw.resize(sz);
    }
    /**
     * @brief return a pointer to the underlying raw buffer
     */
    uint8_t* data()
    {
        return raw.data();
    }
    /**
     * @brief return a const pointer to the underlying raw buffer
     */
    const uint8_t* data() const
    {
        return raw.data();
    }

    /******************************************************************
     * Response operations
     *****************************************************************/
    /**
     * @brief append a series of bytes to the buffer
     *
     * @tparam T - the type pointer to return; must be compatible to a byte
     *
     * @param begin - a pointer to the beginning of the series
     * @param end - a pointer to the end of the series
     */
    template <typename T>
    void append(T* begin, T* end)
    {
        static_assert(
            std::is_same_v<utility::TypeIdDowncast_t<T>, int8_t> ||
                std::is_same_v<utility::TypeIdDowncast_t<T>, uint8_t> ||
                std::is_same_v<utility::TypeIdDowncast_t<T>, char>,
            "begin and end must be signed or unsigned byte pointers");
        // this interface only allows full-byte access; pack in partial bytes
        drain();
        raw.insert(raw.end(), reinterpret_cast<const uint8_t*>(begin),
                   reinterpret_cast<const uint8_t*>(end));
    }

    /**
     * @brief append a series of bits to the buffer
     *
     * Only the lowest @count order of bits will be appended, with the most
     * significant of those bits getting appended first.
     *
     * @param count - number of bits to append
     * @param bits - a byte with count significant bits to append
     */
    void appendBits(size_t count, uint8_t bits)
    {
        // drain whole bytes out
        drain(true);

        // add in the new bits as the higher-order bits, filling LSBit first
        fixed_uint_t<details::bitStreamSize> tmp = bits;
        tmp <<= bitCount;
        bitStream |= tmp;
        bitCount += count;

        // drain any whole bytes we have appended
        drain(true);
    }

    /**
     * @brief empty out the bucket and pack it as bytes LSB-first
     *
     * @param wholeBytesOnly - if true, only the whole bytes will be drained
     */
    void drain(bool wholeBytesOnly = false)
    {
        while (bitCount > 0)
        {
            uint8_t retVal;
            if (bitCount < CHAR_BIT)
            {
                if (wholeBytesOnly)
                {
                    break;
                }
            }
            size_t bitsOut = std::min(static_cast<size_t>(CHAR_BIT), bitCount);
            retVal = static_cast<uint8_t>(bitStream);
            raw.push_back(retVal);
            bitStream >>= bitsOut;
            bitCount -= bitsOut;
        }
    }

    // base empty pack
    int pack()
    {
        return 0;
    }

    /**
     * @brief pack arbitrary values (of any supported type) into the buffer
     *
     * @tparam Arg - the type of the first argument
     * @tparam Args - the type of the optional remaining arguments
     *
     * @param arg - the first argument to pack
     * @param args... - the optional remaining arguments to pack
     *
     * @return int - non-zero on pack errors
     */
    template <typename Arg, typename... Args>
    int pack(Arg&& arg, Args&&... args)
    {
        int packRet =
            details::PackSingle_t<Arg>::op(*this, std::forward<Arg>(arg));
        if (packRet)
        {
            return packRet;
        }
        packRet = pack(std::forward<Args>(args)...);
        drain();
        return packRet;
    }

    /**
     * @brief Prepends another payload to this one
     *
     * Avoid using this unless absolutely required since it inserts into the
     * front of the response payload.
     *
     * @param p - The payload to prepend
     *
     * @retunr int - non-zero on prepend errors
     */
    int prepend(const ipmi::message::Payload& p)
    {
        if (bitCount != 0 || p.bitCount != 0)
        {
            return 1;
        }
        raw.reserve(raw.size() + p.raw.size());
        raw.insert(raw.begin(), p.raw.begin(), p.raw.end());
        return 0;
    }

    /******************************************************************
     * Request operations
     *****************************************************************/
    /**
     * @brief pop a series of bytes from the raw buffer
     *
     * @tparam T - the type pointer to return; must be compatible to a byte
     *
     * @param count - the number of bytes to return
     *
     * @return - a tuple of pointers (begin,begin+count)
     */
    template <typename T>
    auto pop(size_t count)
    {
        static_assert(
            std::is_same_v<utility::TypeIdDowncast_t<T>, int8_t> ||
                std::is_same_v<utility::TypeIdDowncast_t<T>, uint8_t> ||
                std::is_same_v<utility::TypeIdDowncast_t<T>, char>,
            "T* must be signed or unsigned byte pointers");
        // this interface only allows full-byte access; skip partial bits
        if (bitCount)
        {
            // WARN on unused bits?
            discardBits();
        }
        if (count <= (raw.size() - rawIndex))
        {
            auto range = std::make_tuple(
                reinterpret_cast<T*>(raw.data() + rawIndex),
                reinterpret_cast<T*>(raw.data() + rawIndex + count));
            rawIndex += count;
            return range;
        }
        unpackError = true;
        return std::make_tuple(reinterpret_cast<T*>(NULL),
                               reinterpret_cast<T*>(NULL));
    }

    /**
     * @brief fill bit stream with at least count bits for consumption
     *
     * @param count - number of bit needed
     *
     * @return - unpackError
     */
    bool fillBits(size_t count)
    {
        // add more bits to the top end of the bitstream
        // so we consume bits least-significant first
        if (count > (details::bitStreamSize - CHAR_BIT))
        {
            unpackError = true;
            return unpackError;
        }
        while (bitCount < count)
        {
            if (rawIndex < raw.size())
            {
                fixed_uint_t<details::bitStreamSize> tmp = raw[rawIndex++];
                tmp <<= bitCount;
                bitStream |= tmp;
                bitCount += CHAR_BIT;
            }
            else
            {
                // raw has run out of bytes to pop
                unpackError = true;
                return unpackError;
            }
        }
        return false;
    }

    /**
     * @brief consume count bits from bitstream (must call fillBits first)
     *
     * @param count - number of bit needed
     *
     * @return - count bits from stream
     */
    uint8_t popBits(size_t count)
    {
        if (bitCount < count)
        {
            unpackError = true;
            return 0;
        }
        // consume bits low-order bits first
        auto bits = bitStream.convert_to<uint8_t>();
        bits &= ((1 << count) - 1);
        bitStream >>= count;
        bitCount -= count;
        return bits;
    }

    /**
     * @brief discard all partial bits
     */
    void discardBits()
    {
        bitStream = 0;
        bitCount = 0;
    }

    /**
     * @brief fully reset the unpack stream
     */
    void reset()
    {
        discardBits();
        rawIndex = 0;
        unpackError = false;
    }

    /**
     * @brief check to see if the stream has been fully unpacked
     *
     * @return bool - true if the stream has been unpacked and has no errors
     */
    bool fullyUnpacked()
    {
        unpackCheck = true;
        return raw.size() == rawIndex && bitCount == 0 && !unpackError;
    }

    // base empty unpack
    int unpack()
    {
        return 0;
    }

    /**
     * @brief unpack arbitrary values (of any supported type) from the buffer
     *
     * @tparam Arg - the type of the first argument
     * @tparam Args - the type of the optional remaining arguments
     *
     * @param arg - the first argument to unpack
     * @param args... - the optional remaining arguments to unpack
     *
     * @return int - non-zero for unpack error
     */
    template <typename Arg, typename... Args>
    int unpack(Arg&& arg, Args&&... args)
    {
        int unpackRet =
            details::UnpackSingle_t<Arg>::op(*this, std::forward<Arg>(arg));
        if (unpackRet)
        {
            unpackError = true;
            return unpackRet;
        }
        return unpack(std::forward<Args>(args)...);
    }

    /**
     * @brief unpack a tuple of values (of any supported type) from the buffer
     *
     * This will unpack the elements of the tuple as if each one was passed in
     * individually, as if passed into the above variadic function.
     *
     * @tparam Types - the implicitly declared list of the tuple element types
     *
     * @param t - the tuple of values to unpack
     *
     * @return int - non-zero on unpack error
     */
    template <typename... Types>
    int unpack(std::tuple<Types...>& t)
    {
        // roll back checkpoint so that unpacking a tuple is atomic
        size_t priorBitCount = bitCount;
        size_t priorIndex = rawIndex;
        fixed_uint_t<details::bitStreamSize> priorBits = bitStream;

        int ret =
            std::apply([this](Types&... args) { return unpack(args...); }, t);
        if (ret)
        {
            bitCount = priorBitCount;
            bitStream = priorBits;
            rawIndex = priorIndex;
        }

        return ret;
    }

    // partial bytes in the form of bits
    fixed_uint_t<details::bitStreamSize> bitStream;
    size_t bitCount = 0;
    std::vector<uint8_t> raw;
    size_t rawIndex = 0;
    bool trailingOk = true;
    bool unpackCheck = false;
    bool unpackError = false;
};

/**
 * @brief high-level interface to an IPMI response
 *
 * Make it easy to just pack in the response args from the callback into a
 * buffer that goes back to the requester.
 */
struct Response
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *     Allowed:
     *         - Copy operations.
     *         - Move operations.
     *         - Destructor.
     */
    Response() = delete;
    Response(const Response&) = default;
    Response& operator=(const Response&) = default;
    Response(Response&&) = default;
    Response& operator=(Response&&) = default;
    ~Response() = default;

    using ptr = std::shared_ptr<Response>;

    explicit Response(Context::ptr& context) :
        payload(), ctx(context), cc(ccSuccess)
    {
    }

    /**
     * @brief pack arbitrary values (of any supported type) into the payload
     *
     * @tparam Args - the type of the optional arguments
     *
     * @param args... - the optional arguments to pack
     *
     * @return int - non-zero on pack errors
     */
    template <typename... Args>
    int pack(Args&&... args)
    {
        return payload.pack(std::forward<Args>(args)...);
    }

    /**
     * @brief pack a tuple of values (of any supported type) into the payload
     *
     * This will pack the elements of the tuple as if each one was passed in
     * individually, as if passed into the above variadic function.
     *
     * @tparam Types - the implicitly declared list of the tuple element types
     *
     * @param t - the tuple of values to pack
     *
     * @return int - non-zero on pack errors
     */
    template <typename... Types>
    int pack(std::tuple<Types...>& t)
    {
        return payload.pack(t);
    }

    /**
     * @brief Prepends another payload to this one
     *
     * Avoid using this unless absolutely required since it inserts into the
     * front of the response payload.
     *
     * @param p - The payload to prepend
     *
     * @retunr int - non-zero on prepend errors
     */
    int prepend(const ipmi::message::Payload& p)
    {
        return payload.prepend(p);
    }

    Payload payload;
    Context::ptr ctx;
    Cc cc;
};

/**
 * @brief high-level interface to an IPMI request
 *
 * Make it easy to unpack the buffer into the request args for the callback.
 */
struct Request
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *     Allowed:
     *         - Copy operations.
     *         - Move operations.
     *         - Destructor.
     */
    Request() = delete;
    Request(const Request&) = default;
    Request& operator=(const Request&) = default;
    Request(Request&&) = default;
    Request& operator=(Request&&) = default;
    ~Request() = default;

    using ptr = std::shared_ptr<Request>;

    explicit Request(Context::ptr context, std::vector<uint8_t>&& d) :
        payload(std::forward<std::vector<uint8_t>>(d)), ctx(context)
    {
    }

    /**
     * @brief unpack arbitrary values (of any supported type) from the payload
     *
     * @tparam Args - the type of the optional arguments
     *
     * @param args... - the optional arguments to unpack
     *
     * @return int - non-zero for unpack error
     */
    template <typename... Args>
    int unpack(Args&&... args)
    {
        int unpackRet = payload.unpack(std::forward<Args>(args)...);
        if (unpackRet != ipmi::ccSuccess)
        {
            // not all bits were consumed by requested parameters
            return ipmi::ccReqDataLenInvalid;
        }
        if (!payload.trailingOk)
        {
            if (!payload.fullyUnpacked())
            {
                // not all bits were consumed by requested parameters
                return ipmi::ccReqDataLenInvalid;
            }
        }
        return ipmi::ccSuccess;
    }

    /**
     * @brief unpack a tuple of values (of any supported type) from the payload
     *
     * This will unpack the elements of the tuple as if each one was passed in
     * individually, as if passed into the above variadic function.
     *
     * @tparam Types - the implicitly declared list of the tuple element types
     *
     * @param t - the tuple of values to unpack
     *
     * @return int - non-zero on unpack error
     */
    template <typename... Types>
    int unpack(std::tuple<Types...>& t)
    {
        return std::apply([this](Types&... args) { return unpack(args...); },
                          t);
    }

    /** @brief Create a response message that corresponds to this request
     *
     * @return A shared_ptr to the response message created
     */
    Response::ptr makeResponse()
    {
        return std::make_shared<Response>(ctx);
    }

    Payload payload;
    Context::ptr ctx;
};

} // namespace message

} // namespace ipmi

// include packing and unpacking of types
#include <ipmid/message/pack.hpp>
#include <ipmid/message/unpack.hpp>
