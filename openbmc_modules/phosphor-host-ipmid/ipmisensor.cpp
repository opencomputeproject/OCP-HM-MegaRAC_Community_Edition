#include "sensorhandler.hpp"

#include <malloc.h>

extern uint8_t find_type_for_sensor_number(uint8_t);

struct sensorRES_t
{
    uint8_t sensor_number;
    uint8_t operation;
    uint8_t sensor_reading;
    uint8_t assert_state7_0;
    uint8_t assert_state14_8;
    uint8_t deassert_state7_0;
    uint8_t deassert_state14_8;
    uint8_t event_data1;
    uint8_t event_data2;
    uint8_t event_data3;
} __attribute__((packed));

#define ISBITSET(x, y) (((x) >> (y)) & 0x01)
#define ASSERTINDEX 0
#define DEASSERTINDEX 1

// Sensor Type,  Offset, function handler, Dbus Method, Assert value, Deassert
// value
struct lookup_t
{
    uint8_t sensor_type;
    uint8_t offset;
    int (*func)(const sensorRES_t*, const lookup_t*, const char*);
    char member[16];
    char assertion[64];
    char deassertion[64];
};

extern int updateDbusInterface(uint8_t, const char*, const char*);

int set_sensor_dbus_state_simple(const sensorRES_t* pRec,
                                 const lookup_t* pTable, const char* value)
{

    return set_sensor_dbus_state_s(pRec->sensor_number, pTable->member, value);
}

struct event_data_t
{
    uint8_t data;
    char text[64];
};

event_data_t g_fwprogress02h[] = {{0x00, "Unspecified"},
                                  {0x01, "Memory Init"},
                                  {0x02, "HD Init"},
                                  {0x03, "Secondary Proc Init"},
                                  {0x04, "User Authentication"},
                                  {0x05, "User init system setup"},
                                  {0x06, "USB configuration"},
                                  {0x07, "PCI configuration"},
                                  {0x08, "Option ROM Init"},
                                  {0x09, "Video Init"},
                                  {0x0A, "Cache Init"},
                                  {0x0B, "SM Bus init"},
                                  {0x0C, "Keyboard Init"},
                                  {0x0D, "Embedded ctrl init"},
                                  {0x0E, "Docking station attachment"},
                                  {0x0F, "Enable docking station"},
                                  {0x10, "Docking station ejection"},
                                  {0x11, "Disabling docking station"},
                                  {0x12, "Calling OS Wakeup"},
                                  {0x13, "Starting OS"},
                                  {0x14, "Baseboard Init"},
                                  {0x15, ""},
                                  {0x16, "Floppy Init"},
                                  {0x17, "Keyboard Test"},
                                  {0x18, "Pointing Device Test"},
                                  {0x19, "Primary Proc Init"},
                                  {0xFF, "Unknown"}};

event_data_t g_fwprogress00h[] = {
    {0x00, "Unspecified."},
    {0x01, "No system memory detected"},
    {0x02, "No usable system memory"},
    {0x03, "Unrecoverable hard-disk/ATAPI/IDE"},
    {0x04, "Unrecoverable system-board"},
    {0x05, "Unrecoverable diskette"},
    {0x06, "Unrecoverable hard-disk controller"},
    {0x07, "Unrecoverable PS/2 or USB keyboard"},
    {0x08, "Removable boot media not found"},
    {0x09, "Unrecoverable video controller"},
    {0x0A, "No video device detected"},
    {0x0B, "Firmware ROM corruption detected"},
    {0x0C, "CPU voltage mismatch"},
    {0x0D, "CPU speed matching"},
    {0xFF, "unknown"},
};

char* event_data_lookup(event_data_t* p, uint8_t b)
{

    while (p->data != 0xFF)
    {
        if (p->data == b)
        {
            break;
        }
        p++;
    }

    return p->text;
}

