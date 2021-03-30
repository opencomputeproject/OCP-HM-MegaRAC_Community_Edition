#include <ipmid/api.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process/child.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <commandutils.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdbusplus/timer.hpp>
#include <types.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#ifdef INTEL_PFR_ENABLED
#include <spiDev.hpp>
#endif

static constexpr bool DEBUG = true;
static void registerFirmwareFunctions() __attribute__((constructor));

namespace ipmi
{
namespace firmware
{
constexpr Cmd cmdGetFwVersionInfo = 0x20;
constexpr Cmd cmdGetFwSecurityVersionInfo = 0x21;
constexpr Cmd cmdGetFwUpdateChannelInfo = 0x22;
constexpr Cmd cmdGetBmcExecutionContext = 0x23;
constexpr Cmd cmdFwGetRootCertData = 0x25;
constexpr Cmd cmdGetFwUpdateRandomNumber = 0x26;
constexpr Cmd cmdSetFirmwareUpdateMode = 0x27;
constexpr Cmd cmdExitFirmwareUpdateMode = 0x28;
constexpr Cmd cmdGetSetFwUpdateControl = 0x29;
constexpr Cmd cmdGetFirmwareUpdateStatus = 0x2A;
constexpr Cmd cmdSetFirmwareUpdateOptions = 0x2B;
constexpr Cmd cmdFwImageWriteData = 0x2c;
} // namespace firmware
} // namespace ipmi

namespace ipmi
{
// Custom completion codes
constexpr Cc ccUsbAttachOrDetachFailed = 0x80;
constexpr Cc ccNotSupportedInPresentState = 0xD5;

static inline auto responseUsbAttachOrDetachFailed()
{
    return response(ccUsbAttachOrDetachFailed);
}
static inline auto responseNotSupportedInPresentState()
{
    return response(ccNotSupportedInPresentState);
}
} // namespace ipmi

static constexpr const char* bmcStateIntf = "xyz.openbmc_project.State.BMC";
static constexpr const char* bmcStatePath = "/xyz/openbmc_project/state/bmc0";
static constexpr const char* bmcStateReady =
    "xyz.openbmc_project.State.BMC.BMCState.Ready";
static constexpr const char* bmcStateUpdateInProgress =
    "xyz.openbmc_project.State.BMC.BMCState.UpdateInProgress";

static constexpr char firmwareBufferFile[] = "/tmp/fw-download.bin";
static std::chrono::steady_clock::time_point fwRandomNumGenTs;
static constexpr auto fwRandomNumExpirySeconds = std::chrono::seconds(30);
static constexpr size_t fwRandomNumLength = 8;
static std::array<uint8_t, fwRandomNumLength> fwRandomNum;
constexpr char usbCtrlPath[] = "/usr/bin/usb-ctrl";
constexpr char fwUpdateMountPoint[] = "/tmp/usb-fwupd.mnt";
constexpr char fwUpdateUsbVolImage[] = "/tmp/usb-fwupd.img";
constexpr char fwUpdateUSBDevName[] = "fw-usb-mass-storage-dev";
constexpr size_t fwPathMaxLength = 255;

#ifdef INTEL_PFR_ENABLED
uint32_t imgLength = 0;
uint32_t imgType = 0;
bool block0Mapped = false;
static constexpr uint32_t perBlock0MagicNum = 0xB6EAFD19;

static constexpr const char* bmcActivePfmMTDDev = "/dev/mtd/pfm";
static constexpr const char* bmcRecoveryImgMTDDev = "/dev/mtd/rc-image";
static constexpr size_t pfmBaseOffsetInImage = 0x400;
static constexpr size_t rootkeyOffsetInPfm = 0xA0;
static constexpr size_t cskKeyOffsetInPfm = 0x124;
static constexpr size_t cskSignatureOffsetInPfm = 0x19c;
static constexpr size_t certKeyLen = 96;
static constexpr size_t cskSignatureLen = 96;

static constexpr const char* versionIntf =
    "xyz.openbmc_project.Software.Version";

enum class FwGetRootCertDataTag : uint8_t
{
    activeRootKey = 1,
    recoveryRootKey,
    activeCSK,
    recoveryCSK,
};

enum class FWDeviceIDTag : uint8_t
{
    bmcActiveImage = 1,
    bmcRecoveryImage,
};

const static boost::container::flat_map<FWDeviceIDTag, const char*>
    fwVersionIdMap{{FWDeviceIDTag::bmcActiveImage,
                    "/xyz/openbmc_project/software/bmc_active"},
                   {FWDeviceIDTag::bmcRecoveryImage,
                    "/xyz/openbmc_project/software/bmc_recovery"}};
#endif // INTEL_PFR_ENABLED

enum class ChannelIdTag : uint8_t
{
    reserved = 0,
    kcs = 1,
    ipmb = 2,
    rmcpPlus = 3
};

enum class BmcExecutionContext : uint8_t
{
    reserved = 0,
    linuxOs = 0x10,
    bootLoader = 0x11,
};

enum class FwUpdateCtrlReq : uint8_t
{
    getCurrentControlStatus = 0x00,
    imageTransferStart = 0x01,
    imageTransferComplete = 0x02,
    imageTransferAbort = 0x03,
    setFirmwareFilename = 0x04,
    attachUsbDevice = 0x05,
    detachUsbDevice = 0x06
};

constexpr std::size_t operator""_MB(unsigned long long v)
{
    return 1024u * 1024u * v;
}
static constexpr size_t maxFirmwareImageSize = 32_MB;

static bool localDownloadInProgress(void)
{
    struct stat sb;
    if (stat(firmwareBufferFile, &sb) < 0)
    {
        return false;
    }
    return true;
}

class TransferHashCheck
{
  public:
    enum class HashCheck : uint8_t
    {
        notRequested = 0,
        requested,
        sha2Success,
        sha2Failed = 0xe2,
    };

  protected:
    EVP_MD_CTX* ctx;
    std::vector<uint8_t> expectedHash;
    enum HashCheck check;
    bool started;

