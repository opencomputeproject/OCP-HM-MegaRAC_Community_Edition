/**
 * Copyright © 2016 IBM Corporation
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
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "console-server.h"

struct tty_handler {
	struct handler			handler;
	struct console			*console;
	struct ringbuffer_consumer	*rbc;
	struct poller			*poller;
	int				fd;
	int				fd_flags;
	bool				blocked;
};

static struct tty_handler *to_tty_handler(struct handler *handler)
{
	return container_of(handler, struct tty_handler, handler);
}

static void tty_set_fd_blocking(struct tty_handler *th, bool fd_blocking)
{
	int flags;

	flags = th->fd_flags & ~O_NONBLOCK;
	if (!fd_blocking)
		flags |= O_NONBLOCK;

	if (flags != th->fd_flags) {
		fcntl(th->fd, F_SETFL, flags);
		th->fd_flags = flags;
	}
}

/*
 * A "blocked" handler indicates that the last write returned EAGAIN
 * (==EWOULDBLOCK), so we know not to continue writing (for non-forced output),
 * as it'll just return EAGAIN again.
 *
 * Once we detect this, we watch for POLLOUT in the poller events. A
 * POLLOUT indicates that the fd is no longer blocking, so we clear
 * blocked mode and can continue writing.
 */
static void tty_set_blocked(struct tty_handler *th, bool blocked)
{
	int events;

	if (blocked == th->blocked)
		return;

	th->blocked = blocked;
	events = POLLIN;

	if (th->blocked)
		events |= POLLOUT;

	console_poller_set_events(th->console, th->poller, events);
}

static int tty_drain_queue(struct tty_handler *th, size_t force_len)
{
	size_t len, total_len;
	ssize_t wlen;
	uint8_t *buf;

	/* if we're forcing data, we need to clear non-blocking mode */
	if (force_len)
		tty_set_fd_blocking(th, true);

	/* no point writing, we'll just see -EAGAIN */
	else if (th->blocked)
		return 0;

	total_len = 0;

	for (;;) {
		len = ringbuffer_dequeue_peek(th->rbc, total_len, &buf);
		if (!len)
			break;

		/* write as little as possible while blocking */
		if (force_len && force_len < total_len + len)
			len = force_len - total_len;

		wlen = write(th->fd, buf, len);
		if (wlen < 0) {
			if (errno == EINTR)
				continue;
			if ((errno == EAGAIN || errno == EWOULDBLOCK)
					&& !force_len) {
				tty_set_blocked(th, true);
				break;
			}
			warn("failed writing to local tty; disabling");
			return -1;
		}

		total_len += wlen;

		if (force_len && total_len >= force_len)
			break;
	}

	ringbuffer_dequeue_commit(th->rbc, total_len);

	if (force_len)
		tty_set_fd_blocking(th, false);

	return 0;
}

static enum ringbuffer_poll_ret tty_ringbuffer_poll(void *arg, size_t force_len)
{
	struct tty_handler *th = arg;
	int rc;

	rc = tty_drain_queue(th, force_len);
	if (rc) {
		console_poller_unregister(th->console, th->poller);
		return RINGBUFFER_POLL_REMOVE;
	}

	return RINGBUFFER_POLL_OK;
}

static enum poller_ret tty_poll(struct handler *handler,
		int events, void __attribute__((unused)) *data)
{
	struct tty_handler *th = to_tty_handler(handler);
	uint8_t buf[4096];
	ssize_t len;
	int rc;

	if (events & POLLIN) {
		len = read(th->fd, buf, sizeof(buf));
		if (len <= 0)
			goto err;

		console_data_out(th->console, buf, len);
	}

	if (events & POLLOUT) {
		tty_set_blocked(th, false);
		rc = tty_drain_queue(th, 0);
		if (rc)
			goto err;
	}

	return POLLER_OK;

err:
	th->poller = NULL;
	close(th->fd);
	ringbuffer_consumer_unregister(th->rbc);
	return POLLER_REMOVE;
}

