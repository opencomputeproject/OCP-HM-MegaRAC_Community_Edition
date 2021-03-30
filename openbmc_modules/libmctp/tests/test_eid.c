/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libmctp.h>

#include "test-utils.h"


struct test_ctx {
	struct mctp			*mctp;
	struct mctp_binding_test	*binding;
	int				rx_count;
	mctp_eid_t			src_eid;
};

static void test_rx(uint8_t eid, void *data, void *msg, size_t len)
{
	struct test_ctx *ctx = data;

	(void)msg;
	(void)len;

	ctx->rx_count++;
	ctx->src_eid = eid;
}

static void create_packet(struct mctp_hdr *pkt,
		mctp_eid_t src, mctp_eid_t dest)
{
	memset(pkt, 0, sizeof(*pkt));
	pkt->src = src;
	pkt->dest = dest;
	pkt->flags_seq_tag = MCTP_HDR_FLAG_SOM | MCTP_HDR_FLAG_EOM;
}

int main(void)
{
	struct test_ctx _ctx, *ctx = &_ctx;
	const mctp_eid_t local_eid = 8;
	const mctp_eid_t remote_eid = 9;
	const mctp_eid_t other_eid = 10;
	struct {
		struct mctp_hdr	hdr;
		uint8_t		payload[1];
	} pktbuf;

	mctp_test_stack_init(&ctx->mctp, &ctx->binding, local_eid);

	mctp_set_rx_all(ctx->mctp, test_rx, ctx);

	/* check a message addressed to us is received */
	ctx->rx_count = 0;

	create_packet(&pktbuf.hdr, remote_eid, local_eid);

	mctp_binding_test_rx_raw(ctx->binding, &pktbuf, sizeof(pktbuf));

	assert(ctx->rx_count == 1);
	assert(ctx->src_eid == remote_eid);

	/* check a message not addressed to us is not received */
	ctx->rx_count = 0;

	create_packet(&pktbuf.hdr, remote_eid, other_eid);

	mctp_binding_test_rx_raw(ctx->binding, &pktbuf, sizeof(pktbuf));

	assert(ctx->rx_count == 0);

	mctp_binding_test_destroy(ctx->binding);
	mctp_destroy(ctx->mctp);

	return EXIT_SUCCESS;
}
