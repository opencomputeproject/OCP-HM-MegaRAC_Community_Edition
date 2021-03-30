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

#include <bitset>
#include <boost/multiprecision/cpp_int.hpp>
#include <ipmid/utility.hpp>
#include <tuple>

// unsigned fixed-bit sizes
template <unsigned N>
using fixed_uint_t =
    boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
        N, N, boost::multiprecision::unsigned_magnitude,
        boost::multiprecision::unchecked, void>>;
// signed fixed-bit sizes
template <unsigned N>
using fixed_int_t =
    boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
        N, N, boost::multiprecision::signed_magnitude,
        boost::multiprecision::unchecked, void>>;

using uint1_t = fixed_uint_t<1>;
using uint2_t = fixed_uint_t<2>;
using uint3_t = fixed_uint_t<3>;
using uint4_t = fixed_uint_t<4>;
using uint5_t = fixed_uint_t<5>;
using uint6_t = fixed_uint_t<6>;
using uint7_t = fixed_uint_t<7>;
// native uint8_t
using uint9_t = fixed_uint_t<9>;
using uint10_t = fixed_uint_t<10>;
using uint11_t = fixed_uint_t<11>;
using uint12_t = fixed_uint_t<12>;
using uint13_t = fixed_uint_t<13>;
using uint14_t = fixed_uint_t<14>;
using uint15_t = fixed_uint_t<15>;
// native uint16_t
using uint24_t = fixed_uint_t<24>;

// signed fixed-bit sizes
using int2_t = fixed_int_t<2>;
using int3_t = fixed_int_t<3>;
using int4_t = fixed_int_t<4>;
using int5_t = fixed_int_t<5>;
using int6_t = fixed_int_t<6>;
using int7_t = fixed_int_t<7>;
// native int8_t
using int9_t = fixed_int_t<9>;
using int10_t = fixed_int_t<10>;
using int11_t = fixed_int_t<11>;
using int12_t = fixed_int_t<12>;
using int13_t = fixed_int_t<13>;
using int14_t = fixed_int_t<14>;
using int15_t = fixed_int_t<15>;
// native int16_t
using int24_t = fixed_int_t<24>;

// bool is more efficient than a uint1_t
using bit = bool;

// Mechanism for going from uint7_t, int7_t, or std::bitset<7> to 7 bits
// use nrFixedBits<uint7_t> or nrFixedBits<decltype(u7)>
namespace types
{
namespace details
{

template <size_t N>
struct Size
{
    static constexpr size_t value = N;
};

template <unsigned Bits>
constexpr auto getNrBits(const fixed_int_t<Bits>&) -> Size<Bits>;
template <unsigned Bits>
constexpr auto getNrBits(const fixed_uint_t<Bits>&) -> Size<Bits>;
template <size_t Bits>
constexpr auto getNrBits(const std::bitset<Bits>&) -> Size<Bits>;

} // namespace details

/**
 * @brief mechanism to get N from a type like fixed_int_t<N>
 *
 * helper template to extract N from a fixed_(u)int_t variable
 *
 * @tparam T - a type of fixed_int_t<N> or fixed_unint_t<N>
 *
 * @return size_t - evaluates to a constexpr size_t of N
 */
template <typename T>
constexpr auto nrFixedBits =
    decltype(details::getNrBits(std::declval<T>()))::value;

} // namespace types
