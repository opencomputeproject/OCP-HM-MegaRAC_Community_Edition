/* Copyright 2015 IBM
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include <linux/bt-bmc.h>

#include <systemd/sd-bus.h>

static const char *bt_bmc_device = "/dev/ipmi-bt-host";

#define PREFIX "BTBRIDGED"

#define BT_BMC_PATH bt_bmc_device
#define BT_BMC_TIMEOUT_SEC 5
#define BT_MAX_MESSAGE 64

#define DBUS_NAME "org.openbmc.HostIpmi"
#define OBJ_NAME "/org/openbmc/HostIpmi/1"

#define SD_BUS_FD 0
#define BT_FD 1
#define TIMER_FD 2
#define TOTAL_FDS 3

#define MSG_OUT(f_, ...) do { if (verbosity != BT_LOG_NONE) { bt_log(LOG_INFO, f_, ##__VA_ARGS__); } } while(0)
#define MSG_ERR(f_, ...) do { if (verbosity != BT_LOG_NONE) { bt_log(LOG_ERR, f_, ##__VA_ARGS__); } } while(0)

struct ipmi_msg {
	uint8_t netfn;
	uint8_t lun;
	uint8_t seq;
	uint8_t cmd;
	uint8_t cc; /* Only used on responses */
	uint8_t *data;
	size_t data_len;
};

struct bt_queue {
	struct ipmi_msg req;
	struct ipmi_msg rsp;
	struct timespec start;
	int expired;
	sd_bus_message *call;
	struct bt_queue *next;
};

struct btbridged_context {
	struct pollfd fds[TOTAL_FDS];
	struct sd_bus *bus;
	struct bt_queue *bt_q;
};

static void (*bt_vlog)(int p, const char *fmt, va_list args);
static int running = 1;
static enum {
   BT_LOG_NONE = 0,
   BT_LOG_VERBOSE,
   BT_LOG_DEBUG
} verbosity;

static void bt_log_console(int p, const char *fmt, va_list args)
{
	struct timespec time;
	FILE *s = (p < LOG_WARNING) ? stdout : stderr;

	clock_gettime(CLOCK_REALTIME, &time);

	fprintf(s, "[%s %ld.%.9ld] ", PREFIX, time.tv_sec, time.tv_nsec);

	vfprintf(s, fmt, args);
}

__attribute__((format(printf, 2, 3)))
static void bt_log(int p, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	bt_vlog(p, fmt, args);
	va_end(args);
}

static struct bt_queue *bt_q_get_head(struct btbridged_context *context)
{
	return context ? context->bt_q : NULL;
}

static struct bt_queue *bt_q_get_seq(struct btbridged_context *context, uint8_t seq)
{
	struct bt_queue *t;

	assert(context);

	t = context->bt_q;

	while (t && t->req.seq != seq)
		t = t->next;

	return t;
}

static struct bt_queue *bt_q_get_msg(struct btbridged_context *context)
{
	struct bt_queue *t;

	assert(context);

	t = context->bt_q;

	while (t && (!t->call && !t->expired))
		t = t->next;

	return t;
}

static struct bt_queue *bt_q_enqueue(struct btbridged_context *context, uint8_t *bt_data)
{
	struct bt_queue *n;
	struct bt_queue *bt_q;
	int len;

	assert(context && bt_data);

	/*
	 * len here is the length of the array.
	 * Helpfully BT doesn't count the length byte
	 */
	len = bt_data[0] + 1;
	if (len < 4) {
		MSG_ERR("Trying to queue a BT message with a short length (%d)\n", len);
		return NULL;
	}

	bt_q = context->bt_q;

	n = calloc(1, sizeof(struct bt_queue));
	if (!n)
		return NULL;

