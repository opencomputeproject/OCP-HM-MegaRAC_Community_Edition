#include "crc.hpp"

namespace ipmiblob
{

/*
 * This implementation tracks the specification given at
 * http://srecord.sourceforge.net/crc16-ccitt.html
 * Code copied from internal portable sources.
 */
std::uint16_t generateCrc(const std::vector<std::uint8_t>& data)
{
    const std::uint16_t kPoly = 0x1021;
    const std::uint16_t kLeftBit = 0x8000;
    const int kExtraRounds = 2;
    const std::uint8_t* bytes = data.data();
    std::uint16_t crc = 0xFFFF;
    std::size_t i;
    std::size_t j;
    std::size_t size = data.size();

    for (i = 0; i < size + kExtraRounds; ++i)
    {
        for (j = 0; j < 8; ++j)
        {
            bool xor_flag = (crc & kLeftBit) ? 1 : 0;
            crc <<= 1;
            // If this isn't an extra round and the current byte's j'th bit from
            // the left is set, increment the CRC.
            if (i < size && (bytes[i] & (1 << (7 - j))))
            {
                crc++;
            }
            if (xor_flag)
            {
                crc ^= kPoly;
            }
        }
    }

    return crc;
}

} // namespace ipmiblob
