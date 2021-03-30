// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

static bool is_pnor_part(const char *str)
{
	return strcasestr(str, "pnor") != NULL;
}

char *get_dev_mtd(void)
{
	FILE *f;
	char *ret = NULL, *pos = NULL;
	char line[255];

	f = fopen("/proc/mtd", "r");
	if (!f)
		return NULL;

	while (!pos && fgets(line, sizeof(line), f) != NULL) {
		/* Going to have issues if we didn't get the full line */
		if (line[strlen(line) - 1] != '\n')
			break;

		if (is_pnor_part(line)) {
			pos = strchr(line, ':');
			if (!pos)
				break;
		}
	}
	fclose(f);

	if (pos) {
		*pos = '\0';
		if (asprintf(&ret, "/dev/%s", line) == -1)
			ret = NULL;
	}

	return ret;
}