	if (verbosity == BT_LOG_DEBUG) {
		n->req.data = malloc(len - 4);
		if (n->req.data)
			n->req.data = memcpy(n->req.data, bt_data + 4, len - 4);
	}
	n->req.data_len = len - 4;
	/* Don't count the lenfn/ln, seq and command */
	n->req.netfn = bt_data[1] >> 2;
	n->req.lun = bt_data[1] & 0x3;
	n->req.seq = bt_data[2];
	n->req.cmd = bt_data[3];
	if (clock_gettime(CLOCK_MONOTONIC, &n->start) == -1) {
		MSG_ERR("Couldn't clock_gettime(): %s\n", strerror(errno));
		free(n);
		return NULL;
	}
	if (!bt_q) {
		context->bt_q = n;
	} else {
		struct bt_queue *t = bt_q;

		while (t->next)
			t = t->next;

		t->next = n;
	}

	return n;
}

static void bt_q_free(struct bt_queue *bt_q)
{
	if (!bt_q)
		return;

	/* Unrefing sd_bus_message should free(rsp.data) */
	if (bt_q->call)
		sd_bus_message_unref(bt_q->call);

	free(bt_q->req.data);
	free(bt_q);
}

static struct bt_queue *bt_q_drop(struct btbridged_context *context, struct bt_queue *element)
{
	struct bt_queue *r;

	assert(context);

	if (!element || !context || !context->bt_q)
		return NULL;

	if (element == context->bt_q) {
		context->bt_q = context->bt_q->next;
	} else {
		r = context->bt_q;
		while (r && r->next != element)
			r = r->next;

		if (!r) {
			MSG_ERR("Didn't find element %p in queue\n", element);
			bt_q_free(element);
			return NULL;
		}
		r->next = r->next->next;
	}
	bt_q_free(element);

	return context->bt_q;
}

static struct bt_queue *bt_q_dequeue(struct btbridged_context *context)
{
	struct bt_queue *r;
	struct bt_queue *bt_q;

	assert(context);

	bt_q = context->bt_q;
	if (!bt_q) {
		MSG_ERR("Dequeuing empty queue!\n");
		return NULL;
	}

	r = bt_q->next;
	bt_q_free(bt_q);
	context->bt_q = r;

	return r;
}

static int method_send_sms_atn(sd_bus_message *msg, void *userdata,
			       sd_bus_error *ret_error)
{
	int r;
	struct btbridged_context *bt_fd = userdata;

	MSG_OUT("Sending SMS_ATN ioctl (%d) to %s\n",
			BT_BMC_IOCTL_SMS_ATN, BT_BMC_PATH);

	r = ioctl(bt_fd->fds[BT_FD].fd, BT_BMC_IOCTL_SMS_ATN);
	if (r == -1) {
		r = errno;
		MSG_ERR("Couldn't ioctl() to 0x%x, %s: %s\n", bt_fd->fds[BT_FD].fd, BT_BMC_PATH, strerror(r));
		return sd_bus_reply_method_errno(msg, errno, ret_error);
	}

	r = 0;
	return sd_bus_reply_method_return(msg, "x", r);
}

static int method_send_message(sd_bus_message *msg, void *userdata, sd_bus_error *ret_error)
{
	struct btbridged_context *context;
	struct bt_queue *bt_msg;
	uint8_t *data;
	size_t data_sz;
	uint8_t netfn, lun, seq, cmd, cc;
	/*
	 * Doesn't say it anywhere explicitly but it looks like returning 0 or
	 * negative is BAD...
	 */
	int r = 1;

	context = (struct btbridged_context *)userdata;
	if (!context) {
		sd_bus_error_set_const(ret_error, "org.openbmc.error", "Internal error");
		r = 0;
		goto out;
	}
	r = sd_bus_message_read(msg, "yyyyy", &seq, &netfn, &lun, &cmd, &cc);
	if (r < 0) {
		MSG_ERR("Couldn't parse leading bytes of message: %s\n", strerror(-r));
		sd_bus_error_set_const(ret_error, "org.openbmc.error", "Bad message");
		r = -EINVAL;
		goto out;
	}
	r = sd_bus_message_read_array(msg, 'y', (const void **)&data, &data_sz);
	if (r < 0) {
		MSG_ERR("Couldn't parse data bytes of message: %s\n", strerror(-r));
		sd_bus_error_set_const(ret_error, "org.openbmc.error", "Bad message data");
		r = -EINVAL;
		goto out;
	}

	bt_msg = bt_q_get_seq(context, seq);
	if (!bt_msg) {
		sd_bus_error_set_const(ret_error, "org.openbmc.error", "No matching request");
		MSG_ERR("Failed to find matching request for dbus method with seq: 0x%02x\n", seq);
		r = -EINVAL;
		goto out;
	}
	MSG_OUT("Received a dbus response for msg with seq 0x%02x\n", seq);
	bt_msg->call = sd_bus_message_ref(msg);
	bt_msg->rsp.netfn = netfn;
	bt_msg->rsp.lun = lun;
	bt_msg->rsp.seq = seq;
	bt_msg->rsp.cmd = cmd;
	bt_msg->rsp.cc = cc;
	bt_msg->rsp.data_len = data_sz;
	/* Because we've ref'ed the msg, I hope we don't need to memcpy data */
	bt_msg->rsp.data = data;

	/* Now that we have a response */
	context->fds[BT_FD].events |= POLLOUT;

out:
	return r;
}

