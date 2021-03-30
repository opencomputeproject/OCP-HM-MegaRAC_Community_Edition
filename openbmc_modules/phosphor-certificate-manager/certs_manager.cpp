#include "certs_manager.hpp"

#include <openssl/pem.h>
#include <unistd.h>

#include <algorithm>
#include <phosphor-logging/elog-errors.hpp>
#include <xyz/openbmc_project/Certs/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace certs
{
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using InvalidCertificate =
    sdbusplus::xyz::openbmc_project::Certs::Error::InvalidCertificate;
using Reason = xyz::openbmc_project::Certs::InvalidCertificate::REASON;

using X509_REQ_Ptr = std::unique_ptr<X509_REQ, decltype(&::X509_REQ_free)>;
using BIGNUM_Ptr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
using InvalidArgument =
    sdbusplus::xyz::openbmc_project::Common::Error::InvalidArgument;
using Argument = xyz::openbmc_project::Common::InvalidArgument;

constexpr auto SUPPORTED_KEYBITLENGTH = 2048;

Manager::Manager(sdbusplus::bus::bus& bus, sdeventplus::Event& event,
                 const char* path, const CertificateType& type,
                 UnitsToRestart&& unit, CertInstallPath&& installPath) :
    Ifaces(bus, path),
    bus(bus), event(event), objectPath(path), certType(type),
    unitToRestart(std::move(unit)), certInstallPath(std::move(installPath)),
    certParentInstallPath(fs::path(certInstallPath).parent_path())
{
    try
    {
        // Create certificate directory if not existing.
        // Set correct certificate directory permitions.
        fs::path certDirectory;
        try
        {
            if (certType == AUTHORITY)
            {
                certDirectory = certInstallPath;
            }
            else
            {
                certDirectory = certParentInstallPath;
            }

            if (!fs::exists(certDirectory))
            {
                fs::create_directories(certDirectory);
            }

            auto permission = fs::perms::owner_read | fs::perms::owner_write |
                              fs::perms::owner_exec;
            fs::permissions(certDirectory, permission,
                            fs::perm_options::replace);
            storageUpdate();
        }
        catch (fs::filesystem_error& e)
        {
            log<level::ERR>(
                "Failed to create directory", entry("ERR=%s", e.what()),
                entry("DIRECTORY=%s", certParentInstallPath.c_str()));
            report<InternalFailure>();
        }

        // Generating RSA private key file if certificate type is server/client
        if (certType != AUTHORITY)
        {
            createRSAPrivateKeyFile();
        }

        // restore any existing certificates
        createCertificates();

        // watch is not required for authority certificates
        if (certType != AUTHORITY)
        {
            // watch for certificate file create/replace
            certWatchPtr = std::make_unique<
                Watch>(event, certInstallPath, [this]() {
                try
                {
                    // if certificate file existing update it
                    if (!installedCerts.empty())
                    {
                        log<level::INFO>("Inotify callback to update "
                                         "certificate properties");
                        installedCerts[0]->populateProperties();
                    }
                    else
                    {
                        log<level::INFO>(
                            "Inotify callback to create certificate object");
                        createCertificates();
                    }
                }
                catch (const InternalFailure& e)
                {
                    commit<InternalFailure>();
                }
                catch (const InvalidCertificate& e)
                {
                    commit<InvalidCertificate>();
                }
            });
        }
        else
        {
            try
            {
                const std::string singleCertPath = "/etc/ssl/certs/Root-CA.pem";
                if (fs::exists(singleCertPath) && !fs::is_empty(singleCertPath))
                {
                    log<level::NOTICE>(
                        "Legacy certificate detected, will be installed from: ",
                        entry("SINGLE_CERTPATH=%s", singleCertPath.c_str()));
                    install(singleCertPath);
                    if (!fs::remove(singleCertPath))
                    {
                        log<level::ERR>(
                            "Unable to remove old certificate from: ",
                            entry("SINGLE_CERTPATH=%s",
                                  singleCertPath.c_str()));
                        elog<InternalFailure>();
                    }
                }
            }
            catch (const std::exception& ex)
            {
                log<level::ERR>("Error in restoring legacy certificate",
                                entry("ERROR_STR=%s", ex.what()));
            }
        }
    }
    catch (std::exception& ex)
    {
        log<level::ERR>("Error in certificate manager constructor",
                        entry("ERROR_STR=%s", ex.what()));
    }
}

std::string Manager::install(const std::string filePath)
{
    using NotAllowed =
        sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
    using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

    if (certType != phosphor::certs::AUTHORITY && !installedCerts.empty())
    {
        elog<NotAllowed>(Reason("Certificate already exist"));
    }
    else if (certType == phosphor::certs::AUTHORITY &&
             installedCerts.size() >= AUTHORITY_CERTIFICATES_LIMIT)
    {
        elog<NotAllowed>(Reason("Certificates limit reached"));
    }

    std::string certObjectPath;
    if (isCertificateUnique(filePath))
    {
        certObjectPath = objectPath + '/' + std::to_string(certIdCounter);
        installedCerts.emplace_back(std::make_unique<Certificate>(
            bus, certObjectPath, certType, certInstallPath, filePath,
            certWatchPtr, *this));
        reloadOrReset(unitToRestart);
        certIdCounter++;
    }
    else
    {
        elog<NotAllowed>(Reason("Certificate already exist"));
    }

    return certObjectPath;
}

void Manager::deleteAll()
{
    // TODO: #Issue 4 when a certificate is deleted system auto generates
    // certificate file. At present we are not supporting creation of
    // certificate object for the auto-generated certificate file as
    // deletion if only applicable for REST server and Bmcweb does not allow
    // deletion of certificates
    installedCerts.clear();
    storageUpdate();
    reloadOrReset(unitToRestart);
}

void Manager::deleteCertificate(const Certificate* const certificate)
{
    std::vector<std::unique_ptr<Certificate>>::iterator const& certIt =
        std::find_if(installedCerts.begin(), installedCerts.end(),
                     [certificate](std::unique_ptr<Certificate> const& cert) {
                         return (cert.get() == certificate);
                     });
    if (certIt != installedCerts.end())
    {
        installedCerts.erase(certIt);
        storageUpdate();
        reloadOrReset(unitToRestart);
    }
    else
    {
        log<level::ERR>("Certificate does not exist",
                        entry("ID=%s", certificate->getCertId().c_str()));
        elog<InternalFailure>();
    }
}

void Manager::replaceCertificate(Certificate* const certificate,
                                 const std::string& filePath)
{
    if (isCertificateUnique(filePath, certificate))
    {
        certificate->install(filePath);
        storageUpdate();
        reloadOrReset(unitToRestart);
    }
    else
    {
        using NotAllowed =
            sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed;
        using Reason = xyz::openbmc_project::Common::NotAllowed::REASON;

        elog<NotAllowed>(Reason("Certificate already exist"));
    }
}

std::string Manager::generateCSR(
    std::vector<std::string> alternativeNames, std::string challengePassword,
    std::string city, std::string commonName, std::string contactPerson,
    std::string country, std::string email, std::string givenName,
    std::string initials, int64_t keyBitLength, std::string keyCurveId,
    std::string keyPairAlgorithm, std::vector<std::string> keyUsage,
    std::string organization, std::string organizationalUnit, std::string state,
    std::string surname, std::string unstructuredName)
{
    // We support only one CSR.
    csrPtr.reset(nullptr);
    auto pid = fork();
    if (pid == -1)
    {
        log<level::ERR>("Error occurred during forking process");
        report<InternalFailure>();
    }
    else if (pid == 0)
    {
        try
        {
            generateCSRHelper(alternativeNames, challengePassword, city,
                              commonName, contactPerson, country, email,
                              givenName, initials, keyBitLength, keyCurveId,
                              keyPairAlgorithm, keyUsage, organization,
                              organizationalUnit, state, surname,
                              unstructuredName);
            exit(EXIT_SUCCESS);
        }
        catch (const InternalFailure& e)
        {
            // commit the error reported in child process and exit
            // Callback method from SDEvent Loop looks for exit status
            exit(EXIT_FAILURE);
            commit<InternalFailure>();
        }
    }
    else
    {
        using namespace sdeventplus::source;
        Child::Callback callback = [this](Child& eventSource,
                                          const siginfo_t* si) {
            eventSource.set_enabled(Enabled::On);
            if (si->si_status != 0)
            {
                this->createCSRObject(Status::FAILURE);
            }
            else
            {
                this->createCSRObject(Status::SUCCESS);
            }
        };
        try
        {
            sigset_t ss;
            if (sigemptyset(&ss) < 0)
            {
                log<level::ERR>("Unable to initialize signal set");
                elog<InternalFailure>();
            }
            if (sigaddset(&ss, SIGCHLD) < 0)
            {
                log<level::ERR>("Unable to add signal to signal set");
                elog<InternalFailure>();
            }

            // Block SIGCHLD first, so that the event loop can handle it
            if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0)
            {
                log<level::ERR>("Unable to block signal");
                elog<InternalFailure>();
            }
            if (childPtr)
            {
                childPtr.reset();
            }
            childPtr = std::make_unique<Child>(event, pid, WEXITED | WSTOPPED,
                                               std::move(callback));
        }
        catch (const InternalFailure& e)
        {
            commit<InternalFailure>();
        }
    }
    auto csrObjectPath = objectPath + '/' + "csr";
    return csrObjectPath;
}

