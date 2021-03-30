/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2017-2018, American Megatrends Inc.    ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
*
* peciifc.c
*
* Simple interface for PECI
*
******************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "peciifc.h"
#include "checksum.h"

/** \file peciifc.c
    \brief Source for all PECI functionality
*/

#define PECI_GET_DIB        	0xf7
#define PECI_GET_TEMP       	0x00
#define PECI_RDPKG          	0xa0
#define PECI_WRPKG          	0xa4
#define PECI_RDIAMSR        	0xb0
#define PECI_WRIAMSR        	0xb4
#define PECI_RDPCICONFIG	0x60
#define PECI_WRPCICONFIG	0x64
#define PECI_MBX_SEND		0xd0	// NOT IMPLEMENTED
#define PECI_MBX_GET		0xd4	// NOT IMPLEMENTED
#define PECI_RDPCICONFIGLOCAL	0xe0	// NOT IMPLEMENTED
#define PECI_WRPCICONFIGLOCAL	0xe4	// NOT IMPLEMENTED

#define PECI_AWFCS_EN		1<<0

#if defined (DEBUG)
#define dbgprint	printf
#else
#define dbgprint(fmt,arg...)
#endif

/**
 * issue_peci_cmd
 *
 **/
static int issue_peci_cmd(peci_cmd_t *peci_cmd, unsigned long PECI_ISSUE_CMD)
{
    int fd;
    int ioctl_ret;
    int retval = 0;
#if defined (DEBUG)
    int i;
#endif
    char peci_devname[16];
    
    if(!peci_cmd)
    {
        dbgprint ("issue_peci_cmd - Invalid Input\n");
        return -1;
    }

    memset(peci_devname,0,16);
    snprintf(peci_devname, 16, "%s%d", PECI_CTL_FILE, peci_cmd->dev_id);

	/* Open the peci device file */
    fd = open(peci_devname, O_RDWR );
    if( fd == -1 )
	{
		dbgprint ("Opening %s device failed\n",peci_devname);
        retval = -1;
	}
    else
    {
#if defined (DEBUG)
        printf ("\nWrite_buffer : ");
        for (i = 0; i < peci_cmd->write_len; i++)
        {
            printf ("%02x ", peci_cmd->write_buffer[i]);
        }
        printf ("\n");
#endif
        
        ioctl_ret = ioctl(fd, (unsigned long)PECI_ISSUE_CMD, peci_cmd);
        if( ioctl_ret == -1 ) {
	    dbgprint ("Error in the ioctl\n");
            retval = -1;
	}
    }
    (void)close(fd);

	if(peci_cmd->status < 0) {
		dbgprint ("peci cmd status != 0. Error\n");
		retval = -1;
	}

    return(retval);
}

static unsigned char calculate_fcs(peci_cmd_t *peci_cmd)
{
    unsigned char tbuf[PECI_DATA_BUF_SIZE+3];

    tbuf[0] = peci_cmd->target;
    tbuf[1] = peci_cmd->write_len;
    tbuf[2] = peci_cmd->read_len;
    memcpy(&tbuf[3], peci_cmd->write_buffer, peci_cmd->write_len - 1);

    return CalculateCRC8(tbuf, peci_cmd->write_len+2);
}


void peci_cmd_enable_awfcs(peci_cmd_t* peci_cmd)
{
    if(!peci_cmd)
    {
        dbgprint ("peci_cmd_enable_awfcs - Invalid Input\n");
        return;
    }
	peci_cmd->AWFCS = PECI_AWFCS_EN;
}

/**
 * @fn peci_cmd_ping
 * @brief Pings the given Target PECI Client
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 */ 
int peci_cmd_ping(char dev_id, char target)
{
	peci_cmd_t	peci_cmd;

	memset(&peci_cmd, 0, sizeof(peci_cmd_t));

	peci_cmd.dev_id = dev_id;
	peci_cmd.target = target;

	if(issue_peci_cmd(&peci_cmd,PECI_CMD_PING) != 0)
	{
		dbgprint ("Issuing PECI command failed\n");
		return -1;
	}

	return 0;
}