static int bt_host_write(struct btbridged_context *context, struct bt_queue *bt_msg)
{
	struct bt_queue *head;
	uint8_t data[BT_MAX_MESSAGE] = { 0 };
	sd_bus_message *msg = NULL;
	int r = 0, len;

	assert(context);

	if (!bt_msg)
		return -EINVAL;

	head = bt_q_get_head(context);
	if (bt_msg == head) {
		struct itimerspec ts;
		/* Need to adjust the timer */
		ts.it_interval.tv_sec = 0;
		ts.it_interval.tv_nsec = 0;
		if (head->next) {
			ts.it_value = head->next->start;
			ts.it_value.tv_sec += BT_BMC_TIMEOUT_SEC;
			MSG_OUT("Adjusting timer for next element\n");
		} else {
			ts.it_value.tv_nsec = 0;
			ts.it_value.tv_sec = 0;
			MSG_OUT("Disabling timer, no elements remain in queue\n");
		}
		r = timerfd_settime(context->fds[TIMER_FD].fd, TFD_TIMER_ABSTIME, &ts, NULL);
		if (r == -1)
			MSG_ERR("Couldn't set timerfd\n");
	}
	data[1] = (bt_msg->rsp.netfn << 2) | (bt_msg->rsp.lun & 0x3);
	data[2] = bt_msg->rsp.seq;
	data[3] = bt_msg->rsp.cmd;
	data[4] = bt_msg->rsp.cc;
	if (bt_msg->rsp.data_len > sizeof(data) - 5) {
		MSG_ERR("Response message size (%zu) too big, truncating\n", bt_msg->rsp.data_len);
		bt_msg->rsp.data_len = sizeof(data) - 5;
	}
	/* netfn/len + seq + cmd + cc = 4 */
	data[0] = bt_msg->rsp.data_len + 4;
	if (bt_msg->rsp.data_len)
		memcpy(data + 5, bt_msg->rsp.data, bt_msg->rsp.data_len);
	/* Count the data[0] byte */
	len = write(context->fds[BT_FD].fd, data, data[0] + 1);
	if (len == -1) {
		r = errno;
		MSG_ERR("Error writing to %s: %s\n", BT_BMC_PATH, strerror(errno));
		if (bt_msg->call) {
			r = sd_bus_message_new_method_errno(bt_msg->call, &msg, r, NULL);
			if (r < 0)
				MSG_ERR("Couldn't create response error\n");
		}
		goto out;
	} else {
		if (len != data[0] + 1)
			MSG_ERR("Possible short write to %s, desired len: %d, written len: %d\n", BT_BMC_PATH, data[0] + 1, len);
		else
			MSG_OUT("Successfully wrote %d of %d bytes to %s\n", len, data[0] + 1, BT_BMC_PATH);

		if (bt_msg->call) {
			r = sd_bus_message_new_method_return(bt_msg->call, &msg);
			if (r < 0) {
				MSG_ERR("Couldn't create response message\n");
				goto out;
			}
			r = 0; /* Just to be explicit about what we're sending back */
			r = sd_bus_message_append(msg, "x", r);
			if (r < 0) {
				MSG_ERR("Couldn't append result to response\n");
				goto out;
			}

		}
	}

out:
	if (bt_msg->call) {
		if (sd_bus_send(context->bus, msg, NULL) < 0)
			MSG_ERR("Couldn't send response message\n");
		sd_bus_message_unref(msg);
	}
	bt_q_drop(context, bt_msg);

	/*
	 * There isn't another message ready to be sent so turn off POLLOUT
	 */
	if (!bt_q_get_msg(context)) {
		MSG_OUT("Turning off POLLOUT for the BT in poll()\n");
		context->fds[BT_FD].events = POLLIN;
	}

	return r;
}

