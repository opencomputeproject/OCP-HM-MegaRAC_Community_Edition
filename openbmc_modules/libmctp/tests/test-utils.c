/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#include <string.h>
#include <assert.h>

#include <libmctp.h>
#include <libmctp-alloc.h>

#include "test-utils.h"

struct mctp_binding_test {
	struct mctp_binding	binding;
};

static int mctp_binding_test_tx(struct mctp_binding *b __attribute__((unused)),
		struct mctp_pktbuf *pkt __attribute__((unused)))
{
	/* we are not expecting TX packets */
	assert(0);
	return 0;
}

struct mctp_binding_test *mctp_binding_test_init(void)
{
	struct mctp_binding_test *test;

	test = __mctp_alloc(sizeof(*test));
	memset(test, '\0', sizeof(*test));
	test->binding.name = "test";
	test->binding.version = 1;
	test->binding.tx = mctp_binding_test_tx;
	test->binding.pkt_size = MCTP_PACKET_SIZE(MCTP_BTU);
	test->binding.pkt_pad = 0;
	return test;
}

void mctp_binding_test_destroy(struct mctp_binding_test *test)
{
	__mctp_free(test);
}

void mctp_binding_test_rx_raw(struct mctp_binding_test *test,
		void *buf, size_t len)
{
	struct mctp_pktbuf *pkt;

	pkt = mctp_pktbuf_alloc(&test->binding, len);
	assert(pkt);
	memcpy(mctp_pktbuf_hdr(pkt), buf, len);
	mctp_bus_rx(&test->binding, pkt);
}

void mctp_binding_test_register_bus(struct mctp_binding_test *binding,
		struct mctp *mctp, mctp_eid_t eid)
{
	mctp_register_bus(mctp, &binding->binding, eid);
}

void mctp_test_stack_init(struct mctp **mctp,
		struct mctp_binding_test **binding,
		mctp_eid_t eid)
{
	*mctp = mctp_init();
	assert(*mctp);

	*binding = mctp_binding_test_init();
	assert(*binding);

	mctp_binding_test_register_bus(*binding, *mctp, eid);
}
