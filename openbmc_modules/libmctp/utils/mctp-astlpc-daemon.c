/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include "libmctp.h"
#include "libmctp-astlpc.h"

static const mctp_eid_t local_eid = 8;
static const mctp_eid_t remote_eid = 9;

static const uint8_t echo_req = 1;
static const uint8_t echo_resp = 2;

struct ctx {
	struct mctp	*mctp;
};

static void tx_message(struct ctx *ctx, mctp_eid_t eid, void *msg, size_t len)
{
	uint8_t type;

	type = len > 0 ? *(uint8_t *)(msg) : 0x00;

	fprintf(stderr, "TX: dest EID 0x%02x: %zd bytes, first byte [0x%02x]\n",
			eid, len, type);
	mctp_message_tx(ctx->mctp, eid, msg, len);
}

static void rx_message(uint8_t eid, void *data, void *msg, size_t len)
{
	struct ctx *ctx = data;
	uint8_t type;

	type = len > 0 ? *(uint8_t *)(msg) : 0x00;

	fprintf(stderr, "RX: src EID 0x%02x: %zd bytes, first byte [0x%02x]\n",
			eid, len, type);

	if (type == echo_req) {
		*(uint8_t *)(msg) = echo_resp;
		tx_message(ctx, eid, msg, len);
	}
}

int main(void)
{
	struct mctp_binding_astlpc *astlpc;
	struct mctp *mctp;
	struct ctx *ctx, _ctx;
	int rc;

	mctp = mctp_init();
	assert(mctp);

	astlpc = mctp_astlpc_init_fileio();
	assert(astlpc);

	mctp_astlpc_register_bus(astlpc, mctp, local_eid);

	ctx = &_ctx;
	ctx->mctp = mctp;

	mctp_set_rx_all(mctp, rx_message, ctx);

	for (;;) {
#if 0

		struct pollfd pollfds[2];

		pollfds[0].fd = STDIN_FILENO;
		pollfds[0].events = POLLIN;

		pollfds[1].fd = mctp_astlpc_get_fd(astlpc);
		pollfds[1].events = POLLIN;

		rc = poll(pollfds, 2, -1);
		if (rc < 0)
			err(EXIT_FAILURE, "poll");

		if (pollfds[0].revents) {
			uint8_t buf[1024];
			rc = read(STDIN_FILENO, buf, sizeof(buf));
			if (rc == 0)
				break;
			if (rc < 0)
				err(EXIT_FAILURE, "read");
			tx_message(ctx, remote_eid, buf, rc);
		}

		if (pollfds[1].revents) {
			rc = mctp_astlpc_poll(astlpc);
			if (rc)
				break;
		}
#else
		(void)remote_eid;
		rc = mctp_astlpc_poll(astlpc);
		if (rc)
			break;

#endif

	}

	return EXIT_SUCCESS;

}
