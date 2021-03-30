/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define read __read
#include "config.c"
#include "console-socket.c"
#define main __main
#include "console-client.c"
#undef read
#undef main

struct test {
	enum esc_type	esc_type;
	union {
		struct ssh_esc_state ssh;
		struct str_esc_state str;
	} esc_state;
	const char	*in[4];
	int		n_in;
	const char	*exp_out;
	int		exp_rc;
};

struct test_ctx {
	struct console_client	client;
	struct test		*test;
	uint8_t			out[4096];
	int			cur_in;
	int			cur_out;
};


struct test tests[] = {
	{
		/* no escape code */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a"},
		.n_in		= 1,
		.exp_out	= "a",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* no escape code, multiple reads */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a", "b"},
		.n_in		= 2,
		.exp_out	= "ab",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* ssh escape in one read */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a\r~."},
		.n_in		= 1,
		.exp_out	= "a\r",
		.exp_rc		= PROCESS_ESC,
	},
	{
		/* ssh escape, partial ~ is not output. */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a\r~"},
		.n_in		= 1,
		.exp_out	= "a\r",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* ssh escape split into individual reads */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a", "\r", "~", "."},
		.n_in		= 4,
		.exp_out	= "a\r",
		.exp_rc		= PROCESS_ESC,
	},
	{
		/* ssh escape, escaped. */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a\r~~."},
		.n_in		= 1,
		.exp_out	= "a\r~.",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* ssh escape, escaped ~, and not completed. */
		.esc_type	= ESC_TYPE_SSH,
		.in		= {"a\r~~"},
		.n_in		= 1,
		.exp_out	= "a\r~",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* str escape, no match */
		.esc_type	= ESC_TYPE_STR,
		.esc_state	= { .str = { .str = (const uint8_t *)"c" } },
		.in		= {"ab"},
		.n_in		= 1,
		.exp_out	= "ab",
		.exp_rc		= PROCESS_EXIT,
	},
	{
		/* str escape, one byte, as one read */
		.esc_type	= ESC_TYPE_STR,
		.esc_state	= { .str = { .str = (const uint8_t *)"b" } },
		.in		= {"abc"},
		.n_in		= 1,
		.exp_out	= "ab",
		.exp_rc		= PROCESS_ESC,
	},
	{
		/* str escape, multiple bytes, as one read */
		.esc_type	= ESC_TYPE_STR,
		.esc_state	= { .str = { .str = (const uint8_t *)"bc" } },
		.in		= {"abcd"},
		.n_in		= 1,
		.exp_out	= "abc",
		.exp_rc		= PROCESS_ESC,
	},
	{
		/* str escape, multiple bytes, split over reads */
		.esc_type	= ESC_TYPE_STR,
		.esc_state	= { .str = { .str = (const uint8_t *)"bc" } },
		.in		= {"ab", "cd"},
		.n_in		= 2,
		.exp_out	= "abc",
		.exp_rc		= PROCESS_ESC,
	},
	{
		/* str escape, not matched due to intermediate data */
		.esc_type	= ESC_TYPE_STR,
		.esc_state	= { .str = { .str = (const uint8_t *)"ab" } },
		.in		= {"acb"},
		.n_in		= 1,
		.exp_out	= "acb",
		.exp_rc		= PROCESS_EXIT,
	},
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

struct test_ctx ctxs[ARRAY_SIZE(tests)];

int write_buf_to_fd(int fd, const uint8_t *buf, size_t len)
{
	struct test_ctx *ctx = &ctxs[fd];

	assert(ctx->cur_out + len <= sizeof(ctx->out));
	memcpy(ctx->out + ctx->cur_out, buf, len);
	ctx->cur_out += len;

	return 0;
}

ssize_t __read(int fd, void *buf, size_t len)
{
	struct test_ctx *ctx = &ctxs[fd];
	const char *inbuf;
	size_t inlen;

	if (ctx->cur_in >= ctx->test->n_in)
		return 0;

	inbuf = ctx->test->in[ctx->cur_in];
	inlen = strlen(inbuf);
	assert(inlen <= len);
	memcpy(buf, inbuf, inlen);
	ctx->cur_in++;
	return inlen;
}

void run_one_test(int idx, struct test *test, struct test_ctx *ctx)
{
	size_t exp_out_len;
	int rc;

	/* we store the index into the context array as a FD, so we
	 * can refer to it through the read & write callbacks.
	 */
	ctx->client.console_sd = idx;
	ctx->client.fd_in = idx;
	ctx->client.esc_type = test->esc_type;
	memcpy(&ctx->client.esc_state, &test->esc_state,
			sizeof(test->esc_state));
	ctx->test = test;

	for (;;) {
		rc = process_tty(&ctx->client);
		if (rc != PROCESS_OK)
			break;
	}

	exp_out_len = strlen(test->exp_out);

#ifdef DEBUG
	printf("got: rc %d %s(%d), exp: rc %d %s(%ld)\n",
			rc, ctx->out, ctx->cur_out,
			test->exp_rc, test->exp_out, exp_out_len);
	fflush(stdout);
#endif
	assert(rc == test->exp_rc);
	assert(exp_out_len == ctx->cur_out);
	assert(!memcmp(ctx->out, test->exp_out, exp_out_len));
}

int main(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tests); i++)
		run_one_test(i, &tests[i], &ctxs[i]);

	return EXIT_SUCCESS;
}