std::vector<std::unique_ptr<Certificate>>& Manager::getCertificates()
{
    return installedCerts;
}

void Manager::generateCSRHelper(
    std::vector<std::string> alternativeNames, std::string challengePassword,
    std::string city, std::string commonName, std::string contactPerson,
    std::string country, std::string email, std::string givenName,
    std::string initials, int64_t keyBitLength, std::string keyCurveId,
    std::string keyPairAlgorithm, std::vector<std::string> keyUsage,
    std::string organization, std::string organizationalUnit, std::string state,
    std::string surname, std::string unstructuredName)
{
    int ret = 0;

    // set version of x509 req
    int nVersion = 1;
    // TODO: Issue#6 need to make version number configurable
    X509_REQ_Ptr x509Req(X509_REQ_new(), ::X509_REQ_free);
    ret = X509_REQ_set_version(x509Req.get(), nVersion);
    if (ret == 0)
    {
        log<level::ERR>("Error occured during X509_REQ_set_version call");
        elog<InternalFailure>();
    }

    // set subject of x509 req
    X509_NAME* x509Name = X509_REQ_get_subject_name(x509Req.get());

    if (!alternativeNames.empty())
    {
        for (auto& name : alternativeNames)
        {
            addEntry(x509Name, "subjectAltName", name);
        }
    }
    addEntry(x509Name, "challengePassword", challengePassword);
    addEntry(x509Name, "L", city);
    addEntry(x509Name, "CN", commonName);
    addEntry(x509Name, "name", contactPerson);
    addEntry(x509Name, "C", country);
    addEntry(x509Name, "emailAddress", email);
    addEntry(x509Name, "GN", givenName);
    addEntry(x509Name, "initials", initials);
    addEntry(x509Name, "algorithm", keyPairAlgorithm);
    if (!keyUsage.empty())
    {
        for (auto& usage : keyUsage)
        {
            if (isExtendedKeyUsage(usage))
            {
                addEntry(x509Name, "extendedKeyUsage", usage);
            }
            else
            {
                addEntry(x509Name, "keyUsage", usage);
            }
        }
    }
    addEntry(x509Name, "O", organization);
    addEntry(x509Name, "ST", state);
    addEntry(x509Name, "SN", surname);
    addEntry(x509Name, "unstructuredName", unstructuredName);

    EVP_PKEY_Ptr pKey(nullptr, ::EVP_PKEY_free);

    log<level::INFO>("Given Key pair algorithm",
                     entry("KEYPAIRALGORITHM=%s", keyPairAlgorithm.c_str()));

    // Used EC algorithm as default if user did not give algorithm type.
    if (keyPairAlgorithm == "RSA")
        pKey = getRSAKeyPair(keyBitLength);
    else if ((keyPairAlgorithm == "EC") || (keyPairAlgorithm.empty()))
        pKey = generateECKeyPair(keyCurveId);
    else
    {
        log<level::ERR>("Given Key pair algorithm is not supported. Supporting "
                        "RSA and EC only");
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("KEYPAIRALGORITHM"),
            Argument::ARGUMENT_VALUE(keyPairAlgorithm.c_str()));
    }

    ret = X509_REQ_set_pubkey(x509Req.get(), pKey.get());
    if (ret == 0)
    {
        log<level::ERR>("Error occured while setting Public key");
        elog<InternalFailure>();
    }

    // Write private key to file
    writePrivateKey(pKey, PRIV_KEY_FILE_NAME);

    // set sign key of x509 req
    ret = X509_REQ_sign(x509Req.get(), pKey.get(), EVP_sha256());
    if (ret == 0)
    {
        log<level::ERR>("Error occured while signing key of x509");
        elog<InternalFailure>();
    }

    log<level::INFO>("Writing CSR to file");
    fs::path csrFilePath = certParentInstallPath / CSR_FILE_NAME;
    writeCSR(csrFilePath.string(), x509Req);
}