static int set_terminal_baud(struct tty_handler *th, const char *tty_name,
		speed_t speed) {
	struct termios term_options;

	if (tcgetattr(th->fd, &term_options) < 0) {
		warn("Can't get config for %s", tty_name);
		return -1;
	}

	if (cfsetspeed(&term_options, speed) < 0) {
		warn("Couldn't set speeds for %s", tty_name);
		return -1;
	}

	if (tcsetattr(th->fd, TCSAFLUSH, &term_options) < 0) {
		warn("Couldn't commit terminal options for %s", tty_name);
		return -1;
	}

	return 0;
}

static int make_terminal_raw(struct tty_handler *th, const char *tty_name) {
	struct termios term_options;

	if (tcgetattr(th->fd, &term_options) < 0) {
		warn("Can't get config for %s", tty_name);
		return -1;
	}

	/* Disable various input and output processing including character
	 * translation, line edit (canonical) mode, flow control, and special signal
	 * generating characters. */
	cfmakeraw(&term_options);

	if (tcsetattr(th->fd, TCSAFLUSH, &term_options) < 0) {
		warn("Couldn't commit terminal options for %s", tty_name);
		return -1;
	}
	printf("Set %s for raw byte handling\n", tty_name);

	return 0;
}

static int tty_init(struct handler *handler, struct console *console,
		struct config *config __attribute__((unused)))
{
	struct tty_handler *th = to_tty_handler(handler);
	speed_t desired_speed;
	const char *tty_name;
	const char *tty_baud;
	char *tty_path;
	int rc;

	tty_name = config_get_value(config, "local-tty");
	if (!tty_name)
		return -1;

	rc = asprintf(&tty_path, "/dev/%s", tty_name);
	if (!rc)
		return -1;

	th->fd = open(tty_path, O_RDWR | O_NONBLOCK);
	if (th->fd < 0) {
		warn("Can't open %s; disabling local tty", tty_name);
		free(tty_path);
		return -1;
	}

	free(tty_path);
	th->fd_flags = fcntl(th->fd, F_GETFL, 0);

	tty_baud = config_get_value(config, "local-tty-baud");
	if (tty_baud != NULL) {
		rc = config_parse_baud(&desired_speed, tty_baud);
		if (rc) {
			fprintf(stderr, "%s is not a valid baud rate\n",
				tty_baud);
		} else {
			rc = set_terminal_baud(th, tty_name, desired_speed);
			if (rc)
				fprintf(stderr, "Couldn't set baud rate for %s to %s\n",
					tty_name, tty_baud);
		}
	}

	if (make_terminal_raw(th, tty_name) != 0)
		fprintf(stderr, "Couldn't make %s a raw terminal\n", tty_name);

	th->poller = console_poller_register(console, handler, tty_poll, NULL,
			th->fd, POLLIN, NULL);
	th->console = console;
	th->rbc = console_ringbuffer_consumer_register(console,
			tty_ringbuffer_poll, th);

	return 0;
}

static void tty_fini(struct handler *handler)
{
	struct tty_handler *th = to_tty_handler(handler);
	if (th->poller)
		console_poller_unregister(th->console, th->poller);
	close(th->fd);
}

static int tty_baudrate(struct handler *handler, speed_t baudrate)
{
	const char *tty_name = "local-tty";
	struct tty_handler *th = to_tty_handler(handler);

	if (baudrate == 0) {
		return -1;
	}

	if (set_terminal_baud(th, tty_name, baudrate) != 0) {
		fprintf(stderr, "Couldn't set baud rate for %s to %d\n",
			tty_name, baudrate);
		return -1;
	}
	return 0;
}

static struct tty_handler tty_handler = {
	.handler = {
		.name		= "tty",
		.init		= tty_init,
		.fini		= tty_fini,
		.baudrate	= tty_baudrate,
	},
};

console_handler_register(&tty_handler.handler);