  public:
    TransferHashCheck() : check(HashCheck::notRequested), started(false)
    {}
    ~TransferHashCheck()
    {
        if (ctx)
        {
            EVP_MD_CTX_destroy(ctx);
            ctx = NULL;
        }
    }
    void init(const std::vector<uint8_t>& expected)
    {
        expectedHash = expected;
        check = HashCheck::requested;
        ctx = EVP_MD_CTX_create();
        EVP_DigestInit(ctx, EVP_sha256());
    }
    void hash(const std::vector<uint8_t>& data)
    {
        if (!started)
        {
            started = true;
        }
        EVP_DigestUpdate(ctx, data.data(), data.size());
    }
    void clear()
    {
        // if not started, nothing to clear
        if (started)
        {
            if (ctx)
            {
                EVP_MD_CTX_destroy(ctx);
            }
            if (check != HashCheck::notRequested)
            {
                check = HashCheck::requested;
            }
            ctx = EVP_MD_CTX_create();
            EVP_DigestInit(ctx, EVP_sha256());
        }
    }
    enum HashCheck verify()
    {
        if (check == HashCheck::requested)
        {
            unsigned int len = 0;
            std::vector<uint8_t> digest(EVP_MD_size(EVP_sha256()));
            EVP_DigestFinal(ctx, digest.data(), &len);
            if (digest == expectedHash)
            {
                phosphor::logging::log<phosphor::logging::level::INFO>(
                    "Transfer sha2 verify passed.");
                check = HashCheck::sha2Success;
            }
            else
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Transfer sha2 verify failed.");
                check = HashCheck::sha2Failed;
            }
        }
        return check;
    }
    uint8_t status() const
    {
        return static_cast<uint8_t>(check);
    }
};

class MappedFile
{
  public:
    MappedFile(const std::string& fname) : addr(nullptr), fsize(0)
    {
        std::error_code ec;
        size_t sz = std::filesystem::file_size(fname, ec);
        int fd = open(fname.c_str(), O_RDONLY);
        if (!ec || fd < 0)
        {
            return;
        }
        void* tmp = mmap(NULL, sz, PROT_READ, MAP_SHARED, fd, 0);
        close(fd);
        if (tmp == MAP_FAILED)
        {
            return;
        }
        addr = tmp;
        fsize = sz;
    }

    ~MappedFile()
    {
        if (addr)
        {
            munmap(addr, fsize);
        }
    }
    const uint8_t* data() const
    {
        return static_cast<const uint8_t*>(addr);
    }
    size_t size() const
    {
        return fsize;
    }

  private:
    size_t fsize;
    void* addr;
};

class FwUpdateStatusCache
{
  public:
    enum
    {
        fwStateInit = 0,
        fwStateIdle,
        fwStateDownload,
        fwStateVerify,
        fwStateProgram,
        fwStateUpdateSuccess,
        fwStateError = 0x0f,
        fwStateAcCycleRequired = 0x83,
    };
    uint8_t getState()
    {
        if ((fwUpdateState == fwStateIdle || fwUpdateState == fwStateInit) &&
            localDownloadInProgress())
        {
            fwUpdateState = fwStateDownload;
            progressPercent = 0;
        }
        return fwUpdateState;
    }
    void resetStatusCache()
    {
        unlink(firmwareBufferFile);
    }
    void setState(const uint8_t state)
    {
        switch (state)
        {
            case fwStateInit:
            case fwStateIdle:
            case fwStateError:
                resetStatusCache();
                break;
            case fwStateDownload:
            case fwStateVerify:
            case fwStateProgram:
            case fwStateUpdateSuccess:
                break;
            default:
                // Error
                break;
        }
        fwUpdateState = state;
    }
    uint8_t percent()
    {
        return progressPercent;
    }
    void updateActivationPercent(const std::string& objPath)
    {
        std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
        fwUpdateState = fwStateProgram;
        progressPercent = 0;
        match = std::make_shared<sdbusplus::bus::match::match>(
            *busp,
            sdbusplus::bus::match::rules::propertiesChanged(
                objPath, "xyz.openbmc_project.Software.ActivationProgress"),
            [&](sdbusplus::message::message& msg) {
                std::map<std::string, ipmi::DbusVariant> props;
                std::vector<std::string> inVal;
                std::string iface;
                try
                {
                    msg.read(iface, props, inVal);
                }
                catch (const std::exception& e)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Exception caught in get ActivationProgress");
                    return;
                }

                auto it = props.find("Progress");
                if (it != props.end())
                {
                    progressPercent = std::get<uint8_t>(it->second);
                    if (progressPercent == 100)
                    {
                        fwUpdateState = fwStateUpdateSuccess;
                    }
                }
            });
    }
    uint8_t activationTimerTimeout()
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "activationTimerTimeout: Increase percentage...",
            phosphor::logging::entry("PERCENT:%d", progressPercent));
        progressPercent = progressPercent + 5;
        if (progressPercent > 95)
        {
            /*changing the state to ready to update firmware utility */
            fwUpdateState = fwStateUpdateSuccess;
        }
        return progressPercent;
    }
    /* API for changing state to ERROR  */
    void firmwareUpdateAbortState()
    {
        unlink(firmwareBufferFile);
        // changing the state to error
        fwUpdateState = fwStateError;
    }
    void setDeferRestart(bool deferRestart)
    {
        deferRestartState = deferRestart;
    }
    void setInhibitDowngrade(bool inhibitDowngrade)
    {
        inhibitDowngradeState = inhibitDowngrade;
    }
    bool getDeferRestart()
    {
        return deferRestartState;
    }
    bool getInhibitDowngrade()
    {
        return inhibitDowngradeState;
    }

  protected:
    std::shared_ptr<sdbusplus::asio::connection> busp;
    std::shared_ptr<sdbusplus::bus::match::match> match;
    uint8_t fwUpdateState = 0;
    uint8_t progressPercent = 0;
    bool deferRestartState = false;
    bool inhibitDowngradeState = false;
};