static int dispatch_timer(struct btbridged_context *context)
{
	int r = 0;
	if (context->fds[TIMER_FD].revents & POLLIN) {
		struct bt_queue *head;
		sd_bus_message *msg;
		uint64_t counter;

		/* Empty out timerfd so we won't trigger POLLIN continuously */
		r = read(context->fds[TIMER_FD].fd, &counter, sizeof(counter));
		MSG_OUT("Timer fired %" PRIu64 " times\n", counter);

		head = bt_q_get_head(context);
		if (!head) {
			/* Odd, timer expired but we didn't have a message to timeout */
			MSG_ERR("No message found to send timeout\n");
			return 0;
		}
		if (head->call) {
			r = sd_bus_message_new_method_errno(head->call, &msg, ETIMEDOUT, NULL);
			if (r < 0) {
				MSG_ERR("Couldn't create response error\n");
			} else {
				if (sd_bus_send(context->bus, msg, NULL) < 0)
					MSG_ERR("Couldn't send response message\n");
				sd_bus_message_unref(msg);
			}
			MSG_ERR("Message with seq 0x%02x is being timed out despite "
					"appearing to have been responded to. Slow BT?\n", head->rsp.seq);
		}

		/* Set expiry */
		head->expired = 1;
		head->rsp.seq = head->req.seq;
		head->rsp.netfn = head->req.netfn + 1;
		head->rsp.lun = head->req.lun;
		head->rsp.cc = 0xce; /* Command response could not be provided */
		head->rsp.cmd = head->req.cmd;
		/* These should already be zeroed but best to be sure */
		head->rsp.data_len = 0;
		head->rsp.data = NULL;
		head->call = NULL;
		MSG_OUT("Timeout on msg with seq: 0x%02x\n", head->rsp.seq);
		/* Turn on POLLOUT so we'll write this one next */
		context->fds[BT_FD].events |= POLLOUT;
	}

	return 0;
}

static int dispatch_sd_bus(struct btbridged_context *context)
{
	int r = 0;
	if (context->fds[SD_BUS_FD].revents) {
		r = sd_bus_process(context->bus, NULL);
		if (r > 0)
			MSG_OUT("Processed %d dbus events\n", r);
	}

	return r;
}

