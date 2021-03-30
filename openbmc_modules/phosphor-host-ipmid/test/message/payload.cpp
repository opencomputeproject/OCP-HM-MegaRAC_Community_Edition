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
#define SD_JOURNAL_SUPPRESS_LOCATION

#include <systemd/sd-journal.h>

#include <ipmid/api.hpp>
#include <ipmid/message.hpp>
#include <stdexcept>

#include <gtest/gtest.h>

TEST(Payload, InputSize)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    size_t input_size = i.size();
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    ASSERT_EQ(input_size, p.size());
}

TEST(Payload, OutputSize)
{
    ipmi::message::Payload p;
    ASSERT_EQ(0, p.size());
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    p.pack(i);
    ASSERT_EQ(i.size(), p.size());
    p.pack(i);
    p.pack(i);
    ASSERT_EQ(3 * i.size(), p.size());
}

TEST(Payload, Resize)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p;
    p.pack(i);
    p.resize(16);
    ASSERT_EQ(p.size(), 16);
}

TEST(Payload, Data)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p;
    p.pack(i);
    ASSERT_NE(nullptr, p.data());
}

TEST(PayloadResponse, Append)
{
    std::string s("0123456789abcdef");
    ipmi::message::Payload p;
    p.append(s.data(), s.data() + s.size());
    ASSERT_EQ(s.size(), p.size());
}

TEST(PayloadResponse, AppendDrain)
{
    std::string s("0123456789abcdef");
    ipmi::message::Payload p;
    bool b = true;
    // first pack a lone bit
    p.pack(b);
    p.append(s.data(), s.data() + s.size());
    // append will 'drain' first, padding the lone bit into a full byte
    ASSERT_EQ(s.size() + 1, p.size());
}

TEST(PayloadResponse, AppendBits)
{
    ipmi::message::Payload p;
    p.appendBits(3, 0b101);
    ASSERT_EQ(p.bitStream, 0b101);
    p.appendBits(4, 0b1100);
    ASSERT_EQ(p.bitStream, 0b1100101);
    p.appendBits(1, 0b1);
    ASSERT_EQ(p.bitStream, 0);
    ASSERT_EQ(p.bitCount, 0);
    // appended 8 bits, should be one byte
    ASSERT_EQ(p.size(), 1);
    std::vector<uint8_t> k1 = {0b11100101};
    ASSERT_EQ(p.raw, k1);
    p.appendBits(7, 0b1110111);
    // appended 7 more bits, should still be one byte
    ASSERT_EQ(p.size(), 1);
    p.drain();
    // drain forces padding; should be two bytes now
    ASSERT_EQ(p.size(), 2);
    std::vector<uint8_t> k2 = {0b11100101, 0b01110111};
    ASSERT_EQ(p.raw, k2);
}

TEST(PayloadResponse, Drain16Bits)
{
    ipmi::message::Payload p;
    p.bitStream = 0b1011010011001111;
    p.bitCount = 16;
    p.drain();
    ASSERT_EQ(p.size(), 2);
    ASSERT_EQ(p.bitCount, 0);
    ASSERT_EQ(p.bitStream, 0);
    std::vector<uint8_t> k1 = {0b11001111, 0b10110100};
    ASSERT_EQ(p.raw, k1);
}

TEST(PayloadResponse, Drain15Bits)
{
    ipmi::message::Payload p;
    p.bitStream = 0b101101001100111;
    p.bitCount = 15;
    p.drain();
    ASSERT_EQ(p.size(), 2);
    ASSERT_EQ(p.bitCount, 0);
    ASSERT_EQ(p.bitStream, 0);
    std::vector<uint8_t> k1 = {0b1100111, 0b1011010};
    ASSERT_EQ(p.raw, k1);
}

TEST(PayloadResponse, Drain15BitsWholeBytesOnly)
{
    ipmi::message::Payload p;
    p.bitStream = 0b101101001100111;
    p.bitCount = 15;
    p.drain(true);
    // only the first whole byte should have been 'drained' into p.raw
    ASSERT_EQ(p.size(), 1);
    ASSERT_EQ(p.bitCount, 7);
    ASSERT_EQ(p.bitStream, 0b1011010);
    std::vector<uint8_t> k1 = {0b1100111};
    ASSERT_EQ(p.raw, k1);
}

TEST(PayloadRequest, Pop)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    const auto& [vb, ve] = p.pop<uint8_t>(4);
    std::vector<uint8_t> v(vb, ve);
    std::vector<uint8_t> k = {0xbf, 0x04, 0x86, 0x00};
    ASSERT_EQ(v, k);
}

TEST(PayloadRequest, FillBits)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(5);
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0xbf);
    ASSERT_EQ(p.bitCount, 8);
    // should still have 5 bits available, no change
    p.fillBits(5);
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0xbf);
    ASSERT_EQ(p.bitCount, 8);
    // discard 5 bits (low order)
    p.popBits(5);
    // should add another byte into the stream (high order)
    p.fillBits(5);
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0x25);
    ASSERT_EQ(p.bitCount, 11);
}

TEST(PayloadRequest, FillBitsTooManyBits)
{
    std::vector<uint8_t> i = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(72);
    ASSERT_TRUE(p.unpackError);
}

TEST(PayloadRequest, FillBitsNotEnoughBytes)
{
    std::vector<uint8_t> i = {1, 2, 3, 4};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(48);
    ASSERT_TRUE(p.unpackError);
}

TEST(PayloadRequest, PopBits)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(4);
    uint8_t v = p.popBits(4);
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0x0b);
    ASSERT_EQ(p.bitCount, 4);
    ASSERT_EQ(v, 0x0f);
}

TEST(PayloadRequest, PopBitsNoFillBits)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.popBits(4);
    ASSERT_TRUE(p.unpackError);
}