bool Manager::isExtendedKeyUsage(const std::string& usage)
{
    const static std::array<const char*, 6> usageList = {
        "ServerAuthentication", "ClientAuthentication", "OCSPSigning",
        "Timestamping",         "CodeSigning",          "EmailProtection"};
    auto it = std::find_if(
        usageList.begin(), usageList.end(),
        [&usage](const char* s) { return (strcmp(s, usage.c_str()) == 0); });
    return it != usageList.end();
}
EVP_PKEY_Ptr Manager::generateRSAKeyPair(const int64_t keyBitLength)
{
    int ret = 0;
    // generate rsa key
    BIGNUM_Ptr bne(BN_new(), ::BN_free);
    ret = BN_set_word(bne.get(), RSA_F4);
    if (ret == 0)
    {
        log<level::ERR>("Error occured during BN_set_word call");
        elog<InternalFailure>();
    }

    int64_t keyBitLen = keyBitLength;
    // set keybit length to default value if not set
    if (keyBitLen <= 0)
    {
        constexpr auto DEFAULT_KEYBITLENGTH = 2048;
        log<level::INFO>(
            "KeyBitLength is not given.Hence, using default KeyBitLength",
            entry("DEFAULTKEYBITLENGTH=%d", DEFAULT_KEYBITLENGTH));
        keyBitLen = DEFAULT_KEYBITLENGTH;
    }
    RSA* rsa = RSA_new();
    ret = RSA_generate_key_ex(rsa, keyBitLen, bne.get(), NULL);
    if (ret != 1)
    {
        free(rsa);
        log<level::ERR>("Error occured during RSA_generate_key_ex call",
                        entry("KEYBITLENGTH=%PRIu64", keyBitLen));
        elog<InternalFailure>();
    }

    // set public key of x509 req
    EVP_PKEY_Ptr pKey(EVP_PKEY_new(), ::EVP_PKEY_free);
    ret = EVP_PKEY_assign_RSA(pKey.get(), rsa);
    if (ret == 0)
    {
        free(rsa);
        log<level::ERR>("Error occured during assign rsa key into EVP");
        elog<InternalFailure>();
    }

    return pKey;
}

