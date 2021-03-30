/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef MBOX_TEST_UTILS_H
#define MBOX_TEST_UTILS_H

#include <linux/limits.h>

struct tmpf {
	int fd;
	char path[PATH_MAX];
};

/**
 * Initialise a tmpf instance for use, creating a temporary file.
 *
 * @tmpf: A context to initialise with the provided template
 * @template_str: A file basename in mkstemp(3) template form
 *
 * Returns 0 on success, or -1 on error with errno set appropriately
 */
int tmpf_init(struct tmpf *tmpf, const char *template_str);

/**
 * Destroy a tmpf instance, closing the file descriptor and removing the
 * temporary file.
 */
void tmpf_destroy(struct tmpf *tmpf);

#endif /* MBOX_TEST_UTILS_H */
