#pragma once
#include "openssl_alloc.hpp"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>
#include <set>
#include <string>

namespace phosphor
{
namespace software
{
namespace image
{

namespace fs = std::filesystem;
using Key_t = std::string;
using Hash_t = std::string;
using PublicKeyPath = fs::path;
using HashFilePath = fs::path;
using KeyHashPathPair = std::pair<HashFilePath, PublicKeyPath>;
using AvailableKeyTypes = std::set<Key_t>;

// RAII support for openSSL functions.
using BIO_MEM_Ptr = std::unique_ptr<BIO, decltype(&::BIO_free)>;
using EVP_PKEY_Ptr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
using EVP_MD_CTX_Ptr =
    std::unique_ptr<EVP_MD_CTX, decltype(&::EVP_MD_CTX_free)>;

/** @struct CustomFd
 *
 *  RAII wrapper for file descriptor.
 */
struct CustomFd
{
  public:
    CustomFd() = delete;
    CustomFd(const CustomFd&) = delete;
    CustomFd& operator=(const CustomFd&) = delete;
    CustomFd(CustomFd&&) = default;
    CustomFd& operator=(CustomFd&&) = default;
    /** @brief Saves File descriptor and uses it to do file operation
     *
     *  @param[in] fd - File descriptor
     */
    CustomFd(int fd) : fd(fd)
    {}

    ~CustomFd()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    int operator()() const
    {
        return fd;
    }

  private:
    /** @brief File descriptor */
    int fd = -1;
};

/** @struct CustomMap
 *
 *  RAII wrapper for mmap.
 */
struct CustomMap
{
  private:
    /** @brief starting address of the map   */
    void* addr;

    /** @brief length of the mapping   */
    size_t length;

  public:
    CustomMap() = delete;
    CustomMap(const CustomMap&) = delete;
    CustomMap& operator=(const CustomMap&) = delete;
    CustomMap(CustomMap&&) = default;
    CustomMap& operator=(CustomMap&&) = default;

    /** @brief Saves starting address of the map and
     *         and length of the file.
     *  @param[in]  addr - Starting address of the map
     *  @param[in]  length - length of the map
     */
    CustomMap(void* addr, size_t length) : addr(addr), length(length)
    {}

    ~CustomMap()
    {
        munmap(addr, length);
    }

    void* operator()() const
    {
        return addr;
    }
};

/** @class Signature
 *  @brief Contains signature verification functions.
 *  @details The software image class that contains the signature
 *           verification functions for signed image.
 */
class Signature
{
  public:
    Signature() = delete;
    Signature(const Signature&) = delete;
    Signature& operator=(const Signature&) = delete;
    Signature(Signature&&) = default;
    Signature& operator=(Signature&&) = default;
    ~Signature() = default;

    /**
     * @brief Constructs Signature.
     * @param[in]  imageDirPath - image path
     * @param[in]  signedConfPath - Path of public key
     *                              hash function files
     */
    Signature(const fs::path& imageDirPath, const fs::path& signedConfPath);

    /**
     * @brief Image signature verification function.
     *        Verify the Manifest and public key file signature using the
     *        public keys available in the system first. After successful
     *        validation, continue the whole image files signature
     *        validation using the image specific public key and the
     *        hash function.
     *
     *        @return true if signature verification was successful,
     *                     false if not
     */
    bool verify();

  private:
    /**
     * @brief Function used for system level file signature validation
     *        of image specific publickey file and manifest file
     *        using the available public keys and hash functions
     *        in the system.
     *        Refer code-update documentation for more details.
     */
    bool systemLevelVerify();

    /**
     *  @brief Return all key types stored in the BMC based on the
     *         public key and hashfunc files stored in the BMC.
     *
     *  @return list
     */
    AvailableKeyTypes getAvailableKeyTypesFromSystem() const;

    /**
     *  @brief Return public key and hash function file names for the
     *  corresponding key type
     *
     *  @param[in]  key - key type
     *  @return Pair of hash and public key file names
     */
    inline KeyHashPathPair getKeyHashFileNames(const Key_t& key) const;

    /**
     * @brief Verify the file signature using public key and hash function
     *
     * @param[in]  - Image file path
     * @param[in]  - Signature file path
     * @param[in]  - Public key
     * @param[in]  - Hash function name
     * @return true if signature verification was successful, false if not
     */
    bool verifyFile(const fs::path& file, const fs::path& signature,
                    const fs::path& publicKey, const std::string& hashFunc);

    /**
     * @brief Create RSA object from the public key
     * @param[in]  - publickey
     * @param[out] - RSA Object.
     */
    inline RSA* createPublicRSA(const fs::path& publicKey);

    /**
     * @brief Memory map the  file
     * @param[in]  - file path
     * @param[in]  - file size
     * @param[out] - Custom Mmap address
     */
    CustomMap mapFile(const fs::path& path, size_t size);

    /** @brief Directory where software images are placed*/
    fs::path imageDirPath;

    /** @brief Path of public key and hash function files */
    fs::path signedConfPath;

    /** @brief key type defined in mainfest file */
    Key_t keyType;

    /** @brief Hash type defined in mainfest file */
    Hash_t hashType;
};

} // namespace image
} // namespace software
} // namespace phosphor
