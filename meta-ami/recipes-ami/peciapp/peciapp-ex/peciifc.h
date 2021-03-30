/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2017-2018, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */

#ifndef __PECI_IFC_H__
#define __PECI_IFC_H__

/* Device Properties */
#define PECI_MAJOR		45
#define PECI_MINOR		0
#define PECI_MAX_DEVICES 	255
#define PECI_DEV_NAME		"PECI"

#define BYTE	1
#define WORD	2
#define DWORD	4
#define QWORD	8

/* IOCTL Command and structure */
//#define PECI_ISSUE_CMD		_IOR('p', 0, int)
#define PECI_CMD_PING              0xc001b601
#define PECI_CMD_GET_DIB           0xc005b602
#define PECI_CMD_GET_TEMP          0xc003b603
#define PECI_CMD_RD_PKG_CFG        0xc009b604
#define PECI_CMD_WR_PKG_CFG        0xc009b605
#define PECI_CMD_RD_IA_MSR         0xc00cb606
#define PECI_CMD_WR_IA_MSR         0xc00cb607
#define PECI_CMD_RD_PCI_CFG        0xc00ab608
#define PECI_CMD_WR_PCI_CFG        0xc00ab609
#define PECI_CMD_RD_PCI_CFG_LOCAL  0xc00bb60a
#define PECI_CMD_WR_PCI_CFG_LOCAL  0xc00bb60b
#define PECI_CMD_MAX               0xc001b60c

#define PECI_DATA_BUF_SIZE	32 

typedef struct 
{
    unsigned char target;
//    unsigned char dev_id;
    unsigned char read_buffer[PECI_DATA_BUF_SIZE];
    unsigned char domain;
    unsigned char write_len;
    unsigned char write_buffer[PECI_DATA_BUF_SIZE];
    unsigned char read_len;
//    unsigned char read_buffer[PECI_DATA_BUF_SIZE];
    unsigned char dev_id;
    unsigned char AWFCS;
    unsigned char Xmit_Feedback;
    int status;
}__attribute__((packed)) peci_cmd_t;

union peci_data
{
    unsigned char peci_byte;         /*!< Byte data */
    unsigned short peci_word;        /*!< Word data */
    unsigned long peci_dword;        /*!< 4byte data */
    unsigned char peci_qword[8];     /*!< 8byte data */
};

typedef struct
{
    unsigned char device_info;  /*!<Device information */
    unsigned char revision_no;  /*!<Revison Number supported by PECI */
    unsigned char reserved[6];  /*!<Reserved bytes */
}__attribute__((packed)) peci_getdib_res_t;

typedef struct
{
    unsigned char option;       /*!<Read option is performed for BYTE/WORD/DWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned char index;        /*!<The encoding for the requested service */
    unsigned short parameter;   /*!<Specifies the parameter for data being requested */
}__attribute__((packed)) peci_rdpkgconfig_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
    union peci_data data;           /*!< Response data   */
}__attribute__((packed)) peci_rdpkgconfig_res_t;

typedef struct
{
    unsigned char option;       /*!<Write option is performed for BYTE/WORD/DWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned char index;        /*!<The encoding for the requested service */
    unsigned short parameter;   /*!<Specifies the parameter for data being requested */
    union peci_data data;       /*!<Data to be written on the Index. */
}__attribute__((packed)) peci_wrpkgconfig_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
}__attribute__((packed)) peci_wrpkgconfig_res_t;

typedef struct
{
    unsigned char option;       /*!<Read option is performed for BYTE/WORD/DWORD/QWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned char processor_id; /*!<IA MSR Space refers to a specific logical processor in CPU */
    unsigned short msr_addr;    /*!<Address of the MSR */
}__attribute__((packed)) peci_rdiamsr_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
    union peci_data data;            /*!< Response data */
}__attribute__((packed)) peci_rdiamsr_res_t;

