/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */

#define _GNU_SOURCE

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "libmctp.h"
#include "libmctp-serial.h"
#include "libmctp-astlpc.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define __unused __attribute__((unused))

static const mctp_eid_t local_eid_default = 8;
static char sockname[] = "\0mctp-mux";

struct binding {
	const char	*name;
	int		(*init)(struct mctp *mctp, struct binding *binding,
				mctp_eid_t eid, int n_params,
				char * const * params);
	int		(*get_fd)(struct binding *binding);
	int		(*process)(struct binding *binding);
	void		*data;
};

struct client {
	bool		active;
	int		sock;
	uint8_t		type;
};

struct ctx {
	struct mctp	*mctp;
	struct binding	*binding;
	bool		verbose;
	int		local_eid;
	void		*buf;
	size_t		buf_size;

	int		sock;
	struct pollfd	*pollfds;

	struct client	*clients;
	int		n_clients;
};

static void tx_message(struct ctx *ctx, mctp_eid_t eid, void *msg, size_t len)
{
	mctp_message_tx(ctx->mctp, eid, msg, len);
}

static void client_remove_inactive(struct ctx *ctx)
{
	int i;

	for (i = 0; i < ctx->n_clients; i++) {
		struct client *client = &ctx->clients[i];
		if (client->active)
			continue;
		close(client->sock);

		ctx->n_clients--;
		memmove(&ctx->clients[i], &ctx->clients[i+1],
				(ctx->n_clients - i) * sizeof(*ctx->clients));
		ctx->clients = realloc(ctx->clients,
				ctx->n_clients * sizeof(*ctx->clients));
	}
}

static void rx_message(uint8_t eid, void *data, void *msg, size_t len)
{
	struct ctx *ctx = data;
	struct iovec iov[2];
	struct msghdr msghdr;
	bool removed;
	uint8_t type;
	int i, rc;

	if (len < 2)
		return;

	type = *(uint8_t *)msg;

	if (ctx->verbose)
		fprintf(stderr, "MCTP message received: len %zd, type %d\n",
				len, type);

	memset(&msghdr, 0, sizeof(msghdr));
	msghdr.msg_iov = iov;
	msghdr.msg_iovlen = 2;
	iov[0].iov_base = &eid;
	iov[0].iov_len = 1;
	iov[1].iov_base = msg;
	iov[1].iov_len = len;

	for (i = 0; i < ctx->n_clients; i++) {
		struct client *client = &ctx->clients[i];

		if (client->type != type)
			continue;

		if (ctx->verbose)
			fprintf(stderr, "  forwarding to client %d\n", i);

		rc = sendmsg(client->sock, &msghdr, 0);
		if (rc != (ssize_t)(len + 1)) {
			client->active = false;
			removed = true;
		}
	}

	if (removed)
		client_remove_inactive(ctx);

}

static int binding_null_init(struct mctp *mctp __unused,
		struct binding *binding __unused,
		mctp_eid_t eid __unused,
		int n_params, char * const *params __unused)
{
	if (n_params != 0) {
		warnx("null binding doesn't accept parameters");
		return -1;
	}
	return 0;
}

static int binding_serial_init(struct mctp *mctp, struct binding *binding,
		mctp_eid_t eid, int n_params, char * const *params)
{
	struct mctp_binding_serial *serial;
	const char *path;
	int rc;

	if (n_params != 1) {
		warnx("serial binding requires device param");
		return -1;
	}

	path = params[0];

	serial = mctp_serial_init();
	assert(serial);

	rc = mctp_serial_open_path(serial, path);
	if (rc)
		return -1;

	mctp_register_bus(mctp, mctp_binding_serial_core(serial), eid);

	binding->data = serial;

	return 0;
}

static int binding_serial_get_fd(struct binding *binding)
{
	return mctp_serial_get_fd(binding->data);
}

static int binding_serial_process(struct binding *binding)
{
	return mctp_serial_read(binding->data);
}

static int binding_astlpc_init(struct mctp *mctp, struct binding *binding,
		mctp_eid_t eid, int n_params,
		char * const *params __attribute__((unused)))
{
	struct mctp_binding_astlpc *astlpc;

	if (n_params) {
		warnx("astlpc binding does not accept parameters");
		return -1;
	}

	astlpc = mctp_astlpc_init_fileio();
	if (!astlpc) {
		warnx("could not initialise astlpc binding");
		return -1;
	}

	mctp_register_bus(mctp, mctp_binding_astlpc_core(astlpc), eid);

	binding->data = astlpc;
	return 0;
}

