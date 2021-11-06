/*
Copyright (c) 2017, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define SPX_BMC 1
#include <stdint.h>
//AMI_CHANGE_START
//Adapting AMI ipmi service
/*#ifndef SPX_BMC
#include <IntraBmcApi.h>  // IPMI chassis control commands like power and reset
#include <IpmiCmdDefs.h>  // Enums used for chassis control
#include <RmtDbgApi.h>    // Enums used for remote debug connection
#include <stdio.h>
#include <safe_lib.h>
#else*/
#include <stdio.h>
#include <libipmi_struct.h>
#include <IPMI_App.h>
#include <IPMIDefs.h>
#include <IPMI_Chassis.h>
#include <libipmi_session.h>
//#endif
//AMI_CHANGE_END
#include <syslog.h>
#include "ipmi.h"
#include "logging.h"


//AMI_CHANGE_START
//Adapting AMI ipmi service
/*
 * The following functions (reset target and get/set power state)
 * have been implemented using an Intel Internal IPMI service.
 * Other implementations will need to implement these functions
 * according to available platform capabilities.
 */
int IntraBmcCmd(uint8_t u8NetFn, uint8_t u8Cmd, uint8_t *pu8Req, uint32_t u32ReqLen, uint8_t *pu8CompCode, uint8_t *pu8Rsp, uint32_t u32MaxRspLen)
{
	IPMI20_SESSION_T pSession;
	uint8_t byPrivLevel = PRIV_LEVEL_ADMIN;
	int wRet;
	uint32_t dwResLen = u32MaxRspLen;

	wRet = LIBIPMI_Create_IPMI_Local_Session(&pSession,"","",&byPrivLevel,NULL,AUTH_BYPASS_FLAG,5);
	if(wRet != LIBIPMI_E_SUCCESS)
	{
	 syslog(LOG_WARNING,"Cannot Establish IPMI Local Session\n");
		return -1;
	}
	wRet = LIBIPMI_Send_RAW_IPMI2_0_Command(&pSession, PAYLOAD_TYPE_IPMI, u8NetFn, u8Cmd,pu8Req, u32ReqLen,pu8Rsp, &dwResLen,5);

	*pu8CompCode = CC_SUCCESS;

	LIBIPMI_CloseSession(&pSession);

	if(wRet != LIBIPMI_E_SUCCESS)
	{
		*pu8CompCode = CC_UNSPECIFIED_ERR;
		return -1;
	}

	return dwResLen;
}
/*
STATUS ipmi_initialize(ClientAddrT cl_addr)
{
    if (IntraBmcInitialize(1) < 0) {
        return ST_ERR;
    }

    // Notify ipmi of connection for logging and sensor
    return ipmi_rmtdbg_connection(IPMI_RMTDBG_CONNECTED, cl_addr);
}

void ipmi_close(void)
{
    ClientAddrT cl_addr = {0};

    // Notify IPMI session has ended and close ipmi interface.
    ipmi_rmtdbg_connection(IPMI_RMTDBG_DISCONNECT, cl_addr);

    IntraBmcClose();
}
#endif
//AMI_CHANGE_END
*/
// Retry ipmi command if connection fails. Does NOT retry for nonzero compcode
STATUS ipmi_command(int retries, uint8_t u8NetFn, uint8_t u8Cmd,
                     uint8_t *pu8Req, uint32_t u32ReqLen, uint8_t *pu8CompCode,
                     uint8_t *pu8Rsp, uint32_t u32MaxRspLen) {
    STATUS ret = ST_ERR;
    int try;

    for (try=0; try<retries; try++) {
        int nRspSize = IntraBmcCmd(u8NetFn, u8Cmd, pu8Req, u32ReqLen,
                pu8CompCode, pu8Rsp, u32MaxRspLen);
        if (nRspSize >= 0) {
            ret = ST_OK;
            break;
        }
       syslog(LOG_WARNING, "IntraBmcCmd netfn %02x, cmd %02x failed %d/%d, pu8CompCode %d",
                u8NetFn, u8Cmd, try+1, retries, pu8CompCode);
    }
    return ret;
}