static int dispatch_bt(struct btbridged_context *context)
{
	int err = 0;
	int r = 0;

	assert(context);

	if (context->fds[BT_FD].revents & POLLIN) {
		sd_bus_message *msg;
		struct bt_queue *new;
		uint8_t data[BT_MAX_MESSAGE] = { 0 };

		r = read(context->fds[BT_FD].fd, data, sizeof(data));
		if (r < 0) {
			MSG_ERR("Couldn't read from bt: %s\n", strerror(-r));
			goto out1;
		}
		if (r < data[0] + 1) {
			MSG_ERR("Short read from bt (%d vs %d)\n", r, data[0] + 1);
			r = 0;
			goto out1;
		}

		new = bt_q_enqueue(context, data);
		if (!new) {
			r = -ENOMEM;
			goto out1;
		}
		if (new == bt_q_get_head(context)) {
			struct itimerspec ts;
			/*
			 * Enqueued onto an empty list, setup a timer for sending a
			 * timeout
			 */
			ts.it_interval.tv_sec = 0;
			ts.it_interval.tv_nsec = 0;
			ts.it_value.tv_nsec = 0;
			ts.it_value.tv_sec = BT_BMC_TIMEOUT_SEC;
			r = timerfd_settime(context->fds[TIMER_FD].fd, 0, &ts, NULL);
			if (r == -1)
				MSG_ERR("Couldn't set timerfd\n");
		}

		r = sd_bus_message_new_signal(context->bus, &msg, OBJ_NAME, DBUS_NAME, "ReceivedMessage");
		if (r < 0) {
			MSG_ERR("Failed to create signal: %s\n", strerror(-r));
			goto out1;
		}

		r = sd_bus_message_append(msg, "yyyy", new->req.seq, new->req.netfn, new->req.lun, new->req.cmd);
		if (r < 0) {
			MSG_ERR("Couldn't append to signal: %s\n", strerror(-r));
			goto out1_free;
		}

		r = sd_bus_message_append_array(msg, 'y', data + 4, new->req.data_len);
		if (r < 0) {
			MSG_ERR("Couldn't append array to signal: %s\n", strerror(-r));
			goto out1_free;
		}

		MSG_OUT("Sending dbus signal with seq 0x%02x, netfn 0x%02x, lun 0x%02x, cmd 0x%02x\n",
				new->req.seq, new->req.netfn, new->req.lun, new->req.cmd);

		if (verbosity == BT_LOG_DEBUG) {
			int i;
			for (i = 0; i < new->req.data_len; i++) {
				if (i % 8 == 0) {
					if (i)
						printf("\n");
					MSG_OUT("\t");
				}
				printf("0x%02x ", data[i + 4]);
			}
			if (new->req.data_len)
				printf("\n");
		}

		/* Note we only actually keep the request data in the queue when debugging */
		r = sd_bus_send(context->bus, msg, NULL);
		if (r < 0) {
			MSG_ERR("Couldn't emit dbus signal: %s\n", strerror(-r));
			goto out1_free;
		}
		r = 0;
out1_free:
		sd_bus_message_unref(msg);
out1:
		err = r;
	}

	if (context->fds[BT_FD].revents & POLLOUT) {
		struct bt_queue *bt_msg;
		bt_msg = bt_q_get_msg(context);
		if (!bt_msg) {
			/* Odd, this shouldn't really happen */
			MSG_ERR("Got a POLLOUT but no message is ready to be written\n");
			r = 0;
			goto out;
		}
		r = bt_host_write(context, bt_msg);
		if (r < 0)
			MSG_ERR("Problem putting request with seq 0x%02x, netfn 0x%02x, lun 0x%02x, cmd 0x%02x, cc 0x%02x\n"
					"out to %s", bt_msg->rsp.seq, bt_msg->rsp.netfn, bt_msg->rsp.lun,
					bt_msg->rsp.cmd, bt_msg->rsp.cc, BT_BMC_PATH);
		else
			MSG_OUT("Completed request with seq 0x%02x, netfn 0x%02x, lun 0x%02x, cmd 0x%02x, cc 0x%02x\n",
					bt_msg->rsp.seq, bt_msg->rsp.netfn, bt_msg->rsp.lun, bt_msg->rsp.cmd, bt_msg->rsp.cc);
	}
out:
	return err ? err : r;
}

static void usage(const char *name)
{
	fprintf(stderr, "\
Usage %s [--v[v] | --syslog] [-d <DEVICE>]\n\
  --v                    Be verbose\n\
  --vv                   Be verbose and dump entire messages\n\
  -s, --syslog           Log output to syslog (pointless without --verbose)\n\
  -d, --device <DEVICE>  use <DEVICE> file. Default is '%s'\n\n",
		name, bt_bmc_device);
}

