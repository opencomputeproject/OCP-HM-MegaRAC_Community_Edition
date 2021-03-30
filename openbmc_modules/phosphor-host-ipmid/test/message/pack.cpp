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

// TODO: Add testing of Payload response API

TEST(PackBasics, Uint8)
{
    ipmi::message::Payload p;
    uint8_t v = 4;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Uint16)
{
    ipmi::message::Payload p;
    uint16_t v = 0x8604;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04, 0x86};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Uint32)
{
    ipmi::message::Payload p;
    uint32_t v = 0x02008604;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04, 0x86, 0x00, 0x02};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Uint64)
{
    ipmi::message::Payload p;
    uint64_t v = 0x1122334402008604ull;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04, 0x86, 0x00, 0x02, 0x44, 0x33, 0x22, 0x11};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Uint24)
{
    ipmi::message::Payload p;
    uint24_t v = 0x112358;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), types::nrFixedBits<decltype(v)> / CHAR_BIT);
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x58, 0x23, 0x11};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Uint3Uint5)
{
    // individual bytes are packed low-order-bits first
    // v1 will occupy [2:0], v2 will occupy [7:3]
    ipmi::message::Payload p;
    uint3_t v1 = 0x1;
    uint5_t v2 = 0x19;
    p.pack(v1, v2);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), (types::nrFixedBits<decltype(v1)> +
                         types::nrFixedBits<decltype(v2)>) /
                            CHAR_BIT);
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0xc9};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Boolx8)
{
    // individual bytes are packed low-order-bits first
    // [v8, v7, v6, v5, v4, v3, v2, v1]
    ipmi::message::Payload p;
    bool v8 = true, v7 = true, v6 = false, v5 = false;
    bool v4 = true, v3 = false, v2 = false, v1 = true;
    p.pack(v1, v2, v3, v4, v5, v6, v7, v8);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(uint8_t));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0xc9};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Bitset8)
{
    // individual bytes are packed low-order-bits first
    // a bitset for 8 bits fills the full byte
    ipmi::message::Payload p;
    std::bitset<8> v(0xc9);
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() / CHAR_BIT);
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0xc9};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Bitset3Bitset5)
{
    // individual bytes are packed low-order-bits first
    // v1 will occupy [2:0], v2 will occupy [7:3]
    ipmi::message::Payload p;
    std::bitset<3> v1(0x1);
    std::bitset<5> v2(0x19);
    p.pack(v1, v2);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), (v1.size() + v2.size()) / CHAR_BIT);
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0xc9};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Bitset32)
{
    // individual bytes are packed low-order-bits first
    // v1 will occupy 4 bytes, but in LSByte first order
    // v1[7:0] v1[15:9] v1[23:16] v1[31:24]
    ipmi::message::Payload p;
    std::bitset<32> v(0x02008604);
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() / CHAR_BIT);
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04, 0x86, 0x00, 0x02};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Tuple)
{
    // tuples are the new struct, pack a tuple
    ipmi::message::Payload p;
    auto v = std::make_tuple(static_cast<uint16_t>(0x8604), 'A');
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(uint16_t) + sizeof(char));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x04, 0x86, 0x41};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Array4xUint8)
{
    // an array of bytes will be output verbatim, low-order element first
    ipmi::message::Payload p;
    std::array<uint8_t, 4> v = {{0x02, 0x00, 0x86, 0x04}};
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() * sizeof(v[0]));
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {0x02, 0x00, 0x86, 0x04};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Array4xUint32)
{
    // an array of multi-byte values will be output in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    ipmi::message::Payload p;
    std::array<uint32_t, 4> v = {
        {0x11223344, 0x22446688, 0x33557799, 0x12345678}};
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() * sizeof(v[0]));
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34, 0x12};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, VectorUint32)
{
    // a vector of multi-byte values will be output in order low-order
    // element first, each multi-byte element in LSByte order
    // v[0][7:0] v[0][15:9] v[0][23:16] v[0][31:24]
    // v[1][7:0] v[1][15:9] v[1][23:16] v[1][31:24]
    // v[2][7:0] v[2][15:9] v[2][23:16] v[2][31:24]
    // v[3][7:0] v[3][15:9] v[3][23:16] v[3][31:24]
    ipmi::message::Payload p;
    std::vector<uint32_t> v = {
        {0x11223344, 0x22446688, 0x33557799, 0x12345678}};
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() * sizeof(v[0]));
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {0x44, 0x33, 0x22, 0x11, 0x88, 0x66, 0x44, 0x22,
                              0x99, 0x77, 0x55, 0x33, 0x78, 0x56, 0x34, 0x12};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, VectorUint8)
{
    // a vector of bytes will be output verbatim, low-order element first
    ipmi::message::Payload p;
    std::vector<uint8_t> v = {0x02, 0x00, 0x86, 0x04};
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), v.size() * sizeof(v[0]));
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {0x02, 0x00, 0x86, 0x04};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, VectorUnaligned)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(true, std::vector<uint8_t>{1}), 1);
    EXPECT_EQ(p.raw, std::vector<uint8_t>{0b1});
}

