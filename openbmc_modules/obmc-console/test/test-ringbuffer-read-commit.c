
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_read_commit(void)
{
	uint8_t *out_buf, in_buf[] = { 'a', 'b', 'c', };
	struct ringbuffer_consumer *rbc;
	struct ringbuffer *rb;
	size_t len;

	rb = ringbuffer_init(10);
	rbc = ringbuffer_consumer_register(rb, ringbuffer_poll_nop, NULL);

	ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	len = ringbuffer_dequeue_peek(rbc, 0, &out_buf);

	ringbuffer_dequeue_commit(rbc, len);
	len = ringbuffer_dequeue_peek(rbc, 0, &out_buf);
	assert(len == 0);

	ringbuffer_fini(rb);
}

int main(void)
{
	test_read_commit();
	return EXIT_SUCCESS;
}
