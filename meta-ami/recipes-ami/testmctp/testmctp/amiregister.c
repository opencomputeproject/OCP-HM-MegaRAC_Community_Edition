/*
// Copyright (c) 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mctpw.h>
#include <mctp_api.h>

#define UNUSED(x) (void)(x)
#define EID_LIST_SIZE 32

void* context;
volatile bool run_flag = true;
unsigned tout_500ms = 500;
uint16_t vendor_id = 0x8086;
uint16_t vd_message_type = 0x8002;
uint16_t vd_msg_type_mask = 0x80FF;
uint8_t vd_payload[] = {0x01, 0x02, 0x01, 0x89};
static unsigned response_counter = 3;
void network_reconfiguration_callback(void* client_context)
{
    UNUSED(client_context);
    printf("Network reconfiguration_callback.\n");
    return;
}

void signal_handler(int signo)
{
    if (signo == SIGINT && context)
    {
        mctpw_unregister_client(context);
        context = NULL;
        run_flag = false;
        printf("Client unregistered.\n");
    }
}
//NC-SI commands
//clear init state
const u8 ncsi_req[] =   { MCTP_NCSI_MSGTYPE, 0, 1, 0, 0x1, 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//get statics 
const u8 ncsi19_req[] = { MCTP_NCSI_MSGTYPE, 0, 1, 0, 0x2, 0x19, 0x0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
u8 ncsi_resp[512];
u32 resp_len;

//PLDM command
//get version 
const u8 pldm_req[] = { MCTP_PLDM_MSGTYPE, 0x81, 0, 3, 0, 0, 0, 0, 1, 0};
u8 pldm_resp[512];

int main(void)
{
    void* bus_handler;
    int bus, ret;
    bool found = false;
    unsigned eid_list_size;
    mctpw_eid_t eid_list[EID_LIST_SIZE];

    printf("############# mctpw functions testing ##############\n");
    signal(SIGINT, signal_handler);

    /* find first MCTP_OVER_SMBUS binding */
    for (bus = 0; bus < 10; bus++)
    {
/*
        if (mctpw_find_bus_by_binding_type(MCTP_OVER_PCIE_VDM, bus,
                                           &bus_handler) == 0)
*/
        if (mctpw_find_bus_by_binding_type(MCTP_OVER_SMBUS, bus,
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
/*
    printf("Register Vendor PCI client on bus %x \n");
    if ((ret = mctpw_register_client(bus_handler, VDPCI, 0x8086, true, 1,
                                     0xFFFF, network_reconfiguration_callback,
                                     NULL, &context)) < 0)
*/
    printf("Register NC-SI client on bus %x \n",bus );
    if ((ret = mctpw_register_client(bus_handler, NCSI, 0x0000, false, 1,
                                     0xFFFF, NULL,
                                     NULL, &context)) < 0)
    {
        printf("Client registration failed\n" );
        return ret;
    }

    eid_list_size = EID_LIST_SIZE;
    if ((ret = mctpw_get_endpoint_list(context, eid_list, &eid_list_size)) < 0)
    {
        mctpw_unregister_client(context);
        return ret;
    }
    printf("\n On bus %x found %x endpoints.\n",bus,eid_list_size);

    for (unsigned i = 0; i < eid_list_size; i++)
    {  
	printf(" Eid %d = %d \n",i,(unsigned )(eid_list[i]));//static_cast<unsigned>(eid_list[i]));
	
    }

    eid_list_size = EID_LIST_SIZE;
    if ((ret = mctpw_get_matching_endpoint_list(context, eid_list,
                                                &eid_list_size)) < 0)
    {
        mctpw_unregister_client(context);
        return ret;
    }
    
    printf("\n %x endpoints supports registered message type.\n",eid_list_size);
    


    for (unsigned i = 0; i < eid_list_size; i++)
    {
        printf(" Eid %d = %d \n",i,(unsigned )(eid_list[i]));//static_cast<unsigned>(eid_list[i]));
    }

    mctpw_endpoint_properties_t endpoint_prop;
    for (unsigned i = 0; i < eid_list_size; i++)
    {
        if (mctpw_get_endpoint_properties(context, eid_list[i],
                                          &endpoint_prop) == 0)
        {
            printf("\n EP Eid %d properties : \n", (unsigned )(eid_list[i]));    
            printf(" network_id : %x\n", (unsigned )(endpoint_prop.network_id));              
            printf(" mctp_control : %s\n", (endpoint_prop.mctp_control ? "true" : "false")); 
            printf(" pldm : %s\n", (endpoint_prop.pldm ? "true" : "false")); 
            printf(" ncsi : %s\n", (endpoint_prop.ncsi ? "true" : "false")); 
            printf(" ethernet : %s\n", (endpoint_prop.ethernet ? "true" : "false")); 
            printf(" nvme_mgmt_msg : %s\n", (endpoint_prop.nvme_mgmt_msg ? "true" : "false")); 
            printf(" spdm : %s\n", (endpoint_prop.spdm ? "true" : "false")); 
            printf(" vdpci : %s\n", (endpoint_prop.vdpci ? "true" : "false")); 
            printf(" vdiana : %s\n", (endpoint_prop.vdiana ? "true" : "false")); 
        }
    }
    
    // for (unsigned i = 0; i < eid_list_size; i++)
    {

    /* Atomic send-receive, wait for response */
    uint8_t response_buffer[512];
    unsigned response_buffer_length = sizeof(response_buffer);
    unsigned i = 0;
    eid_list[i] = 20;
    printf( "Blocking atomic send-receive request to EID %d\n", eid_list[i]);
    printf( "Request packet\n");
    dump_buf(sizeof(ncsi_req), ncsi_req);
    
    if ((ret = mctpw_send_receive_atomic_message(
             context, eid_list[i], &ncsi_req[1] , sizeof(ncsi_req)-1 ,
             response_buffer, &response_buffer_length, tout_500ms)) == 0)
    {
        printf("Response %d bytes from atomic send receive:\n Payload:\n ",response_buffer_length );
        dump_buf(response_buffer_length, response_buffer);
    }
    else
    {
        printf("Atomic send-receive failed\n");
        // mctpw_unregister_client(context);
        // return ret;
    }
    }
    printf("Press Ctrl+C \n\n");
    while (run_flag == true)
    {
        usleep(1000);
    }

    // mctpw_unregister_client(context);

/*##########################################*/
    printf("\n########### libmctpami test #############\n");
    if (MCTPDbusSetup() == 0)
    {
        mctp_get_dev_list_by_type( MCTPMediumTypeI2C );//i2c
        printf("\n>>> NC-SI Get NC-SI Statistics command <<<\n");
        resp_len = sizeof(ncsi_resp);
        mctp_i2c_send_vdm(
        20,
        ncsi19_req,
        sizeof(ncsi19_req),
        ncsi_resp,
        &resp_len,
        0,
        2,
        100
        );
        printf("NC-SI Resp len:%d\n",resp_len);
        if(resp_len > 0){
            dump_buf(resp_len, ncsi_resp);
        }
        printf("\n>>> PLDM GetPLDMVersion command <<<\n");
        resp_len = sizeof(pldm_resp);
        mctp_pcie_send_vdm(
        9,
        pldm_req,
        sizeof(pldm_req),
        pldm_resp,
        &resp_len,
        0,
        2,
        100
        );
        printf("PLDM Resp len:%d\n",resp_len);
        if(resp_len > 0){
            dump_buf(resp_len, pldm_resp);
        }
        MCTPDbusClose();
        printf("Dbus closed.\n");
    }

/*##########################################*/

    return 0;
}
