/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */
/* Copyright (C) 2018 Evan Lojewski. */

#ifndef BACKEND_H
#define BACKEND_H

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <mtd/mtd-abi.h>

#define FLASH_DIRTY	0x00
#define FLASH_ERASED	0x01

/* Estimate as to how long (milliseconds) it takes to access a MB from flash */
#define FLASH_ACCESS_MS_PER_MB		8000

enum backend_reset_mode { reset_lpc_flash, reset_lpc_memory };

struct backend_ops;

struct backend {
	const struct backend_ops *ops;

	/* Backend private data */
	void *priv;

	/* Flash size from command line (bytes) */
	uint32_t flash_size;

	/* Erase size (as a shift) */
	uint32_t erase_size_shift;
	/* Block size (as a shift) */
	uint32_t block_size_shift;
};

struct backend_ops {
	/*
	 * init() - Main initialization function for backing device
	 * @context:	The backend context pointer
	 * @data:	Additional backend-implementation-specifc data
	 * Return:	Zero on success, otherwise negative error
	 */
	int 	(*init)(struct backend *backend, void *data);

	/*
	 * free() - Main teardown function for backing device
	 * @context:	The backend context pointer
	 */
	void 	(*free)(struct backend *backend);

	/*
	 * copy() - Copy data from the flash device into a provided buffer
	 * @context:	The mbox context pointer
	 * @offset:	The flash offset to copy from (bytes)
	 * @mem:	The buffer to copy into (must be of atleast 'size' bytes)
	 * @size:	The number of bytes to copy
	 * Return:	Number of bytes copied on success, otherwise negative error
	 *		code. flash_copy will copy at most 'size' bytes, but it may
	 *		copy less.
	 */
	int64_t (*copy)(struct backend *backend, uint32_t offset, void *mem,
			uint32_t size);

	/*
	 * set_bytemap() - Set the flash erased bytemap
	 * @context:	The mbox context pointer
	 * @offset:	The flash offset to set (bytes)
	 * @count:	Number of bytes to set
	 * @val:	Value to set the bytemap to
	 *
	 * The flash bytemap only tracks the erased status at the erase block level so
	 * this will update the erased state for an (or many) erase blocks
	 *
	 * Return:	0 if success otherwise negative error code
	 */
	 int 	(*set_bytemap)(struct backend *backend, uint32_t offset,
			       uint32_t count, uint8_t val);

	/*
	 * erase() - Erase the flash
	 * @context:	The backend context pointer
	 * @offset:	The flash offset to erase (bytes)
	 * @size:	The number of bytes to erase
	 *
	 * Return:	0 on success otherwise negative error code
	 */
	int 	(*erase)(struct backend *backend, uint32_t offset,
			 uint32_t count);
	/*
	 * write() - Write the flash from a provided buffer
	 * @context:	The backend context pointer
	 * @offset:	The flash offset to write to (bytes)
	 * @buf:	The buffer to write from (must be of atleast size)
	 * @size:	The number of bytes to write
	 *
	 * Return:	0 on success otherwise negative error code
	 */
	int 	(*write)(struct backend *backend, uint32_t offset, void *buf,
			 uint32_t count);

	/*
	 * validate() - Validates a requested window
	 * @context:	The backend context pointer
	 * @offset:	The requested flash offset
	 * @size:	The requested region size
	 * @ro:		The requested access type: True for read-only, false
	 *		for read-write
	 *
	 * Return:	0 on valid otherwise negative error code
	 */
	int 	(*validate)(struct backend *backend,
			    uint32_t offset, uint32_t size, bool ro);

	/*
	 * reset() - Ready the reserved memory for host startup
	 * @context:    The backend context pointer
	 * @buf:	The LPC reserved memory pointer
	 * @count	The size of the LPC reserved memory region
	 *
	 * Return:      0 on success otherwise negative error code
	 */
	int	(*reset)(struct backend *backend, void *buf, uint32_t count);

