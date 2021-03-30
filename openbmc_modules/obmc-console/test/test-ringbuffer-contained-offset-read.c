
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_contained_offset_read(void)
{
	uint8_t *out_buf, in_buf[] = { 'a', 'b', 'c' };
	struct ringbuffer_consumer *rbc;
	struct ringbuffer *rb;
	size_t len;
	int rc, i;

	rb = ringbuffer_init(10);
	rbc = ringbuffer_consumer_register(rb, ringbuffer_poll_nop, NULL);

	rc = ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	assert(!rc);

	/* test all possible offsets */
	for (i = 0; i <= sizeof(in_buf); i++) {
		len = ringbuffer_dequeue_peek(rbc, i, &out_buf);
		assert(len == sizeof(in_buf) - i);
		if (len)
			assert(!memcmp(in_buf + i, out_buf, len));
	}

	ringbuffer_fini(rb);
}

int main(void)
{
	test_contained_offset_read();
	return EXIT_SUCCESS;
}
