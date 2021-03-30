#include "pldm_fru_cmd.hpp"

#include "pldm_cmd_helper.hpp"

#include <endian.h>

#include <functional>
#include <tuple>

namespace pldmtool
{

namespace fru
{

namespace
{

using namespace pldmtool::helper;

std::vector<std::unique_ptr<CommandInterface>> commands;

} // namespace

class GetFruRecordTableMetadata : public CommandInterface
{
  public:
    ~GetFruRecordTableMetadata() = default;
    GetFruRecordTableMetadata() = delete;
    GetFruRecordTableMetadata(const GetFruRecordTableMetadata&) = delete;
    GetFruRecordTableMetadata(GetFruRecordTableMetadata&&) = default;
    GetFruRecordTableMetadata&
        operator=(const GetFruRecordTableMetadata&) = delete;
    GetFruRecordTableMetadata& operator=(GetFruRecordTableMetadata&&) = default;

    using CommandInterface::CommandInterface;

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc = encode_get_fru_record_table_metadata_req(
            instanceId, request, PLDM_GET_FRU_RECORD_TABLE_METADATA_REQ_BYTES);
        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        uint8_t fru_data_major_version, fru_data_minor_version;
        uint32_t fru_table_maximum_size, fru_table_length;
        uint16_t total_record_set_identifiers, total_table_records;
        uint32_t checksum;

        auto rc = decode_get_fru_record_table_metadata_resp(
            responsePtr, payloadLength, &cc, &fru_data_major_version,
            &fru_data_minor_version, &fru_table_maximum_size, &fru_table_length,
            &total_record_set_identifiers, &total_table_records, &checksum);
        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << std::endl;
            return;
        }
        std::cout << "FRUDATAMajorVersion : "
                  << static_cast<uint32_t>(fru_data_major_version) << std::endl;
        std::cout << "FRUDATAMinorVersion : "
                  << static_cast<uint32_t>(fru_data_minor_version) << std::endl;
        std::cout << "FRUTableMaximumSize : " << fru_table_maximum_size
                  << std::endl;
        std::cout << "FRUTableLength : " << fru_table_length << std::endl;
        std::cout << "Total number of Record Set Identifiers in table : "
                  << total_record_set_identifiers << std::endl;
        std::cout << "Total number of records in table :  "
                  << total_table_records << std::endl;
        std::cout << "FRU DATAStructureTableIntegrityChecksum :  " << checksum
                  << std::endl;
    }
};

class FRUTablePrint
{
  public:
    explicit FRUTablePrint(const uint8_t* table, size_t table_size) :
        table(table), table_size(table_size)
    {}

    void print()
    {
        auto p = table;
        while (!isTableEnd(p))
        {
            auto record =
                reinterpret_cast<const pldm_fru_record_data_format*>(p);
            std::cout << "FRU Record Set Identifier: "
                      << (int)le16toh(record->record_set_id) << std::endl;
            std::cout << "FRU Record Type: "
                      << typeToString(fruRecordTypes, record->record_type)
                      << std::endl;
            std::cout << "Number of FRU fields: " << (int)record->num_fru_fields
                      << std::endl;
            std::cout << "Encoding Type for FRU fields: "
                      << typeToString(fruEncodingType, record->encoding_type)
                      << std::endl;

            auto isGeneralRec = true;
            if (record->record_type != PLDM_FRU_RECORD_TYPE_GENERAL)
            {
                isGeneralRec = false;
            }

            p += sizeof(pldm_fru_record_data_format) -
                 sizeof(pldm_fru_record_tlv);
            for (int i = 0; i < record->num_fru_fields; i++)
            {
                auto tlv = reinterpret_cast<const pldm_fru_record_tlv*>(p);
                if (isGeneralRec)
                {
                    fruFieldPrint(record->record_type, tlv->type, tlv->length,
                                  tlv->value);
                }
                p += sizeof(pldm_fru_record_tlv) - 1 + tlv->length;
            }
        }
    }

  private:
    const uint8_t* table;
    size_t table_size;

    bool isTableEnd(const uint8_t* p)
    {
        auto offset = p - table;
        return (table_size - offset) <= 7;
    }

