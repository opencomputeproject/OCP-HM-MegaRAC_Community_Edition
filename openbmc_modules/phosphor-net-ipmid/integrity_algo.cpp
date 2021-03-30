#include "integrity_algo.hpp"

#include "message_parsers.hpp"

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace cipher
{

namespace integrity
{

AlgoSHA1::AlgoSHA1(const std::vector<uint8_t>& sik) :
    Interface(SHA1_96_AUTHCODE_LENGTH)
{
    k1 = generateKn(sik, rmcp::const_1);
}

std::vector<uint8_t> AlgoSHA1::generateHMAC(const uint8_t* input,
                                            const size_t len) const
{
    std::vector<uint8_t> output(SHA_DIGEST_LENGTH);
    unsigned int mdLen = 0;

    if (HMAC(EVP_sha1(), k1.data(), k1.size(), input, len, output.data(),
             &mdLen) == NULL)
    {
        throw std::runtime_error("Generating integrity data failed");
    }

    // HMAC generates Message Digest to the size of SHA_DIGEST_LENGTH, the
    // AuthCode field length is based on the integrity algorithm. So we are
    // interested only in the AuthCode field length of the generated Message
    // digest.
    output.resize(authCodeLength);

    return output;
}

bool AlgoSHA1::verifyIntegrityData(
    const std::vector<uint8_t>& packet, const size_t length,
    std::vector<uint8_t>::const_iterator integrityData) const
{

    auto output = generateHMAC(
        packet.data() + message::parser::RMCP_SESSION_HEADER_SIZE, length);

    // Verify if the generated integrity data for the packet and the received
    // integrity data matches.
    return (std::equal(output.begin(), output.end(), integrityData));
}

std::vector<uint8_t>
    AlgoSHA1::generateIntegrityData(const std::vector<uint8_t>& packet) const
{
    return generateHMAC(
        packet.data() + message::parser::RMCP_SESSION_HEADER_SIZE,
        packet.size() - message::parser::RMCP_SESSION_HEADER_SIZE);
}

std::vector<uint8_t> AlgoSHA1::generateKn(const std::vector<uint8_t>& sik,
                                          const rmcp::Const_n& const_n) const
{
    unsigned int mdLen = 0;
    std::vector<uint8_t> Kn(sik.size());

    // Generated Kn for the integrity algorithm with the additional key keyed
    // with SIK.
    if (HMAC(EVP_sha1(), sik.data(), sik.size(), const_n.data(), const_n.size(),
             Kn.data(), &mdLen) == NULL)
    {
        throw std::runtime_error("Generating KeyN for integrity "
                                 "algorithm failed");
    }
    return Kn;
}

AlgoSHA256::AlgoSHA256(const std::vector<uint8_t>& sik) :
    Interface(SHA256_128_AUTHCODE_LENGTH)
{
    k1 = generateKn(sik, rmcp::const_1);
}

std::vector<uint8_t> AlgoSHA256::generateHMAC(const uint8_t* input,
                                              const size_t len) const
{
    std::vector<uint8_t> output(SHA256_DIGEST_LENGTH);
    unsigned int mdLen = 0;

    if (HMAC(EVP_sha256(), k1.data(), k1.size(), input, len, output.data(),
             &mdLen) == NULL)
    {
        throw std::runtime_error("Generating HMAC_SHA256_128 failed");
    }

    // HMAC generates Message Digest to the size of SHA256_DIGEST_LENGTH, the
    // AuthCode field length is based on the integrity algorithm. So we are
    // interested only in the AuthCode field length of the generated Message
    // digest.
    output.resize(authCodeLength);

    return output;
}

bool AlgoSHA256::verifyIntegrityData(
    const std::vector<uint8_t>& packet, const size_t length,
    std::vector<uint8_t>::const_iterator integrityData) const
{

    auto output = generateHMAC(
        packet.data() + message::parser::RMCP_SESSION_HEADER_SIZE, length);

    // Verify if the generated integrity data for the packet and the received
    // integrity data matches.
    return (std::equal(output.begin(), output.end(), integrityData));
}

std::vector<uint8_t>
    AlgoSHA256::generateIntegrityData(const std::vector<uint8_t>& packet) const
{
    return generateHMAC(
        packet.data() + message::parser::RMCP_SESSION_HEADER_SIZE,
        packet.size() - message::parser::RMCP_SESSION_HEADER_SIZE);
}

std::vector<uint8_t> AlgoSHA256::generateKn(const std::vector<uint8_t>& sik,
                                            const rmcp::Const_n& const_n) const
{
    unsigned int mdLen = 0;
    std::vector<uint8_t> Kn(sik.size());

    // Generated Kn for the integrity algorithm with the additional key keyed
    // with SIK.
    if (HMAC(EVP_sha256(), sik.data(), sik.size(), const_n.data(),
             const_n.size(), Kn.data(), &mdLen) == NULL)
    {
        throw std::runtime_error("Generating KeyN for integrity "
                                 "algorithm HMAC_SHA256 failed");
    }
    return Kn;
}

} // namespace integrity

} // namespace cipher
