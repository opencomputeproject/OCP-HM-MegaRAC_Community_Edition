/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2010-2011, American Megatrends Inc.        **
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
 * snoopifc.h
 * 
 * A simple library which expose the function to access the snoop driver.
 *
 * Author: Gokulakannan. S <gokulakannans@amiindia.co.in>
 *****************************************************************/

#ifndef SNOOPIFC_H
#define SNOOPIFC_H


/*
 * @fn ReadCurrentBiosCode
 * @brief Function to read the last nCount bytes of current 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
extern int ReadCurrentBiosCode (unsigned char *pBiosCode, int nCount);


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
extern int ReadCurrentBiosCodeByCh (unsigned char *pBiosCode, int nCount, int channel_num);


/*
 * @fn ReadPreviousBiosCode
 * @brief Function to read the last nCount bytes of previous 
 * 			bios code from the snoop driver.
 * @param[out] pBiosCode - pointer to get the bios code.
 * @param[in] nCount 	 - Count for the number of bytes to Read.
 * @retval      number of bytes read, on success,
 *              -1, if failed.
 */
extern int ReadPreviousBiosCode (unsigned char *pBiosCode, int nCount);


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
extern int ReadPreviousBiosCodeByCh (unsigned char *pBiosCode, int nCount, int channel_num);

/*
 * @fn EnableSnoopIRQ
 * @brief Function to enable snoop interrupt
 * @param[in]  channel_num- the target channel number
 * @retval      0, on success,
 *              -1, if failed.
 */
extern int EnableSnoopIRQ (int channel_num);

/*
 * @fn DisableSnoopIRQ
 * @brief Function to disable snoop interrupt
 * @param[in]  channel_num- the target channel number
 * @retval      0, on success,
 *              -1, if failed.
 */
extern int DisableSnoopIRQ (int channel_num);

#endif
