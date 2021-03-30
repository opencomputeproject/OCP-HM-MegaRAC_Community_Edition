/**
 * Copyright © 2017 IBM Corporation
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

#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "console-server.h"

#define min(a,b) ({				\
		const typeof(a) _a = (a);	\
		const typeof(b) _b = (b);	\
		_a < _b ? _a : _b;		\
	})

struct ringbuffer {
	uint8_t				*buf;
	size_t				size;
	size_t				tail;
	struct ringbuffer_consumer	**consumers;
	int				n_consumers;
};

struct ringbuffer_consumer {
	struct ringbuffer		*rb;
	ringbuffer_poll_fn_t		poll_fn;
	void				*poll_data;
	size_t				pos;
};

struct ringbuffer *ringbuffer_init(size_t size)
{
	struct ringbuffer *rb;

	rb = malloc(sizeof(*rb) + size);
	if (!rb)
		return NULL;

	memset(rb, 0, sizeof(*rb));
	rb->size = size;
	rb->buf = (void *)(rb + 1);

	return rb;
}

void ringbuffer_fini(struct ringbuffer *rb)
{
	while (rb->n_consumers)
		ringbuffer_consumer_unregister(rb->consumers[0]);
	free(rb);
}

struct ringbuffer_consumer *ringbuffer_consumer_register(struct ringbuffer *rb,
		ringbuffer_poll_fn_t fn, void *data)
{
	struct ringbuffer_consumer *rbc;
	int n;

	rbc = malloc(sizeof(*rbc));
	rbc->rb = rb;
	rbc->poll_fn = fn;
	rbc->poll_data = data;
	rbc->pos = rb->tail;

	n = rb->n_consumers++;
	rb->consumers = realloc(rb->consumers,
			sizeof(*rb->consumers) * rb->n_consumers);
	rb->consumers[n] = rbc;

	return rbc;
}

void ringbuffer_consumer_unregister(struct ringbuffer_consumer *rbc)
{
	struct ringbuffer *rb = rbc->rb;
	int i;

	for (i = 0; i < rb->n_consumers; i++)
		if (rb->consumers[i] == rbc)
			break;

	assert(i < rb->n_consumers);

	rb->n_consumers--;

	memmove(&rb->consumers[i], &rb->consumers[i+1],
			sizeof(*rb->consumers)	* (rb->n_consumers - i));

	rb->consumers = realloc(rb->consumers,
			sizeof(*rb->consumers) * rb->n_consumers);

	free(rbc);
}

size_t ringbuffer_len(struct ringbuffer_consumer *rbc)
{
	if (rbc->pos <= rbc->rb->tail)
		return rbc->rb->tail - rbc->pos;
	else
		return rbc->rb->tail + rbc->rb->size - rbc->pos;
}

static size_t ringbuffer_space(struct ringbuffer_consumer *rbc)
{
	return rbc->rb->size - ringbuffer_len(rbc) - 1;
}

static int ringbuffer_consumer_ensure_space(
		struct ringbuffer_consumer *rbc, size_t len)
{
	enum ringbuffer_poll_ret prc;
	int force_len;

	if (ringbuffer_space(rbc) >= len)
		return 0;

	force_len = len - ringbuffer_space(rbc);

	prc = rbc->poll_fn(rbc->poll_data, force_len);
	if (prc != RINGBUFFER_POLL_OK)
		return -1;

	return 0;
}

int ringbuffer_queue(struct ringbuffer *rb, uint8_t *data, size_t len)
{
	struct ringbuffer_consumer *rbc;
	size_t wlen;
	int i, rc;

	if (len >= rb->size)
		return -1;

	if (len == 0)
		return 0;

	/* Ensure there is at least len bytes of space available.
	 *
	 * If a client doesn't have sufficient space, perform a blocking write
	 * (by calling ->poll_fn with force_len) to create it.
	 */
	for (i = 0; i < rb->n_consumers; i++) {
		rbc = rb->consumers[i];

		rc = ringbuffer_consumer_ensure_space(rbc, len);
		if (rc) {
			ringbuffer_consumer_unregister(rbc);
			i--;
			continue;
		}

		assert(ringbuffer_space(rbc) >= len);
	}

	/* Now that we know we have enough space, add new data to tail */
	wlen = min(len, rb->size - rb->tail);
	memcpy(rb->buf + rb->tail, data, wlen);
	rb->tail = (rb->tail + wlen) % rb->size;
	len -= wlen;
	data += wlen;

	memcpy(rb->buf, data, len);
	rb->tail += len;


	/* Inform consumers of new data in non-blocking mode, by calling
	 * ->poll_fn with 0 force_len */
	for (i = 0; i < rb->n_consumers; i++) {
		enum ringbuffer_poll_ret prc;

		rbc = rb->consumers[i];
		prc = rbc->poll_fn(rbc->poll_data, 0);
		if (prc == RINGBUFFER_POLL_REMOVE) {
			ringbuffer_consumer_unregister(rbc);
			i--;
		}
	}

	return 0;
}

size_t ringbuffer_dequeue_peek(struct ringbuffer_consumer *rbc, size_t offset,
		uint8_t **data)
{
	struct ringbuffer *rb = rbc->rb;
	size_t pos;
	size_t len;

	if (offset >= ringbuffer_len(rbc))
		return 0;

	pos = (rbc->pos + offset) % rb->size;
	if (pos <= rb->tail)
		len = rb->tail - pos;
	else
		len = rb->size - pos;

	*data = rb->buf + pos;
	return len;
}

int ringbuffer_dequeue_commit(struct ringbuffer_consumer *rbc, size_t len)
{
	assert(len <= ringbuffer_len(rbc));
	rbc->pos = (rbc->pos + len) % rbc->rb->size;
	return 0;
}
