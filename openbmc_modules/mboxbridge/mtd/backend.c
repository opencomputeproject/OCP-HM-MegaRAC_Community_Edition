// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2018 IBM Corp.

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
#include "mtd/backend.h"

static int mtd_dev_init(struct backend *backend, void *data)
{
	const char *path = data;
	struct mtd_data *priv;
	int rc = 0;

	if (!path) {
		MSG_INFO("Discovering PNOR MTD\n");
		path = get_dev_mtd();
	}

	priv = malloc(sizeof(*priv));
	if (!priv) {
		rc = -errno;
		goto out;
	}

	MSG_DBG("Opening %s\n", path);

	priv->fd = open(path, O_RDWR);
	if (priv->fd < 0) {
		MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n", path,
			strerror(errno));
		rc = -errno;
		goto cleanup_priv;
	}

	/* If the file does not support MEMGETINFO it's not an mtd device */
	if (ioctl(priv->fd, MEMGETINFO, &priv->mtd_info) == -1) {
		rc = -errno;
		close(priv->fd);
		goto cleanup_priv;
	}

	if (backend->flash_size == 0) {
		/*
		 * PNOR images for current OpenPOWER systems are at most 64MB
		 * despite the PNOR itself sometimes being as big as 128MB. To
		 * ensure the image read from the PNOR is exposed in the LPC
		 * address space at the location expected by the host firmware,
		 * it is required that the image size be used for
		 * context->flash_size, and not the size of the flash device.
		 *
		 * However, the test cases specify the flash size via special
		 * test APIs (controlling flash behaviour) which don't have
		 * access to the mbox context. Rather than requiring
		 * error-prone assignments in every test case, we instead rely
		 * on context->flash_size being set to the size reported by the
		 * MEMINFO ioctl().
		 *
		 * As this case should never be hit in production (i.e. outside
		 * the test environment), log an error. As a consequence, this
		 * error is expected in the test case output.
		 */
		MSG_ERR(
		    "Flash size MUST be supplied on the commandline. However, "
		    "continuing by assuming flash is %u bytes\n",
		    priv->mtd_info.size);
		backend->flash_size = priv->mtd_info.size;
	}

	/* We know the erase size so we can allocate the flash_erased bytemap */
	backend->erase_size_shift = log_2(priv->mtd_info.erasesize);
	backend->block_size_shift = backend->erase_size_shift;
	priv->flash_bmap = calloc(backend->flash_size
			>> backend->erase_size_shift,
		   sizeof(*priv->flash_bmap));
	MSG_DBG("Flash erase size: 0x%.8x\n", priv->mtd_info.erasesize);

	backend->priv = priv;

out:
	return rc;

cleanup_priv:
	free(priv);
	return rc;
}

static void mtd_dev_free(struct backend *backend)
{
	struct mtd_data *priv = backend->priv;

	free(priv->flash_bmap);
	close(priv->fd);
	free(priv);
}

/* Flash Functions */

int flash_validate(struct mbox_context *context, uint32_t offset,
		   uint32_t size, bool ro)
{
	/* Default behaviour is all accesses are valid */
	return 0;
}

/*
 * mtd_is_erased() - Check if an offset into flash is erased
 * @context:	The mbox context pointer
 * @offset:	The flash offset to check (bytes)
 *
 * Return:	true if erased otherwise false
 */
static inline bool mtd_is_erased(struct backend *backend, uint32_t offset)
{
	const off_t index = offset >> backend->erase_size_shift;
	struct mtd_data *priv = backend->priv;

	return priv->flash_bmap[index] == FLASH_ERASED;
}

/*
 * mtd_set_bytemap() - Set the flash erased bytemap
 * @context:	The backend context pointer
 * @offset:	The flash offset to set (bytes)
 * @count:	Number of bytes to set
 * @val:	Value to set the bytemap to
 *
 * The flash bytemap only tracks the erased status at the erase block level so
 * this will update the erased state for an (or many) erase blocks
 *
 * Return:	0 if success otherwise negative error code
 */
static int mtd_set_bytemap(struct backend *backend, uint32_t offset,
			   uint32_t count, uint8_t val)
{
	struct mtd_data *priv = backend->priv;

	if ((offset + count) > backend->flash_size) {
		return -EINVAL;
	}

	MSG_DBG("Set flash bytemap @ 0x%.8x for 0x%.8x to %s\n", offset, count,
		val ? "ERASED" : "DIRTY");
	memset(priv->flash_bmap + (offset >> backend->erase_size_shift),
	       val,
	       align_up(count, 1 << backend->erase_size_shift) >>
		   backend->erase_size_shift);

	return 0;
}

/*
 * mtd_erase() - Erase the flash
 * @context:	The mbox context pointer
 * @offset:	The flash offset to erase (bytes)
 * @size:	The number of bytes to erase
 *
 * Return:	0 on success otherwise negative error code
 */
