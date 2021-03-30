#include <cstdio>
#include <ipmid/api.hpp>

#define GRPEXT_GET_GROUP_CMD 0
void register_netfn_groupext_functions() __attribute__((constructor));

ipmi_ret_t ipmi_groupext(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                         ipmi_request_t request, ipmi_response_t response,
                         ipmi_data_len_t data_len, ipmi_context_t context)
{
    // Generic return from IPMI commands.
    ipmi_ret_t rc = IPMI_CC_OK;
    uint8_t* p = (uint8_t*)response;

    std::printf("IPMI GROUP EXTENSIONS\n");

    *data_len = 1;
    *p = 0;

    return rc;
}

void register_netfn_groupext_functions()
{
    // <Group Extension Command>
    ipmi_register_callback(NETFUN_GRPEXT, GRPEXT_GET_GROUP_CMD, NULL,
                           ipmi_groupext, PRIVILEGE_USER);

    return;
}
