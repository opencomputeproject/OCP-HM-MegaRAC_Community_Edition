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
#include <ipmid/api.hpp>
#include <ipmid/message.hpp>

#include <gtest/gtest.h>

TEST(Uints, Uint8)
{
    std::vector<uint8_t> i = {0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint8_t k = 0x04;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint8TooManyBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint8_t k = 0x04;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint8InsufficientBytes)
{
    std::vector<uint8_t> i = {};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v = 0;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(Uints, Uint16)
{
    std::vector<uint8_t> i = {0x04, 0x86};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint16_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint16_t k = 0x8604;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint16TooManyBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint16_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint16_t k = 0x8604;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint16InsufficientBytes)
{
    std::vector<uint8_t> i = {0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint16_t v = 0;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(Uints, Uint32)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint32_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint32_t k = 0x02008604;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint32TooManyBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00, 0x02, 0x44};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint32_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint32_t k = 0x02008604;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint32InsufficientBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint32_t v = 0;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(Uints, Uint64)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00, 0x02, 0x44, 0x33, 0x22, 0x11};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint64_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint64_t k = 0x1122334402008604ull;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint64TooManyBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00, 0x02, 0x44,
                              0x33, 0x22, 0x11, 0x55};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint64_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint64_t k = 0x1122334402008604ull;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Uints, Uint64InsufficientBytes)
{
    std::vector<uint8_t> i = {0x04, 0x86, 0x00, 0x02, 0x44, 0x33, 0x22};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint64_t v = 0;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(Uints, Uint24)
{
    std::vector<uint8_t> i = {0x58, 0x23, 0x11};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint24_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint24_t k = 0x112358;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(FixedInts, Uint24TooManyBytes)
{
    std::vector<uint8_t> i = {0x58, 0x23, 0x11, 0x00};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint24_t v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint24_t k = 0x112358;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(FixedInts, Uint24InsufficientBytes)
{
    std::vector<uint8_t> i = {0x58, 0x23};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint24_t v = 0;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(FixedInts, Uint3Uint5)
{
    // individual bytes are unpacked low-order-bits first
    // v1 will use [2:0], v2 will use [7:3]
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint3_t v1;
    uint5_t v2;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint3_t k1 = 0x1;
    uint5_t k2 = 0x19;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
}

TEST(FixedInts, Uint3Uint4TooManyBits)
{
    // high order bit should not get unpacked
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint3_t v1;
    uint4_t v2;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint3_t k1 = 0x1;
    uint4_t k2 = 0x9;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
}

TEST(FixedInts, Uint3Uint6InsufficientBits)
{
    // insufficient bits to unpack v2
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint3_t v1;
    uint6_t v2;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v1, v2), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    uint3_t k1 = 0x1;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    // check that v2 is zero
    ASSERT_EQ(v2, 0);
}

TEST(Bools, Boolx8)
{
    // individual bytes are unpacked low-order-bits first
    // [v8, v7, v6, v5, v4, v3, v2, v1]
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    bool v8, v7, v6, v5;
    bool v4, v3, v2, v1;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2, v3, v4, v5, v6, v7, v8), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    // check that the bytes were correctly unpacked (LSB first)
    bool k8 = true, k7 = true, k6 = false, k5 = false;
    bool k4 = true, k3 = false, k2 = false, k1 = true;
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
    ASSERT_EQ(v5, k5);
    ASSERT_EQ(v6, k6);
    ASSERT_EQ(v7, k7);
    ASSERT_EQ(v8, k8);
}

TEST(Bools, Boolx8TooManyBits)
{
    // high order bit should not get unpacked
    // individual bytes are unpacked low-order-bits first
    // [v7, v6, v5, v4, v3, v2, v1]
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    bool v7, v6, v5;
    bool v4, v3, v2, v1;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2, v3, v4, v5, v6, v7), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    // check that the bytes were correctly unpacked (LSB first)
    bool k7 = true, k6 = false, k5 = false;
    bool k4 = true, k3 = false, k2 = false, k1 = true;
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
    ASSERT_EQ(v5, k5);
    ASSERT_EQ(v6, k6);
    ASSERT_EQ(v7, k7);
}

TEST(Bools, Boolx8InsufficientBits)
{
    // individual bytes are unpacked low-order-bits first
    // [v8, v7, v6, v5, v4, v3, v2, v1]
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    bool v9;
    bool v8, v7, v6, v5;
    bool v4, v3, v2, v1;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v1, v2, v3, v4, v5, v6, v7, v8, v9), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    // check that the bytes were correctly unpacked (LSB first)
    bool k8 = true, k7 = true, k6 = false, k5 = false;
    bool k4 = true, k3 = false, k2 = false, k1 = true;
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
    ASSERT_EQ(v5, k5);
    ASSERT_EQ(v6, k6);
    ASSERT_EQ(v7, k7);
    ASSERT_EQ(v8, k8);
}

