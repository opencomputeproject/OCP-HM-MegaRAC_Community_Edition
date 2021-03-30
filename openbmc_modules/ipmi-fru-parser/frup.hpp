#ifndef OPENBMC_IPMI_FRU_PARSER_H
#define OPENBMC_IPMI_FRU_PARSER_H

#include <systemd/sd-bus.h>

#include <array>
#include <map>
#include <string>
#include <utility>
#include <vector>

enum ipmi_fru_area_type
{
    IPMI_FRU_AREA_INTERNAL_USE = 0x00,
    IPMI_FRU_AREA_CHASSIS_INFO,
    IPMI_FRU_AREA_BOARD_INFO,
    IPMI_FRU_AREA_PRODUCT_INFO,
    IPMI_FRU_AREA_MULTI_RECORD,
    IPMI_FRU_AREA_TYPE_MAX
};

enum openbmc_vpd_key_id
{
    OPENBMC_VPD_KEY_CHASSIS_TYPE = 1, /* not a type/len */
    OPENBMC_VPD_KEY_CHASSIS_PART_NUM,
    OPENBMC_VPD_KEY_CHASSIS_SERIAL_NUM,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM1,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM2,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM3,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM4,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM5,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM6,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM7,
    OPENBMC_VPD_KEY_CHASSIS_CUSTOM8,
    OPENBMC_VPD_KEY_CHASSIS_MAX = OPENBMC_VPD_KEY_CHASSIS_CUSTOM8,
    /* TODO: chassis_custom_fields */

    OPENBMC_VPD_KEY_BOARD_MFG_DATE, /* not a type/len */
    OPENBMC_VPD_KEY_BOARD_MFR,
    OPENBMC_VPD_KEY_BOARD_NAME,
    OPENBMC_VPD_KEY_BOARD_SERIAL_NUM,
    OPENBMC_VPD_KEY_BOARD_PART_NUM,
    OPENBMC_VPD_KEY_BOARD_FRU_FILE_ID,
    OPENBMC_VPD_KEY_BOARD_CUSTOM1,
    OPENBMC_VPD_KEY_BOARD_CUSTOM2,
    OPENBMC_VPD_KEY_BOARD_CUSTOM3,
    OPENBMC_VPD_KEY_BOARD_CUSTOM4,
    OPENBMC_VPD_KEY_BOARD_CUSTOM5,
    OPENBMC_VPD_KEY_BOARD_CUSTOM6,
    OPENBMC_VPD_KEY_BOARD_CUSTOM7,
    OPENBMC_VPD_KEY_BOARD_CUSTOM8,
    OPENBMC_VPD_KEY_BOARD_MAX = OPENBMC_VPD_KEY_BOARD_CUSTOM8,
    /* TODO: board_custom_fields */

    OPENBMC_VPD_KEY_PRODUCT_MFR,
    OPENBMC_VPD_KEY_PRODUCT_NAME,
    OPENBMC_VPD_KEY_PRODUCT_PART_MODEL_NUM,
    OPENBMC_VPD_KEY_PRODUCT_VER,
    OPENBMC_VPD_KEY_PRODUCT_SERIAL_NUM,
    OPENBMC_VPD_KEY_PRODUCT_ASSET_TAG,
    OPENBMC_VPD_KEY_PRODUCT_FRU_FILE_ID,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM1,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM2,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM3,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM4,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM5,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM6,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM7,
    OPENBMC_VPD_KEY_PRODUCT_CUSTOM8,
    OPENBMC_VPD_KEY_PRODUCT_MAX = OPENBMC_VPD_KEY_PRODUCT_CUSTOM8,

    OPENBMC_VPD_KEY_MAX,
    OPENBMC_VPD_KEY_CUSTOM_FIELDS_MAX = 8,

};

using IPMIFruInfo =
    std::array<std::pair<std::string, std::string>, OPENBMC_VPD_KEY_MAX>;

struct IPMIFruData
{
    std::string section;
    std::string property;
    std::string delimiter;
};

using DbusProperty = std::string;
using DbusPropertyVec = std::vector<std::pair<DbusProperty, IPMIFruData>>;

using DbusInterface = std::string;
using DbusInterfaceVec = std::vector<std::pair<DbusInterface, DbusPropertyVec>>;

using FruInstancePath = std::string;

struct FruInstance
{
    uint8_t entityID;
    uint8_t entityInstance;
    FruInstancePath path;
    DbusInterfaceVec interfaces;
};

using FruInstanceVec = std::vector<FruInstance>;

using FruId = uint32_t;
using FruMap = std::map<FruId, FruInstanceVec>;

/* Parse an IPMI write fru data message into a dictionary containing name value
 * pair of VPD entries.*/
int parse_fru(const void* msgbuf, sd_bus_message* vpdtbl);

int parse_fru_area(const uint8_t area, const void* msgbuf, const size_t len,
                   IPMIFruInfo& info);

#endif
