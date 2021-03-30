/*
 * Copyright (C) 2003-2014 FreeIPMI Core Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*****************************************************************************\
 *  Copyright (C) 2007-2014 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-232183
 *
 *  This file is part of Ipmi-fru, a tool used for retrieving
 *  motherboard field replaceable unit (FRU) information. For details,
 *  see http://www.llnl.gov/linux/.
 *
 *  Ipmi-fru is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmi-fru is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmi-fru.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/
#include "frup.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-bus.h>
#include <time.h>
#include <unistd.h>

#define TEXTSTR(a) #a
#define ASSERT(x)                                                              \
    do                                                                         \
    {                                                                          \
        if (0 == (x))                                                          \
        {                                                                      \
            fprintf(stderr,                                                    \
                    "Assertion failed: %s, "                                   \
                    "%d at \'%s\'\n",                                          \
                    __FILE__, __LINE__, TEXTSTR(a));                           \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX 512
#define IPMI_FRU_SENTINEL_VALUE 0xC1
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK 0xC0
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT 0x06
#define IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK 0x3F
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE 0x03

/* OpenBMC defines for Parser */
#define IPMI_FRU_AREA_INTERNAL_USE 0x00
#define IPMI_FRU_AREA_CHASSIS_INFO 0x01
#define IPMI_FRU_AREA_BOARD_INFO 0x02
#define IPMI_FRU_AREA_PRODUCT_INFO 0x03
#define IPMI_FRU_AREA_MULTI_RECORD 0x04
#define IPMI_FRU_AREA_TYPE_MAX 0x05

#define OPENBMC_VPD_KEY_LEN 64
#define OPENBMC_VPD_VAL_LEN 512

struct ipmi_fru_field
{
    uint8_t type_length_field[IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX];
    /* store length of data stored in buffer */
    unsigned int type_length_field_length;
};

typedef struct ipmi_fru_field ipmi_fru_field_t;
/*
 * FRU Parser
 */

typedef struct ipmi_fru_area_info
{
    uint8_t off;
    uint8_t len;
} ipmi_fru_area_info_t;

typedef struct ipmi_fru_common_hdr
{
    uint8_t fmtver;
    uint8_t internal;
    uint8_t chassis;
    uint8_t board;
    uint8_t product;
    uint8_t multirec;
} __attribute__((packed)) ipmi_fru_common_hdr_t;

const char* vpd_key_names[] = {
    "Key Names Table Start",
    "Type",           /*OPENBMC_VPD_KEY_CHASSIS_TYPE*/
    "Part Number",    /*OPENBMC_VPD_KEY_CHASSIS_PART_NUM,*/
    "Serial Number",  /*OPENBMC_VPD_KEY_CHASSIS_SERIAL_NUM,*/
    "Custom Field 1", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM1,*/
    "Custom Field 2", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM2,*/
    "Custom Field 3", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM3,*/
    "Custom Field 4", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM4,*/
    "Custom Field 5", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM5,*/
    "Custom Field 6", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM6,*/
    "Custom Field 7", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM7,*/
    "Custom Field 8", /*OPENBMC_VPD_KEY_CHASSIS_CUSTOM8,*/

    "Mfg Date",
    /* OPENBMC_VPD_KEY_BOARD_MFG_DATE, */ /* not a type/len */
    "Manufacturer",                       /* OPENBMC_VPD_KEY_BOARD_MFR, */
    "Name",                               /* OPENBMC_VPD_KEY_BOARD_NAME, */
    "Serial Number",  /* OPENBMC_VPD_KEY_BOARD_SERIAL_NUM, */
    "Part Number",    /* OPENBMC_VPD_KEY_BOARD_PART_NUM, */
    "FRU File ID",    /* OPENBMC_VPD_KEY_BOARD_FRU_FILE_ID, */
    "Custom Field 1", /*OPENBMC_VPD_KEY_BOARD_CUSTOM1,*/
    "Custom Field 2", /*OPENBMC_VPD_KEY_BOARD_CUSTOM2,*/
    "Custom Field 3", /*OPENBMC_VPD_KEY_BOARD_CUSTOM3,*/
    "Custom Field 4", /*OPENBMC_VPD_KEY_BOARD_CUSTOM4,*/
    "Custom Field 5", /*OPENBMC_VPD_KEY_BOARD_CUSTOM5,*/
    "Custom Field 6", /*OPENBMC_VPD_KEY_BOARD_CUSTOM6,*/
    "Custom Field 7", /*OPENBMC_VPD_KEY_BOARD_CUSTOM7,*/
    "Custom Field 8", /*OPENBMC_VPD_KEY_BOARD_CUSTOM8,*/

    "Manufacturer",   /* OPENBMC_VPD_KEY_PRODUCT_MFR, */
    "Name",           /* OPENBMC_VPD_KEY_PRODUCT_NAME, */
    "Model Number",   /* OPENBMC_VPD_KEY_PRODUCT_PART_MODEL_NUM, */
    "Version",        /* OPENBMC_VPD_KEY_PRODUCT_VER, */
    "Serial Number",  /* OPENBMC_VPD_KEY_PRODUCT_SERIAL_NUM, */
    "Asset Tag",      /* OPENBMC_VPD_KEY_PRODUCT_ASSET_TAG, */
    "FRU File ID",    /* OPENBMC_VPD_KEY_PRODUCT_FRU_FILE_ID, */
    "Custom Field 1", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM1,*/
    "Custom Field 2", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM2,*/
    "Custom Field 3", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM3,*/
    "Custom Field 4", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM4,*/
    "Custom Field 5", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM5,*/
    "Custom Field 6", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM6,*/
    "Custom Field 7", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM7,*/
    "Custom Field 8", /*OPENBMC_VPD_KEY_PRODUCT_CUSTOM8,*/

    "Key Names Table End" /*OPENBMC_VPD_KEY_MAX,*/
};

