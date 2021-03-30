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

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "console-server.h"

#define EXIT_ESCAPE 2

enum process_rc {
	PROCESS_OK = 0,
	PROCESS_ERR,
	PROCESS_EXIT,
	PROCESS_ESC,
};

enum esc_type {
	ESC_TYPE_SSH,
	ESC_TYPE_STR,
};

struct ssh_esc_state {
	uint8_t state;
};

struct str_esc_state {
	const uint8_t *str;
	size_t pos;
};

struct console_client {
	int		console_sd;
	int		fd_in;
	int		fd_out;
	bool		is_tty;
	struct termios	orig_termios;
	enum esc_type	esc_type;
	union {
		struct ssh_esc_state ssh;
		struct str_esc_state str;
	} esc_state;
};

static enum process_rc process_ssh_tty(
		struct console_client *client, const uint8_t *buf, size_t len)
{
	struct ssh_esc_state *esc_state = &client->esc_state.ssh;
	const uint8_t *out_buf = buf;
	int rc;

	for (size_t i = 0; i < len; ++i) {
		switch (buf[i])
		{
		case '.':
			if (esc_state->state != '~') {
				esc_state->state = '\0';
				break;
			}
			return PROCESS_ESC;
		case '~':
			if (esc_state->state != '\r') {
				esc_state->state = '\0';
				break;
			}
			esc_state->state = '~';
			/* We need to print everything to skip the tilde */
			rc = write_buf_to_fd(
				client->console_sd, out_buf, i-(out_buf-buf));
			if (rc < 0)
				return PROCESS_ERR;
			out_buf = &buf[i+1];
			break;
		case '\r':
			esc_state->state = '\r';
			break;
		default:
			esc_state->state = '\0';
		}
	}

	rc = write_buf_to_fd(client->console_sd, out_buf, len-(out_buf-buf));
	return rc < 0 ? PROCESS_ERR : PROCESS_OK;
}

static enum process_rc process_str_tty(
		struct console_client *client, const uint8_t *buf, size_t len)
{
	struct str_esc_state *esc_state = &client->esc_state.str;
	enum process_rc prc = PROCESS_OK;
	size_t i;

	for (i = 0; i < len; ++i) {
		if (buf[i] == esc_state->str[esc_state->pos])
			esc_state->pos++;
		else
			esc_state->pos = 0;

		if (esc_state->str[esc_state->pos] == '\0') {
			prc = PROCESS_ESC;
			++i;
			break;
		}
	}

	if (write_buf_to_fd(client->console_sd, buf, i) < 0)
		return PROCESS_ERR;
	return prc;
}

static enum process_rc process_tty(struct console_client *client)
{
	uint8_t buf[4096];
	ssize_t len;

	len = read(client->fd_in, buf, sizeof(buf));
	if (len < 0)
		return PROCESS_ERR;
	if (len == 0)
		return PROCESS_EXIT;

	switch (client->esc_type)
	{
	case ESC_TYPE_SSH:
		return process_ssh_tty(client, buf, len);
	case ESC_TYPE_STR:
		return process_str_tty(client, buf, len);
	default:
		return PROCESS_ERR;
	}
}


static int process_console(struct console_client *client)
{
	uint8_t buf[4096];
	int len, rc;

	len = read(client->console_sd, buf, sizeof(buf));
	if (len < 0) {
		warn("Can't read from server");
		return PROCESS_ERR;
	}
	if (len == 0) {
		fprintf(stderr, "Connection closed\n");
		return PROCESS_EXIT;
	}

	rc = write_buf_to_fd(client->fd_out, buf, len);
	return rc ? PROCESS_ERR : PROCESS_OK;
}

/*
 * Setup our local file descriptors for IO: use stdin/stdout, and if we're on a
 * TTY, put it in canonical mode
 */
static int client_tty_init(struct console_client *client)
{
	struct termios termios;
	int rc;

	client->fd_in = STDIN_FILENO;
	client->fd_out = STDOUT_FILENO;
	client->is_tty = isatty(client->fd_in);

	if (!client->is_tty)
		return 0;

	rc = tcgetattr(client->fd_in, &termios);
	if (rc) {
		warn("Can't get terminal attributes for console");
		return -1;
	}
	memcpy(&client->orig_termios, &termios, sizeof(client->orig_termios));
	cfmakeraw(&termios);

	rc = tcsetattr(client->fd_in, TCSANOW, &termios);
	if (rc) {
		warn("Can't set terminal attributes for console");
		return -1;
	}

	return 0;
}

