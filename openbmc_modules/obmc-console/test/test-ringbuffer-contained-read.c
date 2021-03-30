
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_contained_read(void)
{
	uint8_t *out_buf, in_buf[] = { 'a', 'b', 'c' };
	struct ringbuffer_consumer *rbc;
	struct ringbuffer *rb;
	size_t len;
	int rc;

	rb = ringbuffer_init(10);
	rbc = ringbuffer_consumer_register(rb, ringbuffer_poll_nop, NULL);

	rc = ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	assert(!rc);

	len = ringbuffer_dequeue_peek(rbc, 0, &out_buf);
	assert(len == sizeof(in_buf));
	assert(!memcmp(in_buf, out_buf, sizeof(in_buf)));

	ringbuffer_fini(rb);
}

int main(void)
{
	test_contained_read();
	return EXIT_SUCCESS;
}