static FwUpdateStatusCache fwUpdateStatus;
std::shared_ptr<TransferHashCheck> xferHashCheck;

static void activateImage(const std::string& objPath)
{
    // If flag is false  means to reboot
    if (fwUpdateStatus.getDeferRestart() == false)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "activating Image: ",
            phosphor::logging::entry("OBJPATH =%s", objPath.c_str()));
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        bus->async_method_call(
            [](const boost::system::error_code ec) {
                if (ec)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "async_method_call error: activateImage failed");
                    return;
                }
            },
            "xyz.openbmc_project.Software.BMC.Updater", objPath,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Software.Activation", "RequestedActivation",
            std::variant<std::string>("xyz.openbmc_project.Software.Activation."
                                      "RequestedActivations.Active"));
    }
    else
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Firmware image activation is deferred.");
    }
    fwUpdateStatus.setState(
        static_cast<uint8_t>(FwUpdateStatusCache::fwStateUpdateSuccess));
}

static bool getFirmwareUpdateMode()
{
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        auto service = ipmi::getService(*busp, bmcStateIntf, bmcStatePath);
        ipmi::Value state = ipmi::getDbusProperty(
            *busp, service, bmcStatePath, bmcStateIntf, "CurrentBMCState");
        std::string bmcState = std::get<std::string>(state);
        return (bmcState == bmcStateUpdateInProgress);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Exception caught while getting BMC state.",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        throw;
    }
}

static void setFirmwareUpdateMode(const bool mode)
{

    std::string bmcState(bmcStateReady);
    if (mode)
    {
        bmcState = bmcStateUpdateInProgress;
    }

    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    try
    {
        auto service = ipmi::getService(*busp, bmcStateIntf, bmcStatePath);
        ipmi::setDbusProperty(*busp, service, bmcStatePath, bmcStateIntf,
                              "CurrentBMCState", bmcState);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Exception caught while setting BMC state.",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        throw;
    }
}

/** @brief check if channel IPMB
 *
 *  This function checks if the command is from IPMB
 *
 * @param[in] ctx - context of current session.
 *  @returns true if the medium is IPMB else return true.
 **/
ipmi::Cc checkIPMBChannel(const ipmi::Context::ptr& ctx, bool& isIPMBChannel)
{
    ipmi::ChannelInfo chInfo;

    if (ipmi::getChannelInfo(ctx->channel, chInfo) != ipmi::ccSuccess)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get Channel Info",
            phosphor::logging::entry("CHANNEL=%d", ctx->channel));
        return ipmi::ccUnspecifiedError;
    }

    if (static_cast<ipmi::EChannelMediumType>(chInfo.mediumType) ==
        ipmi::EChannelMediumType::ipmb)
    {
        isIPMBChannel = true;
    }
    return ipmi::ccSuccess;
}

static void postTransferCompleteHandler(
    std::unique_ptr<sdbusplus::bus::match::match>& fwUpdateMatchSignal)
{
    // Setup timer for watching signal
    static phosphor::Timer timer(
        [&fwUpdateMatchSignal]() { fwUpdateMatchSignal = nullptr; });

    static phosphor::Timer activationStatusTimer([]() {
        if (fwUpdateStatus.activationTimerTimeout() > 95)
        {
            activationStatusTimer.stop();
            fwUpdateStatus.setState(
                static_cast<uint8_t>(FwUpdateStatusCache::fwStateVerify));
        }
    });

    timer.start(std::chrono::microseconds(5000000), false);

    // callback function for capturing signal
    auto callback = [&](sdbusplus::message::message& m) {
        bool flag = false;

        std::vector<std::pair<
            std::string,
            std::vector<std::pair<std::string, std::variant<std::string>>>>>
            intfPropsPair;
        sdbusplus::message::object_path objPath;

        try
        {
            m.read(objPath, intfPropsPair); // Read in the object path
                                            // that was just created
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Exception caught in reading created object path.");
            return;
        }
        // constructing response message
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "New Interface Added.",
            phosphor::logging::entry("OBJPATH=%s", objPath.str.c_str()));
        for (auto& interface : intfPropsPair)
        {
            if (interface.first == "xyz.openbmc_project.Software.Activation")
            {
                // There are chances of getting two signals for
                // InterfacesAdded. So cross check and discrad second instance.
                if (fwUpdateMatchSignal == nullptr)
                {
                    return;
                }
                // Found our interface, disable callbacks
                fwUpdateMatchSignal = nullptr;

                phosphor::logging::log<phosphor::logging::level::INFO>(
                    "Start activationStatusTimer for status.");
                try
                {
                    timer.stop();
                    activationStatusTimer.start(
                        std::chrono::microseconds(3000000), true);
                }
                catch (const std::exception& e)
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Exception caught in start activationStatusTimer.",
                        phosphor::logging::entry("ERROR=%s", e.what()));
                }

                fwUpdateStatus.updateActivationPercent(objPath.str);
                activateImage(objPath.str);
            }
        }
    };

    // Adding matcher
    fwUpdateMatchSignal = std::make_unique<sdbusplus::bus::match::match>(
        *getSdBus(),
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);
}
static bool startFirmwareUpdate(const std::string& uri)
{
    // fwupdate URIs start with file:// or usb:// or tftp:// etc. By the time
    // the code gets to this point, the file should be transferred start the
    // request (creating a new file in /tmp/images causes the update manager to
    // check if it is ready for activation)
    static std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatchSignal;
    postTransferCompleteHandler(fwUpdateMatchSignal);
    std::filesystem::rename(
        uri, "/tmp/images/" +
                 boost::uuids::to_string(boost::uuids::random_generator()()));
    return true;
}

