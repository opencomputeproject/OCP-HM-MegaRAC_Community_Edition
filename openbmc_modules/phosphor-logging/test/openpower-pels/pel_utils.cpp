/**
 * Copyright © 2019 IBM Corporation
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
#include "pel_utils.hpp"

#include "extensions/openpower-pels/private_header.hpp"
#include "extensions/openpower-pels/user_header.hpp"

#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;
using namespace openpower::pels;

std::filesystem::path CleanLogID::pelIDFile{};
std::filesystem::path CleanPELFiles::pelIDFile{};
std::filesystem::path CleanPELFiles::repoPath{};
std::filesystem::path CleanPELFiles::registryPath{};

const std::vector<uint8_t> privateHeaderSection{
    // section header
    0x50, 0x48, // ID 'PH'
    0x00, 0x30, // Size
    0x01, 0x02, // version, subtype
    0x03, 0x04, // comp ID

    0x20, 0x30, 0x05, 0x09, 0x11, 0x1E, 0x1,  0x63, // create timestamp
    0x20, 0x31, 0x06, 0x0F, 0x09, 0x22, 0x3A, 0x00, // commit timestamp
    0xAA,                                           // creatorID
    0x00,                                           // logtype
    0x00,                                           // reserved
    0x02,                                           // section count
    0x90, 0x91, 0x92, 0x93,                         // OpenBMC log ID
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0,    // creator version
    0x50, 0x51, 0x52, 0x53,                         // plid
    0x80, 0x81, 0x82, 0x83};                        // PEL ID

const std::vector<uint8_t> userHeaderSection{
    // section header
    0x55, 0x48, // ID 'UH'
    0x00, 0x18, // Size
    0x01, 0x0A, // version, subtype
    0x0B, 0x0C, // comp ID

    0x10, 0x04,             // subsystem, scope
    0x20, 0x00,             // severity, type
    0x00, 0x00, 0x00, 0x00, // reserved
    0x03, 0x04,             // problem domain, vector
    0x80, 0xC0,             // action flags
    0x00, 0x00, 0x00, 0x00  // reserved
};

const std::vector<uint8_t> srcSectionNoCallouts{

    // Header
    'P', 'S', 0x00, 0x50, 0x01, 0x01, 0x02, 0x02,

    0x02, 0x00, 0x00, // version, flags, reserved
    0x09, 0x00, 0x00, // hex word count, reserved2B
    0x00, 0x48,       // SRC structure size

    // Hex words 2 - 9
    0x02, 0x02, 0x02, 0x55, 0x03, 0x03, 0x03, 0x10, 0x04, 0x04, 0x04, 0x04,
    0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09,
    // ASCII string
    'B', 'D', '8', 'D', '5', '6', '7', '8', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' '};

const std::vector<uint8_t> failingMTMSSection{
    // Header
    0x4D, 0x54, 0x00, 0x1C, 0x01, 0x00, 0x20, 0x00,

    'T',  'T',  'T',  'T',  '-',  'M',  'M',  'M',  '1', '2',
    '3',  '4',  '5',  '6',  '7',  '8',  '9',  'A',  'B', 'C'};

const std::vector<uint8_t> UserDataSection{
    // Header
    0x55, 0x44, 0x00, 0x10, 0x00, 0x00, 0x20, 0x00,

    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

const std::vector<uint8_t> ExtUserHeaderSection{
    // Header
    'E', 'H', 0x00, 0x60, 0x01, 0x00, 0x03, 0x04,

    // MTMS
    'T', 'T', 'T', 'T', '-', 'M', 'M', 'M', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C',

    // Server FW version
    'S', 'E', 'R', 'V', 'E', 'R', '_', 'V', 'E', 'R', 'S', 'I', 'O', 'N', '\0',
    '\0',

    // Subsystem FW Version
    'B', 'M', 'C', '_', 'V', 'E', 'R', 'S', 'I', 'O', 'N', '\0', '\0', '\0',
    '\0', '\0',

    0x00, 0x00, 0x00, 0x00,                         // Reserved
    0x20, 0x25, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, // Ref time
    0x00, 0x00, 0x00,                               // Reserved

    // SymptomID length and symptom ID
    20, 'B', 'D', '8', 'D', '4', '2', '0', '0', '_', '1', '2', '3', '4', '5',
    '6', '7', '8', '\0', '\0', '\0'};

const std::vector<uint8_t> srcFRUIdentityCallout{
    'I', 'D', 0x1C, 0x1D,                     // type, size, flags
    '1', '2', '3',  '4',                      // PN
    '5', '6', '7',  0x00, 'A', 'A', 'A', 'A', // CCIN
    '1', '2', '3',  '4',  '5', '6', '7', '8', // SN
    '9', 'A', 'B',  'C'};

const std::vector<uint8_t> srcPCEIdentityCallout{
    'P', 'E', 0x24, 0x00,                      // type, size, flags
    'T', 'T', 'T',  'T',  '-', 'M', 'M',  'M', // MTM
    '1', '2', '3',  '4',  '5', '6', '7',       // SN
    '8', '9', 'A',  'B',  'C', 'P', 'C',  'E', // Name + null padded
    'N', 'A', 'M',  'E',  '1', '2', 0x00, 0x00, 0x00};

const std::vector<uint8_t> srcMRUCallout{
    'M',  'R',  0x28, 0x04, // ID, size, flags
    0x00, 0x00, 0x00, 0x00, // Reserved
    0x00, 0x00, 0x00, 'H',  // priority 0
    0x01, 0x01, 0x01, 0x01, // MRU ID 0
    0x00, 0x00, 0x00, 'M',  // priority 1
    0x02, 0x02, 0x02, 0x02, // MRU ID 1
    0x00, 0x00, 0x00, 'L',  // priority 2
    0x03, 0x03, 0x03, 0x03, // MRU ID 2
    0x00, 0x00, 0x00, 'H',  // priority 3
    0x04, 0x04, 0x04, 0x04, // MRU ID 3
};

constexpr size_t sectionCountOffset = 27;
constexpr size_t createTimestampPHOffset = 8;
constexpr size_t commitTimestampPHOffset = 16;
constexpr size_t creatorPHOffset = 24;
constexpr size_t obmcIDPHOffset = 28;
constexpr size_t plidPHOffset = 40;
constexpr size_t pelIDPHOffset = 44;
constexpr size_t sevUHOffset = 10;
constexpr size_t actionFlagsUHOffset = 18;

std::vector<uint8_t> pelDataFactory(TestPELType type)
{
    std::vector<uint8_t> data;

    switch (type)
    {
        case TestPELType::pelSimple:
            data.insert(data.end(), privateHeaderSection.begin(),
                        privateHeaderSection.end());
            data.insert(data.end(), userHeaderSection.begin(),
                        userHeaderSection.end());
            data.insert(data.end(), srcSectionNoCallouts.begin(),
                        srcSectionNoCallouts.end());
            data.insert(data.end(), failingMTMSSection.begin(),
                        failingMTMSSection.end());
            data.insert(data.end(), UserDataSection.begin(),
                        UserDataSection.end());
            data.insert(data.end(), ExtUserHeaderSection.begin(),
                        ExtUserHeaderSection.end());
            data.at(sectionCountOffset) = 6;
            break;
        case TestPELType::privateHeaderSection:
            data.insert(data.end(), privateHeaderSection.begin(),
                        privateHeaderSection.end());
            break;
        case TestPELType::userHeaderSection:
            data.insert(data.end(), userHeaderSection.begin(),
                        userHeaderSection.end());
            break;
        case TestPELType::primarySRCSection:
            data.insert(data.end(), srcSectionNoCallouts.begin(),
                        srcSectionNoCallouts.end());
            break;
        case TestPELType::primarySRCSection2Callouts:
        {
            // Start with the no-callouts SRC, and add the callouts section
            // from above.
            auto src = srcSectionNoCallouts;
            auto callouts =
                srcDataFactory(TestSRCType::calloutSection2Callouts);

            src.insert(src.end(), callouts.begin(), callouts.end());

            // Set the flag that says there are callouts
            // One byte after the 8B header
            src[8 + 1] |= 0x01;

            // Set the new sizes
            uint16_t size = src.size();
            Stream stream{src};

            stream.offset(2); // In the header
            stream << size;

            // In the SRC - the size field doesn't include the header
            size -= 8;
            stream.offset(8 + 6);
            stream << size;

            data.insert(data.end(), src.begin(), src.end());
            break;
        }
        case TestPELType::failingMTMSSection:
            data.insert(data.end(), failingMTMSSection.begin(),
                        failingMTMSSection.end());
    }
    return data;
}

std::vector<uint8_t> pelFactory(uint32_t id, char creatorID, uint8_t severity,
                                uint16_t actionFlags, size_t size)
{
    std::vector<uint8_t> data;
    size_t offset = 0;

    auto now = std::chrono::system_clock::now();
    auto timestamp = getBCDTime(now);

    // Start with the default Private Header, and modify it
    data.insert(data.end(), privateHeaderSection.begin(),
                privateHeaderSection.end());
    data.at(creatorPHOffset) = creatorID;

    // Modify the multibyte fields in it
    Stream stream{data};
    stream.offset(createTimestampPHOffset);
    stream << timestamp;
    stream.offset(commitTimestampPHOffset);
    stream << timestamp;
    stream.offset(plidPHOffset);
    stream << id;
    stream.offset(pelIDPHOffset);
    stream << id;
    stream.offset(obmcIDPHOffset);
    stream << id + 500;

    offset = data.size();

    // User Header
    data.insert(data.end(), userHeaderSection.begin(), userHeaderSection.end());
    data.at(offset + sevUHOffset) = severity;
    data.at(offset + actionFlagsUHOffset) = actionFlags >> 8;
    data.at(offset + actionFlagsUHOffset + 1) = actionFlags;

    // Use the default SRC, failing MTMS, and ext user Header sections
    data.insert(data.end(), srcSectionNoCallouts.begin(),
                srcSectionNoCallouts.end());
    data.insert(data.end(), failingMTMSSection.begin(),
                failingMTMSSection.end());
    data.insert(data.end(), ExtUserHeaderSection.begin(),
                ExtUserHeaderSection.end());

    data.at(sectionCountOffset) = 5;

    // Require the size to be enough for all the above sections.
    assert(size >= data.size());
    assert(size <= 16384);

    // Add a UserData section to get the size we need.
    auto udSection = UserDataSection;
    udSection.resize(size - data.size());

    if (!udSection.empty())
    {
        // At least has to be 8B for the header
        assert(udSection.size() >= 8);

        // UD sections must be 4B aligned
        assert(udSection.size() % 4 == 0);

        // Set the new size in the section heder
        Stream udStream{udSection};
        udStream.offset(2);
        udStream << static_cast<uint16_t>(udSection.size());

        data.insert(data.end(), udSection.begin(), udSection.end());
        data[sectionCountOffset]++;
    }

    assert(size == data.size());
    return data;
}

std::vector<uint8_t> srcDataFactory(TestSRCType type)
{
    switch (type)
    {
        case TestSRCType::fruIdentityStructure:
            return srcFRUIdentityCallout;

        case TestSRCType::pceIdentityStructure:
            return srcPCEIdentityCallout;

        case TestSRCType::mruStructure:
            return srcMRUCallout;

        case TestSRCType::calloutStructureA:
        {
            // Add just the FRU identity substructure to the base structure
            std::vector<uint8_t> data{
                0xFF, 0x28, 'H', 4,   // size, flags, priority, LC length
                'U',  '4',  '2', 0x00 // LC
            };

            data.insert(data.end(), srcFRUIdentityCallout.begin(),
                        srcFRUIdentityCallout.end());

            // The final size
            data[0] = data.size();
            return data;
        }
        case TestSRCType::calloutStructureB:
        {
            // Add all 3 substructures to the base structure

            std::vector<uint8_t> data{
                0xFF, 0x2F, 'L', 8, // size, flags, priority, LC length
                'U',  '1',  '2', '-', 'P', '1', 0x00, 0x00 // LC
            };
            data.insert(data.end(), srcFRUIdentityCallout.begin(),
                        srcFRUIdentityCallout.end());
            data.insert(data.end(), srcPCEIdentityCallout.begin(),
                        srcPCEIdentityCallout.end());
            data.insert(data.end(), srcMRUCallout.begin(), srcMRUCallout.end());

            // The final size
            data[0] = data.size();
            return data;
        }
        case TestSRCType::calloutSection2Callouts:
        {
            std::vector<uint8_t> data{0xC0, 0x00, 0x00,
                                      0x00}; // ID, flags, length in words

            // Add 2 callouts
            auto callout = srcDataFactory(TestSRCType::calloutStructureA);
            data.insert(data.end(), callout.begin(), callout.end());

            callout = srcDataFactory(TestSRCType::calloutStructureB);
            data.insert(data.end(), callout.begin(), callout.end());

            // Set the actual word length value at offset 2
            Stream stream{data};
            uint16_t wordLength = data.size() / 4;
            stream.offset(2);
            stream << wordLength;
            stream.offset(0);

            return data;
        }
    }
    return {};
}

std::unique_ptr<std::vector<uint8_t>> readPELFile(const fs::path& path)
{
    std::ifstream file{path};

    auto pel = std::make_unique<std::vector<uint8_t>>(
        std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return pel;
}