static int binding_astlpc_get_fd(struct binding *binding)
{
	return mctp_astlpc_get_fd(binding->data);
}

static int binding_astlpc_process(struct binding *binding)
{
	return mctp_astlpc_poll(binding->data);
}

struct binding bindings[] = {
	{
		.name = "null",
		.init = binding_null_init,
	},
	{
		.name = "serial",
		.init = binding_serial_init,
		.get_fd = binding_serial_get_fd,
		.process = binding_serial_process,
	},
	{
		.name = "astlpc",
		.init = binding_astlpc_init,
		.get_fd = binding_astlpc_get_fd,
		.process = binding_astlpc_process,
	}
};

struct binding *binding_lookup(const char *name)
{
	struct binding *binding;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bindings); i++) {
		binding = &bindings[i];

		if (!strcmp(binding->name, name))
			return binding;
	}

	return NULL;
}

static int socket_init(struct ctx *ctx)
{
	struct sockaddr_un addr;
	int namelen, rc;

	namelen = sizeof(sockname) - 1;
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, sockname, namelen);

	ctx->sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (ctx->sock < 0) {
		warn("can't create socket");
		return -1;
	}

	rc = bind(ctx->sock, (struct sockaddr *)&addr,
			sizeof(addr.sun_family) + namelen);
	if (rc) {
		warn("can't bind socket");
		goto err_close;
	}

	rc = listen(ctx->sock, 1);
	if (rc) {
		warn("can't listen on socket");
		goto err_close;
	}

	return 0;

err_close:
	close(ctx->sock);
	return -1;
}

static int socket_process(struct ctx *ctx)
{
	struct client *client;
	int fd;

	fd = accept4(ctx->sock, NULL, 0, SOCK_NONBLOCK);
	if (fd < 0)
		return -1;

	ctx->n_clients++;
	ctx->clients = realloc(ctx->clients,
			ctx->n_clients * sizeof(struct client));

	client = &ctx->clients[ctx->n_clients-1];
	memset(client, 0, sizeof(*client));
	client->active = true;
	client->sock = fd;

	return 0;
}

static int client_process_recv(struct ctx *ctx, int idx)
{
	struct client *client = &ctx->clients[idx];
	uint8_t eid;
	ssize_t len;
	int rc;

	/* are we waiting for a type message? */
	if (!client->type) {
		uint8_t type;
		rc = read(client->sock, &type, 1);
		if (rc <= 0)
			goto out_close;

		if (type == 0) {
			rc = -1;
			goto out_close;
		}
		if (ctx->verbose)
			fprintf(stderr, "client[%d] registered for type %u\n",
					idx, type);
		client->type = type;
		return 0;
	}

	len = recv(client->sock, NULL, 0, MSG_PEEK | MSG_TRUNC);
	if (len < 0) {
		if (errno != ECONNRESET)
			warn("can't receive (peek) from client");

		rc = -1;
		goto out_close;
	}

	if ((size_t)len > ctx->buf_size) {
		void *tmp;

		tmp = realloc(ctx->buf, len);
		if (!tmp) {
			warn("can't allocate for incoming message");
			rc = -1;
			goto out_close;
		}
		ctx->buf = tmp;
		ctx->buf_size = len;
	}

	rc = recv(client->sock, ctx->buf, ctx->buf_size, 0);
	if (rc < 0) {
		if (errno != ECONNRESET)
			warn("can't receive from client");
		rc = -1;
		goto out_close;
	}

	if (rc <= 0) {
		rc = -1;
		goto out_close;
	}

	eid = *(uint8_t *)ctx->buf;

	if (ctx->verbose)
		fprintf(stderr,
			"client[%d] sent message: dest 0x%02x len %d\n",
			idx, eid, rc - 1);


	if (eid == ctx->local_eid)
		rx_message(eid, ctx, ctx->buf + 1, rc - 1);
	else
		tx_message(ctx, eid, ctx->buf + 1, rc - 1);

	return 0;

out_close:
	client->active = false;
	return rc;
}