static int transferImageFromFile(const std::string& uri, bool move = true)
{
    std::error_code ec;
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Transfer Image From File.",
        phosphor::logging::entry("URI=%s", uri.c_str()));
    if (move)
    {
        std::filesystem::rename(uri, firmwareBufferFile, ec);
    }
    else
    {
        std::filesystem::copy(uri, firmwareBufferFile,
                              std::filesystem::copy_options::overwrite_existing,
                              ec);
    }
    if (xferHashCheck)
    {
        MappedFile mappedfw(uri);
        xferHashCheck->hash(
            {mappedfw.data(), mappedfw.data() + mappedfw.size()});
    }
    if (ec.value())
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Image copy failed.");
    }
    return ec.value();
}

template <typename... ArgTypes>
static int executeCmd(const char* path, ArgTypes&&... tArgs)
{
    boost::process::child execProg(path, const_cast<char*>(tArgs)...);
    execProg.wait();
    return execProg.exit_code();
}

static int transferImageFromUsb(const std::string& uri)
{
    int ret, sysret;
    char fwpath[fwPathMaxLength];
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Transfer Image From USB.",
        phosphor::logging::entry("URI=%s", uri.c_str()));
    ret = executeCmd(usbCtrlPath, "mount", fwUpdateUsbVolImage,
                     fwUpdateMountPoint);
    if (ret)
    {
        return ret;
    }

    std::string usb_path = std::string(fwUpdateMountPoint) + "/" + uri;
    ret = transferImageFromFile(usb_path, false);

    executeCmd(usbCtrlPath, "cleanup", fwUpdateUsbVolImage, fwUpdateMountPoint);
    return ret;
}

static bool transferFirmwareFromUri(const std::string& uri)
{
    static constexpr char fwUriFile[] = "file://";
    static constexpr char fwUriUsb[] = "usb://";
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Transfer Image From URI.",
        phosphor::logging::entry("URI=%s", uri.c_str()));
    if (boost::algorithm::starts_with(uri, fwUriFile))
    {
        std::string fname = uri.substr(sizeof(fwUriFile) - 1);
        if (fname != firmwareBufferFile)
        {
            return 0 == transferImageFromFile(fname);
        }
        return true;
    }
    if (boost::algorithm::starts_with(uri, fwUriUsb))
    {
        std::string fname = uri.substr(sizeof(fwUriUsb) - 1);
        return 0 == transferImageFromUsb(fname);
    }
    return false;
}

/* Get USB-mass-storage device status: inserted => true, ejected => false */
static bool getUsbStatus()
{
    std::filesystem::path usbDevPath =
        std::filesystem::path("/sys/kernel/config/usb_gadget") /
        fwUpdateUSBDevName;
    return (std::filesystem::exists(usbDevPath) ? true : false);
}

/* Insert the USB-mass-storage device status: success => 0, failure => non-0 */
static int attachUsbDevice()
{
    if (getUsbStatus())
    {
        return 1;
    }
    int ret = executeCmd(usbCtrlPath, "setup", fwUpdateUsbVolImage,
                         std::to_string(maxFirmwareImageSize / 1_MB).c_str());
    if (!ret)
    {
        ret = executeCmd(usbCtrlPath, "insert", fwUpdateUSBDevName,
                         fwUpdateUsbVolImage);
    }
    return ret;
}

/* Eject the USB-mass-storage device status: success => 0, failure => non-0 */
static int detachUsbDevice()
{
    if (!getUsbStatus())
    {
        return 1;
    }
    return executeCmd(usbCtrlPath, "eject", fwUpdateUSBDevName);
}
static uint8_t getActiveBootImage(ipmi::Context::ptr ctx)
{
    constexpr uint8_t undefinedImage = 0x00;
    constexpr uint8_t primaryImage = 0x01;
    constexpr uint8_t secondaryImage = 0x02;
    constexpr const char* secondaryFitImageStartAddr = "22480000";

    uint8_t bootImage = primaryImage;
    boost::system::error_code ec;
    std::string value = ctx->bus->yield_method_call<std::string>(
        ctx->yield, ec, "xyz.openbmc_project.U_Boot.Environment.Manager",
        "/xyz/openbmc_project/u_boot/environment/mgr",
        "xyz.openbmc_project.U_Boot.Environment.Manager", "Read", "bootcmd");
    if (ec)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to read the bootcmd value");
        /* don't fail, just give back undefined until it is ready */
        bootImage = undefinedImage;
    }

    /* cheking for secondary FitImage Address 22480000  */
    else if (value.find(secondaryFitImageStartAddr) != std::string::npos)
    {
        bootImage = secondaryImage;
    }
    else
    {
        bootImage = primaryImage;
    }

    return bootImage;
}

#ifdef INTEL_PFR_ENABLED
using fwVersionInfoType = std::tuple<uint8_t,   // ID Tag
                                     uint8_t,   // Major Version Number
                                     uint8_t,   // Minor Version Number
                                     uint32_t,  // Build Number
                                     uint32_t,  // Build Timestamp
                                     uint32_t>; // Update Timestamp
