#pragma once

#include "watch.hpp"

#include <openssl/x509.h>

#include <filesystem>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Certs/Certificate/server.hpp>
#include <xyz/openbmc_project/Certs/Replace/server.hpp>
#include <xyz/openbmc_project/Object/Delete/server.hpp>

namespace phosphor
{
namespace certs
{
using DeleteIface = sdbusplus::xyz::openbmc_project::Object::server::Delete;
using CertificateIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Certs::server::Certificate>;
using ReplaceIface = sdbusplus::xyz::openbmc_project::Certs::server::Replace;
using CertIfaces = sdbusplus::server::object::object<CertificateIface,
                                                     ReplaceIface, DeleteIface>;

using CertificateType = std::string;
using CertInstallPath = std::string;
using CertUploadPath = std::string;
using InputType = std::string;
using InstallFunc = std::function<void(const std::string&)>;
using AppendPrivKeyFunc = std::function<void(const std::string&)>;
using CertWatchPtr = std::unique_ptr<Watch>;
using namespace phosphor::logging;

// for placeholders
using namespace std::placeholders;
namespace fs = std::filesystem;

class Manager; // Forward declaration for Certificate Manager.

// Supported Types.
static constexpr auto SERVER = "server";
static constexpr auto CLIENT = "client";
static constexpr auto AUTHORITY = "authority";

// RAII support for openSSL functions.
using X509_Ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
using X509_STORE_CTX_Ptr =
    std::unique_ptr<X509_STORE_CTX, decltype(&::X509_STORE_CTX_free)>;

/** @class Certificate
 *  @brief OpenBMC Certificate entry implementation.
 *  @details A concrete implementation for the
 *  xyz.openbmc_project.Certs.Certificate DBus API
 *  xyz.openbmc_project.Certs.Instal DBus API
 */
class Certificate : public CertIfaces
{
  public:
    Certificate() = delete;
    Certificate(const Certificate&) = delete;
    Certificate& operator=(const Certificate&) = delete;
    Certificate(Certificate&&) = delete;
    Certificate& operator=(Certificate&&) = delete;
    virtual ~Certificate();

    /** @brief Constructor for the Certificate Object
     *  @param[in] bus - Bus to attach to.
     *  @param[in] objPath - Object path to attach to
     *  @param[in] type - Type of the certificate
     *  @param[in] installPath - Path of the certificate to install
     *  @param[in] uploadPath - Path of the certificate file to upload
     *  @param[in] watchPtr - watch on self signed certificate pointer
     */
    Certificate(sdbusplus::bus::bus& bus, const std::string& objPath,
                const CertificateType& type, const CertInstallPath& installPath,
                const CertUploadPath& uploadPath, const CertWatchPtr& watchPtr,
                Manager& parent);

    /** @brief Validate and Replace/Install the certificate file
     *  Install/Replace the existing certificate file with another
     *  (possibly CA signed) Certificate file.
     *  @param[in] filePath - Certificate file path.
     */
    void install(const std::string& filePath);

    /** @brief Validate certificate and replace the existing certificate
     *  @param[in] filePath - Certificate file path.
     */
    void replace(const std::string filePath) override;

    /** @brief Populate certificate properties by parsing certificate file
     */
    void populateProperties();

    /**
     * @brief Obtain certificate ID.
     *
     * @return Certificate ID.
     */
    std::string getCertId() const;

    /**
     * @brief Check if provied certificate is the same as the current one.
     *
     * @param[in] certPath - File path for certificate to check.
     *
     * @return Checking result. Return true if certificates are the same,
     *         false if not.
     */
    bool isSame(const std::string& certPath);

    /**
     * @brief Update certificate storage.
     */
    void storageUpdate();

    /**
     * @brief Delete the certificate
     */
    void delete_() override;