static int binding_init(struct ctx *ctx, const char *name,
		int argc, char * const *argv)
{
	int rc;

	ctx->binding = binding_lookup(name);
	if (!ctx->binding) {
		warnx("no such binding '%s'", name);
		return -1;
	}

	rc = ctx->binding->init(ctx->mctp, ctx->binding, ctx->local_eid,
			argc, argv);
	return rc;
}

enum {
	FD_BINDING = 0,
	FD_SOCKET,
	FD_NR,
};

static int run_daemon(struct ctx *ctx)
{
	bool clients_changed = false;
	int rc, i;

	ctx->pollfds = malloc(FD_NR * sizeof(struct pollfd));

	if (ctx->binding->get_fd) {
		ctx->pollfds[FD_BINDING].fd =
			ctx->binding->get_fd(ctx->binding);
		ctx->pollfds[FD_BINDING].events = POLLIN;
	} else {
		ctx->pollfds[FD_BINDING].fd = -1;
		ctx->pollfds[FD_BINDING].events = 0;
	}

	ctx->pollfds[FD_SOCKET].fd = ctx->sock;
	ctx->pollfds[FD_SOCKET].events = POLLIN;

	mctp_set_rx_all(ctx->mctp, rx_message, ctx);

	for (;;) {
		if (clients_changed) {
			int i;

			ctx->pollfds = realloc(ctx->pollfds,
					(ctx->n_clients + FD_NR) *
						sizeof(struct pollfd));

			for (i = 0; i < ctx->n_clients; i++) {
				ctx->pollfds[FD_NR+i].fd =
					ctx->clients[i].sock;
				ctx->pollfds[FD_NR+i].events = POLLIN;
			}
			clients_changed = false;
		}

		rc = poll(ctx->pollfds, ctx->n_clients + FD_NR, -1);
		if (rc < 0) {
			warn("poll failed");
			break;
		}

		if (!rc)
			continue;

		if (ctx->pollfds[FD_BINDING].revents) {
			rc = 0;
			if (ctx->binding->process)
				rc = ctx->binding->process(ctx->binding);
			if (rc)
				break;
		}

		for (i = 0; i < ctx->n_clients; i++) {
			if (!ctx->pollfds[FD_NR+i].revents)
				continue;

			rc = client_process_recv(ctx, i);
			if (rc)
				clients_changed = true;
		}

		if (ctx->pollfds[FD_SOCKET].revents) {
			rc = socket_process(ctx);
			if (rc)
				break;
			clients_changed = true;
		}

		if (clients_changed)
			client_remove_inactive(ctx);

	}


	free(ctx->pollfds);

	return rc;
}

static const struct option options[] = {
	{ "verbose", no_argument, 0, 'v' },
	{ "eid", required_argument, 0, 'e' },
	{ 0 },
};

static void usage(const char *progname)
{
	unsigned int i;

	fprintf(stderr, "usage: %s <binding> [params]\n", progname);
	fprintf(stderr, "Available bindings:\n");
	for (i = 0; i < ARRAY_SIZE(bindings); i++)
		fprintf(stderr, "  %s\n", bindings[i].name);
}

int main(int argc, char * const *argv)
{
	struct ctx *ctx, _ctx;
	int rc;

	ctx = &_ctx;
	ctx->clients = NULL;
	ctx->n_clients = 0;
	ctx->local_eid = local_eid_default;
	ctx->verbose = false;

	for (;;) {
		rc = getopt_long(argc, argv, "e:v", options, NULL);
		if (rc == -1)
			break;
		switch (rc) {
		case 'v':
			ctx->verbose = true;
			break;
		case 'e':
			ctx->local_eid = atoi(optarg);
			break;
		default:
			fprintf(stderr, "Invalid argument\n");
			return EXIT_FAILURE;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "missing binding argument\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* setup initial buffer */
	ctx->buf_size = 4096;
	ctx->buf = malloc(ctx->buf_size);

	mctp_set_log_stdio(ctx->verbose ? MCTP_LOG_DEBUG : MCTP_LOG_WARNING);

	ctx->mctp = mctp_init();
	assert(ctx->mctp);

	rc = binding_init(ctx, argv[optind], argc - optind - 1, argv + optind + 1);
	if (rc)
		return EXIT_FAILURE;

	rc = socket_init(ctx);
	if (rc)
		return EXIT_FAILURE;

	rc = run_daemon(ctx);

	return rc ? EXIT_FAILURE : EXIT_SUCCESS;

}
