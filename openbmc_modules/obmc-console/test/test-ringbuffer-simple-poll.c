
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "ringbuffer.c"
#include "ringbuffer-test-utils.c"

void test_simple_poll(void)
{
	uint8_t in_buf[] = { 'a', 'b', 'c' };
	struct rb_test_ctx _ctx, *ctx;
	struct ringbuffer *rb;
	int rc;

	ctx = &_ctx;
	ringbuffer_test_context_init(ctx);

	rb = ringbuffer_init(10);
	ctx->rbc = ringbuffer_consumer_register(rb,
			ringbuffer_poll_append_all, ctx);

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
	test_simple_poll();
	return EXIT_SUCCESS;
}