/**
 * @fn peci_cmd_read_temp
 * @brief Get the client CPU temperature with reference to Tcc
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[out] res - pointer to the output buffer.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_read_temp(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, char *read_buffer)
{
	int i;
	peci_cmd_t	peci_cmd;

	if(!read_buffer)
	{
		dbgprint ("ReadTemp - Invalid Input\n");
		return -1;
	}

	memset(&peci_cmd, 0, sizeof(peci_cmd_t));

	peci_cmd.dev_id = dev_id;
	peci_cmd.target = target;
	peci_cmd.read_len = 2;
	peci_cmd.write_len = 1;
	peci_cmd.domain = domain;
	peci_cmd.Xmit_Feedback = Xmit_Feedback;
	peci_cmd.AWFCS = awfcs;
	peci_cmd.write_buffer[0]= PECI_GET_TEMP | (0x01 << peci_cmd.domain);

	if(issue_peci_cmd(&peci_cmd,PECI_CMD_GET_TEMP) != 0)
	{
		dbgprint ("Issuing PECI command failed\n");
		return -1;
	}

	*read_len = (int)peci_cmd.read_len;	
	for(i=0;i<peci_cmd.read_len;i++)
	{
		read_buffer[i]=peci_cmd.read_buffer[i];
	}

	return 0;
}

/**
 * @fn peci_cmd_get_dib
 * @brief Get the client revision number and number of supported domains.
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[out] res - pointer to the output buffer.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_get_dib(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *pecigetdibres)
{
    int i;
    peci_cmd_t	peci_cmd;

    if(!pecigetdibres)
    {
        dbgprint ("GetDIB - Invalid Input\n");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = 8;
    peci_cmd.write_len = 1;
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;
    peci_cmd.write_buffer[0]= PECI_GET_DIB;

    if(issue_peci_cmd(&peci_cmd,PECI_CMD_GET_DIB) != 0)
    {
        dbgprint ("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        pecigetdibres[i]=peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_rdpkgconfig
 * @brief Provides Read access to the package configuration space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_rdpkgconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *pecirdpkgreq, unsigned char *pecirdpkgres)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_rdpkgconfig_req_t *pecirdpkgcfgreq = (peci_rdpkgconfig_req_t *)pecirdpkgreq;

    if(!pecirdpkgreq || !pecirdpkgres)
    {
        dbgprint ("RdPkgConfig - Invalid Input\n");
        return -1;
    }
    else if (pecirdpkgcfgreq->option != BYTE && pecirdpkgcfgreq->option != WORD && pecirdpkgcfgreq->option != DWORD)
    {
        dbgprint ("RdPkgConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = pecirdpkgcfgreq->option + 1; // Add 1 for completion code.
    peci_cmd.write_len = 5;
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;
    
    peci_cmd.write_buffer[0] = PECI_RDPKG | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = pecirdpkgcfgreq->host_id;
    peci_cmd.write_buffer[2] = pecirdpkgcfgreq->index;
    peci_cmd.write_buffer[3] = (pecirdpkgcfgreq->parameter & 0x00ff);
    peci_cmd.write_buffer[4] = (pecirdpkgcfgreq->parameter & 0xff00) >> 8;
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_RD_PKG_CFG) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        pecirdpkgres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_wrpkgconfig
 * @brief Provides Write access to the package configuration space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_wrpkgconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpkgreq, unsigned char *peciwrpkgres)
{
    int i;
    peci_cmd_t	peci_cmd;

   peci_wrpkgconfig_req_t *peciwrpkgcfgreq = (peci_wrpkgconfig_req_t *)peciwrpkgreq;

    if(!peciwrpkgreq|| !peciwrpkgres)
    {
        dbgprint("WrPkgConfig - Invalid Input\n");
        return -1;
    }
    else if (peciwrpkgcfgreq->option != BYTE && peciwrpkgcfgreq->option != WORD && peciwrpkgcfgreq->option != DWORD)
    {
        dbgprint ("WrPkgConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = 1; // Completion Code
    peci_cmd.write_len = peciwrpkgcfgreq->option + 6;  // 5 [1:cmd code] [1:host_id] 
                                          // [1:index] [2:parameter] [1:AW FCS]
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs; 
    peci_cmd.write_buffer[0] = PECI_WRPKG | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = peciwrpkgcfgreq->host_id;
    peci_cmd.write_buffer[2] = peciwrpkgcfgreq->index;
    peci_cmd.write_buffer[3] = (peciwrpkgcfgreq->parameter & 0x00ff);
    peci_cmd.write_buffer[4] = (peciwrpkgcfgreq->parameter & 0xff00) >> 8;
    
    for (i = 0; i < peciwrpkgcfgreq->option; i++)
    {
        peci_cmd.write_buffer[i + 5] = ((unsigned char *)(&peciwrpkgcfgreq->data))[i];
    }

    // FCS is calculated and invert the MSB to get AW FCS. 
    peci_cmd.write_buffer[peci_cmd.write_len - 1] = calculate_fcs(&peci_cmd) ^ 0x80;

    *read_len = (int)peci_cmd.read_len;	
    if (issue_peci_cmd(&peci_cmd,PECI_CMD_WR_PKG_CFG) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    for(i=0;i<peci_cmd.read_len;i++)
    {
        peciwrpkgres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_rdiamsr
 * @brief Provides Read access to the Model specific Registers
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_rdiamsr(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *pecirdiamsrreq, unsigned char *pecirdiamsrres)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_rdiamsr_req_t *pecirdiamsrcfgreq = (peci_rdiamsr_req_t *)pecirdiamsrreq;
    
    if(!pecirdiamsrreq || !pecirdiamsrres)
    {
        dbgprint ("RdIAmsr - Invalid Input\n");
        return -1;
    }
    else if (pecirdiamsrcfgreq->option != BYTE && pecirdiamsrcfgreq->option != WORD && 
             pecirdiamsrcfgreq->option != DWORD && pecirdiamsrcfgreq->option != QWORD)
    {
        dbgprint ("RdIAmsr - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = pecirdiamsrcfgreq->option + 1; // Add 1 for completion code.
    peci_cmd.write_len = 5;
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;
    
    peci_cmd.write_buffer[0] = PECI_RDIAMSR | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = pecirdiamsrcfgreq->host_id;
    peci_cmd.write_buffer[2] = pecirdiamsrcfgreq->processor_id;
    peci_cmd.write_buffer[3] = (pecirdiamsrcfgreq->msr_addr & 0x00ff);
    peci_cmd.write_buffer[4] = (pecirdiamsrcfgreq->msr_addr & 0xff00) >> 8;
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_RD_IA_MSR) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        pecirdiamsrres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_wriamsr
 * @brief Provides Write access to the IA-MSR space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_wriamsr(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwriamsrreq, unsigned char *peciwriamsrres)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_wriamsr_req_t *peciwriamsrcfgreq  = (peci_wriamsr_req_t *)peciwriamsrreq;
    
    if(!peciwriamsrreq || !peciwriamsrres)
    {
        dbgprint("WrIaMsr - Invalid Input\n");
        return -1;
    }
    else if (peciwriamsrcfgreq->option != BYTE && peciwriamsrcfgreq->option != WORD && peciwriamsrcfgreq->option != DWORD)
    {
        dbgprint ("WrIaMsr - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = 1; // Completion Code
    peci_cmd.write_len = peciwriamsrcfgreq->option + 6;  // 5 [1:cmd code] [1:host_id] 
                                          	// [1:index] [2:parameter] [1:AW FCS]
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;

    peci_cmd.write_buffer[0] = PECI_WRIAMSR | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = peciwriamsrcfgreq->host_id;
    peci_cmd.write_buffer[2] = peciwriamsrcfgreq->processor_id;
    peci_cmd.write_buffer[3] = (peciwriamsrcfgreq->msr_addr & 0x00ff);
    peci_cmd.write_buffer[4] = (peciwriamsrcfgreq->msr_addr & 0xff00) >> 8;
    
    for (i = 0; i < peciwriamsrcfgreq->option; i++)
    {
        peci_cmd.write_buffer[i + 5] = ((unsigned char *)(&peciwriamsrcfgreq->data))[i];
    }

    // FCS is calculated and invert the MSB to get AW FCS. 
    peci_cmd.write_buffer[peci_cmd.write_len - 1] = calculate_fcs(&peci_cmd) ^ 0x80;
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_WR_IA_MSR) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        peciwriamsrres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_rdpciconfig
 * @brief Provides Read access to the PCI Configuration Space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - Pointer to the input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_rdpciconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *pecirdpcireq, unsigned char *pecirdpcires)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_rdpciconfig_req_t *pecirdpcicfgreq = (peci_rdpciconfig_req_t *)pecirdpcireq;
    
    if(!pecirdpcireq || !pecirdpcires)
    {
        dbgprint ("RdPciConfig - Invalid Input\n");
        return -1;
    }
    else if (pecirdpcicfgreq->option != BYTE && pecirdpcicfgreq->option != WORD && 
             pecirdpcicfgreq->option != DWORD && pecirdpcicfgreq->option != QWORD)
    {
        dbgprint ("RdPciConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = pecirdpcicfgreq->option + 1; // Add 1 for completion code.
    peci_cmd.write_len = 6;

    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;

    peci_cmd.write_buffer[0] = PECI_RDPCICONFIG | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = pecirdpcicfgreq->host_id;
    peci_cmd.write_buffer[2] = (pecirdpcicfgreq->pci_config_addr & 0x00ff);		// Register[11:0]
    peci_cmd.write_buffer[3] = (pecirdpcicfgreq->pci_config_addr >> 8 ) & 0x00ff;	// Function[14:12]
    peci_cmd.write_buffer[4] = (pecirdpcicfgreq->pci_config_addr >> 16) & 0x00ff;	// Device[19:15]
    peci_cmd.write_buffer[5] = (pecirdpcicfgreq->pci_config_addr >> 24) & 0x00ff;	// Bus[27:20] & Reserved[31:28]
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_RD_PCI_CFG) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        pecirdpcires[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_wrpciconfig
 * @brief Provides Write access to the PCI Configuration Space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] req - Pointer to the input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_wrpciconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpcireq, unsigned char *peciwrpcires)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_wrpciconfig_req_t *peciwrpcicfgreq = (peci_wrpciconfig_req_t *)peciwrpcireq;

    if(!peciwrpcireq || !peciwrpcires)
    {
        dbgprint("WrPciConfig - Invalid Input\n");
        return -1;
    }
    else if (peciwrpcicfgreq->option != BYTE && peciwrpcicfgreq->option != WORD && peciwrpcicfgreq->option != DWORD)
    {
        dbgprint ("WrPciConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = 1; // Completion Code
    peci_cmd.write_len = peciwrpcicfgreq->option + 6;  // 5 [1:cmd code] [1:host_id] 
                                          	// [1:index] [2:parameter] [1:AW FCS]
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;

    peci_cmd.write_buffer[0] = PECI_WRPCICONFIG | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = peciwrpcicfgreq->host_id;
    peci_cmd.write_buffer[2] = (peciwrpcicfgreq->pci_config_addr & 0x00ff);		// Register[11:0]
    peci_cmd.write_buffer[3] = (peciwrpcicfgreq->pci_config_addr >> 8 ) & 0x00ff;	// Function[14:12]
    peci_cmd.write_buffer[4] = (peciwrpcicfgreq->pci_config_addr >> 16) & 0x00ff;	// Device[19:15]
    peci_cmd.write_buffer[5] = (peciwrpcicfgreq->pci_config_addr >> 24) & 0x00ff;	// Bus[27:20] & Reserved[31:28]
    
    for (i = 0; i < peciwrpcicfgreq->option; i++)
    {
        peci_cmd.write_buffer[i + 6] = ((unsigned char *)(&peciwrpcicfgreq->data))[i];
    }

    // FCS is calculated and invert the MSB to get AW FCS. 
    peci_cmd.write_buffer[peci_cmd.write_len - 1] = calculate_fcs(&peci_cmd) ^ 0x80;
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_WR_PCI_CFG) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        peciwrpcires[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}


/**
 * @fn peci_cmd_rdpciconfiglocal
 * @brief Provides Read access to Local PCI Configuration Space in the Processor
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] req - Pointer to the input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_rdpciconfiglocal(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char*peciwrpcilocalreq, unsigned char *peciwrpcilocalres)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_wrpciconfig_req_t *peciwrpcicfglocalreq = (peci_wrpciconfig_req_t *)peciwrpcilocalreq;
    
    if(!peciwrpcilocalreq || !peciwrpcilocalres)
    {
        dbgprint ("RdPciConfig - Invalid Input\n");
        return -1;
    }
    else if (peciwrpcicfglocalreq->option != BYTE && peciwrpcicfglocalreq->option != WORD && 
             peciwrpcicfglocalreq->option != DWORD && peciwrpcicfglocalreq->option != QWORD)
    {
        dbgprint ("RdPciConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = peciwrpcicfglocalreq->option + 1; // Add 1 for completion code.
    peci_cmd.write_len = 5;

    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;

    peci_cmd.write_buffer[0] = PECI_RDPCICONFIGLOCAL | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = peciwrpcicfglocalreq->host_id;
    peci_cmd.write_buffer[2] = (peciwrpcicfglocalreq->pci_config_addr & 0x00ff);		// Register[11:0]
    peci_cmd.write_buffer[3] = (peciwrpcicfglocalreq->pci_config_addr >> 8 ) & 0x00ff;	// Function[14:12]
    peci_cmd.write_buffer[4] = (peciwrpcicfglocalreq->pci_config_addr >> 16) & 0x00ff;	// Device[19:15], Bus[23:20]
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_RD_PCI_CFG_LOCAL) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        peciwrpcilocalres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_cmd_wrpciconfiglocal
 * @brief Provides Write access to the Local PCI Configuration Space in the Processor
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] req - Pointer to the input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_wrpciconfiglocal(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpcilocalreq, unsigned char *peciwrpcilocalres)
{
    int i;
    peci_cmd_t	peci_cmd;

    peci_wrpciconfig_req_t *peciwrpcicfglocalreq = (peci_wrpciconfig_req_t *)peciwrpcilocalreq;

    if(!peciwrpcilocalreq || !peciwrpcilocalres)
    {
        dbgprint("WrPciConfig - Invalid Input\n");
        return -1;
    }
    else if (peciwrpcicfglocalreq->option != BYTE && peciwrpcicfglocalreq->option != WORD && peciwrpcicfglocalreq->option != DWORD)
    {
        dbgprint ("WrPciConfig - Invalid Option");
        return -1;
    }

    memset(&peci_cmd, 0, sizeof(peci_cmd_t));

    peci_cmd.dev_id = dev_id;
    peci_cmd.target = target;
    peci_cmd.read_len = 1; // Completion Code
    peci_cmd.write_len = peciwrpcicfglocalreq->option + 6;  // 5 [1:cmd code] [1:host_id] 
                                          	// [1:index] [2:parameter] [1:AW FCS]
    peci_cmd.domain = domain;
    peci_cmd.Xmit_Feedback = Xmit_Feedback;
    peci_cmd.AWFCS = awfcs;

    peci_cmd.write_buffer[0] = PECI_WRPCICONFIGLOCAL | (0x01 << peci_cmd.domain);
    peci_cmd.write_buffer[1] = peciwrpcicfglocalreq->host_id;
    peci_cmd.write_buffer[2] = (peciwrpcicfglocalreq->pci_config_addr & 0x00ff);		// Register[11:0]
    peci_cmd.write_buffer[3] = (peciwrpcicfglocalreq->pci_config_addr >> 8 ) & 0x00ff;	// Function[14:12]
    peci_cmd.write_buffer[4] = (peciwrpcicfglocalreq->pci_config_addr >> 16) & 0x00ff;	// Device[19:15], Bus[23:20]
    
    for (i = 0; i < peciwrpcicfglocalreq->option; i++)
    {
        peci_cmd.write_buffer[i + 5] = ((unsigned char *)(&peciwrpcicfglocalreq->data))[i];
    }

    // FCS is calculated and invert the MSB to get AW FCS. 
    peci_cmd.write_buffer[peci_cmd.write_len - 1] = calculate_fcs(&peci_cmd) ^ 0x80;
    
    if(issue_peci_cmd(&peci_cmd,PECI_CMD_WR_PCI_CFG_LOCAL) != 0)
    {
        dbgprint("Issuing PECI command failed\n");
        return -1;
    }

    *read_len = (int)peci_cmd.read_len;	
    for(i=0;i<peci_cmd.read_len;i++)
    {
        peciwrpcilocalres[i] = peci_cmd.read_buffer[i];
    }

    return 0;
}

/**
 * @fn peci_generic_cmd
 * @brief Provides Generic communication interface with the PECI Client
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] domain - Domain 0 or 1 for commands supporting multi-domain feature 
 * @param[in] Xmit_Feedback - When enabled with 1, will dump the entire response including the Tx Feedback from PECI client
 *				If SOC doesn't support it, it will be Ignored, and only Data will be sent out.
 * @param[in] awfcs - When enabled with 1, will enable the Auto Write FCS (AWFCS) feature in the PECI Controller. 
 *			If SOC doesn't support it, this feature will be ignored.
 * @param[in] write_buf - Pointer to the input buffer to the PECI Client
 * @param[in] write_len - Size of data in the Input buffer being sent to PECI Client
 * @param[out] read_buf - Pointer to the output data buffer to receive from PECI Client
 * @param[out] read_len - Size to the output data received from PECI Client.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_generic_cmd(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, char *write_buf, char write_len, char *read_buf, char read_len)
{
	int i;
	peci_cmd_t peci_cmd;
	
	if(!read_buf)
	{
		dbgprint ("ERR - Read buffer address not valid\n");
		return -1;
	}
	if(write_len)
	{
		if(!write_buf)
		{
			dbgprint ("ERR - Write buffer address not valid\n");
			return -1;
		}
	}

	memset(&peci_cmd, 0, sizeof(peci_cmd_t));	
	
	peci_cmd.dev_id = dev_id;
	peci_cmd.target = target;
	peci_cmd.read_len = read_len;
	peci_cmd.write_len = write_len;
    	peci_cmd.domain = domain;
    	peci_cmd.Xmit_Feedback = Xmit_Feedback;
    	peci_cmd.AWFCS = awfcs;

	write_buf[0] |= (0x01 << domain);

	for(i=0;(i < write_len);i++)
		peci_cmd.write_buffer[i] = write_buf[i]; 
	
	if(issue_peci_cmd(&peci_cmd,PECI_CMD_MAX) != 0)
	{
		dbgprint ("Issuing PECI command failed\n");
		return -1;
	}
	
	for(i=0;i<peci_cmd.read_len;i++)
		read_buf[i] = peci_cmd.read_buffer[i];

	return 0;
}