static int client_init(struct console_client *client, const char *socket_id)
{
	struct sockaddr_un addr;
	socket_path_t path;
	ssize_t len;
	int rc;

	client->console_sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (!client->console_sd) {
		warn("Can't open socket");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	len = console_socket_path(&addr, socket_id);
	if (len < 0) {
		if (errno)
			warn("Failed to configure socket: %s", strerror(errno));
		else
			warn("Socket name length exceeds buffer limits");
		goto cleanup;
	}

	rc = connect(client->console_sd, (struct sockaddr *)&addr,
			sizeof(addr) - sizeof(addr.sun_path) + len);
	if (!rc)
		return 0;

	console_socket_path_readable(&addr, len, path);
	warn("Can't connect to console server '@%s'", path);
cleanup:
	close(client->console_sd);
	return -1;
}

static void client_fini(struct console_client *client)
{
	if (client->is_tty)
		tcsetattr(client->fd_in, TCSANOW, &client->orig_termios);
	close(client->console_sd);
}

int main(int argc, char *argv[])
{
	struct console_client _client, *client;
	struct pollfd pollfds[2];
	enum process_rc prc = PROCESS_OK;
	const char *config_path = NULL;
	struct config *config = NULL;
	const char *socket_id = NULL;
	const uint8_t *esc = NULL;
	int rc;

	client = &_client;
	memset(client, 0, sizeof(*client));
	client->esc_type = ESC_TYPE_SSH;

	for (;;) {
		rc = getopt(argc, argv, "c:e:i:");
		if (rc == -1)
			break;

		switch (rc) {
		case 'c':
			if (optarg[0] == '\0') {
				fprintf(stderr, "Config str cannot be empty\n");
				return EXIT_FAILURE;
			}
			config_path = optarg;
			break;
		case 'e':
			if (optarg[0] == '\0') {
				fprintf(stderr, "Escape str cannot be empty\n");
				return EXIT_FAILURE;
			}
			esc = (const uint8_t*)optarg;
			break;
		case 'i':
			if (optarg[0] == '\0') {
				fprintf(stderr, "Socket ID str cannot be empty\n");
				return EXIT_FAILURE;
			}
			socket_id = optarg;
			break;
		default:
			fprintf(stderr,
				"Usage: %s "
				"[-e <escape sequence>]"
				"[-i <socket ID>]"
				"[-c <config>]\n",
				argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (config_path) {
		config = config_init(config_path);
		if (!config) {
			warnx("Can't read configuration, exiting.");
			return EXIT_FAILURE;
		}

		if (!esc)
			esc = (const uint8_t *)config_get_value(config, "escape-sequence");

		if (!socket_id)
			socket_id = config_get_value(config, "socket-id");
	}

	if (esc) {
		client->esc_type = ESC_TYPE_STR;
		client->esc_state.str.str = esc;
	}

	rc = client_init(client, socket_id);
	if (rc)
		goto out_config_fini;

	rc = client_tty_init(client);
	if (rc)
		goto out_client_fini;

	for (;;) {
		pollfds[0].fd = client->fd_in;
		pollfds[0].events = POLLIN;
		pollfds[1].fd = client->console_sd;
		pollfds[1].events = POLLIN;

		rc = poll(pollfds, 2, -1);
		if (rc < 0) {
			warn("Poll failure");
			break;
		}

		if (pollfds[0].revents)
			prc = process_tty(client);

		if (prc == PROCESS_OK && pollfds[1].revents)
			prc = process_console(client);

		rc = (prc == PROCESS_ERR) ? -1 : 0;
		if (prc != PROCESS_OK)
			break;
	}

out_client_fini:
	client_fini(client);

out_config_fini:
	if (config_path)
		config_fini(config);

	if (prc == PROCESS_ESC)
		return EXIT_ESCAPE;
	return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}

