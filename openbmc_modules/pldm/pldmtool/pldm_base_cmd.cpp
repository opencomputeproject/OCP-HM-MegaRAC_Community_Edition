#include "pldm_base_cmd.hpp"

#include "libpldm/utils.h"

#include "pldm_cmd_helper.hpp"

#ifdef OEM_IBM
#include "libpldm/file_io.h"
#include "libpldm/host.h"
#endif

namespace pldmtool
{

namespace base
{

namespace
{

using namespace pldmtool::helper;

std::vector<std::unique_ptr<CommandInterface>> commands;
const std::map<const char*, pldm_supported_types> pldmTypes{
    {"base", PLDM_BASE},   {"platform", PLDM_PLATFORM},
    {"bios", PLDM_BIOS},   {"fru", PLDM_FRU},
#ifdef OEM_IBM
    {"oem-ibm", PLDM_OEM},
#endif
};

const std::map<const char*, pldm_supported_commands> pldmBaseCmds{
    {"GetTID", PLDM_GET_TID},
    {"GetPLDMVersion", PLDM_GET_PLDM_VERSION},
    {"GetPLDMTypes", PLDM_GET_PLDM_TYPES},
    {"GetPLDMCommands", PLDM_GET_PLDM_COMMANDS}};

const std::map<const char*, pldm_bios_commands> pldmBiosCmds{
    {"GetBIOSTable", PLDM_GET_BIOS_TABLE},
    {"SetBIOSAttributeCurrentValue", PLDM_SET_BIOS_ATTRIBUTE_CURRENT_VALUE},
    {"GetBIOSAttributeCurrentValueByHandle",
     PLDM_GET_BIOS_ATTRIBUTE_CURRENT_VALUE_BY_HANDLE},
    {"GetDateTime", PLDM_GET_DATE_TIME},
    {"SetDateTime", PLDM_SET_DATE_TIME}};

const std::map<const char*, pldm_platform_commands> pldmPlatformCmds{
    {"SetNumericEffecterValue", PLDM_SET_NUMERIC_EFFECTER_VALUE},
    {"SetStateEffecterStates", PLDM_SET_STATE_EFFECTER_STATES},
    {"GetPDR", PLDM_GET_PDR}};

const std::map<const char*, pldm_fru_commands> pldmFruCmds{
    {"GetFRURecordTableMetadata", PLDM_GET_FRU_RECORD_TABLE_METADATA},
    {"GetFRURecordTable", PLDM_GET_FRU_RECORD_TABLE},
    {"GetFRURecordByOption", PLDM_GET_FRU_RECORD_BY_OPTION}};

#ifdef OEM_IBM
const std::map<const char*, pldm_host_commands> pldmIBMHostCmds{
    {"GetAlertStatus", PLDM_HOST_GET_ALERT_STATUS}};

const std::map<const char*, pldm_fileio_commands> pldmIBMFileIOCmds{
    {"GetFileTable", PLDM_GET_FILE_TABLE},
    {"ReadFile", PLDM_READ_FILE},
    {"WriteFile", PLDM_WRITE_FILE},
    {"ReadFileInToMemory", PLDM_READ_FILE_INTO_MEMORY},
    {"WriteFileFromMemory", PLDM_WRITE_FILE_FROM_MEMORY},
    {"ReadFileByTypeIntoMemory", PLDM_READ_FILE_BY_TYPE_INTO_MEMORY},
    {"WriteFileByTypeFromMemory", PLDM_WRITE_FILE_BY_TYPE_FROM_MEMORY},
    {"NewFileAvailable", PLDM_NEW_FILE_AVAILABLE},
    {"ReadFileByType", PLDM_READ_FILE_BY_TYPE},
    {"WriteFileByType", PLDM_WRITE_FILE_BY_TYPE},
    {"FileAck", PLDM_FILE_ACK}};
#endif

} // namespace

class GetPLDMTypes : public CommandInterface
{
  public:
    ~GetPLDMTypes() = default;
    GetPLDMTypes() = delete;
    GetPLDMTypes(const GetPLDMTypes&) = delete;
    GetPLDMTypes(GetPLDMTypes&&) = default;
    GetPLDMTypes& operator=(const GetPLDMTypes&) = delete;
    GetPLDMTypes& operator=(GetPLDMTypes&&) = default;

    using CommandInterface::CommandInterface;

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        auto rc = encode_get_types_req(instanceId, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        std::vector<bitfield8_t> types(8);
        auto rc = decode_get_types_resp(responsePtr, payloadLength, &cc,
                                        types.data());
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << "\n";
            return;
        }

        printPldmTypes(types);
    }

  private:
    void printPldmTypes(std::vector<bitfield8_t>& types)
    {
        std::cout << "Supported types:";
        for (int i = 0; i < PLDM_MAX_TYPES; i++)
        {
            bitfield8_t b = types[i / 8];
            if (b.byte & (1 << i % 8))
            {
                std::cout << " " << i;
                auto it = std::find_if(
                    pldmTypes.begin(), pldmTypes.end(),
                    [i](const auto& typePair) { return typePair.second == i; });
                if (it != pldmTypes.end())
                {

                    std::cout << "(" << it->first << ")";
                }
            }
        }

        std::cout << std::endl;
    }
};

class GetPLDMVersion : public CommandInterface
{
  public:
    ~GetPLDMVersion() = default;
    GetPLDMVersion() = delete;
    GetPLDMVersion(const GetPLDMVersion&) = delete;
    GetPLDMVersion(GetPLDMVersion&&) = default;
    GetPLDMVersion& operator=(const GetPLDMVersion&) = delete;
    GetPLDMVersion& operator=(GetPLDMVersion&&) = default;

