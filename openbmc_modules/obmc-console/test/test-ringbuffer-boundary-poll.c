
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_boundary_poll(void)
{
	uint8_t in_buf[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	struct rb_test_ctx _ctx, *ctx = &_ctx;
	struct ringbuffer *rb;
	int rc;

	ringbuffer_test_context_init(ctx);

	rb = ringbuffer_init(10);

	ctx->rbc = ringbuffer_consumer_register(rb,
			ringbuffer_poll_append_all, ctx);

	/* don't consume initial data in the poll callback */
	ctx->ignore_poll = true;

	/* queue and dequeue, so our tail is non-zero */
	ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	ringbuffer_dequeue_commit(ctx->rbc, sizeof(in_buf));

	/* start queueing data */
	ctx->ignore_poll = false;

	/* ensure we're getting the second batch of data back */
	in_buf[0] = 'A';

	rc = ringbuffer_queue(rb, in_buf, sizeof(in_buf));
	assert(!rc);

	assert(ctx->count == 1);
	assert(ctx->len == sizeof(in_buf));
	assert(!memcmp(in_buf, ctx->data, ctx->len));

	ringbuffer_fini(rb);
	ringbuffer_test_context_fini(ctx);
}

int main(void)
{
	test_boundary_poll();
	return EXIT_SUCCESS;
}