static const sd_bus_vtable ipmid_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("sendMessage", "yyyyyay", "x", &method_send_message, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("setAttention", "", "x", &method_send_sms_atn, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_SIGNAL("ReceivedMessage", "yyyyay", 0),
	SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
	struct btbridged_context *context;
	const char *name = argv[0];
	int opt, polled, r;

	static const struct option long_options[] = {
		{ "device",  required_argument, NULL, 'd' },
		{ "v",       no_argument, (int *)&verbosity, BT_LOG_VERBOSE },
		{ "vv",      no_argument, (int *)&verbosity, BT_LOG_DEBUG   },
		{ "syslog",  no_argument, 0,          's'         },
		{ 0,         0,           0,          0           }
	};

	context = calloc(1, sizeof(*context));

	bt_vlog = &bt_log_console;
	while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
		switch (opt) {
			case 0:
				break;
			case 'd':
				bt_bmc_device = optarg;
				break;
			case 's':
				/* Avoid a double openlog() */
				if (bt_vlog != &vsyslog) {
					openlog(PREFIX, LOG_ODELAY, LOG_DAEMON);
					bt_vlog = &vsyslog;
				}
				break;
			default:
				usage(name);
				exit(EXIT_FAILURE);
		}
	}

	if (verbosity == BT_LOG_VERBOSE)
		MSG_OUT("Verbose logging\n");

	if (verbosity == BT_LOG_DEBUG)
		MSG_OUT("Debug logging\n");

	MSG_OUT("Starting\n");
	r = sd_bus_default_system(&context->bus);
	if (r < 0) {
		MSG_ERR("Failed to connect to system bus: %s\n", strerror(-r));
		goto finish;
	}

	MSG_OUT("Registering dbus methods/signals\n");
	r = sd_bus_add_object_vtable(context->bus,
	                             NULL,
	                             OBJ_NAME,
	                             DBUS_NAME,
	                             ipmid_vtable,
	                             context);
	if (r < 0) {
		MSG_ERR("Failed to issue method call: %s\n", strerror(-r));
		goto finish;
	}

	MSG_OUT("Requesting dbus name: %s\n", DBUS_NAME);
	r = sd_bus_request_name(context->bus, DBUS_NAME, SD_BUS_NAME_ALLOW_REPLACEMENT|SD_BUS_NAME_REPLACE_EXISTING);
	if (r < 0) {
		MSG_ERR("Failed to acquire service name: %s\n", strerror(-r));
		goto finish;
	}

	MSG_OUT("Getting dbus file descriptors\n");
	context->fds[SD_BUS_FD].fd = sd_bus_get_fd(context->bus);
	if (context->fds[SD_BUS_FD].fd < 0) {
		r = -errno;
		MSG_OUT("Couldn't get the bus file descriptor: %s\n", strerror(errno));
		goto finish;
	}

	MSG_OUT("Opening %s\n", BT_BMC_PATH);
	context->fds[BT_FD].fd = open(BT_BMC_PATH, O_RDWR | O_NONBLOCK);
	if (context->fds[BT_FD].fd < 0) {
		r = -errno;
		MSG_ERR("Couldn't open %s with flags O_RDWR: %s\n", BT_BMC_PATH, strerror(errno));
		goto finish;
	}

	MSG_OUT("Creating timer fd\n");
	context->fds[TIMER_FD].fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (context->fds[TIMER_FD].fd < 0) {
		r = -errno;
		MSG_ERR("Couldn't create timer fd: %s\n", strerror(errno));
		goto finish;
	}
	context->fds[SD_BUS_FD].events = POLLIN;
	context->fds[BT_FD].events = POLLIN;
	context->fds[TIMER_FD].events = POLLIN;

	MSG_OUT("Entering polling loop\n");

	while (running) {
		polled = poll(context->fds, TOTAL_FDS, 1000);
		if (polled == 0)
			continue;
		if (polled < 0) {
			r = -errno;
			MSG_ERR("Error from poll(): %s\n", strerror(errno));
			goto finish;
		}
		r = dispatch_sd_bus(context);
		if (r < 0) {
			MSG_ERR("Error handling dbus event: %s\n", strerror(-r));
			goto finish;
		}
		r = dispatch_bt(context);
		if (r < 0) {
			MSG_ERR("Error handling BT event: %s\n", strerror(-r));
			goto finish;
		}
		r = dispatch_timer(context);
		if (r < 0) {
			MSG_ERR("Error handling timer event: %s\n", strerror(-r));
			goto finish;
		}
	}

finish:
	if (bt_q_get_head(context)) {
		MSG_ERR("Unresponded BT Message!\n");
		while (bt_q_dequeue(context));
	}
	close(context->fds[BT_FD].fd);
	close(context->fds[TIMER_FD].fd);
	sd_bus_unref(context->bus);
	free(context);

	return r;
}

