
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_poll_force(void)
{
	uint8_t in_buf[] = { 'a', 'b', 'c', 'd', 'e', 'f', };
	struct rb_test_ctx _ctx, *ctx = &_ctx;
	struct ringbuffer *rb;
	int rc;

	ringbuffer_test_context_init(ctx);

	rb = ringbuffer_init(5);

	ctx->rbc = ringbuffer_consumer_register(rb,
			ringbuffer_poll_append_all, ctx);

	ctx->force_only = true;

	/* fill the ringbuffer */
	rc = ringbuffer_queue(rb, in_buf, 4);
	assert(!rc);

	assert(ctx->count == 0);

	/* add more data */
	rc = ringbuffer_queue(rb, in_buf + 4, 2);
	assert(!rc);

	/* we should have had a forced poll for the initial two bytes */
	assert(ctx->count == 1);
	assert(ctx->len == 2);
	assert(!memcmp(in_buf, ctx->data, 2));

	ringbuffer_fini(rb);
	ringbuffer_test_context_fini(ctx);
}

int main(void)
{
	test_poll_force();
	return EXIT_SUCCESS;
}