TEST(Bitsets, Bitset8)
{
    // individual bytes are unpacked low-order-bits first
    // a bitset for 8 bits fills the full byte
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<8> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::bitset<8> k(0xc9);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Bitsets, Bitset7TooManyBits)
{
    // individual bytes are unpacked low-order-bits first
    // a bitset for 8 bits fills the full byte
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<7> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<7> k(0x49);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Bitsets, Bitset9InsufficientBits)
{
    // individual bytes are unpacked low-order-bits first
    // a bitset for 8 bits fills the full byte
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<9> v;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<9> k(0);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Bitsets, Bitset3Bitset5)
{
    // individual bytes are unpacked low-order-bits first
    // v1 will use [2:0], v2 will use [7:3]
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<3> v1;
    std::bitset<5> v2;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::bitset<3> k1(0x1);
    std::bitset<5> k2(0x19);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
}

TEST(Bitsets, Bitset3Bitset4TooManyBits)
{
    // high order bit should not get unpacked
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<3> v1;
    std::bitset<4> v2;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<3> k1 = 0x1;
    std::bitset<4> k2 = 0x9;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
}

TEST(Bitsets, Bitset3Bitset6InsufficientBits)
{
    // insufficient bits to unpack v2
    std::vector<uint8_t> i = {0xc9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<3> v1;
    std::bitset<6> v2;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v1, v2), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<3> k1 = 0x1;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    // check that v2 is zero
    ASSERT_EQ(v2, 0);
}

