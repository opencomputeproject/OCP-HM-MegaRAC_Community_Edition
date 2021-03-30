/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */
#pragma once

#include <limits.h>

struct mbox_context;
struct vpnor_partition_table;

struct vpnor_partition_paths
{
    char ro_loc[PATH_MAX];
    char rw_loc[PATH_MAX];
    char prsv_loc[PATH_MAX];
    char patch_loc[PATH_MAX];
};

struct vpnor_data {
	struct vpnor_partition_table *vpnor;
	struct vpnor_partition_paths paths;
};

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Populate the path object with the default partition paths
 *
 *  @param[in/out] paths - A paths object in which to store the defaults
 *
 *  Returns 0 if the call succeeds, else a negative error code.
 */
#ifdef VIRTUAL_PNOR_ENABLED
void vpnor_default_paths(struct vpnor_partition_paths *paths);
#else
static inline void vpnor_default_paths(struct vpnor_partition_paths *paths)
{
    memset(paths, 0, sizeof(*paths));
}
#endif

#ifdef __cplusplus
}
#endif
