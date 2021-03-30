// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.
// Copyright (C) 2018 Evan Lojewski.

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <mtd/mtd-abi.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "backend.h"
#include "lpc.h"
#include "mboxd.h"

#define MIN(__x__, __y__)  (((__x__) < (__y__)) ? (__x__) : (__y__))

#define FILE_ERASE_SIZE (4 * 1024)

struct file_data {
	int fd;
};

static int file_dev_init(struct backend *backend, void *data)
{
	struct mtd_info_user info;
	const char *path = data;
	struct file_data *priv;
	struct stat statbuf;
	int rc = 0;

	if (!path) {
		MSG_ERR("No PNOR file specified\n");
		return -EINVAL;
	}

	priv = malloc(sizeof(*priv));
	if (!priv)
		return -errno;

	MSG_DBG("Opening %s\n", path);

	priv->fd = open(path, O_RDWR);
	if (priv->fd < 0) {
		MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n", path,
			strerror(errno));
		rc = -errno;
		goto cleanup_data;
	}

	/* Don't attach to mtd devices. */
	rc = ioctl(priv->fd, MEMGETINFO, &info);
	if (rc != -1) {
		rc = -errno;
		goto cleanup_fd;
	}

	rc = fstat(priv->fd, &statbuf);
	if (rc < 0) {
		rc = -errno;
		goto cleanup_fd;
	}

	if (backend->flash_size == 0) {
		MSG_INFO("Flash size should be supplied on the commandline.\n");
		backend->flash_size = statbuf.st_size;
	}

	/* Pick an erase size congruent with partition alignment */
	backend->erase_size_shift = log_2(FILE_ERASE_SIZE);
	backend->block_size_shift = backend->erase_size_shift;
	MSG_DBG("Flash erase size: 0x%.8x\n", FILE_ERASE_SIZE);

	backend->priv = priv;
	return rc;

cleanup_fd:
	close(priv->fd);
cleanup_data:
	free(priv);
	return rc;
}

static void file_dev_free(struct backend *backend)
{
	struct file_data *priv = backend->priv;

	close(priv->fd);
	free(priv);
}

/* Flash Functions */

/*
 * file_erase() - Erase the flash
 * @context:	The mbox context pointer
 * @offset:	The flash offset to erase (bytes)
 * @size:	The number of bytes to erase
 *
 * Return:	0 on success otherwise negative error code
 */
static int file_erase(struct backend *backend, uint32_t offset, uint32_t count)
{
	const uint32_t erase_size = 1 << backend->erase_size_shift;
	struct file_data *priv = backend->priv;
	struct erase_info_user erase_info = {0};
	int rc;

	MSG_DBG("Erase flash @ 0x%.8x for 0x%.8x\n", offset, count);

	uint8_t* erase_buf = (uint8_t*)malloc(count);
	if (!erase_buf) {
		MSG_ERR("Couldn't malloc erase buffer. %s\n", strerror(errno));
		return -1;
	}
	memset(erase_buf, 0xFF, erase_size);
	rc = pwrite(priv->fd, erase_buf, count, offset);
	free(erase_buf);

	if (rc < 0) {
		MSG_ERR("Couldn't erase flash at 0x%.8x\n", erase_info.start);
		return -errno;
	}


	return 0;
}

#define CHUNKSIZE (64 * 1024)

/*
 * file_copy() - Copy data from the flash device into a provided buffer
 * @context:	The backend context pointer
 * @offset:	The flash offset to copy from (bytes)
 * @mem:	The buffer to copy into (must be of atleast 'size' bytes)
 * @size:	The number of bytes to copy
 * Return:	Number of bytes copied on success, otherwise negative error
 *		code. file_copy will copy at most 'size' bytes, but it may
 *		copy less.
 */
static int64_t file_copy(struct backend *backend, uint32_t offset,
			  void *mem, uint32_t size)
{
	struct file_data *priv = backend->priv;
	int32_t size_read;
	void *start = mem;

	MSG_DBG("Copy flash to %p for size 0x%.8x from offset 0x%.8x\n", mem,
		size, offset);
	if (lseek(priv->fd, offset, SEEK_SET) != offset) {
		MSG_ERR("Couldn't seek flash at pos: %u %s\n", offset,
			strerror(errno));
		return -errno;
	}

	do {
		size_read = read(priv->fd, mem, min_u32(CHUNKSIZE, size));
		if (size_read < 0) {
			MSG_ERR("Couldn't copy file into ram: %s\n",
				strerror(errno));
			return -errno;
		}

		size -= size_read;
		mem += size_read;
	} while (size && size_read);

	return size_read ? mem - start : -EIO;
}

/*
 * file_write() - Write the flash from a provided buffer
 * @context:	The mbox context pointer
 * @offset:	The flash offset to write to (bytes)
 * @buf:	The buffer to write from (must be of atleast size)
 * @size:	The number of bytes to write
 *
 * Return:	0 on success otherwise negative error code
 */
static int file_write(struct backend *backend, uint32_t offset, void *buf,
		       uint32_t count)
{
	struct file_data *priv = backend->priv;
	uint32_t buf_offset = 0;
	int rc;

	MSG_DBG("Write flash @ 0x%.8x for 0x%.8x from %p\n", offset, count,
		buf);

	if (lseek(priv->fd, offset, SEEK_SET) != offset) {
		MSG_ERR("Couldn't seek flash at pos: %u %s\n", offset,
			strerror(errno));
		return -errno;
	}

	while (count) {
		rc = write(priv->fd, buf + buf_offset, count);
		if (rc < 0) {
			MSG_ERR("Couldn't write to file, write lost: %s\n",
				strerror(errno));
			return -errno;
		}
		count -= rc;
		buf_offset += rc;
	}

	return 0;
}

/*
 * file_reset() - Reset the lpc bus mapping
 * @context:    The backend context pointer
 * @buf:	Pointer to the LPC reserved memory
 * @count:	The size of the LPC reserved memory
 *
 * Return:      0 on success otherwise negative error code
 */
static int file_reset(struct backend *backend, void *buf, uint32_t count)
{
	struct file_data *priv = backend->priv;
	size_t len;
	int rc;

	len = MIN(backend->flash_size, count);

	/* Ugh, otherwise we need to parse the FFS image */
	assert(len == backend->flash_size);

	/* Preload Flash contents into memory window */
	rc = pread(priv->fd, buf, len, 0);
	if (rc < 0)
		return -errno;

	return reset_lpc_memory;
}

static const struct backend_ops file_ops = {
	.init = file_dev_init,
	.free = file_dev_free,
	.copy = file_copy,
	.erase = file_erase,
	.write = file_write,
	.reset = file_reset,
	.validate = NULL,
	.align_offset = NULL,
};

struct backend backend_get_file(void)
{
	struct backend be = {0};

	be.ops = &file_ops;

	return be;
}

int backend_probe_file(struct backend *master, const char *path)
{
	struct backend with;

	assert(master);
	with = backend_get_file();

	return backend_init(master, &with, (void *)path);
}