TEST(Bitsets, Bitset32)
{
    // individual bytes are unpacked low-order-bits first
    // v1 will use 4 bytes, but in LSByte first order
    // v1[7:0] v1[15:9] v1[23:16] v1[31:24]
    std::vector<uint8_t> i = {0xb4, 0x86, 0x91, 0xc2};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<32> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::bitset<32> k(0xc29186b4);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Bitsets, Bitset31TooManyBits)
{
    // high order bit should not get unpacked
    std::vector<uint8_t> i = {0xb4, 0x86, 0x91, 0xc2};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<31> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<31> k(0x429186b4);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(Bitsets, Bitset33InsufficientBits)
{
    // insufficient bits to unpack v2
    std::vector<uint8_t> i = {0xb4, 0x86, 0x91, 0xc2};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::bitset<33> v;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked (comprehends unpack errors)
    ASSERT_FALSE(p.fullyUnpacked());
    std::bitset<33> k(0);
    // check that v is zero
    ASSERT_EQ(v, 0);
}

TEST(Arrays, Array4xUint8)
{
    // an array of bytes will be read verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint8_t, 4> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::array<uint8_t, 4> k = {{0x02, 0x00, 0x86, 0x04}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Arrays, Array4xUint8TooManyBytes)
{
    // last byte should not get unpacked
    // an array of bytes will be read verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04, 0x22};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint8_t, 4> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::array<uint8_t, 4> k = {{0x02, 0x00, 0x86, 0x04}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Arrays, Array4xUint8InsufficientBytes)
{
    // last byte should not get unpacked
    // an array of bytes will be read verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint8_t, 4> v;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    // arrays of uint8_t will be unpacked all at once
    // so nothing will get unpacked
    std::array<uint8_t, 4> k = {{0, 0, 0, 0}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Arrays, Array4xUint32)
{
    // an array of multi-byte values will be unpacked in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    std::vector<uint8_t> i = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34, 0x12};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint32_t, 4> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::array<uint32_t, 4> k = {
        {0x11223344, 0x22446688, 0x33557799, 0x12345678}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Arrays, Array4xUint32TooManyBytes)
{
    // last byte should not get unpacked
    // an array of multi-byte values will be unpacked in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    std::vector<uint8_t> i = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66,
                              0x44, 0x22, 0x99, 0x77, 0x55, 0x33,
                              0x78, 0x56, 0x34, 0x12, 0xaa};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint32_t, 4> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::array<uint32_t, 4> k = {
        {0x11223344, 0x22446688, 0x33557799, 0x12345678}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Arrays, Array4xUint32InsufficientBytes)
{
    // last value should not get unpacked
    // an array of multi-byte values will be unpacked in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    std::vector<uint8_t> i = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::array<uint32_t, 4> v;
    // check that the number of bytes matches
    ASSERT_NE(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    // arrays of uint32_t will be unpacked in a way that looks atomic
    std::array<uint32_t, 4> k = {{0, 0, 0, 0}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Vectors, VectorUint32)
{
    // a vector of multi-byte values will be unpacked in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    std::vector<uint8_t> i = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34, 0x12};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<uint32_t> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::vector<uint32_t> k = {0x11223344, 0x22446688, 0x33557799, 0x12345678};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

// combination of TooManyBytes and InsufficientBytes because
// vectors will attempt to unpack full <T>s until the end of the input
TEST(Vectors, VectorUint32NonIntegralBytes)
{
    // last value should not get unpacked
    // a vector of multi-byte values will be unpacked in order low-order
    // element first, each multi-byte element in LSByte order,
    // and will attempt to consume all bytes remaining
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    std::vector<uint8_t> i = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<uint32_t> v;
    // check that the vector unpacks successfully
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    // arrays of uint32_t will be unpacked one at a time, so the
    // last entry should not get unpacked properly
    std::vector<uint32_t> k = {0x11223344, 0x22446688, 0x33557799};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Vectors, VectorUint8)
{
    // a vector of bytes will be unpacked verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<uint8_t> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::vector<uint8_t> k = {0x02, 0x00, 0x86, 0x04};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Vectors, VectorEmptyOk)
{
    // an empty input vector to show that unpacking elements is okay
    std::vector<uint8_t> i{};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<uint32_t> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::vector<uint32_t> k{};
    // check that the unpacked vector is empty as expected
    ASSERT_EQ(v, k);
}

TEST(Vectors, VectorOfTuplesOk)
{
    // a vector of bytes will be unpacked verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<std::tuple<uint8_t, uint8_t>> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::vector<std::tuple<uint8_t, uint8_t>> k = {{0x02, 0x00}, {0x86, 0x04}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(Vectors, VectorOfTuplesInsufficientBytes)
{
    // a vector of bytes will be unpacked verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04, 0xb4};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::vector<std::tuple<uint8_t, uint8_t>> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::vector<std::tuple<uint8_t, uint8_t>> k = {{0x02, 0x00}, {0x86, 0x04}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

// Cannot test TooManyBytes or InsufficientBytes for vector<uint8_t>
// because it will always unpack whatever bytes are remaining
// TEST(Vectors, VectorUint8TooManyBytes) {}
// TEST(Vectors, VectorUint8InsufficientBytes) {}

TEST(UnpackAdvanced, OptionalOk)
{
    // a vector of bytes will be unpacked verbatim, low-order element first
    std::vector<uint8_t> i = {0xbe, 0x02, 0x00, 0x86, 0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::optional<std::tuple<uint8_t, uint32_t>> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    std::optional<std::tuple<uint8_t, uint32_t>> k{{0xbe, 0x04860002}};
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(UnpackAdvanced, OptionalInsufficientBytes)
{
    // a vector of bytes will be unpacked verbatim, low-order element first
    std::vector<uint8_t> i = {0x02, 0x00, 0x86, 0x04};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    std::optional<std::tuple<uint8_t, uint32_t>> v;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    std::optional<std::tuple<uint8_t, uint32_t>> k;
    // check that the bytes were correctly unpacked (in byte order)
    ASSERT_EQ(v, k);
}

TEST(UnpackAdvanced, Uints)
{
    // all elements will be unpacked in order, with each multi-byte
    // element being processed LSByte first
    // v1[7:0] v2[7:0] v2[15:8] v3[7:0] v3[15:8] v3[23:16] v3[31:24]
    // v4[7:0] v4[15:8] v4[23:16] v4[31:24]
    // v4[39:25] v4[47:40] v4[55:48] v4[63:56]
    std::vector<uint8_t> i = {0x02, 0x04, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55,
                              0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v1;
    uint16_t v2;
    uint32_t v3;
    uint64_t v4;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2, v3, v4), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint8_t k1 = 0x02;
    uint16_t k2 = 0x0604;
    uint32_t k3 = 0x44332211;
    uint64_t k4 = 0xccbbaa9988776655ull;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
}

TEST(UnpackAdvanced, TupleInts)
{
    // all elements will be unpacked in order, with each multi-byte
    // element being processed LSByte first
    // v1[7:0] v2[7:0] v2[15:8] v3[7:0] v3[15:8] v3[23:16] v3[31:24]
    // v4[7:0] v4[15:8] v4[23:16] v4[31:24]
    // v4[39:25] v4[47:40] v4[55:48] v4[63:56]
    std::vector<uint8_t> i = {0x02, 0x04, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55,
                              0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v1;
    uint16_t v2;
    uint32_t v3;
    uint64_t v4;
    auto v = std::make_tuple(v1, v2, v3, v4);
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint8_t k1 = 0x02;
    uint16_t k2 = 0x0604;
    uint32_t k3 = 0x44332211;
    uint64_t k4 = 0xccbbaa9988776655ull;
    auto k = std::make_tuple(k1, k2, k3, k4);
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v, k);
}

TEST(UnpackAdvanced, BoolsnBitfieldsnFixedIntsOhMy)
{
    // each element will be unpacked, filling the low-order bits first
    // with multi-byte values getting unpacked LSByte first
    // v1 will use k[0][1:0]
    // v2 will use k[0][2]
    // v3[4:0] will use k[0][7:3], v3[6:5] will use k[1][1:0]
    // v4 will use k[1][2]
    // v5 will use k[1][7:3]
    std::vector<uint8_t> i = {0x9e, 0xdb};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint2_t v1;
    bool v2;
    std::bitset<7> v3;
    bool v4;
    uint5_t v5;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2, v3, v4, v5), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint2_t k1 = 2;          // binary 0b10
    bool k2 = true;          // binary 0b1
    std::bitset<7> k3(0x73); // binary 0b1110011
    bool k4 = false;         // binary 0b0
    uint5_t k5 = 27;         // binary 0b11011
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
    ASSERT_EQ(v5, k5);
}

TEST(UnpackAdvanced, UnalignedBitUnpacking)
{
    // unaligned multi-byte values will be unpacked the same as
    // other bits, effectively reading from a large value, low-order
    // bits first, then consuming the stream LSByte first
    // v1 will use k[0][1:0]
    // v2[5:0] will use k[0][7:2], v2[7:6] will use k[1][1:0]
    // v3 will use k[1][2]
    // v4[4:0] will use k[1][7:3] v4[12:5] will use k[2][7:0]
    // v4[15:13] will use k[3][2:0]
    // v5 will use k[3][3]
    // v6[3:0] will use k[3][7:0] v6[11:4] will use k[4][7:0]
    // v6[19:12] will use k[5][7:0] v6[27:20] will use k[6][7:0]
    // v6[31:28] will use k[7][3:0]
    // v7 will use k[7][7:4]
    std::vector<uint8_t> i = {0x96, 0xd2, 0x2a, 0xcd, 0xd3, 0x3b, 0xbc, 0x9d};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint2_t v1;
    uint8_t v2;
    bool v3;
    uint16_t v4;
    bool v5;
    uint32_t v6;
    uint4_t v7;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, v2, v3, v4, v5, v6, v7), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint2_t k1 = 2;           // binary 0b10
    uint8_t k2 = 0xa5;        // binary 0b10100101
    bool k3 = false;          // binary 0b0
    uint16_t k4 = 0xa55a;     // binary 0b1010010101011010
    bool k5 = true;           // binary 0b1
    uint32_t k6 = 0xdbc3bd3c; // binary 0b11011011110000111011110100111100
    uint4_t k7 = 9;           // binary 0b1001
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    ASSERT_EQ(v2, k2);
    ASSERT_EQ(v3, k3);
    ASSERT_EQ(v4, k4);
    ASSERT_EQ(v5, k5);
    ASSERT_EQ(v6, k6);
    ASSERT_EQ(v7, k7);
}