ipmi::RspType<uint8_t, std::vector<fwVersionInfoType>> ipmiGetFwVersionInfo()
{
    // Byte 1 - Count (N) Number of devices data is being returned for.
    // Bytes  2:16 - Device firmare information(fwVersionInfoType)
    // Bytes - 17:(15xN) - Repeat of 2 through 16

    std::vector<fwVersionInfoType> fwVerInfoList;
    std::shared_ptr<sdbusplus::asio::connection> busp = getSdBus();
    for (const auto& fwDev : fwVersionIdMap)
    {
        std::string verStr;
        try
        {
            auto service = ipmi::getService(*busp, versionIntf, fwDev.second);

            ipmi::Value result = ipmi::getDbusProperty(
                *busp, service, fwDev.second, versionIntf, "Version");
            verStr = std::get<std::string>(result);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Failed to fetch Version property",
                phosphor::logging::entry("ERROR=%s", e.what()),
                phosphor::logging::entry("PATH=%s", fwDev.second),
                phosphor::logging::entry("INTERFACE=%s", versionIntf));
            continue;
        }

        if (verStr.empty())
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Version is empty.",
                phosphor::logging::entry("PATH=%s", fwDev.second),
                phosphor::logging::entry("INTERFACE=%s", versionIntf));
            continue;
        }

        // BMC Version format: <major>.<minor>-<build bum>-<build hash>
        std::vector<std::string> splitVer;
        boost::split(splitVer, verStr, boost::is_any_of(".-"));
        if (splitVer.size() < 3)
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Invalid Version format.",
                phosphor::logging::entry("Version=%s", verStr.c_str()),
                phosphor::logging::entry("PATH=%s", fwDev.second));
            continue;
        }

        uint8_t majorNum = 0;
        uint8_t minorNum = 0;
        uint32_t buildNum = 0;
        try
        {
            majorNum = std::stoul(splitVer[0], nullptr, 16);
            minorNum = std::stoul(splitVer[1], nullptr, 16);
            buildNum = std::stoul(splitVer[2], nullptr, 16);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Failed to convert stoul.",
                phosphor::logging::entry("ERROR=%s", e.what()));
            continue;
        }

        // Build Timestamp - Not supported.
        // Update Timestamp - TODO: Need to check with CPLD team.
        fwVerInfoList.emplace_back(
            fwVersionInfoType(static_cast<uint8_t>(fwDev.first), majorNum,
                              minorNum, buildNum, 0, 0));
    }

    return ipmi::responseSuccess(fwVerInfoList.size(), fwVerInfoList);
}
using fwSecurityVersionInfoType = std::tuple<uint8_t,  // ID Tag
                                             uint8_t,  // BKC Version
                                             uint8_t>; // SVN Version
ipmi::RspType<uint8_t, std::vector<fwSecurityVersionInfoType>>
    ipmiGetFwSecurityVersionInfo()
{
    // TODO: Need to add support.
    return ipmi::responseInvalidCommand();
}

ipmi::RspType<std::array<uint8_t, certKeyLen>,
              std::optional<std::array<uint8_t, cskSignatureLen>>>
    ipmiGetFwRootCertData(const ipmi::Context::ptr& ctx, uint8_t certId)
{
    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Command not supported. Failed to get root certificate data.");
        return ipmi::responseCommandNotAvailable();
    }

    size_t certKeyOffset = 0;
    size_t cskSigOffset = 0;
    std::string mtdDev;

    switch (static_cast<FwGetRootCertDataTag>(certId))
    {
        case FwGetRootCertDataTag::activeRootKey:
        {
            mtdDev = bmcActivePfmMTDDev;
            certKeyOffset = rootkeyOffsetInPfm;
            break;
        }
        case FwGetRootCertDataTag::recoveryRootKey:
        {
            mtdDev = bmcRecoveryImgMTDDev;
            certKeyOffset = pfmBaseOffsetInImage + rootkeyOffsetInPfm;
            break;
        }
        case FwGetRootCertDataTag::activeCSK:
        {
            mtdDev = bmcActivePfmMTDDev;
            certKeyOffset = cskKeyOffsetInPfm;
            cskSigOffset = cskSignatureOffsetInPfm;
            break;
        }
        case FwGetRootCertDataTag::recoveryCSK:
        {
            mtdDev = bmcRecoveryImgMTDDev;
            certKeyOffset = pfmBaseOffsetInImage + cskKeyOffsetInPfm;
            cskSigOffset = pfmBaseOffsetInImage + cskSignatureOffsetInPfm;
            break;
        }
        default:
        {
            return ipmi::responseInvalidFieldRequest();
        }
    }

    std::array<uint8_t, certKeyLen> certKey = {0};

    try
    {
        SPIDev spiDev(mtdDev);
        spiDev.spiReadData(certKeyOffset, certKeyLen, certKey.data());

        if (cskSigOffset)
        {
            std::array<uint8_t, cskSignatureLen> cskSignature = {0};
            spiDev.spiReadData(cskSigOffset, cskSignatureLen,
                               cskSignature.data());
            return ipmi::responseSuccess(certKey, cskSignature);
        }
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Exception caught in ipmiGetFwRootCertData",
            phosphor::logging::entry("MSG=%s", e.what()));
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess(certKey, std::nullopt);
}
#endif // INTEL_PFR_ENABLED

static constexpr uint8_t channelListSize = 3;
/** @brief implements Maximum Firmware Transfer size command
 *  @parameter
 *   -  none
 *  @returns IPMI completion code plus response data
 *   - count - channel count
 *   - channelList - channel list information
 */
ipmi::RspType<uint8_t, // channel count
              std::array<std::tuple<uint8_t, uint32_t>,
                         channelListSize> // Channel List
              >
    ipmiFirmwareMaxTransferSize()
{
    constexpr size_t kcsMaxBufSize = 128;
    constexpr size_t rmcpPlusMaxBufSize = 50 * 1024;
    // Byte 1 - Count (N) Number of devices data is being returned for.
    // Byte 2 - ID Tag 00 – reserved 01 – kcs 02 – rmcp+, 03 - ipmb
    // Byte 3-6 - transfer size (little endian)
    // Bytes - 7:(5xN) - Repeat of 2 through 6
    constexpr std::array<std::tuple<uint8_t, uint32_t>, channelListSize>
        channelList = {
            {{static_cast<uint8_t>(ChannelIdTag::kcs), kcsMaxBufSize},
             {static_cast<uint8_t>(ChannelIdTag::rmcpPlus),
              rmcpPlusMaxBufSize}}};

    return ipmi::responseSuccess(channelListSize, channelList);
}

ipmi::RspType<uint8_t, uint8_t>
    ipmiGetBmcExecutionContext(ipmi::Context::ptr ctx)
{
    // Byte 1 - Current execution context
    //          0x10 - Linux OS, 0x11 - Bootloader, Forced-firmware updat mode
    // Byte 2 - Partition pointer
    //          0x01 - primary, 0x02 - secondary
    uint8_t partitionPtr = getActiveBootImage(ctx);

    return ipmi::responseSuccess(
        static_cast<uint8_t>(BmcExecutionContext::linuxOs), partitionPtr);
}
/** @brief Get Firmware Update Random Number
 *
 *  This function generate the random number used for
 *  setting the firmware update mode as authentication key.
 *
 * @param[in] ctx - context of current session
 *  @returns IPMI completion code along with
 *   - random number
 **/
