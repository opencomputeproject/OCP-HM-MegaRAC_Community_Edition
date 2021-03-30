#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include "snoop_ioctl.h"

#define MAX_CODE_SIZE	2048

/*
 * @fn ReadBiosCodes
 * @brief Function to read the last nCount bytes of specified 
 * 			bios code from the snoop driver.
 * @param[in]  nCode	 - Flag to read current or previous.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in]  nCount 	 - Count for the number of bytes to Read.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
static int
ReadBiosCodes(int nCode, unsigned char *pBiosCode, int nCount, int channel_num)
{
	int fd, nLen = 0, nRet = -1;
	unsigned char code[MAX_CODE_SIZE];
    char devfile[32];

	//open the snoop driver
    snprintf( devfile, 32, "/dev/snoop%d", channel_num );
	fd = open( devfile, O_RDWR );
	if(fd == -1)
	{
		printf("opening snoop device failed tooohhoo \n");
		return nRet;
	}
	
	nLen = ioctl(fd, (unsigned long)nCode, (unsigned long) code );

	/* Check whether the driver returned a INT number*/
	if (nLen == -1)
	{
		close(fd);
		return nRet;
	}
	
	if (nCount < nLen)
	{
		// Given count is lesser than the available codes,
		// so extract only the last nCount bytes from code.
		memcpy (pBiosCode, code + (nLen - nCount), nCount);
		nRet = nCount;
	}
	else
	{
		// Given count is larger, so copy only nLen bytes  
		memcpy (pBiosCode, code, nLen);
		nRet = nLen;
	}
	
#if 0
    printf("Bios Codes :\n");

    for(i=0;i<(nRet);i++)
    {
        printf("%02X ", pBiosCode[i]);
        if ((i>0) && (((i+1)%16)==0))
            printf("\n");
    }
    printf("\n");
#endif

    close(fd);
    return nRet;
}


/*
 * @fn ReadCurrentBiosCode
 * @brief Function to read the last nCount bytes of current 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
int ReadCurrentBiosCode (unsigned char *pBiosCode, int nCount)
{
	return ReadBiosCodes(READ_CURRENT_CODES, pBiosCode, nCount, 0);
}

/*
 * @fn ReadCurrentBiosCodeByCh
 * @brief Function to read the last nCount bytes of current 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @param[in] channel_num- the target channel number.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
int ReadCurrentBiosCodeByCh (unsigned char *pBiosCode, int nCount, int channel_num)
{
	return ReadBiosCodes(READ_CURRENT_CODES, pBiosCode, nCount, channel_num);
}


/*
 * @fn ReadPreviousBiosCode
 * @brief Function to read the last nCount bytes of previous 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
int ReadPreviousBiosCode (unsigned char *pBiosCode, int nCount)
{
	return ReadBiosCodes(READ_PREVIOUS_CODES, pBiosCode, nCount, 0);
}


/*
 * @fn ReadPreviousBiosCodeByCh
 * @brief Function to read the last nCount bytes of previous 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @param[in] channel_num- the target channel number.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
int ReadPreviousBiosCodeByCh (unsigned char *pBiosCode, int nCount, int channel_num)
{
	return ReadBiosCodes(READ_PREVIOUS_CODES, pBiosCode, nCount, channel_num);
}