    explicit GetPLDMVersion(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-t,--type", pldmType, "pldm supported type")
            ->required()
            ->transform(CLI::CheckedTransformer(pldmTypes, CLI::ignore_case));
    }
    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_VERSION_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc = encode_get_version_req(instanceId, 0, PLDM_GET_FIRSTPART,
                                         pldmType, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0, transferFlag = 0;
        uint32_t transferHandle = 0;
        ver32_t version;
        auto rc =
            decode_get_version_resp(responsePtr, payloadLength, &cc,
                                    &transferHandle, &transferFlag, &version);
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << "\n";
            return;
        }
        char buffer[16] = {0};
        ver2str(&version, buffer, sizeof(buffer));
        std::cout << "Type " << pldmType;
        auto it = std::find_if(
            pldmTypes.begin(), pldmTypes.end(),
            [&](const auto& typePair) { return typePair.second == pldmType; });

        if (it != pldmTypes.end())
        {
            std::cout << "(" << it->first << ")";
        }
        std::cout << ": " << buffer << std::endl;
    }

  private:
    pldm_supported_types pldmType;
};

class GetTID : public CommandInterface
{
  public:
    ~GetTID() = default;
    GetTID() = delete;
    GetTID(const GetTID&) = delete;
    GetTID(GetTID&&) = default;
    GetTID& operator=(const GetTID&) = delete;
    GetTID& operator=(GetTID&&) = default;

    using CommandInterface::CommandInterface;

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        auto rc = encode_get_tid_req(instanceId, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        uint8_t tid = 0;
        std::vector<bitfield8_t> types(8);
        auto rc = decode_get_tid_resp(responsePtr, payloadLength, &cc, &tid);
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << "\n";
            return;
        }
        std::cout << "TID : " << static_cast<uint32_t>(tid) << std::endl;
    }
};

class GetPLDMCommands : public CommandInterface
{
  public:
    ~GetPLDMCommands() = default;
    GetPLDMCommands() = delete;
    GetPLDMCommands(const GetPLDMCommands&) = delete;
    GetPLDMCommands(GetPLDMCommands&&) = default;
    GetPLDMCommands& operator=(const GetPLDMCommands&) = delete;
    GetPLDMCommands& operator=(GetPLDMCommands&&) = default;

    explicit GetPLDMCommands(const char* type, const char* name,
                             CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-t,--type", pldmType, "pldm supported type")
            ->required()
            ->transform(CLI::CheckedTransformer(pldmTypes, CLI::ignore_case));
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_COMMANDS_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
        ver32_t version{0xFF, 0xFF, 0xFF, 0xFF};
        auto rc =
            encode_get_commands_req(instanceId, pldmType, version, request);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        std::vector<bitfield8_t> cmdTypes(32);
        auto rc = decode_get_commands_resp(responsePtr, payloadLength, &cc,
                                           cmdTypes.data());
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << "\n";
            return;
        }
        printPldmCommands(cmdTypes, pldmType);
    }

  private:
    pldm_supported_types pldmType;

    template <typename CommandMap>
    void printCommand(CommandMap& commandMap, int i)
    {
        auto it = std::find_if(
            commandMap.begin(), commandMap.end(),
            [i](const auto& typePair) { return typePair.second == i; });
        if (it != commandMap.end())
        {
            std::cout << "(" << it->first << ")";
        }
    }

    void printPldmCommands(std::vector<bitfield8_t>& cmdTypes,
                           pldm_supported_types pldmType)
    {
        std::cout << "Supported Commands :";
        for (int i = 0; i < PLDM_MAX_CMDS_PER_TYPE; i++)
        {
            bitfield8_t b = cmdTypes[i / 8];
            if (b.byte & (1 << i % 8))
            {
                std::cout << " " << i;
                switch (pldmType)
                {
                    case PLDM_BASE:
                        printCommand(pldmBaseCmds, i);
                        break;
                    case PLDM_PLATFORM:
                        printCommand(pldmPlatformCmds, i);
                        break;
                    case PLDM_BIOS:
                        printCommand(pldmBiosCmds, i);
                        break;
                    case PLDM_FRU:
                        printCommand(pldmFruCmds, i);
                        break;
                    case PLDM_OEM:
#ifdef OEM_IBM
                        printCommand(pldmIBMHostCmds, i);
                        printCommand(pldmIBMFileIOCmds, i);
#endif
                        break;
                    default:
                        break;
                }
            }
        }
        std::cout << std::endl;
    }
};

void registerCommand(CLI::App& app)
{
    auto base = app.add_subcommand("base", "base type command");
    base->require_subcommand(1);

    auto getPLDMTypes =
        base->add_subcommand("GetPLDMTypes", "get pldm supported types");
    commands.push_back(
        std::make_unique<GetPLDMTypes>("base", "GetPLDMTypes", getPLDMTypes));

    auto getPLDMVersion =
        base->add_subcommand("GetPLDMVersion", "get version of a certain type");
    commands.push_back(std::make_unique<GetPLDMVersion>(
        "base", "GetPLDMVersion", getPLDMVersion));

    auto getPLDMTID = base->add_subcommand("GetTID", "get Terminus ID (TID)");
    commands.push_back(std::make_unique<GetTID>("base", "GetTID", getPLDMTID));

    auto getPLDMCommands = base->add_subcommand(
        "GetPLDMCommands", "get supported commands of pldm type");
    commands.push_back(std::make_unique<GetPLDMCommands>(
        "base", "GetPLDMCommands", getPLDMCommands));
}

} // namespace base
} // namespace pldmtool