EVP_PKEY_Ptr Manager::generateECKeyPair(const std::string& curveId)
{
    std::string curId(curveId);

    if (curId.empty())
    {
        // secp224r1 is equal to RSA 2048 KeyBitLength. Refer RFC 5349
        constexpr auto DEFAULT_KEYCURVEID = "secp224r1";
        log<level::INFO>(
            "KeyCurveId is not given. Hence using default curve id",
            entry("DEFAULTKEYCURVEID=%s", DEFAULT_KEYCURVEID));
        curId = DEFAULT_KEYCURVEID;
    }

    int ecGrp = OBJ_txt2nid(curId.c_str());

    if (ecGrp == NID_undef)
    {
        log<level::ERR>(
            "Error occured during convert the curve id string format into NID",
            entry("KEYCURVEID=%s", curId.c_str()));
        elog<InternalFailure>();
    }

    EC_KEY* ecKey = EC_KEY_new_by_curve_name(ecGrp);

    if (ecKey == NULL)
    {
        log<level::ERR>(
            "Error occured during create the EC_Key object from NID",
            entry("ECGROUP=%d", ecGrp));
        elog<InternalFailure>();
    }

    // If you want to save a key and later load it with
    // SSL_CTX_use_PrivateKey_file, then you must set the OPENSSL_EC_NAMED_CURVE
    // flag on the key.
    EC_KEY_set_asn1_flag(ecKey, OPENSSL_EC_NAMED_CURVE);

    int ret = EC_KEY_generate_key(ecKey);

    if (ret == 0)
    {
        EC_KEY_free(ecKey);
        log<level::ERR>("Error occured during generate EC key");
        elog<InternalFailure>();
    }

    EVP_PKEY_Ptr pKey(EVP_PKEY_new(), ::EVP_PKEY_free);
    ret = EVP_PKEY_assign_EC_KEY(pKey.get(), ecKey);
    if (ret == 0)
    {
        EC_KEY_free(ecKey);
        log<level::ERR>("Error occured during assign EC Key into EVP");
        elog<InternalFailure>();
    }

    return pKey;
}

