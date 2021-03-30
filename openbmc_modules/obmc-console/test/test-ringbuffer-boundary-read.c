
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_boundary_read(void)
{
	uint8_t *out_buf, in_buf[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	struct ringbuffer_consumer *rbc;
	struct ringbuffer *rb;
	size_t len, pos;
	int rc;

	assert(sizeof(in_buf) * 2 > 10);

	rb = ringbuffer_init(10);
	rbc = ringbuffer_consumer_register(rb, ringbuffer_poll_nop, NULL);

	/* queue and dequeue, so our tail is non-zero */
	ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	ringbuffer_dequeue_commit(rbc, sizeof(in_buf));

	/* ensure we're getting the second batch of data back */
	in_buf[0] = 'A';

	/* the next queue should cross the end of the buffer */
	rc = ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	assert(!rc);

	/* dequeue everything we can */
	pos = 0;
	for (;;) {
		len = ringbuffer_dequeue_peek(rbc, pos, &out_buf);
		if (len == 0)
			break;
		assert(!memcmp(in_buf+pos, out_buf, len));
		pos += len;
	}
	assert(pos == sizeof(in_buf));

	ringbuffer_fini(rb);
}

int main(void)
{
	test_boundary_read();
	return EXIT_SUCCESS;
}