/*
 * --------------------------------------------------------------------
 *
 * --------------------------------------------------------------------
 */

static size_t _to_time_str(uint32_t mfg_date_time, char* timestr, uint32_t len)
{
    struct tm tm;
    time_t t;
    size_t s;

    ASSERT(timestr);
    ASSERT(len);

    memset(&tm, '\0', sizeof(struct tm));

    t = mfg_date_time;
    gmtime_r(&t, &tm);
    s = strftime(timestr, len, "%F - %H:%M:%S", &tm);

    return s;
}

/* private method to parse type/length */
static int _parse_type_length(const void* areabuf, unsigned int areabuflen,
                              unsigned int current_area_offset,
                              uint8_t* number_of_data_bytes,
                              ipmi_fru_field_t* field)
{
    const uint8_t* areabufptr = (const uint8_t*)areabuf;
    uint8_t type_length;
    uint8_t type_code;

    ASSERT(areabuf);
    ASSERT(areabuflen);
    ASSERT(number_of_data_bytes);

    type_length = areabufptr[current_area_offset];

    /* ipmi workaround
     *
     * dell p weredge r610
     *
     * my reading of the fru spec is that all non-custom fields are
     * required to be listed by the vendor.  however, on this
     * motherboard, some areas list this, indicating that there is
     * no more data to be parsed.  so now, for "required" fields, i
     * check to see if the type-length field is a sentinel before
     * calling this function.
     */

    ASSERT(type_length != IPMI_FRU_SENTINEL_VALUE);

    type_code = (type_length & IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK) >>
                IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT;
    (*number_of_data_bytes) =
        type_length & IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK;

    /* special case: this shouldn't be a length of 0x01 (see type/length
     * byte format in fru information storage definition).
     */
    if (type_code == IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE &&
        (*number_of_data_bytes) == 0x01)
    {
        return (-1);
    }

    if ((current_area_offset + 1 + (*number_of_data_bytes)) > areabuflen)
    {
        return (-1);
    }

    if (field)
    {
        memset(field->type_length_field, '\0',
               IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX);
        memcpy(field->type_length_field, &areabufptr[current_area_offset],
               1 + (*number_of_data_bytes));
        field->type_length_field_length = 1 + (*number_of_data_bytes);
    }

    return (0);
}

