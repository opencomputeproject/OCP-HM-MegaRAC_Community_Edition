#include "config.h"

#include "image_verify.hpp"

#include "images.hpp"
#include "version.hpp"

#include <fcntl.h>
#include <openssl/err.h>
#include <sys/stat.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <fstream>
#include <set>

namespace phosphor
{
namespace software
{
namespace image
{

using namespace phosphor::logging;
using namespace phosphor::software::manager;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;

constexpr auto keyTypeTag = "KeyType";
constexpr auto hashFunctionTag = "HashType";

Signature::Signature(const fs::path& imageDirPath,
                     const fs::path& signedConfPath) :
    imageDirPath(imageDirPath),
    signedConfPath(signedConfPath)
{
    fs::path file(imageDirPath / MANIFEST_FILE_NAME);

    keyType = Version::getValue(file, keyTypeTag);
    hashType = Version::getValue(file, hashFunctionTag);
}

AvailableKeyTypes Signature::getAvailableKeyTypesFromSystem() const
{
    AvailableKeyTypes keyTypes{};

    // Find the path of all the files
    if (!fs::is_directory(signedConfPath))
    {
        log<level::ERR>("Signed configuration path not found in the system");
        elog<InternalFailure>();
    }

    // Look for all the hash and public key file names get the key value
    // For example:
    // /etc/activationdata/OpenBMC/publickey
    // /etc/activationdata/OpenBMC/hashfunc
    // /etc/activationdata/GA/publickey
    // /etc/activationdata/GA/hashfunc
    // Set will have OpenBMC, GA

    for (const auto& p : fs::recursive_directory_iterator(signedConfPath))
    {
        if ((p.path().filename() == HASH_FILE_NAME) ||
            (p.path().filename() == PUBLICKEY_FILE_NAME))
        {
            // extract the key types
            // /etc/activationdata/OpenBMC/  -> get OpenBMC from the path
            auto key = p.path().parent_path();
            keyTypes.insert(key.filename());
        }
    }

    return keyTypes;
}

inline KeyHashPathPair Signature::getKeyHashFileNames(const Key_t& key) const
{
    fs::path hashpath(signedConfPath / key / HASH_FILE_NAME);
    fs::path keyPath(signedConfPath / key / PUBLICKEY_FILE_NAME);

    return std::make_pair(std::move(hashpath), std::move(keyPath));
}

bool Signature::verify()
{
    try
    {
        // Verify the MANIFEST and publickey file using available
        // public keys and hash on the system.
        if (false == systemLevelVerify())
        {
            log<level::ERR>("System level Signature Validation failed");
            return false;
        }

        // image specific publickey file name.
        fs::path publicKeyFile(imageDirPath / PUBLICKEY_FILE_NAME);

        // Validate the BMC image files.
        for (const auto& bmcImage : bmcImages)
        {
            // Build Image File name
            fs::path file(imageDirPath);
            file /= bmcImage;

            // Build Signature File name
            fs::path sigFile(file);
            sigFile.replace_extension(SIGNATURE_FILE_EXT);

            // Verify the signature.
            auto valid = verifyFile(file, sigFile, publicKeyFile, hashType);
            if (valid == false)
            {
                log<level::ERR>("Image file Signature Validation failed",
                                entry("IMAGE=%s", bmcImage.c_str()));
                return false;
            }
        }
        // Validate the optional image files.
        auto optionalImages = getOptionalImages();
        for (const auto& optionalImage : optionalImages)
        {
            // Build Image File name
            fs::path file(imageDirPath);
            file /= optionalImage;

            if (fs::exists(file))
            {
                // Build Signature File name
                fs::path sigFile(file);
                sigFile.replace_extension(SIGNATURE_FILE_EXT);

                // Verify the signature.
                auto valid = verifyFile(file, sigFile, publicKeyFile, hashType);
                if (valid == false)
                {
                    log<level::ERR>("Image file Signature Validation failed",
                                    entry("IMAGE=%s", optionalImage.c_str()));
                    return false;
                }
            }
        }

        log<level::DEBUG>("Successfully completed Signature vaildation.");

        return true;
    }
    catch (const InternalFailure& e)
    {
        return false;
    }
    catch (const std::exception& e)
    {
        log<level::ERR>(e.what());
        return false;
    }
}

bool Signature::systemLevelVerify()
{
    // Get available key types from the system.
    auto keyTypes = getAvailableKeyTypesFromSystem();
    if (keyTypes.empty())
    {
        log<level::ERR>("Missing Signature configuration data in system");
        elog<InternalFailure>();
    }

    // Build publickey and its signature file name.
    fs::path pkeyFile(imageDirPath / PUBLICKEY_FILE_NAME);
    fs::path pkeyFileSig(pkeyFile);
    pkeyFileSig.replace_extension(SIGNATURE_FILE_EXT);

    // Build manifest and its signature file name.
    fs::path manifestFile(imageDirPath / MANIFEST_FILE_NAME);
    fs::path manifestFileSig(manifestFile);
    manifestFileSig.replace_extension(SIGNATURE_FILE_EXT);

    auto valid = false;

    // Verify the file signature with available key types
    // public keys and hash function.
    // For any internal failure during the key/hash pair specific
    // validation, should continue the validation with next
    // available Key/hash pair.
    for (const auto& keyType : keyTypes)
    {
        auto keyHashPair = getKeyHashFileNames(keyType);

        auto hashFunc = Version::getValue(keyHashPair.first, hashFunctionTag);

        try
        {
            // Verify manifest file signature
            valid = verifyFile(manifestFile, manifestFileSig,
                               keyHashPair.second, hashFunc);
            if (valid)
            {
                // Verify publickey file signature.
                valid = verifyFile(pkeyFile, pkeyFileSig, keyHashPair.second,
                                   hashFunc);
                if (valid)
                {
                    break;
                }
            }
        }
        catch (const InternalFailure& e)
        {
            valid = false;
        }
    }
    return valid;
}

bool Signature::verifyFile(const fs::path& file, const fs::path& sigFile,
                           const fs::path& publicKey,
                           const std::string& hashFunc)
{

    // Check existence of the files in the system.
    if (!(fs::exists(file) && fs::exists(sigFile)))
    {
        log<level::ERR>("Failed to find the Data or signature file.",
                        entry("FILE=%s", file.c_str()));
        elog<InternalFailure>();
    }

    // Create RSA.
    auto publicRSA = createPublicRSA(publicKey);
    if (publicRSA == nullptr)
    {
        log<level::ERR>("Failed to create RSA",
                        entry("FILE=%s", publicKey.c_str()));
        elog<InternalFailure>();
    }

    // Assign key to RSA.
    EVP_PKEY_Ptr pKeyPtr(EVP_PKEY_new(), ::EVP_PKEY_free);
    EVP_PKEY_assign_RSA(pKeyPtr.get(), publicRSA);

    // Initializes a digest context.
    EVP_MD_CTX_Ptr rsaVerifyCtx(EVP_MD_CTX_new(), ::EVP_MD_CTX_free);

    // Adds all digest algorithms to the internal table
    OpenSSL_add_all_digests();

    // Create Hash structure.
    auto hashStruct = EVP_get_digestbyname(hashFunc.c_str());
    if (!hashStruct)
    {
        log<level::ERR>("EVP_get_digestbynam: Unknown message digest",
                        entry("HASH=%s", hashFunc.c_str()));
        elog<InternalFailure>();
    }

    auto result = EVP_DigestVerifyInit(rsaVerifyCtx.get(), nullptr, hashStruct,
                                       nullptr, pKeyPtr.get());

    if (result <= 0)
    {
        log<level::ERR>("Error occurred during EVP_DigestVerifyInit",
                        entry("ERRCODE=%lu", ERR_get_error()));
        elog<InternalFailure>();
    }

    // Hash the data file and update the verification context
    auto size = fs::file_size(file);
    auto dataPtr = mapFile(file, size);

    result = EVP_DigestVerifyUpdate(rsaVerifyCtx.get(), dataPtr(), size);
    if (result <= 0)
    {
        log<level::ERR>("Error occurred during EVP_DigestVerifyUpdate",
                        entry("ERRCODE=%lu", ERR_get_error()));
        elog<InternalFailure>();
    }

    // Verify the data with signature.
    size = fs::file_size(sigFile);
    auto signature = mapFile(sigFile, size);

    result = EVP_DigestVerifyFinal(
        rsaVerifyCtx.get(), reinterpret_cast<unsigned char*>(signature()),
        size);

    // Check the verification result.
    if (result < 0)
    {
        log<level::ERR>("Error occurred during EVP_DigestVerifyFinal",
                        entry("ERRCODE=%lu", ERR_get_error()));
        elog<InternalFailure>();
    }

    if (result == 0)
    {
        log<level::ERR>("EVP_DigestVerifyFinal:Signature validation failed",
                        entry("PATH=%s", sigFile.c_str()));
        return false;
    }
    return true;
}

inline RSA* Signature::createPublicRSA(const fs::path& publicKey)
{
    RSA* rsa = nullptr;
    auto size = fs::file_size(publicKey);

    // Read public key file
    auto data = mapFile(publicKey, size);

    BIO_MEM_Ptr keyBio(BIO_new_mem_buf(data(), -1), &::BIO_free);
    if (keyBio.get() == nullptr)
    {
        log<level::ERR>("Failed to create new BIO Memory buffer");
        elog<InternalFailure>();
    }

    rsa = PEM_read_bio_RSA_PUBKEY(keyBio.get(), &rsa, nullptr, nullptr);

    return rsa;
}

CustomMap Signature::mapFile(const fs::path& path, size_t size)
{

    CustomFd fd(open(path.c_str(), O_RDONLY));

    return CustomMap(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd(), 0),
                     size);
}

} // namespace image
} // namespace software
} // namespace phosphor