TEST(PackBasics, StringView)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(std::string_view{"\x24\x30\x11"}), 0);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0x24, 0x30, 0x11}));
}

TEST(PackBasics, StringViewUnaligned)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(true, std::string_view{"abc"}), 1);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1}));
}

TEST(PackBasics, OptionalEmpty)
{
    // an optional will only pack if the value is present
    ipmi::message::Payload p;
    std::optional<uint32_t> v;
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), 0);
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, OptionalContainsValue)
{
    // an optional will only pack if the value is present
    ipmi::message::Payload p;
    std::optional<uint32_t> v(0x04860002);
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(uint32_t));
    // check that the bytes were correctly packed (in byte order)
    std::vector<uint8_t> k = {0x02, 0x00, 0x86, 0x04};
    ASSERT_EQ(p.raw, k);
}

TEST(PackBasics, Payload)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(true), 0);
    EXPECT_EQ(p.pack(ipmi::message::Payload({0x24, 0x30})), 0);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1, 0x24, 0x30}));
}

TEST(PackBasics, PayloadUnaligned)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(true, ipmi::message::Payload({0x24})), 1);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1}));
}

TEST(PackBasics, PayloadOtherUnaligned)
{
    ipmi::message::Payload p, q;
    q.appendBits(1, 1);
    EXPECT_EQ(p.pack(true), 0);
    EXPECT_EQ(p.pack(q), 1);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1}));
}

TEST(PackBasics, PrependPayload)
{
    ipmi::message::Payload p;
    EXPECT_EQ(p.pack(true), 0);
    EXPECT_EQ(p.prepend(ipmi::message::Payload({0x24, 0x30})), 0);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0x24, 0x30, 0b1}));
}

TEST(PackBasics, PrependPayloadUnaligned)
{
    ipmi::message::Payload p;
    p.appendBits(1, 1);
    EXPECT_EQ(p.prepend(ipmi::message::Payload({0x24})), 1);
    p.drain();
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1}));
}

TEST(PackBasics, PrependPayloadOtherUnaligned)
{
    ipmi::message::Payload p, q;
    q.appendBits(1, 1);
    EXPECT_EQ(p.pack(true), 0);
    EXPECT_EQ(p.prepend(q), 1);
    EXPECT_EQ(p.raw, std::vector<uint8_t>({0b1}));
}

TEST(PackAdvanced, Uints)
{
    // all elements will be processed in order, with each multi-byte
    // element being processed LSByte first
    // v1[7:0] v2[7:0] v2[15:8] v3[7:0] v3[15:8] v3[23:16] v3[31:24]
    // v4[7:0] v4[15:8] v4[23:16] v4[31:24]
    // v4[39:25] v4[47:40] v4[55:48] v4[63:56]
    ipmi::message::Payload p;
    uint8_t v1 = 0x02;
    uint16_t v2 = 0x0604;
    uint32_t v3 = 0x44332211;
    uint64_t v4 = 0xccbbaa9988776655ull;
    p.pack(v1, v2, v3, v4);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v1) + sizeof(v2) + sizeof(v3) + sizeof(v4));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x02, 0x04, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55,
                              0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    ASSERT_EQ(p.raw, k);
}

TEST(PackAdvanced, TupleInts)
{
    // all elements will be processed in order, with each multi-byte
    // element being processed LSByte first
    // v1[7:0] v2[7:0] v2[15:8] v3[7:0] v3[15:8] v3[23:16] v3[31:24]
    // v4[7:0] v4[15:8] v4[23:16] v4[31:24]
    // v4[39:25] v4[47:40] v4[55:48] v4[63:56]
    ipmi::message::Payload p;
    uint8_t v1 = 0x02;
    uint16_t v2 = 0x0604;
    uint32_t v3 = 0x44332211;
    uint64_t v4 = 0xccbbaa9988776655ull;
    auto v = std::make_tuple(v1, v2, v3, v4);
    p.pack(v);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(v1) + sizeof(v2) + sizeof(v3) + sizeof(v4));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x02, 0x04, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55,
                              0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
    ASSERT_EQ(p.raw, k);
}

TEST(PackAdvanced, VariantArray)
{
    ipmi::message::Payload p;
    std::variant<std::array<uint8_t, 2>, uint32_t> variant;
    auto data = std::array<uint8_t, 2>{2, 4};
    variant = data;

    p.pack(variant);
    ASSERT_EQ(p.size(), sizeof(data));

    // check that the bytes were correctly packed packed (LSB first)
    std::vector<uint8_t> k = {2, 4};
    ASSERT_EQ(p.raw, k);
}