ipmi::RspType<std::array<uint8_t, fwRandomNumLength>>
    ipmiGetFwUpdateRandomNumber(const ipmi::Context::ptr& ctx)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Generate FW update random number");
    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Channel not supported. Failed to fetch FW update random number");
        return ipmi::responseCommandNotAvailable();
    }
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<> dist{0, 255};

    fwRandomNumGenTs = std::chrono::steady_clock::now();

    for (int i = 0; i < fwRandomNumLength; i++)
    {
        fwRandomNum[i] = dist(gen);
    }

    return ipmi::responseSuccess(fwRandomNum);
}

/** @brief Set Firmware Update Mode
 *
 *  This function sets BMC into firmware update mode
 *  after validating Random number obtained from the Get
 *  Firmware Update Random Number command
 *
 * @param[in] ctx - context of current session
 * @parameter randNum - Random number(token)
 * @returns IPMI completion code
 **/
ipmi::RspType<>
    ipmiSetFirmwareUpdateMode(const ipmi::Context::ptr& ctx,
                              std::array<uint8_t, fwRandomNumLength>& randNum)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Start FW update mode");

    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Channel not supported. Failed to set FW update mode");
        return ipmi::responseCommandNotAvailable();
    }
    /* Firmware Update Random number is valid for 30 seconds only */
    auto timeElapsed = (std::chrono::steady_clock::now() - fwRandomNumGenTs);
    if (std::chrono::duration_cast<std::chrono::microseconds>(timeElapsed)
            .count() > std::chrono::duration_cast<std::chrono::microseconds>(
                           fwRandomNumExpirySeconds)
                           .count())
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Firmware update random number expired.");
        return ipmi::responseInvalidFieldRequest();
    }

    /* Validate random number */
    for (int i = 0; i < fwRandomNumLength; i++)
    {
        if (fwRandomNum[i] != randNum[i])
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Invalid random number specified.");
            return ipmi::responseInvalidFieldRequest();
        }
    }

    try
    {
        if (getFirmwareUpdateMode())
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "Already firmware update is in progress.");
            return ipmi::responseBusy();
        }
    }
    catch (const std::exception& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    // FIXME? c++ doesn't off an option for exclusive file creation
    FILE* fp = fopen(firmwareBufferFile, "wx");
    if (!fp)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Unable to open file.");
        return ipmi::responseUnspecifiedError();
    }
    fclose(fp);

    try
    {
        setFirmwareUpdateMode(true);
    }
    catch (const std::exception& e)
    {
        unlink(firmwareBufferFile);
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

/** @brief implements exit firmware update mode command
 *  @param None
 *
 *  @returns IPMI completion code
 */
ipmi::RspType<> ipmiExitFirmwareUpdateMode(const ipmi::Context::ptr& ctx)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Exit FW update mode");
    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Command not supported. Failed to exit firmware update mode");
        return ipmi::responseCommandNotAvailable();
    }

    switch (fwUpdateStatus.getState())
    {
        case FwUpdateStatusCache::fwStateInit:
        case FwUpdateStatusCache::fwStateIdle:
            return ipmi::responseInvalidFieldRequest();
            break;
        case FwUpdateStatusCache::fwStateDownload:
        case FwUpdateStatusCache::fwStateVerify:
            break;
        case FwUpdateStatusCache::fwStateProgram:
            break;
        case FwUpdateStatusCache::fwStateUpdateSuccess:
        case FwUpdateStatusCache::fwStateError:
            break;
        case FwUpdateStatusCache::fwStateAcCycleRequired:
            return ipmi::responseInvalidFieldRequest();
            break;
    }
    fwUpdateStatus.firmwareUpdateAbortState();

    try
    {
        setFirmwareUpdateMode(false);
    }
    catch (const std::exception& e)
    {
        return ipmi::responseUnspecifiedError();
    }

    return ipmi::responseSuccess();
}

/** @brief implements Get/Set Firmware Update Control
 *  @param[in] ctx - context of current session
 *  @parameter
 *   - Byte 1: Control Byte
 *   - Byte 2: Firmware filename length (Optional)
 *   - Byte 3:N: Firmware filename data (Optional)
 *  @returns IPMI completion code plus response data
 *   - Byte 2: Current control status
 **/