	/*
	 * align_offset() - Align the offset to avoid overlap
	 * @context:	The backend context pointer
	 * @offset:	The flash offset
	 * @window_size:The window size
	 *
	 * Return:      0 on success otherwise negative error code
	 */
	int	(*align_offset)(struct backend *backend, uint32_t *offset,
				uint32_t window_size);
};

/* Make this better */
static inline int backend_init(struct backend *master, struct backend *with,
			       void *data)
{
	int rc;

	assert(master);

	/* FIXME: A bit hacky? */
	with->flash_size = master->flash_size;
	*master = *with;

#ifndef NDEBUG
	/* Set some poison values to ensure backends init properly */
	master->erase_size_shift = 33;
	master->block_size_shift = 34;
#endif

	if (!(master && master->ops && master->ops->init))
		return -ENOTSUP;

	rc = master->ops->init(master, data);
	if (rc < 0)
		return rc;

	assert(master->erase_size_shift < 32);
	assert(master->block_size_shift < 32);

	return 0;
}

static inline void backend_free(struct backend *backend)
{
	assert(backend);

	if (backend->ops->free)
		backend->ops->free(backend);
}

static inline int64_t backend_copy(struct backend *backend,
				   uint32_t offset, void *mem, uint32_t size)
{
	assert(backend);
	assert(backend->ops->copy);
	return backend->ops->copy(backend, offset, mem, size);

}

static inline int backend_set_bytemap(struct backend *backend,
				      uint32_t offset, uint32_t count,
				      uint8_t val)
{
	assert(backend);

	if (backend->ops->set_bytemap)
		return backend->ops->set_bytemap(backend, offset, count, val);

	return 0;
}

static inline int backend_erase(struct backend *backend, uint32_t offset,
				uint32_t count)
{
	assert(backend);
	if (backend->ops->erase)
		return backend->ops->erase(backend, offset, count);

	return 0;
}

static inline int backend_write(struct backend *backend, uint32_t offset,
				void *buf, uint32_t count)
{
	assert(backend);
	assert(backend->ops->write);
	return backend->ops->write(backend, offset, buf, count);
}

static inline int backend_validate(struct backend *backend,
				   uint32_t offset, uint32_t size, bool ro)
{
	assert(backend);

	if (backend->ops->validate)
		return backend->ops->validate(backend, offset, size, ro);

	return 0;
}

static inline int backend_reset(struct backend *backend, void *buf,
				uint32_t count)
{
	assert(backend);
	assert(backend->ops->reset);
	return backend->ops->reset(backend, buf, count);
}


static inline int backend_align_offset(struct backend *backend, uint32_t *offset, uint32_t window_size)
{
	assert(backend);
	if (backend->ops->align_offset){
		return  backend->ops->align_offset(backend, offset, window_size);
	}else{
		/*
		 * It would be nice to align the offsets which we map to window
		 * size, this will help prevent overlap which would be an
		 * inefficient use of our reserved memory area (we would like
		 * to "cache" as much of the acutal flash as possible in
		 * memory). If we're protocol V1 however we must ensure the
		 * offset requested is exactly mapped.
		 */
		*offset &= ~(window_size - 1);
		return 0;
	}
}

struct backend backend_get_mtd(void);
int backend_probe_mtd(struct backend *master, const char *path);

struct backend backend_get_file(void);
int backend_probe_file(struct backend *master, const char *path);

/* Avoid dependency on vpnor/mboxd_pnor_partition_table.h */
struct vpnor_partition_paths;
#ifdef VIRTUAL_PNOR_ENABLED
struct backend backend_get_vpnor(void);

int backend_probe_vpnor(struct backend *master,
                        const struct vpnor_partition_paths *paths);
#else
static inline struct backend backend_get_vpnor(void)
{
	struct backend be = { 0 };

	return be;
}

static inline int backend_probe_vpnor(struct backend *master,
				      const struct vpnor_partition_paths *paths)
{
	return -ENOTSUP;
}
#endif

#endif /* BACKEND_H */
