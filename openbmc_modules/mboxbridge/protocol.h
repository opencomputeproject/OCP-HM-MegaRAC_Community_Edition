/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */

#ifndef PROTOCOL_H
#define PROTOCOL_H

struct mbox_context;
struct transport_ops;

/*
 * The GET_MBOX_INFO command is special as it can change the interface based on
 * negotiation. As such we need to accommodate all response types
 */
struct protocol_get_info {
	struct {
		uint8_t api_version;
	} req;
	struct {
		uint8_t api_version;
		union {
			struct {
				uint16_t read_window_size;
				uint16_t write_window_size;
			} v1;
			struct {
				uint8_t block_size_shift;
				uint16_t timeout;
			} v2;
		};
	} resp;
};

struct protocol_get_flash_info {
	struct {
		union {
			struct {
				uint32_t flash_size;
				uint32_t erase_size;
			} v1;
			struct {
				uint16_t flash_size;
				uint16_t erase_size;
			} v2;
		};
	} resp;
};

struct protocol_create_window {
	struct {
		uint16_t offset;
		uint16_t size;
		uint8_t id;
		bool ro;
	} req;
	struct {
		uint16_t lpc_address;
		uint16_t size;
		uint16_t offset;
	} resp;
};

struct protocol_mark_dirty {
	struct {
		union {
			struct {
				uint16_t offset;
				uint32_t size;
			} v1;
			struct {
				uint16_t offset;
				uint16_t size;
			} v2;
		};
	} req;
};

struct protocol_erase {
	struct {
		uint16_t offset;
		uint16_t size;
	} req;
};

struct protocol_flush {
	struct {
		uint16_t offset;
		uint32_t size;
	} req;
};

struct protocol_close {
	struct {
		uint8_t flags;
	} req;
};

struct protocol_ack {
	struct {
		uint8_t flags;
	} req;
};

struct protocol_ops {
	int (*reset)(struct mbox_context *context);
	int (*get_info)(struct mbox_context *context,
			struct protocol_get_info *io);
	int (*get_flash_info)(struct mbox_context *context,
			      struct protocol_get_flash_info *io);
	int (*create_window)(struct mbox_context *context,
			     struct protocol_create_window *io);
	int (*mark_dirty)(struct mbox_context *context,
			  struct protocol_mark_dirty *io);
	int (*erase)(struct mbox_context *context, struct protocol_erase *io);
	int (*flush)(struct mbox_context *context, struct protocol_flush *io);
	int (*close)(struct mbox_context *context, struct protocol_close *io);
	int (*ack)(struct mbox_context *context, struct protocol_ack *io);
};

int protocol_init(struct mbox_context *context);
void protocol_free(struct mbox_context *context);

/* Sneaky reset: Don't tell the host */
int __protocol_reset(struct mbox_context *context);

/* Noisy reset: Tell the host */
int protocol_reset(struct mbox_context *context);

int protocol_events_put(struct mbox_context *context,
			const struct transport_ops *ops);
int protocol_events_set(struct mbox_context *context, uint8_t bmc_event);
int protocol_events_clear(struct mbox_context *context, uint8_t bmc_event);

#endif /* PROTOCOL_H */
