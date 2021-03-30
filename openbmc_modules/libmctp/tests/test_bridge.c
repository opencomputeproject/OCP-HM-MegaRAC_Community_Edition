/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libmctp.h>
#include <libmctp-alloc.h>

#include "test-utils.h"

struct mctp_binding_bridge {
	struct mctp_binding	binding;
	int			rx_count;
	int			tx_count;
	uint8_t			last_pkt_data;
};

struct test_ctx {
	struct mctp			*mctp;
	struct mctp_binding_bridge	*bindings[2];
};

static int mctp_binding_bridge_tx(struct mctp_binding *b,
		struct mctp_pktbuf *pkt)
{
	struct mctp_binding_bridge *binding = container_of(b,
			struct mctp_binding_bridge, binding);

	binding->tx_count++;
	assert(mctp_pktbuf_size(pkt) == sizeof(struct mctp_hdr) + 1);
	binding->last_pkt_data = *(uint8_t *)mctp_pktbuf_data(pkt);

	return 0;
}

static void mctp_binding_bridge_rx(struct mctp_binding_bridge *binding,
		uint8_t key)
{
	struct mctp_pktbuf *pkt;
	struct mctp_hdr *hdr;
	uint8_t *buf;

	pkt = mctp_pktbuf_alloc(&binding->binding,
			sizeof(struct mctp_hdr) + 1);
	assert(pkt);

	hdr = mctp_pktbuf_hdr(pkt);
	hdr->flags_seq_tag = MCTP_HDR_FLAG_SOM | MCTP_HDR_FLAG_EOM;

	/* arbitrary src/dest, as we're bridging */
	hdr->src = 1;
	hdr->dest = 2;

	buf = mctp_pktbuf_data(pkt);
	*buf = key;

	binding->rx_count++;
	mctp_bus_rx(&binding->binding, pkt);
}

static struct mctp_binding_bridge *mctp_binding_bridge_init(void)
{
	struct mctp_binding_bridge *binding;

	binding = __mctp_alloc(sizeof(*binding));
	memset(binding, 0, sizeof(*binding));
	binding->binding.name = "test";
	binding->binding.version = 1;
	binding->binding.tx = mctp_binding_bridge_tx;
	binding->binding.pkt_size = MCTP_PACKET_SIZE(MCTP_BTU);
	binding->binding.pkt_pad = 0;
	return binding;
}

int main(void)
{
	struct test_ctx _ctx, *ctx = &_ctx;

	ctx->mctp = mctp_init();
	ctx->bindings[0] = mctp_binding_bridge_init();
	ctx->bindings[1] = mctp_binding_bridge_init();

	mctp_bridge_busses(ctx->mctp,
			&ctx->bindings[0]->binding,
			&ctx->bindings[1]->binding);

	mctp_binding_set_tx_enabled(&ctx->bindings[0]->binding, true);
	mctp_binding_set_tx_enabled(&ctx->bindings[1]->binding, true);

	mctp_binding_bridge_rx(ctx->bindings[0], 0xaa);
	assert(ctx->bindings[0]->tx_count == 0);
	assert(ctx->bindings[1]->tx_count == 1);
	assert(ctx->bindings[1]->last_pkt_data == 0xaa);

	mctp_binding_bridge_rx(ctx->bindings[1], 0x55);
	assert(ctx->bindings[1]->tx_count == 1);
	assert(ctx->bindings[0]->tx_count == 1);
	assert(ctx->bindings[0]->last_pkt_data == 0x55);

	__mctp_free(ctx->bindings[1]);
	__mctp_free(ctx->bindings[0]);
	mctp_destroy(ctx->mctp);

	return EXIT_SUCCESS;
}