int ipmi_fru_chassis_info_area(const void* areabuf, unsigned int areabuflen,
                               uint8_t* chassis_type,
                               ipmi_fru_field_t* chassis_part_number,
                               ipmi_fru_field_t* chassis_serial_number,
                               ipmi_fru_field_t* chassis_custom_fields,
                               unsigned int chassis_custom_fields_len)
{
    const uint8_t* areabufptr = (const uint8_t*)areabuf;
    unsigned int area_offset = 0;
    unsigned int custom_fields_index = 0;
    uint8_t number_of_data_bytes;
    int rv = -1;

    if (!areabuf || !areabuflen)
    {
        return (-1);
    }

    if (chassis_part_number)
        memset(chassis_part_number, '\0', sizeof(ipmi_fru_field_t));
    if (chassis_serial_number)
        memset(chassis_serial_number, '\0', sizeof(ipmi_fru_field_t));
    if (chassis_custom_fields && chassis_custom_fields_len)
        memset(chassis_custom_fields, '\0',
               sizeof(ipmi_fru_field_t) * chassis_custom_fields_len);

    if (chassis_type)
        (*chassis_type) = areabufptr[area_offset];
    area_offset++;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, chassis_part_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, chassis_serial_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    while (area_offset < areabuflen &&
           areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
        ipmi_fru_field_t* field_ptr = NULL;

        if (chassis_custom_fields && chassis_custom_fields_len)
        {
            if (custom_fields_index < chassis_custom_fields_len)
                field_ptr = &chassis_custom_fields[custom_fields_index];
            else
            {
                goto cleanup;
            }
        }

        if (_parse_type_length(areabufptr, areabuflen, area_offset,
                               &number_of_data_bytes, field_ptr) < 0)
            goto cleanup;

        area_offset += 1; /* type/length byte */
        area_offset += number_of_data_bytes;
        custom_fields_index++;
    }

out:
    rv = 0;
cleanup:
    return (rv);
}

int ipmi_fru_board_info_area(
    const void* areabuf, unsigned int areabuflen, uint8_t* language_code,
    uint32_t* mfg_date_time, ipmi_fru_field_t* board_manufacturer,
    ipmi_fru_field_t* board_product_name, ipmi_fru_field_t* board_serial_number,
    ipmi_fru_field_t* board_part_number, ipmi_fru_field_t* board_fru_file_id,
    ipmi_fru_field_t* board_custom_fields, unsigned int board_custom_fields_len)
{
    const uint8_t* areabufptr = (const uint8_t*)areabuf;
    uint32_t mfg_date_time_tmp = 0;
    unsigned int area_offset = 0;
    unsigned int custom_fields_index = 0;
    uint8_t number_of_data_bytes;
    int rv = -1;

    if (!areabuf || !areabuflen)
    {
        return (-1);
    }

    if (board_manufacturer)
        memset(board_manufacturer, '\0', sizeof(ipmi_fru_field_t));
    if (board_product_name)
        memset(board_product_name, '\0', sizeof(ipmi_fru_field_t));
    if (board_serial_number)
        memset(board_serial_number, '\0', sizeof(ipmi_fru_field_t));
    if (board_part_number)
        memset(board_part_number, '\0', sizeof(ipmi_fru_field_t));
    if (board_fru_file_id)
        memset(board_fru_file_id, '\0', sizeof(ipmi_fru_field_t));
    if (board_custom_fields && board_custom_fields_len)
        memset(board_custom_fields, '\0',
               sizeof(ipmi_fru_field_t) * board_custom_fields_len);

    if (language_code)
        (*language_code) = areabufptr[area_offset];
    area_offset++;

    if (mfg_date_time)
    {
        struct tm tm;
        time_t t;

        /* mfg_date_time is little endian - see spec */
        mfg_date_time_tmp |= areabufptr[area_offset];
        area_offset++;
        mfg_date_time_tmp |= (areabufptr[area_offset] << 8);
        area_offset++;
        mfg_date_time_tmp |= (areabufptr[area_offset] << 16);
        area_offset++;

        /* mfg_date_time is in minutes, so multiple by 60 to get seconds */
        mfg_date_time_tmp *= 60;

        /* posix says individual calls need not clear/set all portions of
         * 'struct tm', thus passing 'struct tm' between functions could
         * have issues.  so we need to memset.
         */
        memset(&tm, '\0', sizeof(struct tm));

        /* in fru, epoch is 0:00 hrs 1/1/96
         *
         * so convert into ansi epoch
         */

        tm.tm_year = 96; /* years since 1900 */
        tm.tm_mon = 0;   /* months since january */
        tm.tm_mday = 1;  /* 1-31 */
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        tm.tm_isdst = -1;

        if ((t = mktime(&tm)) == (time_t)-1)
        {
            goto cleanup;
        }

        mfg_date_time_tmp += (uint32_t)t;
        (*mfg_date_time) = mfg_date_time_tmp;
    }
    else
        area_offset += 3;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, board_manufacturer) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, board_product_name) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, board_serial_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, board_part_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, board_fru_file_id) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    while (area_offset < areabuflen &&
           areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
        ipmi_fru_field_t* field_ptr = NULL;

        if (board_custom_fields && board_custom_fields_len)
        {
            if (custom_fields_index < board_custom_fields_len)
                field_ptr = &board_custom_fields[custom_fields_index];
            else
            {
                goto cleanup;
            }
        }

        if (_parse_type_length(areabufptr, areabuflen, area_offset,
                               &number_of_data_bytes, field_ptr) < 0)
            goto cleanup;

        area_offset += 1; /* type/length byte */
        area_offset += number_of_data_bytes;
        custom_fields_index++;
    }