    static inline const std::map<uint8_t, const char*> fruEncodingType{
        {PLDM_FRU_ENCODING_UNSPECIFIED, "Unspecified"},
        {PLDM_FRU_ENCODING_ASCII, "ASCII"},
        {PLDM_FRU_ENCODING_UTF8, "UTF8"},
        {PLDM_FRU_ENCODING_UTF16, "UTF16"},
        {PLDM_FRU_ENCODING_UTF16LE, "UTF16LE"},
        {PLDM_FRU_ENCODING_UTF16BE, "UTF16BE"}};

    static inline const std::map<uint8_t, const char*> fruRecordTypes{
        {PLDM_FRU_RECORD_TYPE_GENERAL, "General"},
        {PLDM_FRU_RECORD_TYPE_OEM, "OEM"}};

    std::string typeToString(std::map<uint8_t, const char*> typeMap,
                             uint8_t type)
    {
        auto typeString = std::to_string(type);
        try
        {
            return typeString + "(" + typeMap.at(type) + ")";
        }
        catch (const std::out_of_range& e)
        {
            return typeString;
        }
    }

    using FruFieldParser =
        std::function<std::string(const uint8_t* value, uint8_t length)>;

    using FieldType = uint8_t;
    using RecordType = uint8_t;
    using FieldName = std::string;
    using FruFieldTypes =
        std::map<FieldType, std::tuple<FieldName, FruFieldParser>>;

    static std::string fruFieldParserString(const uint8_t* value,
                                            uint8_t length)
    {
        return std::string(reinterpret_cast<const char*>(value), length);
    }

    static std::string fruFieldParserTimestamp(const uint8_t*, uint8_t)
    {
        return std::string("TODO");
    }

    static std::string fruFieldParserU32(const uint8_t* value, uint8_t length)
    {
        assert(length == 4);
        uint32_t v;
        std::memcpy(&v, value, length);
        return std::to_string(le32toh(v));
    }

