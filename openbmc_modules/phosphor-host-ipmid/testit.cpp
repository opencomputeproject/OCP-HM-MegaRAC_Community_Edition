
#include <stdint.h>
#include <stdio.h>
#include <string.h>

unsigned char g_sensortype[][2] = {
    {0xC3, 0x01}, {0x07, 0x02}, {0x0F, 0x05}, {0x0c, 0x1F}, {0xFF, 0xff}};

uint8_t find_type_for_sensor_number(uint8_t sensor_number)
{

    int i = 0;
    uint8_t rc;

    while (g_sensortype[i][0] != 0xff)
    {
        if (g_sensortype[i][1] == sensor_number)
        {
            break;
        }
        else
        {
            i++;
        }
    }

    rc = g_sensortype[i][0];

    if (rc == 0xFF)
    {
        rc = 0;
    }
    return rc;
}

char g_results_method[64];
char g_results_value[64];

int set_sensor_dbus_state_s(unsigned char number, const char* member,
                            const char* value)
{
    strcpy(g_results_method, member);
    strcpy(g_results_value, value);

    return 0;
}

int set_sensor_dbus_state_y(unsigned char number, char const* member,
                            uint8_t value)
{

    char val[2];

    snprintf(val, 2, "%d", value);

    strcpy(g_results_method, member);
    strcpy(g_results_value, val);

    return 0;
}

extern int updateSensorRecordFromSSRAESC(const void* record);

// DIMM Present
uint8_t testrec_sensor1[] = {0x1F, 0xa9, 0x00, 0x40, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00};

// DIMM Not present
uint8_t testrec_sensor2[] = {0x1F, 0xa9, 0x00, 0x00, 0x00,
                             0x40, 0x00, 0x00, 0x00, 0x00};

// DIMM Not present
uint8_t testrec_procfailed[] = {0x02, 0xa9, 0x00, 0x00, 0x00,
                                0x00, 0x01, 0x00, 0x00, 0x00};

// Virtual Sensor 5, setting a Value of 0h
uint8_t testrec_bootprogress[] = {0x05, 0xa9, 0x00, 0x04, 0x00,
                                  0x00, 0x00, 0x00, 0x0E, 0x00};

// Virtual Sensor setting a boot count
uint8_t testrec_bootcount[] = {0x01, 0x09, 0x00, 0x03, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00};

// Invalid sensor number
uint8_t testrec_invalidnumber[] = {0x35, 0xa9, 0x00, 0x04, 0x00,
                                   0x00, 0x00, 0x00, 0x03, 0x00};

int check_results(int rc, const char* method, const char* value)
{
    if (strcmp(g_results_method, method))
    {
        log<level::ERR>("Method Failed", entry("EXPECT=%s", method),
                        entry("GOT=%s", g_results_method));
        return -1;
    }
    if (strcmp(g_results_value, value))
    {
        log<level::ERR>("Value failed", entry("EXPECT=%s", value),
                        entry("GOT=%s", g_results_method));
        return -2;
    }

    return 0;
}

void testprep(void)
{
    memset(g_results_method, 0, sizeof(g_results_method));
    memset(g_results_value, 0, sizeof(g_results_value));
}

int main()
{

    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_bootprogress),
                  "setValue", "FW Progress, Docking station attachment");
    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_sensor1), "setPresent",
                  "True");
    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_sensor2), "setPresent",
                  "False");
    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_procfailed), "setFault",
                  "False");
    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_bootcount), "setValue",
                  "3");
    testprep();
    check_results(updateSensorRecordFromSSRAESC(testrec_invalidnumber), "", "");

    return 0;
}
