/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2017-2018, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy, Building 200, Norcross,         **
 **                                                            **
 **        Georgia 30093, USA. Phone-(770)-246-8600.           **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * crc8.c
 * Function which calculates CRC-8 Polynomial.
 * 
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "crc8.h"
#include "checksum.h"

#define CRC8_INIT_VALUE		0x0000
#define CRC8_XOR_VALUE		0x0000


/* Local Functions */

static void CRC8_InitChecksum( unsigned char *crcvalue ) 
{
    (*crcvalue) = CRC8_INIT_VALUE;
}

static void CRC8_UpdateChecksum( unsigned char *crcvalue, const void *data, int length )
{
    unsigned char crc;
    const unsigned char *buf = (const unsigned char *) data;

    crc = (*crcvalue);
    while( length-- ) 
    {
        crc = crctable[crc ^ *buf++];
    }
    (*crcvalue) = crc;
}

static void CRC8_FinishChecksum( unsigned char *crcvalue ) 
{
    (*crcvalue) ^= CRC8_XOR_VALUE;
}

/* Exposed functions */

/**
 * @fn CalculateCRC8
 * @brief Calculate the CRC-8 polynomial for the given data.
 * @param[in] data - Input data to calculate CRC8.
 * @param[in] length - Length of data.
 * @retval      Calculated CRC8 value.
 */

unsigned char CalculateCRC8( const void *data, int length )
{
    unsigned char crc;

    CRC8_InitChecksum( &crc );
    CRC8_UpdateChecksum( &crc, data, length );
    CRC8_FinishChecksum( &crc );
    
    return crc;
}
