
#pragma once

#if defined(__GNUC__)  /* GNU C */
#define STRUCT_POSTFIX(x) __attribute__((x))
#else
#define STRUCT_POSTFIX(x)
#endif

#define MCTPMediumTypeI2C 1
#define MCTPMediumTypePCI 2

// Different MsgTypes supported by MCTP Protocol
#define MCTP_MAX_MSGTYPE		0x05
#define MCTP_CTRL_MSGTYPE		0x00
#define MCTP_PLDM_MSGTYPE		0x01
#define MCTP_NCSI_MSGTYPE		0x02
#define MCTP_NVMEMI_MSGTYPE		0x04
#define MCTP_PCI_MSGTYPE		0x7E
#define MCTP_IANA_MSGTYPE		0x7F
#define NULL_MSGTYPE			0xFF
#define MCTP_MSGTYPE_MASK		0x7F

typedef unsigned char u8;
typedef unsigned int  u32;

struct _mctp_dev_t
{
   unsigned char Data_Assigned;
   unsigned char BusOwner;
   int MediumType;
   unsigned int PhysAddr;
   unsigned char EID;
   unsigned char UUID[16];
   unsigned char Discovered;
   unsigned char isBridge;
   unsigned char PoolSize;
   unsigned char StartingEID;
   unsigned char Version[2][4];
   unsigned char MsgTypes[10];
   unsigned char isStaticEID;
   unsigned char VendorID[2];
   unsigned char IANAEnterNum[4];
   unsigned char DeviceIDs[2];
} STRUCT_POSTFIX(packed);
typedef struct _mctp_dev_t mctp_dev_t;

int mctp_get_dev_list(u8 *rspData, u32 *rspLen);
int mctp_get_dev_list_by_type( u8 media_type);
int mctp_pci_send_vdm(
      u8        eID,
      const u8* reqData,
      u32       reqLen,
      u8*       rspData,
      u32*      rspLen,
      u8        callback,
      u32       tries,
      u32       timeoutMSec);

int mctp_i2c_send_vdm(
      u8        eID,
      const u8* reqData,
      u32       reqLen,
      u8*       rspData,
      u32*      rspLen,
      u8        callback,
      u32       tries,
      u32       timeoutMSec);

int MCTPDbusSetup(void);
void MCTPDbusClose(void);
int dump_buf(int len, u8* buf);