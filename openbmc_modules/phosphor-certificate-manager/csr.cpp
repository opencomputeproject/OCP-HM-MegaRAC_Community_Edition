#include "config.h"

#include "csr.hpp"

#include <openssl/pem.h>

#include <filesystem>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <xyz/openbmc_project/Certs/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

namespace phosphor
{
namespace certs
{

using X509_REQ_Ptr = std::unique_ptr<X509_REQ, decltype(&::X509_REQ_free)>;
using BIO_Ptr = std::unique_ptr<BIO, decltype(&::BIO_free_all)>;
using InternalFailure =
    sdbusplus::xyz::openbmc_project::Common::Error::InternalFailure;
using namespace phosphor::logging;
namespace fs = std::filesystem;

CSR::CSR(sdbusplus::bus::bus& bus, const char* path,
         CertInstallPath&& installPath, const Status& status) :
    CSRIface(bus, path, true),
    bus(bus), objectPath(path), certInstallPath(std::move(installPath)),
    csrStatus(status)
{
    // Emit deferred signal.
    this->emit_object_added();
}

std::string CSR::cSR()
{
    if (csrStatus == Status::FAILURE)
    {
        log<level::ERR>("Failure in Generating CSR");
        elog<InternalFailure>();
    }
    fs::path csrFilePath = certInstallPath;
    csrFilePath = csrFilePath.parent_path() / CSR_FILE_NAME;
    if (!fs::exists(csrFilePath))
    {
        log<level::ERR>("CSR file doesn't exists",
                        entry("FILENAME=%s", csrFilePath.c_str()));
        elog<InternalFailure>();
    }

    FILE* fp = std::fopen(csrFilePath.c_str(), "r");
    X509_REQ_Ptr x509Req(PEM_read_X509_REQ(fp, NULL, NULL, NULL),
                         ::X509_REQ_free);
    if (x509Req == NULL || fp == NULL)
    {
        if (fp != NULL)
        {
            std::fclose(fp);
        }
        log<level::ERR>("ERROR occured while reading CSR file",
                        entry("FILENAME=%s", csrFilePath.c_str()));
        elog<InternalFailure>();
    }
    std::fclose(fp);

    BIO_Ptr bio(BIO_new(BIO_s_mem()), ::BIO_free_all);
    int ret = PEM_write_bio_X509_REQ(bio.get(), x509Req.get());
    if (ret <= 0)
    {
        log<level::ERR>("Error occured while calling PEM_write_bio_X509_REQ");
        elog<InternalFailure>();
    }

    BUF_MEM* mem = NULL;
    BIO_get_mem_ptr(bio.get(), &mem);
    std::string pem(mem->data, mem->length);
    return pem;
}

} // namespace certs
} // namespace phosphor
