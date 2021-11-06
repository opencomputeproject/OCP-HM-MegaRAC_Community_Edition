#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "mctpw.h"
#include "mctp_api.h"

#define UNUSED(x) (void)(x)
#define EID_LIST_SIZE 32

static int MCTP_Dev_Count;
static mctp_dev_t MCTP_DEV_List[EID_LIST_SIZE];
/*Debug used definition*/
#define TCRIT printf
#ifdef DEBUG
#define TINFO printf
#define TDBG printf
#define DUMP_BUF(len, buf) dump_buf(len, buf)
#else
#define TINFO printf
#define TDBG 
#define DUMP_BUF(len, buf) 
#endif
int dump_buf(int len, u8* buf)
{
    int i;
    printf("%s \n",__func__);
    if(len <= 0 ){
        printf("%s : error length:%d.\n",__func__,len);
        return -1;
    }
    for(i=0;i<len;i++){
        printf("0x%02X ",buf[i]);
        if(i>0 && ((i+1)%8) == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
    return 0;
}

/*##################################################*/
static u8 MCTPDbusOpened = 0;

int mctp_get_dev_list_by_type(
u8 media_type
)
{
    void* bus_handler;
    int bus, ret;
    void* mctp_context;
    unsigned eid_list_size;
    mctpw_eid_t eid_list[EID_LIST_SIZE];
    bool found          = false;
    for(media_type = MCTPMediumTypeI2C; media_type<= MCTPMediumTypePCI; media_type++ )
    {
        /* find first any MCTP binding on either SMBUS or PCIe */
        for (bus = 0; bus < 10; bus++)
        {
            if (mctpw_find_bus_by_binding_type(media_type == MCTPMediumTypeI2C ? MCTP_OVER_SMBUS : MCTP_OVER_PCIE_VDM, bus,
                                            &bus_handler) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            continue; //checking other media
        }
        TINFO("\n%s: Found MCTP %s binding.\n",__func__, media_type == MCTPMediumTypeI2C ? "SMBUS":"PCIe");
        //not for specific message type.
        TINFO("Register MCTP message %d client on bus %d \n", 0 , bus);
        if ((ret = mctpw_register_client(bus_handler, 0, 0x0000, false, 1,
                                        0xFFFF, NULL,
                                        NULL, &mctp_context)) < 0)
        {
            TCRIT("Client registration failed\n" );
            return ret;
        }
        eid_list_size = EID_LIST_SIZE;
        if ((ret = mctpw_get_endpoint_list(mctp_context, eid_list, &eid_list_size)) < 0)
        {
            mctpw_unregister_client(mctp_context);
            return ret;
        }
    
        TINFO("\n On bus %d found %d endpoints.\n",bus,eid_list_size);
        if( media_type == MCTPMediumTypeI2C){
            MCTP_Dev_Count = 0;
        }
        mctpw_endpoint_properties_t endpoint_prop;
        u8 MsgTypeCnt;
        for (unsigned i = 0; i < eid_list_size; i++)
        {
            if (mctpw_get_endpoint_properties(mctp_context, eid_list[i],
                                            &endpoint_prop) == 0)
            {
                TDBG("\n EP Eid %d properties : \n", (unsigned )(eid_list[i]));    
                TDBG(" network_id : %x\n", (unsigned )(endpoint_prop.network_id));              
                TDBG(" mctp_control : %s\n", (endpoint_prop.mctp_control ? "true" : "false")); 
                TDBG(" pldm : %s\n", (endpoint_prop.pldm ? "true" : "false")); 
                TDBG(" ncsi : %s\n", (endpoint_prop.ncsi ? "true" : "false")); 
                TDBG(" ethernet : %s\n", (endpoint_prop.ethernet ? "true" : "false")); 
                TDBG(" nvme_mgmt_msg : %s\n", (endpoint_prop.nvme_mgmt_msg ? "true" : "false")); 
                TDBG(" spdm : %s\n", (endpoint_prop.spdm ? "true" : "false")); 
                TDBG(" vdpci : %s\n", (endpoint_prop.vdpci ? "true" : "false")); 
                TDBG(" vdiana : %s\n", (endpoint_prop.vdiana ? "true" : "false")); 
                
                MCTP_DEV_List[MCTP_Dev_Count].EID           = eid_list[i];
                MCTP_DEV_List[MCTP_Dev_Count].MediumType    = media_type;
                memcpy((void*)&(MCTP_DEV_List[MCTP_Dev_Count].UUID[0]),(void*)&endpoint_prop.uuid[0], 16);
                MsgTypeCnt = 0;
                if(endpoint_prop.mctp_control   ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_CTRL_MSGTYPE;} 
                if(endpoint_prop.pldm           ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_PLDM_MSGTYPE;}
                if(endpoint_prop.ncsi           ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_NCSI_MSGTYPE;}
                // if(endpoint_prop.ethernet       ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = ;}
                if(endpoint_prop.nvme_mgmt_msg  ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_NVMEMI_MSGTYPE;}
                // if(endpoint_prop.spdm           ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = ;}
                if(endpoint_prop.vdpci          ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_PCI_MSGTYPE;}
                if(endpoint_prop.vdiana         ){MCTP_DEV_List[MCTP_Dev_Count].MsgTypes[MsgTypeCnt++] = MCTP_IANA_MSGTYPE;}
                /*TODO, vendor ID for PCIe devices.*/
                MCTP_Dev_Count++;
            }
        }
        TDBG("%s: Current Devices found:%d.\n\n", __func__, MCTP_Dev_Count);
        mctpw_unregister_client(mctp_context);
    }
    TDBG("%s: Total Devices found:%d.\n", __func__, MCTP_Dev_Count);
    return ret;
}

int mctp_send_vdm(
      u8 media_type,
      u8        eID,
      const u8* reqData,
      u32       reqLen,
      u8*       rspData,
      u32*      rspLen,
      u8        callback,
      u32       tries,
      u32       timeoutMSec)
{
    void* bus_handler;
    int bus, ret;
    bool found          = false;
    u8 MsgType          = reqData[0];
    const u8* pPayLoad  = &reqData[1];

    void* mctp_context;
    /* find first any MCTP binding on either SMBUS or PCIe */
    for (bus = 0; bus < 10; bus++)
    {
        if (mctpw_find_bus_by_binding_type(media_type == MCTPMediumTypeI2C ? MCTP_OVER_SMBUS : MCTP_OVER_PCIE_VDM, bus,
                                           &bus_handler) == 0)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        return -ENOENT;
    }
    TINFO("\n%s: Found MCTP %s binding.\n",__func__, media_type == MCTPMediumTypeI2C ? "SMBUS":"PCIe");

    TINFO("Register MCTP message %d client on bus %d \n", MsgType , bus);
    if ((ret = mctpw_register_client(bus_handler, MsgType, 0x0000, false, 1,
                                     0xFFFF, NULL,
                                     NULL, &mctp_context)) < 0)
    {
        TCRIT("Client registration failed\n" );
        return ret;
    }

    TDBG( "Blocking atomic send-receive request to EID %d, timeout:%d\n", eID,timeoutMSec);
    TDBG("%s, request len:%d\n",__func__,reqLen );
    DUMP_BUF(reqLen , reqData);
    if ((ret = mctpw_send_receive_atomic_message(
             mctp_context, eID, &reqData[1] , reqLen-1 ,
             rspData, (unsigned*)rspLen, timeoutMSec)) == 0)
    {
        TDBG("%s, Resp read Len:%d\n",__func__,*rspLen );
        // if ( *rspLen > 0 ){
        // DUMP_BUF(*rspLen , resp_buf);
        // }
    }
    else
    {
        *rspLen = 0;
        TCRIT("Atomic send-receive failed\n");
    }
    mctpw_unregister_client(mctp_context);
    return ret;
}

int mctp_i2c_send_vdm(
      u8        eID,
      const u8* reqData,
      u32       reqLen,
      u8*       rspData,
      u32*      rspLen,
      u8        callback,
      u32       tries,
      u32       timeoutMSec)
{
     return mctp_send_vdm(
      MCTPMediumTypeI2C,
      eID,
      reqData,
      reqLen,
      rspData,
      rspLen,
      callback,
      tries,
      timeoutMSec);
      
}
int mctp_pcie_send_vdm(
      u8        eID,
      const u8* reqData,
      u32       reqLen,
      u8*       rspData,
      u32*      rspLen,
      u8        callback,
      u32       tries,
      u32       timeoutMSec)
{
     return mctp_send_vdm(
      MCTPMediumTypePCI,
      eID,
      reqData,
      reqLen,
      rspData,
      rspLen,
      callback,
      tries,
      timeoutMSec);

}

int MCTPDbusSetup(void)
{
	MCTPDbusOpened = 1;

	return 0;
}

void MCTPDbusClose(void)
{
    if (MCTPDbusOpened)
    {
        TINFO("%s close dbus\n", __func__);
    }
    MCTPDbusOpened = 0;
}