//  The fw progress sensor contains some additional information that needs to be
//  processed prior to calling the dbus code.
int set_sensor_dbus_state_fwprogress(const sensorRES_t* pRec,
                                     const lookup_t* pTable, const char* value)
{

    char valuestring[128];
    char* p = valuestring;

    switch (pTable->offset)
    {

        case 0x00:
            std::snprintf(
                p, sizeof(valuestring), "POST Error, %s",
                event_data_lookup(g_fwprogress00h, pRec->event_data2));
            break;
        case 0x01: /* Using g_fwprogress02h for 0x01 because that's what the
                      ipmi spec says to do */
            std::snprintf(
                p, sizeof(valuestring), "FW Hang, %s",
                event_data_lookup(g_fwprogress02h, pRec->event_data2));
            break;
        case 0x02:
            std::snprintf(
                p, sizeof(valuestring), "FW Progress, %s",
                event_data_lookup(g_fwprogress02h, pRec->event_data2));
            break;
        default:
            std::snprintf(
                p, sizeof(valuestring),
                "Internal warning, fw_progres offset unknown (0x%02x)",
                pTable->offset);
            break;
    }

    return set_sensor_dbus_state_s(pRec->sensor_number, pTable->member, p);
}

// Handling this special OEM sensor by coping what is in byte 4.  I also think
// that is odd considering byte 3 is for sensor reading.  This seems like a
// misuse of the IPMI spec
int set_sensor_dbus_state_osbootcount(const sensorRES_t* pRec,
                                      const lookup_t* pTable, const char* value)
{
    return set_sensor_dbus_state_y(pRec->sensor_number, "setValue",
                                   pRec->assert_state7_0);
}

int set_sensor_dbus_state_system_event(const sensorRES_t* pRec,
                                       const lookup_t* pTable,
                                       const char* value)
{
    char valuestring[128];
    char* p = valuestring;

    switch (pTable->offset)
    {

        case 0x00:
            std::snprintf(p, sizeof(valuestring), "System Reconfigured");
            break;
        case 0x01:
            std::snprintf(p, sizeof(valuestring), "OEM Boot Event");
            break;
        case 0x02:
            std::snprintf(p, sizeof(valuestring),
                          "Undetermined System Hardware Failure");
            break;
        case 0x03:
            std::snprintf(
                p, sizeof(valuestring),
                "System Failure see error log for more details (0x%02x)",
                pRec->event_data2);
            break;
        case 0x04:
            std::snprintf(
                p, sizeof(valuestring),
                "System Failure see PEF error log for more details (0x%02x)",
                pRec->event_data2);
            break;
        default:
            std::snprintf(
                p, sizeof(valuestring),
                "Internal warning, system_event offset unknown (0x%02x)",
                pTable->offset);
            break;
    }

    return set_sensor_dbus_state_s(pRec->sensor_number, pTable->member, p);
}

//  This table lists only senors we care about telling dbus about.
//  Offset definition cab be found in section 42.2 of the IPMI 2.0
//  spec.  Add more if/when there are more items of interest.
lookup_t g_ipmidbuslookup[] = {

    {0xe9, 0x00, set_sensor_dbus_state_simple, "setValue", "Disabled",
     ""}, // OCC Inactive 0
    {0xe9, 0x01, set_sensor_dbus_state_simple, "setValue", "Enabled",
     ""}, // OCC Active 1
    // Turbo Allowed
    {0xda, 0x00, set_sensor_dbus_state_simple, "setValue", "True", "False"},
    // Power Supply Derating
    {0xb4, 0x00, set_sensor_dbus_state_simple, "setValue", "", ""},
    // Power Cap
    {0xC2, 0x00, set_sensor_dbus_state_simple, "setValue", "", ""},
    {0x07, 0x07, set_sensor_dbus_state_simple, "setPresent", "True", "False"},
    {0x07, 0x08, set_sensor_dbus_state_simple, "setFault", "True", "False"},
    {0x0C, 0x06, set_sensor_dbus_state_simple, "setPresent", "True", "False"},
    {0x0C, 0x04, set_sensor_dbus_state_simple, "setFault", "True", "False"},
    {0x0F, 0x02, set_sensor_dbus_state_fwprogress, "setValue", "True", "False"},
    {0x0F, 0x01, set_sensor_dbus_state_fwprogress, "setValue", "True", "False"},
    {0x0F, 0x00, set_sensor_dbus_state_fwprogress, "setValue", "True", "False"},
    {0xC7, 0x01, set_sensor_dbus_state_simple, "setFault", "True", "False"},
    {0xc3, 0x00, set_sensor_dbus_state_osbootcount, "setValue", "", ""},
    {0x1F, 0x00, set_sensor_dbus_state_simple, "setValue",
     "Boot completed (00)", ""},
    {0x1F, 0x01, set_sensor_dbus_state_simple, "setValue",
     "Boot completed (01)", ""},
    {0x1F, 0x02, set_sensor_dbus_state_simple, "setValue", "PXE boot completed",
     ""},
    {0x1F, 0x03, set_sensor_dbus_state_simple, "setValue",
     "Diagnostic boot completed", ""},
    {0x1F, 0x04, set_sensor_dbus_state_simple, "setValue",
     "CD-ROM boot completed", ""},
    {0x1F, 0x05, set_sensor_dbus_state_simple, "setValue", "ROM boot completed",
     ""},
    {0x1F, 0x06, set_sensor_dbus_state_simple, "setValue",
     "Boot completed (06)", ""},
    {0x12, 0x00, set_sensor_dbus_state_system_event, "setValue", "", ""},
    {0x12, 0x01, set_sensor_dbus_state_system_event, "setValue", "", ""},
    {0x12, 0x02, set_sensor_dbus_state_system_event, "setValue", "", ""},
    {0x12, 0x03, set_sensor_dbus_state_system_event, "setValue", "", ""},
    {0x12, 0x04, set_sensor_dbus_state_system_event, "setValue", "", ""},
    {0xCA, 0x00, set_sensor_dbus_state_simple, "setValue", "Disabled", ""},
    {0xCA, 0x01, set_sensor_dbus_state_simple, "setValue", "Enabled", ""},
    {0xFF, 0xFF, NULL, "", "", ""}};