ipmi::RspType<bool, bool, bool, bool, uint4_t>
    ipmiGetSetFirmwareUpdateControl(const ipmi::Context::ptr& ctx,
                                    const uint8_t controlReq,
                                    const std::optional<std::string>& fileName)
{
    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Channel not supported. Failed to get or set FW update control");
        return ipmi::responseCommandNotAvailable();
    }

    static std::string fwXferUriPath;
    static bool imageTransferStarted = false;
    static bool imageTransferCompleted = false;
    static bool imageTransferAborted = false;

    if ((controlReq !=
         static_cast<uint8_t>(FwUpdateCtrlReq::setFirmwareFilename)) &&
        (fileName))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid request field (Filename).");
        return ipmi::responseInvalidFieldRequest();
    }

    static bool usbAttached = getUsbStatus();

    switch (static_cast<FwUpdateCtrlReq>(controlReq))
    {
        case FwUpdateCtrlReq::getCurrentControlStatus:
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Get status");
            break;
        case FwUpdateCtrlReq::imageTransferStart:
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Set transfer start");
            imageTransferStarted = true;
            // reset buffer to empty (truncate file)
            std::ofstream out(firmwareBufferFile,
                              std::ofstream::binary | std::ofstream::trunc);
            fwXferUriPath = std::string("file://") + firmwareBufferFile;
            if (xferHashCheck)
            {
                xferHashCheck->clear();
            }
            // Setting state to download
            fwUpdateStatus.setState(
                static_cast<uint8_t>(FwUpdateStatusCache::fwStateDownload));
#ifdef INTEL_PFR_ENABLED
            imgLength = 0;
            imgType = 0;
            block0Mapped = false;
#endif
        }
        break;
        case FwUpdateCtrlReq::imageTransferComplete:
        {
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Set transfer complete.");
            if (usbAttached)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "USB should be detached to perform this operation.");
                return ipmi::responseNotSupportedInPresentState();
            }
            // finish transfer based on URI
            if (!transferFirmwareFromUri(fwXferUriPath))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "transferFirmwareFromUri failed.");
                return ipmi::responseUnspecifiedError();
            }
            // transfer complete
            if (xferHashCheck)
            {
                if (TransferHashCheck::HashCheck::sha2Success !=
                    xferHashCheck->verify())
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "xferHashCheck failed.");
                    return ipmi::responseUnspecifiedError();
                }
            }
            // Set state to verify and start the update
            fwUpdateStatus.setState(
                static_cast<uint8_t>(FwUpdateStatusCache::fwStateVerify));
            // start the request
            if (!startFirmwareUpdate(firmwareBufferFile))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "startFirmwareUpdate failed.");
                return ipmi::responseUnspecifiedError();
            }
            imageTransferCompleted = true;
        }
        break;
        case FwUpdateCtrlReq::imageTransferAbort:
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Set transfer abort.");
            if (usbAttached)
            {
                if (detachUsbDevice())
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Detach USB device failed.");
                    return ipmi::responseUsbAttachOrDetachFailed();
                }
                usbAttached = false;
            }
            // During abort request reset the state to Init by cleaning update
            // file.
            fwUpdateStatus.firmwareUpdateAbortState();
            imageTransferAborted = true;
            break;
        case FwUpdateCtrlReq::setFirmwareFilename:
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Set filename.");
            if (!fileName || ((*fileName).length() == 0))
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Invalid Filename specified.");
                return ipmi::responseInvalidFieldRequest();
            }

            fwXferUriPath = *fileName;
            break;
        case FwUpdateCtrlReq::attachUsbDevice:
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Attach USB device.");
            if (usbAttached)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "USB device is already attached.");
                return ipmi::responseInvalidFieldRequest();
            }
            if (attachUsbDevice())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Attach USB device failed.");
                return ipmi::responseUsbAttachOrDetachFailed();
            }
            usbAttached = true;
            break;
        case FwUpdateCtrlReq::detachUsbDevice:
            phosphor::logging::log<phosphor::logging::level::INFO>(
                "ipmiGetSetFirmwareUpdateControl: Detach USB device.");
            if (!usbAttached)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "USB device is not attached.");
                return ipmi::responseInvalidFieldRequest();
            }
            if (detachUsbDevice())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Detach USB device failed.");
                return ipmi::responseUsbAttachOrDetachFailed();
            }
            usbAttached = false;
            break;
        default:
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Invalid control option specified.");
            return ipmi::responseInvalidFieldRequest();
    }

    return ipmi::responseSuccess(imageTransferStarted, imageTransferCompleted,
                                 imageTransferAborted, usbAttached, uint4_t(0));
}

/** @brief implements firmware get status command
 *  @parameter
 *   -  none
 *  @returns IPMI completion code plus response data
 *   - status     -  processing status
 *   - percentage -  percentage completion
 *   - check      -  channel integrity check status
 **/
ipmi::RspType<uint8_t, // status
              uint8_t, // percentage
              uint8_t  // check
              >
    ipmiGetFirmwareUpdateStatus()

{
    // Byte 1 - status (0=init, 1=idle, 2=download, 3=validate, 4=write,
    //                  5=ready, f=error, 83=ac cycle required)
    // Byte 2 - percent
    // Byte 3 - integrity check status (0=none, 1=req, 2=sha2ok, e2=sha2fail)
    uint8_t status = fwUpdateStatus.getState();
    uint8_t percent = fwUpdateStatus.percent();
    uint8_t check = xferHashCheck ? xferHashCheck->status() : 0;

    // Status code.
    return ipmi::responseSuccess(status, percent, check);
}

ipmi::RspType<bool, bool, bool, uint5_t> ipmiSetFirmwareUpdateOptions(
    const ipmi::Context::ptr& ctx, bool noDowngradeMask, bool deferRestartMask,
    bool sha2CheckMask, uint5_t reserved1, bool noDowngrade, bool deferRestart,
    bool sha2Check, uint5_t reserved2,
    std::optional<std::vector<uint8_t>> integrityCheckVal)
{
    phosphor::logging::log<phosphor::logging::level::INFO>(
        "Set firmware update options.");
    bool isIPMBChannel = false;

    if (checkIPMBChannel(ctx, isIPMBChannel) != ipmi::ccSuccess)
    {
        return ipmi::responseUnspecifiedError();
    }
    if (isIPMBChannel)
    {
        phosphor::logging::log<phosphor::logging::level::INFO>(
            "Channel not supported. Failed to set firmware update options");
        return ipmi::responseCommandNotAvailable();
    }
    bool noDowngradeState = fwUpdateStatus.getInhibitDowngrade();
    bool deferRestartState = fwUpdateStatus.getDeferRestart();
    bool sha2CheckState = xferHashCheck ? true : false;

    if (noDowngradeMask && (noDowngradeState != noDowngrade))
    {
        fwUpdateStatus.setInhibitDowngrade(noDowngrade);
        noDowngradeState = noDowngrade;
    }
    if (deferRestartMask && (deferRestartState != deferRestart))
    {
        fwUpdateStatus.setDeferRestart(deferRestart);
        deferRestartState = deferRestart;
    }
    if (sha2CheckMask)
    {
        if (sha2Check)
        {
            auto hashSize = EVP_MD_size(EVP_sha256());
            if ((*integrityCheckVal).size() != hashSize)
            {
                phosphor::logging::log<phosphor::logging::level::DEBUG>(
                    "Invalid size of Hash specified.");
                return ipmi::responseInvalidFieldRequest();
            }
            xferHashCheck = std::make_shared<TransferHashCheck>();
            xferHashCheck->init(*integrityCheckVal);
        }
        else
        {
            // delete the xferHashCheck object
            xferHashCheck.reset();
        }
        sha2CheckState = sha2CheckMask;
    }
    return ipmi::responseSuccess(noDowngradeState, deferRestartState,
                                 sha2CheckState, reserved1);
}

