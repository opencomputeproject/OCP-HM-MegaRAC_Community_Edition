/******************************************************************************
 *
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with no express
 * or implied warranties, other than those that are expressly stated in the
 * License.
 *
 ******************************************************************************/

#include "OemData.h"

#include "utils.h"

#ifdef OEMDATA_SECTION
int logOemDataICX1(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    (void)cpuInfo;
    cJSON_AddStringToObject(pJsonChild, "Test", "1");
    return ACD_SUCCESS;
}

static const SOemDataVx sOemDataVx[] = {
    {cd_icx, logOemDataICX1},
    {cd_icx2, logOemDataICX1},
    {cd_icxd, logOemDataICX1},
};

/******************************************************************************
 *
 *   logOemData
 *
 *
 *
 ******************************************************************************/
int logOemData(CPUInfo* cpuInfo, cJSON* pJsonChild)
{
    if (pJsonChild == NULL)
    {
        return ACD_INVALID_OBJECT;
    }
    for (uint32_t i = 0; i < (sizeof(sOemDataVx) / sizeof(SOemDataVx)); i++)
    {
        if (cpuInfo->model == sOemDataVx[i].cpuModel)
        {
            return sOemDataVx[i].logOemDataVx(cpuInfo, pJsonChild);
        }
    }
    return ACD_FAILURE;
}
#endif