out:
    rv = 0;
cleanup:
    return (rv);
}

int ipmi_fru_product_info_area(
    const void* areabuf, unsigned int areabuflen, uint8_t* language_code,
    ipmi_fru_field_t* product_manufacturer_name, ipmi_fru_field_t* product_name,
    ipmi_fru_field_t* product_part_model_number,
    ipmi_fru_field_t* product_version, ipmi_fru_field_t* product_serial_number,
    ipmi_fru_field_t* product_asset_tag, ipmi_fru_field_t* product_fru_file_id,
    ipmi_fru_field_t* product_custom_fields,
    unsigned int product_custom_fields_len)
{
    const uint8_t* areabufptr = (const uint8_t*)areabuf;
    unsigned int area_offset = 0;
    unsigned int custom_fields_index = 0;
    uint8_t number_of_data_bytes;
    int rv = -1;

    if (!areabuf || !areabuflen)
    {
        return (-1);
    }

    if (product_manufacturer_name)
        memset(product_manufacturer_name, '\0', sizeof(ipmi_fru_field_t));
    if (product_name)
        memset(product_name, '\0', sizeof(ipmi_fru_field_t));
    if (product_part_model_number)
        memset(product_part_model_number, '\0', sizeof(ipmi_fru_field_t));
    if (product_version)
        memset(product_version, '\0', sizeof(ipmi_fru_field_t));
    if (product_serial_number)
        memset(product_serial_number, '\0', sizeof(ipmi_fru_field_t));
    if (product_asset_tag)
        memset(product_asset_tag, '\0', sizeof(ipmi_fru_field_t));
    if (product_fru_file_id)
        memset(product_fru_file_id, '\0', sizeof(ipmi_fru_field_t));
    if (product_custom_fields && product_custom_fields_len)
        memset(product_custom_fields, '\0',
               sizeof(ipmi_fru_field_t) * product_custom_fields_len);

    if (language_code)
        (*language_code) = areabufptr[area_offset];
    area_offset++;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes,
                           product_manufacturer_name) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, product_name) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes,
                           product_part_model_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, product_version) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, product_serial_number) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, product_asset_tag) < 0)
        goto cleanup;
    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
        goto out;

    if (_parse_type_length(areabufptr, areabuflen, area_offset,
                           &number_of_data_bytes, product_fru_file_id) < 0)
        goto cleanup;

    area_offset += 1; /* type/length byte */
    area_offset += number_of_data_bytes;

    while (area_offset < areabuflen &&
           areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
        ipmi_fru_field_t* field_ptr = NULL;

        if (product_custom_fields && product_custom_fields_len)
        {
            if (custom_fields_index < product_custom_fields_len)
                field_ptr = &product_custom_fields[custom_fields_index];
            else
            {
                goto cleanup;
            }
        }

        if (_parse_type_length(areabufptr, areabuflen, area_offset,
                               &number_of_data_bytes, field_ptr) < 0)
            goto cleanup;

        area_offset += 1; /* type/length byte */
        area_offset += number_of_data_bytes;
        custom_fields_index++;
    }

