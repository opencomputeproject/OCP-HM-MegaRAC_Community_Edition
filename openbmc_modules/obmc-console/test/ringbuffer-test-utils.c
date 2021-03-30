
struct rb_test_ctx {
	struct ringbuffer_consumer	*rbc;
	bool	ignore_poll;
	bool	force_only;
	int	count;
	uint8_t	*data;
	int	len;
};

void ringbuffer_test_context_init(struct rb_test_ctx *ctx)
{
	ctx->count = 0;
	ctx->data = NULL;
	ctx->len = 0;
	ctx->ignore_poll = false;
	ctx->force_only = false;
}

void ringbuffer_test_context_fini(struct rb_test_ctx *ctx)
{
	free(ctx->data);
}

enum ringbuffer_poll_ret ringbuffer_poll_nop(
		void *data __attribute__((unused)),
		size_t force_len __attribute__((unused)))
{
	return RINGBUFFER_POLL_OK;
}

enum ringbuffer_poll_ret ringbuffer_poll_append_all(void *data,
		size_t force_len)
{
	struct rb_test_ctx *ctx = data;
	size_t len, total_len;
	uint8_t *buf;

	if (ctx->ignore_poll)
		return RINGBUFFER_POLL_OK;

	if (ctx->force_only && !force_len)
		return RINGBUFFER_POLL_OK;

	ctx->count++;

	total_len = 0;
	for (;;) {
		len = ringbuffer_dequeue_peek(ctx->rbc, total_len, &buf);
		if (!len)
			break;

		if (ctx->force_only && total_len + len > force_len)
			len = force_len - total_len;

		ctx->data = realloc(ctx->data, ctx->len + len);
		memcpy(ctx->data + ctx->len, buf, len);
		ctx->len += len;
		total_len += len;

		if (ctx->force_only && total_len >= force_len)
			break;
	}
	ringbuffer_dequeue_commit(ctx->rbc, total_len);

	return RINGBUFFER_POLL_OK;
}

void ringbuffer_dump(struct ringbuffer *rb)
{
	struct ringbuffer_consumer *rbc;
	int i, j;

	printf("---- ringbuffer (%d consumer%s)\n", rb->n_consumers,
			rb->n_consumers == 1 ? "" : "s");

	for (i = 0; i < rb->size; i++) {
		bool has_consumer = false;
		const char *prefix = "";

		if (rb->tail == i)
			prefix = "tail=>";

		printf("%6s %02x", prefix, rb->buf[i]);
		for (j = 0; j < rb->n_consumers; j++) {
			rbc = rb->consumers[j];
			if (rbc->pos != i)
				continue;
			if (!has_consumer)
				printf(" <=");
			printf("c[%d],len=%zd ", j, ringbuffer_len(rbc));
			has_consumer = true;
		}
		printf("\n");
	}
}

