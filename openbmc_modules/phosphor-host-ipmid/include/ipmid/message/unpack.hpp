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

#include <array>
#include <ipmid/message/types.hpp>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace ipmi
{

namespace message
{

namespace details
{

/**************************************
 * ipmi return type helpers
 **************************************/

template <typename NumericType, size_t byteIndex = 0>
void UnpackBytes(uint8_t* pointer, NumericType& i)
{
    if constexpr (byteIndex < sizeof(NumericType))
    {
        i |= static_cast<NumericType>(*pointer) << (CHAR_BIT * byteIndex);
        UnpackBytes<NumericType, byteIndex + 1>(pointer + 1, i);
    }
}

template <typename NumericType, size_t byteIndex = 0>
void UnpackBytesUnaligned(Payload& p, NumericType& i)
{
    if constexpr (byteIndex < sizeof(NumericType))
    {
        i |= static_cast<NumericType>(p.popBits(CHAR_BIT))
             << (CHAR_BIT * byteIndex);
        UnpackBytesUnaligned<NumericType, byteIndex + 1>(p, i);
    }
}

/** @struct UnpackSingle
 *  @brief Utility to unpack a single C++ element from a Payload
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam T - Type of element to unpack.
 */
template <typename T>
struct UnpackSingle
{
    /** @brief Do the operation to unpack element.
     *
     *  @param[in] p - Payload to unpack from.
     *  @param[out] t - The reference to unpack item into.
     */
    static int op(Payload& p, T& t)
    {
        if constexpr (std::is_fundamental<T>::value)
        {
            t = 0;
            if (p.bitCount)
            {
                if (p.fillBits(CHAR_BIT * sizeof(t)))
                {
                    return 1;
                }
                UnpackBytesUnaligned<T>(p, t);
            }
            else
            {
                // copy out bits from vector....
                if (p.raw.size() < (p.rawIndex + sizeof(t)))
                {
                    return 1;
                }
                auto iter = p.raw.data() + p.rawIndex;
                t = 0;
                UnpackBytes<T>(iter, t);
                p.rawIndex += sizeof(t);
            }
            return 0;
        }
        else if constexpr (utility::is_tuple<T>::value)
        {
            bool priorError = p.unpackError;
            size_t priorIndex = p.rawIndex;
            // more stuff to unroll if partial bytes are out
            size_t priorBitCount = p.bitCount;
            fixed_uint_t<details::bitStreamSize> priorBits = p.bitStream;
            int ret = p.unpack(t);
            if (ret != 0)
            {
                t = T();
                p.rawIndex = priorIndex;
                p.bitStream = priorBits;
                p.bitCount = priorBitCount;
                p.unpackError = priorError;
            }
            return ret;
        }
        else
        {
            static_assert(
                utility::dependent_false<T>::value,
                "Attempt to unpack a type that has no IPMI unpack operation");
        }
    }
};

/** @struct UnpackSingle
 *  @brief Utility to unpack a single C++ element from a Payload
 *
 *  Specialization to unpack std::string represented as a
 *  UCSD-Pascal style string
 */
template <>
struct UnpackSingle<std::string>
{
    static int op(Payload& p, std::string& t)
    {
        // pop len first
        if (p.rawIndex > (p.raw.size() - sizeof(uint8_t)))
        {
            return 1;
        }
        uint8_t len = p.raw[p.rawIndex++];
        // check to see that there are n bytes left
        auto [first, last] = p.pop<char>(len);
        if (first == last)
        {
            return 1;
        }
        t.reserve(last - first);
        t.insert(0, first, (last - first));
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for fixed_uint_t types
 */
template <unsigned N>
struct UnpackSingle<fixed_uint_t<N>>
{
    static int op(Payload& p, fixed_uint_t<N>& t)
    {
        static_assert(N <= (details::bitStreamSize - CHAR_BIT));
        constexpr size_t count = N;
        // acquire enough bits in the stream to fulfill the Payload
        if (p.fillBits(count))
        {
            return -1;
        }
        fixed_uint_t<details::bitStreamSize> bitmask = ((1 << count) - 1);
        t = (p.bitStream & bitmask).convert_to<fixed_uint_t<N>>();
        p.bitStream >>= count;
        p.bitCount -= count;
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for bool. */
template <>
struct UnpackSingle<bool>
{
    static int op(Payload& p, bool& b)
    {
        // acquire enough bits in the stream to fulfill the Payload
        if (p.fillBits(1))
        {
            return -1;
        }
        b = static_cast<bool>(p.bitStream & 0x01);
        // clear bits from stream
        p.bitStream >>= 1;
        p.bitCount -= 1;
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for std::bitset<N>
 */
template <size_t N>
struct UnpackSingle<std::bitset<N>>
{
    static int op(Payload& p, std::bitset<N>& t)
    {
        static_assert(N <= (details::bitStreamSize - CHAR_BIT));
        size_t count = N;
        // acquire enough bits in the stream to fulfill the Payload
        if (p.fillBits(count))
        {
            return -1;
        }
        fixed_uint_t<details::bitStreamSize> bitmask =
            ~fixed_uint_t<details::bitStreamSize>(0) >>
            (details::bitStreamSize - count);
        t |= (p.bitStream & bitmask).convert_to<unsigned long long>();
        p.bitStream >>= count;
        p.bitCount -= count;
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for std::optional<T> */
template <typename T>
struct UnpackSingle<std::optional<T>>
{
    static int op(Payload& p, std::optional<T>& t)
    {
        bool priorError = p.unpackError;
        size_t priorIndex = p.rawIndex;
        // more stuff to unroll if partial bytes are out
        size_t priorBitCount = p.bitCount;
        fixed_uint_t<details::bitStreamSize> priorBits = p.bitStream;
        t.emplace();
        int ret = UnpackSingle<T>::op(p, *t);
        if (ret != 0)
        {
            t.reset();
            p.rawIndex = priorIndex;
            p.bitStream = priorBits;
            p.bitCount = priorBitCount;
            p.unpackError = priorError;
        }
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for std::array<T, N> */
template <typename T, size_t N>
struct UnpackSingle<std::array<T, N>>
{
    static int op(Payload& p, std::array<T, N>& t)
    {
        int ret = 0;
        size_t priorIndex = p.rawIndex;
        for (auto& v : t)
        {
            ret = UnpackSingle<T>::op(p, v);
            if (ret)
            {
                p.rawIndex = priorIndex;
                t = std::array<T, N>();
                break;
            }
        }
        return ret;
    }
};

/** @brief Specialization of UnpackSingle for std::array<uint8_t> */
template <size_t N>
struct UnpackSingle<std::array<uint8_t, N>>
{
    static int op(Payload& p, std::array<uint8_t, N>& t)
    {
        if (p.raw.size() - p.rawIndex < N)
        {
            t.fill(0);
            return -1;
        }
        // copy out the bytes
        std::copy(p.raw.begin() + p.rawIndex, p.raw.begin() + p.rawIndex + N,
                  t.begin());
        p.rawIndex += N;
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for std::vector<T> */
template <typename T>
struct UnpackSingle<std::vector<T>>
{
    static int op(Payload& p, std::vector<T>& t)
    {
        while (p.rawIndex < p.raw.size())
        {
            t.emplace_back();
            if (UnpackSingle<T>::op(p, t.back()))
            {
                t.pop_back();
                break;
            }
        }
        // unpacking a vector is always successful:
        // either stuff was unpacked successfully (return 0)
        // or stuff was not unpacked, but should still return
        // success because an empty vector or a not-fully-unpacked
        // payload is not a failure.
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for std::vector<uint8_t> */
template <>
struct UnpackSingle<std::vector<uint8_t>>
{
    static int op(Payload& p, std::vector<uint8_t>& t)
    {
        // copy out the remainder of the message
        t.reserve(p.raw.size() - p.rawIndex);
        t.insert(t.begin(), p.raw.begin() + p.rawIndex, p.raw.end());
        p.rawIndex = p.raw.size();
        return 0;
    }
};

/** @brief Specialization of UnpackSingle for Payload */
template <>
struct UnpackSingle<Payload>
{
    static int op(Payload& p, Payload& t)
    {
        t = p;
        // mark that this payload is being included in the args
        p.trailingOk = true;
        return 0;
    }
};

} // namespace details

} // namespace message

} // namespace ipmi