static int mtd_erase(struct backend *backend, uint32_t offset, uint32_t count)
{
	const uint32_t erase_size = 1 << backend->erase_size_shift;
	struct mtd_data *priv = backend->priv;
	struct erase_info_user erase_info = {0};
	int rc;

	MSG_DBG("Erase flash @ 0x%.8x for 0x%.8x\n", offset, count);

	/*
	 * We have an erased_bytemap for the flash so we want to avoid erasing
	 * blocks which we already know to be erased. Look for runs of blocks
	 * which aren't erased and erase the entire run at once to avoid how
	 * often we have to call the erase ioctl. If the block is already
	 * erased then there's nothing we need to do.
	 */
	while (count) {
		if (!mtd_is_erased(backend, offset)) { /* Need to erase */
			if (!erase_info.length) { /* Start of not-erased run */
				erase_info.start = offset;
			}
			erase_info.length += erase_size;
		} else if (erase_info.length) { /* Already erased|end of run? */
			/* Erase the previous run which just ended */
			MSG_DBG("Erase flash @ 0x%.8x for 0x%.8x\n",
				erase_info.start, erase_info.length);
			rc = ioctl(priv->fd, MEMERASE, &erase_info);
			if (rc < 0) {
				MSG_ERR("Couldn't erase flash at 0x%.8x\n",
					erase_info.start);
				return -errno;
			}
			/* Mark ERASED where we just erased */
			mtd_set_bytemap(backend, erase_info.start,
					erase_info.length, FLASH_ERASED);
			erase_info.start = 0;
			erase_info.length = 0;
		}

		offset += erase_size;
		count -= erase_size;
	}

	if (erase_info.length) {
		MSG_DBG("Erase flash @ 0x%.8x for 0x%.8x\n", erase_info.start,
			erase_info.length);
		rc = ioctl(priv->fd, MEMERASE, &erase_info);
		if (rc < 0) {
			MSG_ERR("Couldn't erase flash at 0x%.8x\n",
				erase_info.start);
			return -errno;
		}
		/* Mark ERASED where we just erased */
		mtd_set_bytemap(backend, erase_info.start, erase_info.length,
				FLASH_ERASED);
	}

	return 0;
}

#define CHUNKSIZE (64 * 1024)

/*
 * mtd_copy() - Copy data from the flash device into a provided buffer
 * @context:	The backend context pointer
 * @offset:	The flash offset to copy from (bytes)
 * @mem:	The buffer to copy into (must be of atleast 'size' bytes)
 * @size:	The number of bytes to copy
 * Return:	Number of bytes copied on success, otherwise negative error
 *		code. mtd_copy will copy at most 'size' bytes, but it may
 *		copy less.
 */
static int64_t mtd_copy(struct backend *backend, uint32_t offset,
			  void *mem, uint32_t size)
{
	struct mtd_data *priv = backend->priv;
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
		size_read = read(priv->fd, mem,
				 min_u32(CHUNKSIZE, size));
		if (size_read < 0) {
			MSG_ERR("Couldn't copy mtd into ram: %s\n",
				strerror(errno));
			return -errno;
		}

		size -= size_read;
		mem += size_read;
	} while (size && size_read);

	return size_read ? mem - start : -EIO;
}

/*
 * mtd_write() - Write the flash from a provided buffer
 * @context:	The mbox context pointer
 * @offset:	The flash offset to write to (bytes)
 * @buf:	The buffer to write from (must be of atleast size)
 * @size:	The number of bytes to write
 *
 * Return:	0 on success otherwise negative error code
 */
static int mtd_write(struct backend *backend, uint32_t offset, void *buf,
		       uint32_t count)
{
	struct mtd_data *priv = backend->priv;
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
			MSG_ERR("Couldn't write to flash, write lost: %s\n",
				strerror(errno));
			return -errno;
		}
		/* Mark *NOT* erased where we just wrote */
		mtd_set_bytemap(backend, offset + buf_offset, rc, FLASH_DIRTY);
		count -= rc;
		buf_offset += rc;
	}

	return 0;
}

/*
 * mtd_reset() - Reset the lpc bus mapping
 * @context:    The mbox context pointer
 *
 * Return:      A value from enum backend_reset_mode, otherwise a negative
 *		error code
 */
static int mtd_reset(struct backend *backend,
		     void *buf __attribute__((unused)),
		     uint32_t count __attribute__((unused)))
{
	return reset_lpc_flash;
}

static const struct backend_ops mtd_ops = {
	.init = mtd_dev_init,
	.free = mtd_dev_free,
	.copy = mtd_copy,
	.set_bytemap = mtd_set_bytemap,
	.erase = mtd_erase,
	.write = mtd_write,
	.validate = NULL,
	.reset = mtd_reset,
	.align_offset = NULL,
};

struct backend backend_get_mtd(void)
{
	struct backend be = {0};

	be.ops = &mtd_ops;

	return be;
}

int backend_probe_mtd(struct backend *master, const char *path)
{
	struct backend with;

	assert(master);
	with = backend_get_mtd();

	return backend_init(master, &with, (void *)path);
}