void Manager::writePrivateKey(const EVP_PKEY_Ptr& pKey,
                              const std::string& privKeyFileName)
{
    log<level::INFO>("Writing private key to file");
    // write private key to file
    fs::path privKeyPath = certParentInstallPath / privKeyFileName;

    FILE* fp = std::fopen(privKeyPath.c_str(), "w");
    if (fp == NULL)
    {
        log<level::ERR>("Error occured creating private key file");
        elog<InternalFailure>();
    }
    int ret = PEM_write_PrivateKey(fp, pKey.get(), NULL, NULL, 0, 0, NULL);
    std::fclose(fp);
    if (ret == 0)
    {
        log<level::ERR>("Error occured while writing private key to file");
        elog<InternalFailure>();
    }
}

void Manager::addEntry(X509_NAME* x509Name, const char* field,
                       const std::string& bytes)
{
    if (bytes.empty())
    {
        return;
    }
    int ret = X509_NAME_add_entry_by_txt(
        x509Name, field, MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>(bytes.c_str()), -1, -1, 0);
    if (ret != 1)
    {
        log<level::ERR>("Unable to set entry", entry("FIELD=%s", field),
                        entry("VALUE=%s", bytes.c_str()));
        elog<InternalFailure>();
    }
}

void Manager::createCSRObject(const Status& status)
{
    if (csrPtr)
    {
        csrPtr.reset(nullptr);
    }
    auto csrObjectPath = objectPath + '/' + "csr";
    csrPtr = std::make_unique<CSR>(bus, csrObjectPath.c_str(),
                                   certInstallPath.c_str(), status);
}

void Manager::writeCSR(const std::string& filePath, const X509_REQ_Ptr& x509Req)
{
    if (fs::exists(filePath))
    {
        log<level::INFO>("Removing the existing file",
                         entry("FILENAME=%s", filePath.c_str()));
        if (!fs::remove(filePath.c_str()))
        {
            log<level::ERR>("Unable to remove the file",
                            entry("FILENAME=%s", filePath.c_str()));
            elog<InternalFailure>();
        }
    }

    FILE* fp = NULL;

    if ((fp = std::fopen(filePath.c_str(), "w")) == NULL)
    {
        log<level::ERR>("Error opening the file to write the CSR",
                        entry("FILENAME=%s", filePath.c_str()));
        elog<InternalFailure>();
    }

    int rc = PEM_write_X509_REQ(fp, x509Req.get());
    if (!rc)
    {
        log<level::ERR>("PEM write routine failed",
                        entry("FILENAME=%s", filePath.c_str()));
        std::fclose(fp);
        elog<InternalFailure>();
    }
    std::fclose(fp);
}

void Manager::createCertificates()
{
    auto certObjectPath = objectPath + '/';

    if (certType == phosphor::certs::AUTHORITY)
    {
        // Check whether install path is a directory.
        if (!fs::is_directory(certInstallPath))
        {
            log<level::ERR>("Certificate installation path exists and it is "
                            "not a directory");
            elog<InternalFailure>();
            return;
        }

        for (auto& path : fs::directory_iterator(certInstallPath))
        {
            try
            {
                // Assume here any regular file located in certificate directory
                // contains certificates body. Do not want to use soft links
                // would add value.
                if (fs::is_regular_file(path))
                {
                    installedCerts.emplace_back(std::make_unique<Certificate>(
                        bus, certObjectPath + std::to_string(certIdCounter++),
                        certType, certInstallPath, path.path(), certWatchPtr,
                        *this));
                }
            }
            catch (const InternalFailure& e)
            {
                report<InternalFailure>();
            }
            catch (const InvalidCertificate& e)
            {
                report<InvalidCertificate>(
                    Reason("Existing certificate file is corrupted"));
            }
        }
    }
    else if (fs::exists(certInstallPath))
    {
        try
        {
            installedCerts.emplace_back(std::make_unique<Certificate>(
                bus, certObjectPath + '1', certType, certInstallPath,
                certInstallPath, certWatchPtr, *this));
        }
        catch (const InternalFailure& e)
        {
            report<InternalFailure>();
        }
        catch (const InvalidCertificate& e)
        {
            report<InvalidCertificate>(
                Reason("Existing certificate file is corrupted"));
        }
    }
}