TEST(PayloadRequest, DiscardBits)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(5);
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0xbf);
    ASSERT_EQ(p.bitCount, 8);
    p.discardBits();
    ASSERT_FALSE(p.unpackError);
    ASSERT_EQ(p.bitStream, 0);
    ASSERT_EQ(p.bitCount, 0);
}

TEST(PayloadRequest, FullyUnpacked)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint32_t v1;
    p.unpack(v1);
    // still one remaining byte
    ASSERT_FALSE(p.fullyUnpacked());
    p.fillBits(3);
    p.popBits(3);
    // still five remaining bits
    ASSERT_FALSE(p.fullyUnpacked());
    p.fillBits(5);
    p.popBits(5);
    // fully unpacked, should be no errors
    ASSERT_TRUE(p.fullyUnpacked());
    p.fillBits(4);
    // fullyUnpacked fails because an attempt to unpack too many bytes
    ASSERT_FALSE(p.fullyUnpacked());
}

TEST(PayloadRequest, ResetInternal)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    p.fillBits(4);
    p.unpackError = true;
    p.reset();
    ASSERT_EQ(p.rawIndex, 0);
    ASSERT_EQ(p.bitStream, 0);
    ASSERT_EQ(p.bitCount, 0);
    ASSERT_FALSE(p.unpackError);
}

TEST(PayloadRequest, ResetUsage)
{
    // Payload.reset is used to rewind the unpacking to the initial
    // state. This is needed so that OEM commands can unpack the group
    // number or the IANA to determine which handler needs to be called
    std::vector<uint8_t> i = {0x04, 0x86};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v1;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1), 0);
    // check that the payload was not fully unpacked
    ASSERT_FALSE(p.fullyUnpacked());
    uint8_t k1 = 0x04;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    // do a reset on the payload
    p.reset();
    // unpack a uint16
    uint16_t v2;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v2), 0);
    // check that the payload was fully unpacked
    ASSERT_TRUE(p.fullyUnpacked());
    uint16_t k2 = 0x8604;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v2, k2);
}

TEST(PayloadRequest, PartialPayload)
{
    std::vector<uint8_t> i = {0xbf, 0x04, 0x86, 0x00, 0x02};
    ipmi::message::Payload p(std::forward<std::vector<uint8_t>>(i));
    uint8_t v1;
    ipmi::message::Payload localPayload;
    // check that the number of bytes matches
    ASSERT_EQ(p.unpack(v1, localPayload), 0);
    // check that the payload was partially unpacked and not in error
    ASSERT_FALSE(p.fullyUnpacked());
    ASSERT_FALSE(p.unpackError);
    // check that the 'extracted' payload is not fully unpacked
    ASSERT_FALSE(localPayload.fullyUnpacked());
    uint8_t k1 = 0xbf;
    // check that the bytes were correctly unpacked (LSB first)
    ASSERT_EQ(v1, k1);
    uint32_t v2;
    // unpack using the 'extracted' payload
    ASSERT_EQ(localPayload.unpack(v2), 0);
    ASSERT_TRUE(localPayload.fullyUnpacked());
    uint32_t k2 = 0x02008604;
    ASSERT_EQ(v2, k2);
}

std::vector<std::string> logs;

extern "C" {
int sd_journal_send(const char* format, ...)
{
    logs.push_back(format);
    return 0;
}

int sd_journal_send_with_location(const char* file, const char* line,
                                  const char* func, const char* format, ...)
{
    logs.push_back(format);
    return 0;
}
}

class PayloadLogging : public testing::Test
{
  public:
    void SetUp()
    {
        logs.clear();
    }
};

TEST_F(PayloadLogging, TrailingOk)
{
    {
        ipmi::message::Payload p({1, 2});
    }
    EXPECT_EQ(logs.size(), 0);
}

TEST_F(PayloadLogging, EnforcingUnchecked)
{
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
    }
    EXPECT_EQ(logs.size(), 1);
}

TEST_F(PayloadLogging, EnforcingUncheckedUnpacked)
{
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
        uint8_t out;
        p.unpack(out, out);
    }
    EXPECT_EQ(logs.size(), 1);
}

TEST_F(PayloadLogging, EnforcingUncheckedError)
{
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
        uint32_t out;
        p.unpack(out);
    }
    EXPECT_EQ(logs.size(), 0);
}

TEST_F(PayloadLogging, EnforcingChecked)
{
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
        EXPECT_FALSE(p.fullyUnpacked());
    }
    EXPECT_EQ(logs.size(), 0);
}

TEST_F(PayloadLogging, EnforcingCheckedUnpacked)
{
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
        uint8_t out;
        p.unpack(out, out);
        EXPECT_TRUE(p.fullyUnpacked());
    }
    EXPECT_EQ(logs.size(), 0);
}

TEST_F(PayloadLogging, EnforcingUnpackPayload)
{
    {
        ipmi::message::Payload p;
        {
            ipmi::message::Payload q({1, 2});
            q.trailingOk = false;
            q.unpack(p);
        }
        EXPECT_EQ(logs.size(), 0);
    }
    EXPECT_EQ(logs.size(), 1);
}

TEST_F(PayloadLogging, EnforcingMove)
{
    {
        ipmi::message::Payload p;
        {
            ipmi::message::Payload q({1, 2});
            q.trailingOk = false;
            p = std::move(q);
        }
        EXPECT_EQ(logs.size(), 0);
    }
    EXPECT_EQ(logs.size(), 1);
}

TEST_F(PayloadLogging, EnforcingException)
{
    try
    {
        ipmi::message::Payload p({1, 2});
        p.trailingOk = false;
        throw std::runtime_error("test");
    }
    catch (...)
    {
    }
    EXPECT_EQ(logs.size(), 0);
}