out:
    rv = 0;
cleanup:
    return (rv);
}

void _append_to_dict(uint8_t vpd_key_id, uint8_t* vpd_key_val,
                     IPMIFruInfo& info)
{
    int type_length = vpd_key_val[0];
    int type_code = (type_length & IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK) >>
                    IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT;
    int vpd_val_len =
        type_length & IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK;

    /* Needed to convert each uint8_t byte to a ascii */
    char bin_byte[3] = {0};

    /*
     * Max number of characters needed to represent 1 unsigned byte in string
     * is number of bytes multiplied by 2. Extra 3 for 0x and a ending '\0';
     */
    char bin_in_ascii_len = vpd_val_len * 2 + 3;

    /* Binary converted to ascii in array */
    char* bin_in_ascii = (char*)malloc(bin_in_ascii_len);

    /* For reading byte from the area */
    int val = 0;

    char* bin_copy = &((char*)bin_in_ascii)[2];

    switch (type_code)
    {
        case 0:
            memset(bin_in_ascii, 0x0, bin_in_ascii_len);

            /* Offset 1 is where actual data starts */
            for (val = 1; val <= vpd_val_len; val++)
            {
                /* 2 bytes for data and 1 for terminating '\0' */
                snprintf(bin_byte, 3, "%02x", vpd_key_val[val]);

                /* Its a running string so strip off the '\0' */
                strncat(bin_copy, bin_byte, 2);
            }

            /* We need the data represented as 0x...... */
            if (vpd_val_len > 0)
            {
                memcpy(bin_in_ascii, "0x", 2);
            }
#if IPMI_FRU_PARSER_DEBUG
            printf("_append_to_dict: VPD Key = [%s] : Type Code = [BINARY] :"
                   " Len = [%d] : Val = [%s]\n",
                   vpd_key_names[vpd_key_id], vpd_val_len, bin_in_ascii);
#endif
            info[vpd_key_id] =
                std::make_pair(vpd_key_names[vpd_key_id], bin_in_ascii);
            break;

        case 3:
#if IPMI_FRU_PARSER_DEBUG
            printf("_append_to_dict: VPD Key = [%s] : Type Code=[ASCII+Latin]"
                   " : Len = [%d] : Val = [%s]\n",
                   vpd_key_names[vpd_key_id], vpd_val_len, &vpd_key_val[1]);
#endif
            info[vpd_key_id] = std::make_pair(
                vpd_key_names[vpd_key_id],
                std::string(vpd_key_val + 1, vpd_key_val + 1 + type_length));
            break;
    }

    if (bin_in_ascii)
    {
        free(bin_in_ascii);
        bin_in_ascii = NULL;
    }
}