  private:
    /**
     * @brief Return error if ceritificate expiry date is gt 2038
     *
     * Parse the certificate and return error if certificate expiry date
     * is gt 2038.
     *
     * @param[in] cert  Reference to certificate object uploaded
     *
     * @return void
     */
    void validateCertificateExpiryDate(const X509_Ptr& cert);

    /**
     * @brief Populate certificate properties by parsing given certificate file
     *
     * @param[in] certPath   Path to certificate that should be parsed
     *
     * @return void
     */
    void populateProperties(const std::string& certPath);

    /** @brief Load Certificate file into the X509 structure.
     *  @param[in] filePath - Certificate and key full file path.
     *  @return pointer to the X509 structure.
     */
    X509_Ptr loadCert(const std::string& filePath);

    /** @brief Check and append private key to the certificate file
     *         If private key is not present in the certificate file append the
     *         certificate file with private key existing in the system.
     *  @param[in] filePath - Certificate and key full file path.
     *  @return void.
     */
    void checkAndAppendPrivateKey(const std::string& filePath);

    /** @brief Public/Private key compare function.
     *         Comparing private key against certificate public key
     *         from input .pem file.
     *  @param[in] filePath - Certificate and key full file path.
     *  @return Return true if Key compare is successful,
     *          false if not
     */
    bool compareKeys(const std::string& filePath);

    /**
     * @brief Generate certificate ID based on provided certificate file.
     *
     * @param[in] certPath - Certificate file path.
     *
     * @return Certificate ID as formatted string.
     */
    std::string generateCertId(const std::string& certPath);

    /**
     * @brief Generate file name which is unique in the provided directory.
     *
     * @param[in] directoryPath - Directory path.
     *
     * @return File path.
     */
    std::string generateUniqueFilePath(const std::string& directoryPath);

    /**
     * @brief Generate authority certificate file path corresponding with
     * OpenSSL requirements.
     *
     * Prepare authority certificate file path for provied certificate.
     * OpenSSL puts some restrictions on the certificate file name pattern.
     * Certificate full file name needs to consists of basic file name which
     * is certificate subject name hash and file name extension which is an
     * integer. More over, certificates files names extensions must be
     * consecutive integer numbers in case many certificates with the same
     * subject name.
     * https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/ssl__context/add_verify_path.html
     * https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_load_verify_locations.html
     *
     * @param[in] certSrcFilePath - Certificate source file path.
     * @param[in] certDstDirPath - Certificate destination directory path.
     *
     * @return Authority certificate file path.
     */
    std::string generateAuthCertFileX509Path(const std::string& certSrcFilePath,
                                             const std::string& certDstDirPath);

    /**
     * @brief Generate authority certificate file path based on provided
     * certificate source file path.
     *
     * @param[in] certSrcFilePath - Certificate source file path.
     *
     * @return Authority certificate file path.
     */
    std::string generateAuthCertFilePath(const std::string& certSrcFilePath);

    /**
     * @brief Generate certificate file path based on provided certificate
     * source file path.
     *
     * @param[in] certSrcFilePath - Certificate source file path.
     *
     * @return Certificate file path.
     */
    std::string generateCertFilePath(const std::string& certSrcFilePath);

    /** @brief Type specific function pointer map */
    std::unordered_map<InputType, InstallFunc> typeFuncMap;

    /** @brief sdbusplus handler */
    sdbusplus::bus::bus& bus;

    /** @brief object path */
    std::string objectPath;

    /** @brief Type of the certificate */
    CertificateType certType;

    /** @brief Stores certificate ID */
    std::string certId;

    /** @brief Stores certificate file path */
    std::string certFilePath;

    /** @brief Certificate file installation path */
    CertInstallPath certInstallPath;

    /** @brief Type specific function pointer map for appending private key */
    std::unordered_map<InputType, AppendPrivKeyFunc> appendKeyMap;

    /** @brief Certificate file create/update watch */
    const CertWatchPtr& certWatchPtr;

    /** @brief Reference to Certificate Manager */
    Manager& manager;
};

} // namespace certs
} // namespace phosphor