ipmi::RspType<uint32_t>
    ipmiFwImageWriteData(const std::vector<uint8_t>& writeData)
{
    const uint8_t ccCmdNotSupportedInPresentState = 0xD5;
    size_t writeDataLen = writeData.size();

    if (!writeDataLen)
    {
        return ipmi::responseReqDataLenInvalid();
    }

    if (fwUpdateStatus.getState() != FwUpdateStatusCache::fwStateDownload)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Invalid firmware update state.");
        return ipmi::response(ccCmdNotSupportedInPresentState);
    }

    std::ofstream out(firmwareBufferFile,
                      std::ofstream::binary | std::ofstream::app);
    if (!out)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "Error while opening file.");
        return ipmi::responseUnspecifiedError();
    }

    uint64_t fileDataLen = out.tellp();

    if ((fileDataLen + writeDataLen) > maxFirmwareImageSize)
    {
        phosphor::logging::log<phosphor::logging::level::DEBUG>(
            "Firmware image size exceeds the limit");
        return ipmi::responseInvalidFieldRequest();
    }

    const char* data = reinterpret_cast<const char*>(writeData.data());
    out.write(data, writeDataLen);
    out.close();

    if (xferHashCheck)
    {
        xferHashCheck->hash(writeData);
    }

#ifdef INTEL_PFR_ENABLED
    /* PFR image block 0 - As defined in HAS */
    struct PFRImageBlock0
    {
        uint32_t tag;
        uint32_t pcLength;
        uint32_t pcType;
        uint32_t reserved1;
        uint8_t hash256[32];
        uint8_t hash384[48];
        uint8_t reserved2[32];
    } __attribute__((packed));

    /* Get the PFR block 0 data and read the uploaded image
     * information( Image type, length etc) */
    if (((fileDataLen + writeDataLen) >= sizeof(PFRImageBlock0)) &&
        (!block0Mapped))
    {
        struct PFRImageBlock0 block0Data = {0};

        std::ifstream inFile(firmwareBufferFile,
                             std::ios::binary | std::ios::in);
        inFile.read(reinterpret_cast<char*>(&block0Data), sizeof(block0Data));
        inFile.close();

        uint32_t magicNum = block0Data.tag;

        /* Validate the magic number */
        if (magicNum != perBlock0MagicNum)
        {
            phosphor::logging::log<phosphor::logging::level::DEBUG>(
                "PFR image magic number not matched");
            return ipmi::responseInvalidFieldRequest();
        }
        // Note:imgLength, imgType and block0Mapped are in global scope, as
        // these are used in cascaded updates.
        imgLength = block0Data.pcLength;
        imgType = block0Data.pcType;
        block0Mapped = true;
    }
#endif // end of INTEL_PFR_ENABLED
    return ipmi::responseSuccess(writeDataLen);
}

static void registerFirmwareFunctions()
{
    // guarantee that we start with an already timed out timestamp
    fwRandomNumGenTs =
        std::chrono::steady_clock::now() - fwRandomNumExpirySeconds;
    fwUpdateStatus.setState(
        static_cast<uint8_t>(FwUpdateStatusCache::fwStateInit));

    unlink(firmwareBufferFile);

#ifdef INTEL_PFR_ENABLED
    // Following commands are supported only for PFR enabled platforms
    // CMD:0x20 - Get Firmware Version Information
    // CMD:0x21 - Get Firmware Security Version Information
    // CMD:0x25 - Get Root Certificate Data

    // get firmware version information
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetFwVersionInfo,
                          ipmi::Privilege::Admin, ipmiGetFwVersionInfo);

    // get firmware security version information
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetFwSecurityVersionInfo,
                          ipmi::Privilege::Admin, ipmiGetFwSecurityVersionInfo);

    // get root certificate data
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdFwGetRootCertData,
                          ipmi::Privilege::Admin, ipmiGetFwRootCertData);
#endif

    // get firmware update channel information (max transfer sizes)
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetFwUpdateChannelInfo,
                          ipmi::Privilege::Admin, ipmiFirmwareMaxTransferSize);

    // get bmc execution context
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetBmcExecutionContext,
                          ipmi::Privilege::Admin, ipmiGetBmcExecutionContext);

    // Get Firmware Update Random number
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetFwUpdateRandomNumber,
                          ipmi::Privilege::Admin, ipmiGetFwUpdateRandomNumber);

    // Set Firmware Update Mode
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdSetFirmwareUpdateMode,
                          ipmi::Privilege::Admin, ipmiSetFirmwareUpdateMode);

    // Exit Firmware Update Mode
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdExitFirmwareUpdateMode,
                          ipmi::Privilege::Admin, ipmiExitFirmwareUpdateMode);

    // Get/Set Firmware Update Control
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetSetFwUpdateControl,
                          ipmi::Privilege::Admin,
                          ipmiGetSetFirmwareUpdateControl);

    // Get Firmware Update Status
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdGetFirmwareUpdateStatus,
                          ipmi::Privilege::Admin, ipmiGetFirmwareUpdateStatus);

    // Set Firmware Update Options
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdSetFirmwareUpdateOptions,
                          ipmi::Privilege::Admin, ipmiSetFirmwareUpdateOptions);
    // write image data
    ipmi::registerHandler(ipmi::prioOpenBmcBase, ipmi::netFnFirmware,
                          ipmi::firmware::cmdFwImageWriteData,
                          ipmi::Privilege::Admin, ipmiFwImageWriteData);
    return;
}