int parse_fru_area(const uint8_t area, const void* msgbuf, const size_t len,
                   IPMIFruInfo& info)
{
    int rv = -1;
    int i = 0;

    /* Chassis */
    uint8_t chassis_type;
    /* Board */
    uint32_t mfg_date_time;
    /* Product */
    // unsigned int product_custom_fields_len;

    // ipmi_fru_area_info_t fru_area_info [ IPMI_FRU_AREA_TYPE_MAX ];
    ipmi_fru_field_t vpd_info[OPENBMC_VPD_KEY_MAX];
    char timestr[OPENBMC_VPD_VAL_LEN];

    // uint8_t* ipmi_fru_field_str=NULL;
    // ipmi_fru_common_hdr_t* chdr = NULL;
    // uint8_t* hdr = NULL;

    ASSERT(msgbuf);

    for (i = 0; i < OPENBMC_VPD_KEY_MAX; i++)
    {
        memset(vpd_info[i].type_length_field, '\0',
               IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX);
        vpd_info[i].type_length_field_length = 0;
    }

    switch (area)
    {
        case IPMI_FRU_AREA_CHASSIS_INFO:
#if IPMI_FRU_PARSER_DEBUG
            printf("Chassis : Buf len = [%d]\n", len);
#endif
            ipmi_fru_chassis_info_area(
                (uint8_t*)msgbuf + 2, len, &chassis_type,
                &vpd_info[OPENBMC_VPD_KEY_CHASSIS_PART_NUM],
                &vpd_info[OPENBMC_VPD_KEY_CHASSIS_SERIAL_NUM],
                &vpd_info[OPENBMC_VPD_KEY_CHASSIS_CUSTOM1],
                OPENBMC_VPD_KEY_CUSTOM_FIELDS_MAX);

            /* Populate VPD Table */
            for (i = 1; i <= OPENBMC_VPD_KEY_CHASSIS_MAX; i++)
            {
                if (i == OPENBMC_VPD_KEY_CHASSIS_TYPE)
                {
#if IPMI_FRU_PARSER_DEBUG
                    printf("Chassis : Appending [%s] = [%d]\n",
                           vpd_key_names[i], chassis_type);
#endif
                    info[i] = std::make_pair(vpd_key_names[i],
                                             std::to_string(chassis_type));
                    continue;
                }
                _append_to_dict(i, vpd_info[i].type_length_field, info);
            }
            break;
        case IPMI_FRU_AREA_BOARD_INFO:
#if IPMI_FRU_PARSER_DEBUG
            printf("Board : Buf len = [%d]\n", len);
#endif
            ipmi_fru_board_info_area(
                (uint8_t*)msgbuf + 2, len, NULL, &mfg_date_time,
                &vpd_info[OPENBMC_VPD_KEY_BOARD_MFR],
                &vpd_info[OPENBMC_VPD_KEY_BOARD_NAME],
                &vpd_info[OPENBMC_VPD_KEY_BOARD_SERIAL_NUM],
                &vpd_info[OPENBMC_VPD_KEY_BOARD_PART_NUM],
                &vpd_info[OPENBMC_VPD_KEY_BOARD_FRU_FILE_ID],
                &vpd_info[OPENBMC_VPD_KEY_BOARD_CUSTOM1],
                OPENBMC_VPD_KEY_CUSTOM_FIELDS_MAX);

            /* Populate VPD Table */
            for (i = OPENBMC_VPD_KEY_BOARD_MFG_DATE;
                 i <= OPENBMC_VPD_KEY_BOARD_MAX; i++)
            {
                if (i == OPENBMC_VPD_KEY_BOARD_MFG_DATE)
                {
                    _to_time_str(mfg_date_time, timestr, OPENBMC_VPD_VAL_LEN);
#if IPMI_FRU_PARSER_DEBUG
                    printf("Board : Appending [%s] = [%s]\n", vpd_key_names[i],
                           timestr);
#endif
                    info[i] =
                        std::make_pair(vpd_key_names[i], std::string(timestr));
                    continue;
                }
                _append_to_dict(i, vpd_info[i].type_length_field, info);
            }
            break;
        case IPMI_FRU_AREA_PRODUCT_INFO:
#if IPMI_FRU_PARSER_DEBUG
            printf("Product : Buf len = [%d]\n", len);
#endif
            ipmi_fru_product_info_area(
                (uint8_t*)msgbuf + 2, len, NULL,
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_MFR],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_NAME],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_PART_MODEL_NUM],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_VER],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_SERIAL_NUM],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_ASSET_TAG],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_FRU_FILE_ID],
                &vpd_info[OPENBMC_VPD_KEY_PRODUCT_CUSTOM1],
                OPENBMC_VPD_KEY_CUSTOM_FIELDS_MAX);

            for (i = OPENBMC_VPD_KEY_PRODUCT_MFR;
                 i <= OPENBMC_VPD_KEY_PRODUCT_MAX; ++i)
            {
                _append_to_dict(i, vpd_info[i].type_length_field, info);
            }
            break;
        default:
            /* TODO: Parse Multi Rec / Internal use area */
            break;
    }

#if IPMI_FRU_PARSER_DEBUG
    printf("parse_fru_area : Dictionary Packing Complete\n");
#endif
    rv = 0;
    return (rv);
}
