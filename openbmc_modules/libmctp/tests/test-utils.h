/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#ifndef _MCTP_TESTS_TEST_UTILS_H
#define _MCTP_TESTS_TEST_UTILS_H

#include <libmctp.h>
#include <container_of.h>

/* test binding implementation */

/* standard binding interface */
struct mctp_binding_test *mctp_binding_test_init(void);
void mctp_binding_test_destroy(struct mctp_binding_test *test);
void mctp_binding_test_register_bus(struct mctp_binding_test *binding,
		struct mctp *mctp, mctp_eid_t eid);

/* internal test binding interface */
void mctp_binding_test_rx_raw(struct mctp_binding_test *test,
		void *buf, size_t len);

/* gerneral utility functions */

/* create a MCTP stack, and add a test binding, using the specified EID */
void mctp_test_stack_init(struct mctp **mctp,
		struct mctp_binding_test **binding,
		mctp_eid_t eid);

#endif /* _MCTP_TESTS_TEST_UTILS_H */
