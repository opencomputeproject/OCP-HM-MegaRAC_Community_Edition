/**
 * Copyright © 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <endian.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include <linux/types.h>

#include "console-server.h"

struct log_handler {
	struct handler			handler;
	struct console			*console;
	struct ringbuffer_consumer	*rbc;
	int				fd;
	size_t				size;
	size_t				maxsize;
	int				pagesize;
};


static const char *default_filename = LOCALSTATEDIR "/log/obmc-console.log";
static const size_t default_logsize = 16 * 1024;

static struct log_handler *to_log_handler(struct handler *handler)
{
	return container_of(handler, struct log_handler, handler);
}

static int log_trim(struct log_handler *lh, size_t space)
{
	int rc, n_shift_pages, shift_len, shift_start;
	off_t pos;
	void *buf;

	pos = lseek(lh->fd, 0, SEEK_CUR);

	n_shift_pages = (space + lh->pagesize - 1) / lh->pagesize;
	shift_start = n_shift_pages * lh->pagesize;
	shift_len = pos - (n_shift_pages * lh->pagesize);

	buf = mmap(NULL, pos, PROT_READ | PROT_WRITE, MAP_SHARED, lh->fd, 0);
	if (buf == MAP_FAILED)
		return -1;

	memmove(buf, buf + shift_start, shift_len);

	munmap(buf, pos);

	lh->size = shift_len;
	rc = ftruncate(lh->fd, lh->size);
	if (rc)
		warn("failed to truncate file");
	lseek(lh->fd, 0, SEEK_END);

	return 0;

}

static int log_data(struct log_handler *lh, uint8_t *buf, size_t len)
{
	int rc;

	if (len > lh->maxsize) {
		buf += len - lh->maxsize;
		len = lh->maxsize;
	}

	if (lh->size + len > lh->maxsize) {
		rc = log_trim(lh, len);
		if (rc)
			return rc;
	}

	rc = write_buf_to_fd(lh->fd, buf, len);
	if (rc)
		return rc;

	lh->size += len;

	return 0;
}

static enum ringbuffer_poll_ret log_ringbuffer_poll(void *arg,
		size_t force_len __attribute__((unused)))
{
	struct log_handler *lh = arg;
	uint8_t *buf;
	size_t len;
	int rc;

	/* we log synchronously, so just dequeue everything we can, and
	 * commit straight away. */
	for (;;) {
		len = ringbuffer_dequeue_peek(lh->rbc, 0, &buf);
		if (!len)
			break;

		rc = log_data(lh, buf, len);
		if (rc)
			return RINGBUFFER_POLL_REMOVE;

		ringbuffer_dequeue_commit(lh->rbc, len);
	}

	return RINGBUFFER_POLL_OK;
}

static int log_init(struct handler *handler, struct console *console,
		struct config *config)
{
	struct log_handler *lh = to_log_handler(handler);
	const char *filename, *logsize_str;
	size_t logsize = default_logsize;
	int rc;

	lh->console = console;
	lh->pagesize = 4096;
	lh->size = 0;

	logsize_str = config_get_value(config, "logsize");
	rc = config_parse_logsize(logsize_str, &logsize);
	if (logsize_str != NULL && rc) {
		logsize = default_logsize;
		warn("Invalid logsize. Default to %ukB",
                     (unsigned int)(logsize >> 10));
	}
	lh->maxsize = logsize <= lh->pagesize ? lh->pagesize + 1 : logsize;

	filename = config_get_value(config, "logfile");
	if (!filename)
		filename = default_filename;

	lh->fd = open(filename, O_RDWR | O_CREAT, 0644);
	if (lh->fd < 0) {
		warn("Can't open log buffer file %s", filename);
		return -1;
	}
	rc = ftruncate(lh->fd, 0);
	if (rc) {
		warn("Can't truncate file %s", filename);
		close(lh->fd);
		return -1;
	}

	lh->rbc = console_ringbuffer_consumer_register(console,
			log_ringbuffer_poll, lh);

	return 0;
}

static void log_fini(struct handler *handler)
{
	struct log_handler *lh = to_log_handler(handler);
	ringbuffer_consumer_unregister(lh->rbc);
	close(lh->fd);
}

static struct log_handler log_handler = {
	.handler = {
		.name		= "log",
		.init		= log_init,
		.fini		= log_fini,
	},
};

console_handler_register(&log_handler.handler);