typedef struct
{
    unsigned char option;       /*!<Read option is performed for BYTE/WORD/DWORD/QWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned char processor_id; /*!<IA MSR Space refers to a specific logical processor in CPU */
    unsigned short msr_addr;    /*!<Address of the MSR */
    union peci_data data;	/*!<Data to be written on the Index. */
}__attribute__((packed)) peci_wriamsr_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
    union peci_data data;            /*!< Response data */
}__attribute__((packed)) peci_wriamsr_res_t;

typedef struct
{
    unsigned char option;       /*!<Read option is performed for BYTE/WORD/DWORD/QWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned long pci_config_addr;    /*!<Address of the MSR */
}__attribute__((packed)) peci_rdpciconfig_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
    union peci_data data;            /*!< Response data */
}__attribute__((packed)) peci_rdpciconfig_res_t;

typedef struct
{
    unsigned char option;       /*!<Read option is performed for BYTE/WORD/DWORD/QWORD */
    unsigned char host_id;      /*!<Host ID[7:1] and Retry[0] */
    unsigned long pci_config_addr;    /*!<Address of the MSR */
    union peci_data data;	/*!<Data to be written on the Index. */
}__attribute__((packed)) peci_wrpciconfig_req_t;

typedef struct
{
    unsigned char completion_code;  /*!< Completion code */
    union peci_data data;            /*!< Response data */
}__attribute__((packed)) peci_wrpciconfig_res_t;


#define PECI_CTL_FILE	"/dev/peci"

int peci_cmd_ping(char dev_id, char target);
int peci_cmd_read_temp(char dev_id, char target, char domain, char xmit_feedback, char awfcs, int *read_len, char *read_buffer);
int peci_generic_cmd(char dev_id, char target, char domain, char xmit_feedback, char awfcs, char *write_buf, char write_len, char *read_buf, char read_len);

/**
 * @fn peci_cmd_get_dib
 * @brief Get the client revision number and number of supported domains.
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[out] res - pointer to the output buffer.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_get_dib(char dev_id, char target, char domain, char xmit_feedback, char awfcs, int *read_len, unsigned char *pecigetdibres);

/**
 * @fn peci_cmd_rdpkgconfig
 * @brief Get the client revision number and number of supported domains.
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[out] read_buffer - pointer to the output buffer.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_rdpkgconfig(char dev_id, char target, char domain, char xmit_feedback, char awfcs, int *read_len, unsigned char *pecirdpkgreq, unsigned char *pecirdpkgres);

/**
 * @fn peci_cmd_wrpkgconfig
 * @brief Provides Write access to the package configuration space
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
 *              -1, if invalid data in the request.
 */
int peci_cmd_wrpkgconfig(char dev_id, char target, char domain, char xmit_feedback, char awfcs, int *read_len, unsigned char *peciwrpkgreq, unsigned char *peciwrpkgres);

/**
 * @fn peci_cmd_rdiamsr
 * @brief Provides Read access to the Model specific Registers
 * @param[in] dev_id - Device ID.
 * @param[in] target - Target address.
 * @param[in] req - pointer to the Input structure.
 * @param[out] res - Pointer to the output structure.
 * @retval      0, on success,
int peci_cmd_wrpciconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpcireq, unsigned char *peciwrpcires); *              -1, if invalid data in the request.
 */
int peci_cmd_rdiamsr(char dev_id, char target, char domain, char xmit_feedback, char awfcs, int *read_len,  unsigned char *pecirdiamsrreq, unsigned char *pecirdiamsrres);
int peci_cmd_wriamsr(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwriamsrreq, unsigned char *peciwriamsrres);
int peci_cmd_rdpciconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *pecirdpcireq, unsigned char *pecirdpcires);
int peci_cmd_wrpciconfig(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpcireq, unsigned char *peciwrpcires);
int peci_cmd_rdpciconfiglocal(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char*peciwrpcilocalreq, unsigned char *peciwrpcilocalres);
int peci_cmd_wrpciconfiglocal(char dev_id, char target, char domain, char Xmit_Feedback, char awfcs, int *read_len, unsigned char *peciwrpcilocalreq, unsigned char *peciwrpcilocalres);

void peci_cmd_enable_awfcs(peci_cmd_t* peci_cmd);

#endif