STATUS ipmi_reset_target(void)
{
    uint8_t u8ReqData[1];  // size based on the IPMI chassis control command (1 byte)
    uint8_t u8CompCode = 0xFF;
//AMI_CHANGE_START
//Adapting AMI ipmi service
#ifdef SPX_BMC
    uint8_t u8RspData[5];
#endif

    u8ReqData[0] = CMD_WARM_RESET;
#ifndef SPX_BMC
    STATUS ret = ipmi_command(2, NETFN_CHASSIS, CMD_CHASSIS_CONTROL, u8ReqData,
                               sizeof(u8ReqData), &u8CompCode, NULL, 0);
#else
    STATUS ret = ipmi_command(2, NETFN_CHASSIS, CMD_CHASSIS_CONTROL, u8ReqData,
                               sizeof(u8ReqData), &u8CompCode, &u8RspData[0], sizeof(u8RspData));
#endif
//AMI_CHANGE_END
    if ((ret != ST_OK) || (u8CompCode != 0)) {
        return ST_ERR;
    }
    return ST_OK;
}

STATUS ipmi_get_power_state(int* powerState)
{
    uint8_t u8RspData[5] = {0};  // size based on the response from chassis status
    uint8_t u8CompCode = 0xFF;
    // This could also be a GPIO read of the CPU_PWRGD but the intraBMC process
    // has better error handling
    // this->get_gpio_data(CPU_PWRGD)

    // Get the current chassis power state
    STATUS ret = ipmi_command(2, NETFN_CHASSIS, CMD_GET_CHASSIS_STATUS, NULL, 0,
                               &u8CompCode, &u8RspData[0], sizeof(u8RspData));
    if (ret != ST_OK) {
        return ret;
    }

    *powerState = u8RspData[1] & 1;  // only care about the lsb in the second element

    return ST_OK;
}

STATUS ipmi_set_power_state(const bool assert)
{
    int curPwrState;
    uint8_t u8ReqData[1];  // size based on the IPMI chassis control command (1 byte)
    uint8_t u8CompCode = 0xFF;
//AMI_CHANGE_START
//Adapting AMI ipmi service
#ifdef SPX_BMC
    uint8_t u8RspData[5];
#endif

    if (!assert) {
        // We don't care about the de-asserted state since the physical
        // pin control does not exist
        return ST_OK;
    }

    if (ipmi_get_power_state(&curPwrState) != ST_OK) {
        return ST_ERR;
    }

    // assert power opposite of the current power state
    u8ReqData[0] = (curPwrState + 1) % 2;
#ifndef SPX_BMC
    STATUS ret = ipmi_command(2, NETFN_CHASSIS, CMD_CHASSIS_CONTROL, u8ReqData, 1,
                               &u8CompCode, NULL, 0);
#else
    STATUS ret = ipmi_command(2, NETFN_CHASSIS, CMD_CHASSIS_CONTROL, u8ReqData, 1,
                               &u8CompCode, &u8RspData[0], sizeof(u8RspData));
#endif
//AMI_CHANGE_END
    if ((ret != ST_OK) || (u8CompCode != 0)) {
        return ST_ERR;
    }
    return ST_OK;
}
/*
//AMI_CHANGE_START
//remote dbg connection check not available
#ifndef SPX_BMC
STATUS ipmi_rmtdbg_connection(const ipmiRmtDbgSessType sesType, ClientAddrT cl_addr)
{
    uint8_t u8ReqData[2+sizeof(ClientAddrT)] = {0};
    uint8_t u8RspData[128];
    uint8_t u8CompCode = 0xFF;
    int datalen = 2;

    if(cl_addr == NULL)
        return ST_ERR;

    u8ReqData[0] = (uint8_t)RMTDBG_IPMICMD_CONNECT;
    switch (sesType) {
        case IPMI_RMTDBG_CONNECTED:
            ASD_log_buffer(LogType_Debug, cl_addr, sizeof(cl_addr), "RDCONN");
            u8ReqData[1] = (uint8_t)RMTDBG_CONNECT_START;
            datalen += sizeof(ClientAddrT);
            memcpy_s(&u8ReqData[2], sizeof(u8ReqData)-2, cl_addr, sizeof(ClientAddrT));
            break;
        case IPMI_RMTDBG_DISCONNECT:
            u8ReqData[1] = (uint8_t)RMTDBG_CONNECT_END;
            break;
        default:
            return ST_ERR;
    }
    STATUS ret = ipmi_command(2, NETFN_INTEL_OEM_GENERAL, CMD_REMOTE_DBG_PARAM,
                               u8ReqData, datalen,
                               &u8CompCode, u8RspData, sizeof(u8RspData));
    if ((ret != ST_OK) || (u8CompCode != 0)) {
        return ST_ERR;
    }
    return ST_OK;
}
#endif
//AMI_CHANGE_END
*/
