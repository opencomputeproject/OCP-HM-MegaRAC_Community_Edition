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

#pragma once

#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <termios.h> /* for speed_t */
#include <time.h>
#include <systemd/sd-bus.h>
#include <sys/time.h>
#include <sys/un.h>

struct console;
struct config;

/* Handler API.
 *
 * Console data handlers: these implement the functions that process
 * data coming out of the main tty device.
 *
 * Handlers are registered at link time using the console_handler_register()
 * macro. We call each handler's ->init() function at startup, and ->fini() at
 * exit.
 *
 * Handlers will almost always want to register a ringbuffer consumer, which
 * provides data coming from the tty. Use cosole_register_ringbuffer_consumer()
 * for this. To send data to the tty, use console_data_out().
 *
 * If a handler needs to monitor a separate file descriptor for events, use the
 * poller API, through console_poller_register().
 */
struct handler {
	const char	*name;
	int		(*init)(struct handler *handler,
				struct console *console,
				struct config *config);
	void		(*fini)(struct handler *handler);
	int		(*baudrate)(struct handler *handler,
				    speed_t baudrate);
	bool		active;
};

#define __handler_name(n) __handler_  ## n
#define  _handler_name(n) __handler_name(n)

#define console_handler_register(h) \
	static const \
		__attribute__((section("handlers"))) \
		__attribute__((used)) \
		struct handler * _handler_name(__COUNTER__) = h;

int console_data_out(struct console *console, const uint8_t *data, size_t len);

/* poller API */
struct poller;

enum poller_ret {
	POLLER_OK = 0,
	POLLER_REMOVE,
	POLLER_EXIT,
};

typedef enum poller_ret (*poller_event_fn_t)(struct handler *handler,
					int revents, void *data);
typedef enum poller_ret (*poller_timeout_fn_t)(struct handler *handler,
					void *data);

struct poller *console_poller_register(struct console *console,
		struct handler *handler, poller_event_fn_t event_fn,
		poller_timeout_fn_t timeout_fn, int fd, int events,
		void *data);

void console_poller_unregister(struct console *console, struct poller *poller);

void console_poller_set_events(struct console *console, struct poller *poller,
		int events);

void console_poller_set_timeout(struct console *console, struct poller *poller,
				const struct timeval *interval);

/* ringbuffer API */
enum ringbuffer_poll_ret {
	RINGBUFFER_POLL_OK = 0,
	RINGBUFFER_POLL_REMOVE,
};

typedef enum ringbuffer_poll_ret (*ringbuffer_poll_fn_t)(void *data,
		size_t force_len);

struct ringbuffer;
struct ringbuffer_consumer;

struct ringbuffer *ringbuffer_init(size_t size);
void ringbuffer_fini(struct ringbuffer *rb);

struct ringbuffer_consumer *ringbuffer_consumer_register(struct ringbuffer *rb,
		ringbuffer_poll_fn_t poll_fn, void *data);

void ringbuffer_consumer_unregister(struct ringbuffer_consumer *rbc);

int ringbuffer_queue(struct ringbuffer *rb, uint8_t *data, size_t len);

size_t ringbuffer_dequeue_peek(struct ringbuffer_consumer *rbc, size_t offset,
		uint8_t **data);

int ringbuffer_dequeue_commit(struct ringbuffer_consumer *rbc, size_t len);

size_t ringbuffer_len(struct ringbuffer_consumer *rbc);

/* console wrapper around ringbuffer consumer registration */
struct ringbuffer_consumer *console_ringbuffer_consumer_register(
		struct console *console,
		ringbuffer_poll_fn_t poll_fn, void *data);

/* config API */
struct config;
const char *config_get_value(struct config *config, const char *name);
struct config *config_init(const char *filename);
void config_fini(struct config *config);

int config_parse_baud(speed_t *speed, const char *baud_string);
uint32_t parse_baud_to_int(speed_t speed);
speed_t parse_int_to_baud(uint32_t baud);
int config_parse_logsize(const char *size_str, size_t *size);

/* socket paths */
ssize_t console_socket_path(struct sockaddr_un *addr, const char *id);

typedef char (socket_path_t)[sizeof(((struct sockaddr_un *)NULL)->sun_path)];
ssize_t console_socket_path_readable(const struct sockaddr_un *addr,
				     size_t addrlen, socket_path_t path);

/* utils */
int write_buf_to_fd(int fd, const uint8_t *buf, size_t len);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define offsetof(type, member) \
	((unsigned long)&((type *)NULL)->member)

#define container_of(ptr, type, member) \
	((type *)((void *)((ptr) - offsetof(type, member))))

#define BUILD_ASSERT(c) \
	do { \
		char __c[(c)?1:-1] __attribute__((unused)); \
	} while (0)