TEST(PackAdvanced, BoolsnBitfieldsnFixedIntsOhMy)
{
    // each element will be added, filling the low-order bits first
    // with multi-byte values getting added LSByte first
    // v1 will occupy k[0][1:0]
    // v2 will occupy k[0][2]
    // v3[4:0] will occupy k[0][7:3], v3[6:5] will occupy k[1][1:0]
    // v4 will occupy k[1][2]
    // v5 will occupy k[1][7:3]
    ipmi::message::Payload p;
    uint2_t v1 = 2;          // binary 0b10
    bool v2 = true;          // binary 0b1
    std::bitset<7> v3(0x73); // binary 0b1110011
    bool v4 = false;         // binary 0b0
    uint5_t v5 = 27;         // binary 0b11011
    // concat binary: 0b1101101110011110 -> 0xdb9e -> 0x9e 0xdb (LSByte first)
    p.pack(v1, v2, v3, v4, v5);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(uint16_t));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x9e, 0xdb};
    ASSERT_EQ(p.raw, k);
}

TEST(PackAdvanced, UnalignedBitPacking)
{
    // unaligned multi-byte values will be packed the same as
    // other bits, effectively building up a large value, low-order
    // bits first, then outputting a stream of LSByte values
    // v1 will occupy k[0][1:0]
    // v2[5:0] will occupy k[0][7:2], v2[7:6] will occupy k[1][1:0]
    // v3 will occupy k[1][2]
    // v4[4:0] will occupy k[1][7:3] v4[12:5] will occupy k[2][7:0]
    // v4[15:13] will occupy k[3][2:0]
    // v5 will occupy k[3][3]
    // v6[3:0] will occupy k[3][7:0] v6[11:4] will occupy k[4][7:0]
    // v6[19:12] will occupy k[5][7:0] v6[27:20] will occupy k[6][7:0]
    // v6[31:28] will occupy k[7][3:0]
    // v7 will occupy k[7][7:4]
    ipmi::message::Payload p;
    uint2_t v1 = 2;           // binary 0b10
    uint8_t v2 = 0xa5;        // binary 0b10100101
    bool v3 = false;          // binary 0b0
    uint16_t v4 = 0xa55a;     // binary 0b1010010101011010
    bool v5 = true;           // binary 0b1
    uint32_t v6 = 0xdbc3bd3c; // binary 0b11011011110000111011110100111100
    uint4_t v7 = 9;           // binary 0b1001
    // concat binary:
    //   0b1001110110111100001110111101001111001101001010101101001010010110
    //   -> 0x9dbc3bd3cd2ad296 -> 0x96 0xd2 0x2a 0xcd 0xd3 0x3b 0xbc 0x9d
    p.pack(v1, v2, v3, v4, v5, v6, v7);
    // check that the number of bytes matches
    ASSERT_EQ(p.size(), sizeof(uint64_t));
    // check that the bytes were correctly packed (LSB first)
    std::vector<uint8_t> k = {0x96, 0xd2, 0x2a, 0xcd, 0xd3, 0x3b, 0xbc, 0x9d};
    ASSERT_EQ(p.raw, k);
}

TEST(PackAdvanced, ComplexOptionalTuple)
{
    constexpr size_t macSize = 6;
    // inspired from a real-world case of Get Session Info
    constexpr uint8_t handle = 0x23;       // handle for active session
    constexpr uint8_t maxSessions = 15;    // number of possible active sessions
    constexpr uint8_t currentSessions = 4; // number of current active sessions
    std::optional<                         // only returned for active session
        std::tuple<uint8_t,                // user ID
                   uint8_t,                // privilege
                   uint4_t,                // channel number
                   uint4_t                 // protocol (RMCP+)
                   >>
        activeSession;
    std::optional<           // only returned for channel type LAN
        std::tuple<uint32_t, // IPv4 address
                   std::array<uint8_t, macSize>, // MAC address
                   uint16_t                      // port
                   >>
        lanSession;

    constexpr uint8_t userID = 7;
    constexpr uint8_t priv = 4;
    constexpr uint4_t channel = 2;
    constexpr uint4_t protocol = 1;
    activeSession.emplace(userID, priv, channel, protocol);
    constexpr std::array<uint8_t, macSize> macAddr{0};
    lanSession.emplace(0x0a010105, macAddr, 55327);

    ipmi::message::Payload p;
    p.pack(handle, maxSessions, currentSessions, activeSession, lanSession);
    ASSERT_EQ(p.size(), sizeof(handle) + sizeof(maxSessions) +
                            sizeof(currentSessions) + 3 * sizeof(uint8_t) +
                            sizeof(uint32_t) + sizeof(uint8_t) * macSize +
                            sizeof(uint16_t));
    uint8_t protocol_channel =
        (static_cast<uint8_t>(protocol) << 4) | static_cast<uint8_t>(channel);
    std::vector<uint8_t> k = {handle, maxSessions, currentSessions, userID,
                              priv, protocol_channel,
                              // ip addr
                              0x05, 0x01, 0x01, 0x0a,
                              // mac addr
                              0, 0, 0, 0, 0, 0,
                              // port
                              0x1f, 0xd8};
    ASSERT_EQ(p.raw, k);
}