    static inline const FruFieldTypes fruGeneralFieldTypes = {
        {PLDM_FRU_FIELD_TYPE_CHASSIS, {"Chassis", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_MODEL, {"Model", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_PN, {"Part Number", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_SN, {"Serial Number", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_MANUFAC, {"Manufacturer", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_MANUFAC_DATE,
         {"Manufacture Date", fruFieldParserTimestamp}},
        {PLDM_FRU_FIELD_TYPE_VENDOR, {"Vendor", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_NAME, {"Name", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_SKU, {"SKU", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_VERSION, {"Version", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_ASSET_TAG, {"Asset Tag", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_DESC, {"Description", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_EC_LVL,
         {"Engineering Change Level", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_OTHER,
         {"Other Information", fruFieldParserString}},
        {PLDM_FRU_FIELD_TYPE_IANA, {"Vendor IANA", fruFieldParserU32}},
    };

    static inline const FruFieldTypes fruOEMFieldTypes = {
        {1, {"Vendor IANA", fruFieldParserU32}},

    };

    static inline const std::map<RecordType, FruFieldTypes> fruFieldTypes{
        {PLDM_FRU_RECORD_TYPE_GENERAL, fruGeneralFieldTypes},
        {PLDM_FRU_RECORD_TYPE_OEM, fruOEMFieldTypes}};

    void fruFieldPrint(uint8_t recordType, uint8_t type, uint8_t length,
                       const uint8_t* value)
    {
        auto& [typeString, parser] = fruFieldTypes.at(recordType).at(type);

        std::cout << "\tFRU Field Type: " << typeString << std::endl;
        std::cout << "\tFRU Field Length: " << (int)(length) << std::endl;
        std::cout << "\tFRU Field Value: " << parser(value, length)
                  << std::endl;
    }
};

class GetFRURecordByOption : public CommandInterface
{
  public:
    ~GetFRURecordByOption() = default;
    GetFRURecordByOption() = delete;
    GetFRURecordByOption(const GetFRURecordByOption&) = delete;
    GetFRURecordByOption(GetFruRecordTableMetadata&&) = delete;
    GetFRURecordByOption& operator=(const GetFRURecordByOption&) = delete;
    GetFRURecordByOption& operator=(GetFRURecordByOption&&) = delete;

    explicit GetFRURecordByOption(const char* type, const char* name,
                                  CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-i, --identifier", recordSetIdentifier,
                        "Record Set Identifier\n"
                        "Possible values: {All record sets = 0, Specific "
                        "record set = 1 – 65535}")
            ->required();
        app->add_option("-r, --record", recordType,
                        "Record Type\n"
                        "Possible values: {All record types = 0, Specific "
                        "record types = 1 – 255}")
            ->required();
        app->add_option("-f, --field", fieldType,
                        "Field Type\n"
                        "Possible values: {All record field types = 0, "
                        "Specific field types = 1 – 15}")
            ->required();
    }

    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        if (fieldType != 0 && recordType == 0)
        {
            throw std::invalid_argument("if field type is non-zero, the record "
                                        "type shall also be non-zero");
        }

        auto payloadLength = sizeof(pldm_get_fru_record_by_option_req);

        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) + payloadLength,
                                        0);
        auto reqMsg = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc = encode_get_fru_record_by_option_req(
            instanceId, 0 /* DataTransferHandle */, 0 /* FRUTableHandle */,
            recordSetIdentifier, recordType, fieldType, PLDM_GET_FIRSTPART,
            reqMsg, payloadLength);

        return {rc, requestMsg};
    }

    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc;
        uint32_t dataTransferHandle;
        uint8_t transferFlag;
        variable_field fruData;

        auto rc = decode_get_fru_record_by_option_resp(
            responsePtr, payloadLength, &cc, &dataTransferHandle, &transferFlag,
            &fruData);

        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << std::endl;
            return;
        }

        FRUTablePrint tablePrint(fruData.ptr, fruData.length);
        tablePrint.print();
    }

  private:
    uint16_t recordSetIdentifier;
    uint8_t recordType;
    uint8_t fieldType;
};

class GetFruRecordTable : public CommandInterface
{
  public:
    ~GetFruRecordTable() = default;
    GetFruRecordTable() = delete;
    GetFruRecordTable(const GetFruRecordTable&) = delete;
    GetFruRecordTable(GetFruRecordTable&&) = default;
    GetFruRecordTable& operator=(const GetFruRecordTable&) = delete;
    GetFruRecordTable& operator=(GetFruRecordTable&&) = default;

    using CommandInterface::CommandInterface;
    std::pair<int, std::vector<uint8_t>> createRequestMsg() override
    {
        std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr) +
                                        PLDM_GET_FRU_RECORD_TABLE_REQ_BYTES);
        auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());

        auto rc = encode_get_fru_record_table_req(
            instanceId, 0, PLDM_START_AND_END, request,
            requestMsg.size() - sizeof(pldm_msg_hdr));
        return {rc, requestMsg};
    }
    void parseResponseMsg(pldm_msg* responsePtr, size_t payloadLength) override
    {
        uint8_t cc = 0;
        uint32_t next_data_transfer_handle = 0;
        uint8_t transfer_flag = 0;
        size_t fru_record_table_length = 0;
        std::vector<uint8_t> fru_record_table_data(payloadLength);

        auto rc = decode_get_fru_record_table_resp(
            responsePtr, payloadLength, &cc, &next_data_transfer_handle,
            &transfer_flag, fru_record_table_data.data(),
            &fru_record_table_length);

        if (rc != PLDM_SUCCESS || cc != PLDM_SUCCESS)
        {
            std::cerr << "Response Message Error: "
                      << "rc=" << rc << ",cc=" << (int)cc << std::endl;
            return;
        }

        FRUTablePrint tablePrint(fru_record_table_data.data(),
                                 fru_record_table_length);
        tablePrint.print();
    }
};

void registerCommand(CLI::App& app)
{
    auto fru = app.add_subcommand("fru", "FRU type command");
    fru->require_subcommand(1);
    auto getFruRecordTableMetadata = fru->add_subcommand(
        "GetFruRecordTableMetadata", "get FRU record table metadata");
    commands.push_back(std::make_unique<GetFruRecordTableMetadata>(
        "fru", "GetFruRecordTableMetadata", getFruRecordTableMetadata));

    auto getFRURecordByOption =
        fru->add_subcommand("GetFRURecordByOption", "get FRU Record By Option");
    commands.push_back(std::make_unique<GetFRURecordByOption>(
        "fru", "GetFRURecordByOption", getFRURecordByOption));

    auto getFruRecordTable =
        fru->add_subcommand("GetFruRecordTable", "get FRU Record Table");
    commands.push_back(std::make_unique<GetFruRecordTable>(
        "fru", "GetFruRecordTable", getFruRecordTable));
}

} // namespace fru

} // namespace pldmtool
