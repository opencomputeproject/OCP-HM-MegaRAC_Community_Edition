/*
 * Copyright (c) 2015 IBM Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define _BSD_SOURCE

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-bus-vtable.h>

#include <ipmitool/log.h>
#include <ipmitool/ipmi.h>
#include <ipmitool/ipmi_intf.h>

static sd_bus *bus;
static uint8_t seq;
static struct ipmi_rs rsp;
static bool reply_received;

static const char *bus_name = "org.openbmc.HostIpmi.ipmitool";
static const char *object_path = "/org/openbmc/HostIpmi/ipmitool";
static const char *interface = "org.openbmc.HostIpmi";

static struct ipmi_rs *ipmi_dbus_sendrecv(struct ipmi_intf *intf,
		struct ipmi_rq *req)
{
	sd_bus_message *msg;
	int rc;

	(void)intf;

	rsp.ccode = 0xff;

	rc = sd_bus_message_new_signal(bus, &msg, object_path,
			interface, "ReceivedMessage");
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to create message: %s\n",
				__func__, strerror(-rc));
		goto out;
	}

	rc = sd_bus_message_append(msg, "yyyy",
			++seq,
			req->msg.netfn,
			req->msg.lun,
			req->msg.cmd);
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to init bytes\n", __func__);
		goto out_free;
	}

	rc = sd_bus_message_append_array(msg, 'y', req->msg.data,
			req->msg.data_len);
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to init body\n", __func__);
		goto out_free;
	}

	rc = sd_bus_send(bus, msg, NULL);
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to send dbus message\n",
				__func__);
		goto out_free;
	}

	for (reply_received = false; !reply_received;) {
		rc = sd_bus_wait(bus, -1);
		sd_bus_process(bus, NULL);
	}

out_free:
	sd_bus_message_unref(msg);
out:
	return &rsp;
}

static int ipmi_dbus_method_send_message(sd_bus_message *msg, void *userdata,
		sd_bus_error *error)
{
	uint8_t recv_seq, recv_netfn, recv_lun, recv_cmd, recv_cc;
	const void *data;
	size_t n;
	int rc;

	(void)userdata;
	(void)error;

	rc = sd_bus_message_read(msg, "yyyyy", &recv_seq, &recv_netfn,
			&recv_lun, &recv_cmd, &recv_cc);
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to read reply\n", __func__);
		goto out;
	}

	rc = sd_bus_message_read_array(msg, 'y', &data, &n);
	if (rc < 0) {
		lprintf(LOG_ERR, "%s: failed to read reply data\n", __func__);
		goto out;
	}

	if (n > sizeof(rsp.data)) {
		lprintf(LOG_ERR, "%s: data too long!\n", __func__);
		goto out;
	}

	if (recv_seq == seq) {
		rsp.ccode = recv_cc;
		rsp.data_len = n;
		memcpy(rsp.data, data, rsp.data_len);
		reply_received = true;
	}

out:
	sd_bus_reply_method_return(msg, "x", 0);
	return 0;
}

static const sd_bus_vtable dbus_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_SIGNAL("ReceivedMessage", "yyyyay", 0),
	SD_BUS_METHOD("sendMessage", "yyyyyay", "x",
			ipmi_dbus_method_send_message,
			SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

static int ipmi_dbus_setup(struct ipmi_intf *intf)
{
	const char *name;
	int rc;

	rc = sd_bus_default(&bus);
	if (rc < 0) {
		lprintf(LOG_ERR, "Can't connect to session bus: %s\n",
				strerror(-rc));
		return -1;
	}

	sd_bus_add_object_vtable(bus, NULL, object_path, interface,
			dbus_vtable, NULL);

	sd_bus_request_name(bus, bus_name, SD_BUS_NAME_REPLACE_EXISTING);

	sd_bus_flush(bus);
	sd_bus_get_unique_name(bus, &name);
	intf->opened = 1;

	return 0;
}

static void ipmi_dbus_close(struct ipmi_intf *intf)
{
	if (intf->opened)
		sd_bus_close(bus);
	intf->opened = 0;
}

struct ipmi_intf ipmi_dbus_intf = {
	.name		= "dbus",
	.desc		= "OpenBMC dbus interface",
	.setup		= ipmi_dbus_setup,
	.close		= ipmi_dbus_close,
	.sendrecv	= ipmi_dbus_sendrecv,
};