void Manager::createRSAPrivateKeyFile()
{
    fs::path rsaPrivateKeyFileName =
        certParentInstallPath / RSA_PRIV_KEY_FILE_NAME;

    try
    {
        if (!fs::exists(rsaPrivateKeyFileName))
        {
            writePrivateKey(generateRSAKeyPair(SUPPORTED_KEYBITLENGTH),
                            RSA_PRIV_KEY_FILE_NAME);
        }
    }
    catch (const InternalFailure& e)
    {
        report<InternalFailure>();
    }
}

EVP_PKEY_Ptr Manager::getRSAKeyPair(const int64_t keyBitLength)
{
    if (keyBitLength != SUPPORTED_KEYBITLENGTH)
    {
        log<level::ERR>(
            "Given Key bit length is not supported",
            entry("GIVENKEYBITLENGTH=%d", keyBitLength),
            entry("SUPPORTEDKEYBITLENGTH=%d", SUPPORTED_KEYBITLENGTH));
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("KEYBITLENGTH"),
            Argument::ARGUMENT_VALUE(std::to_string(keyBitLength).c_str()));
    }
    fs::path rsaPrivateKeyFileName =
        certParentInstallPath / RSA_PRIV_KEY_FILE_NAME;

    FILE* privateKeyFile = std::fopen(rsaPrivateKeyFileName.c_str(), "r");
    if (!privateKeyFile)
    {
        log<level::ERR>("Unable to open RSA private key file to read",
                        entry("RSAKEYFILE=%s", rsaPrivateKeyFileName.c_str()),
                        entry("ERRORREASON=%s", strerror(errno)));
        elog<InternalFailure>();
    }

    EVP_PKEY_Ptr privateKey(
        PEM_read_PrivateKey(privateKeyFile, nullptr, nullptr, nullptr),
        ::EVP_PKEY_free);
    std::fclose(privateKeyFile);

    if (!privateKey)
    {
        log<level::ERR>("Error occured during PEM_read_PrivateKey call");
        elog<InternalFailure>();
    }
    return privateKey;
}

void Manager::storageUpdate()
{
    if (certType == phosphor::certs::AUTHORITY)
    {
        // Remove symbolic links in the certificate directory
        for (auto& certPath : fs::directory_iterator(certInstallPath))
        {
            try
            {
                if (fs::is_symlink(certPath))
                {
                    fs::remove(certPath);
                }
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    "Failed to remove symlink for certificate",
                    entry("ERR=%s", e.what()),
                    entry("SYMLINK=%s", certPath.path().string().c_str()));
                elog<InternalFailure>();
            }
        }
    }

    for (const auto& cert : installedCerts)
    {
        cert->storageUpdate();
    }
}

void Manager::reloadOrReset(const UnitsToRestart& unit)
{
    if (!unit.empty())
    {
        try
        {
            constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
            constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
            constexpr auto SYSTEMD_INTERFACE =
                "org.freedesktop.systemd1.Manager";

            auto method =
                bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                    SYSTEMD_INTERFACE, "ReloadOrRestartUnit");
            method.append(unit, "replace");
            bus.call_noreply(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            log<level::ERR>("Failed to reload or restart service",
                            entry("ERR=%s", e.what()),
                            entry("UNIT=%s", unit.c_str()));
            elog<InternalFailure>();
        }
    }
}

bool Manager::isCertificateUnique(const std::string& filePath,
                                  const Certificate* const certToDrop)
{
    if (std::any_of(
            installedCerts.begin(), installedCerts.end(),
            [&filePath, certToDrop](std::unique_ptr<Certificate> const& cert) {
                return cert.get() != certToDrop && cert->isSame(filePath);
            }))
    {
        return false;
    }
    else
    {
        return true;
    }
}

} // namespace certs
} // namespace phosphor