void reportSensorEventAssert(const sensorRES_t* pRec, int index)
{
    lookup_t* pTable = &g_ipmidbuslookup[index];
    (*pTable->func)(pRec, pTable, pTable->assertion);
}
void reportSensorEventDeassert(const sensorRES_t* pRec, int index)
{
    lookup_t* pTable = &g_ipmidbuslookup[index];
    (*pTable->func)(pRec, pTable, pTable->deassertion);
}

int findindex(const uint8_t sensor_type, int offset, int* index)
{

    int i = 0, rc = 0;
    lookup_t* pTable = g_ipmidbuslookup;

    do
    {
        if (((pTable + i)->sensor_type == sensor_type) &&
            ((pTable + i)->offset == offset))
        {
            rc = 1;
            *index = i;
            break;
        }
        i++;
    } while ((pTable + i)->sensor_type != 0xFF);

    return rc;
}

bool shouldReport(uint8_t sensorType, int offset, int* index)
{
    bool rc = false;

    if (findindex(sensorType, offset, index))
    {
        rc = true;
    }
    if (rc == false)
    {
#ifdef __IPMI_DEBUG__
        log<level::DEBUG>("LOOKATME: Sensor should not be reported",
                          entry("SENSORTYPE=0x%02x", sensorType),
                          entry("OFFSET=0x%02x", offset));
#endif
    }

    return rc;
}

int updateSensorRecordFromSSRAESC(const void* record)
{
    auto pRec = static_cast<const sensorRES_t*>(record);
    uint8_t stype;
    int index;

    stype = find_type_for_sensor_number(pRec->sensor_number);

    // 0xC3 types use the assertion7_0 for the value to be set
    // so skip the reseach and call the correct event reporting
    // function
    if (stype == 0xC3)
    {
        shouldReport(stype, 0x00, &index);
        reportSensorEventAssert(pRec, index);
    }
    else
    {
        // Scroll through each bit position .  Determine
        // if any bit is either asserted or Deasserted.
        for (int i = 0; i < 8; i++)
        {

            if ((ISBITSET(pRec->assert_state7_0, i)) &&
                (shouldReport(stype, i, &index)))
            {
                reportSensorEventAssert(pRec, index);
            }
            if ((ISBITSET(pRec->assert_state14_8, i)) &&
                (shouldReport(stype, i + 8, &index)))
            {
                reportSensorEventAssert(pRec, index);
            }
            if ((ISBITSET(pRec->deassert_state7_0, i)) &&
                (shouldReport(stype, i, &index)))
            {
                reportSensorEventDeassert(pRec, index);
            }
            if ((ISBITSET(pRec->deassert_state14_8, i)) &&
                (shouldReport(stype, i + 8, &index)))
            {
                reportSensorEventDeassert(pRec, index);
            }
        }
    }

    return 0;
}
