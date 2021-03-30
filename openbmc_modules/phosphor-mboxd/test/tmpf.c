// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test/tmpf.h"

static const char *tmpf_dir = "/tmp/";

int tmpf_init(struct tmpf *tmpf, const char *template)
{
	strcpy(tmpf->path, tmpf_dir);
	strncat(tmpf->path, template, sizeof(tmpf->path) - sizeof(tmpf_dir));

	tmpf->fd = mkstemp(tmpf->path);
	if (tmpf->fd < 0) {
		perror("mkstemp");
		return -1;
	}

	return 0;
}

void tmpf_destroy(struct tmpf *tmpf)
{
	if (tmpf->fd)
		close(tmpf->fd);

	if (tmpf->path)
		unlink(tmpf->path);
}
