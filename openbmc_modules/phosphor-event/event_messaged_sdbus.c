#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <systemd/sd-bus.h>
#include "message.hpp"
#include "event_messaged_sdbus.h"
#include <syslog.h>

/*****************************************************************************/
/* This set of functions are responsible for interactions with events over   */
/* dbus.  Logs come in a couple of different ways...                         */ 
/*     1) From the calls from acceptHostMessage, acceptTestMessage           */
/*     2) At startup and logs that exist alreafy are re-added                */
/*                                                                           */
/* event_record_t when loaded contain all strings and data stream for a log  */
/*                                                                           */
/* Functions naming convention                                               */
/*     prop_x    : callable dbus properties.                                 */
/*     method_x  : callable dbus functions.                                  */
/*                                                                           */
/*****************************************************************************/
const char *event_path = "/org/openbmc/records/events";

sd_bus      *bus   = NULL;
sd_bus_slot *slot  = NULL;

event_record_t *gCachedRec = NULL;

typedef struct messageEntry_t {

	size_t         logid;
	sd_bus_slot   *messageslot;
	sd_bus_slot   *deleteslot;
	sd_bus_slot   *associationslot;
	event_manager *em;

} messageEntry_t;

static int remove_log_from_dbus(messageEntry_t *node);

static void message_entry_close(messageEntry_t *m)
{
	free(m);
	return;
}

static void message_entry_new(messageEntry_t **m, uint16_t logid, event_manager *em)
{
	*m          = malloc(sizeof(messageEntry_t));
	(*m)->logid = logid;
	(*m)->em    = em;
	return;
}

// After calling this function the gCachedRec will be set
static event_record_t* message_record_open(event_manager *em, uint16_t logid)
{

	int r = 0;
	event_record_t *rec;

	// A simple caching technique because each 
	// property needs to extract data from the
	// same data blob.  
	if (gCachedRec == NULL) {
		if (message_load_log(em, logid, &rec)) {
			gCachedRec = rec;
			return gCachedRec;
		} else 
			return NULL;
	} 

	if (logid == gCachedRec->logid) {
		r = 1;

	} else {
		message_free_log(em, gCachedRec);
		gCachedRec = NULL;

		r = message_load_log(em, logid, &rec);
		if (r)
			gCachedRec = rec;
	}

	return (r ? gCachedRec : NULL);
}

static int prop_message_assoc(sd_bus *bus,
			const char *path,
			const char *interface,
			const char *property,
			sd_bus_message *reply,
			void *userdata,
			sd_bus_error *error)
{
	int r=0;
	messageEntry_t *m = (messageEntry_t*) userdata;
	event_record_t *rec;
	char *p;
	char *token;

	rec = message_record_open(m->em, m->logid);
	if (!rec) {
		fprintf(stderr,"Warning missing event log for %zx\n", m->logid);
		sd_bus_error_set(error,
			SD_BUS_ERROR_FILE_NOT_FOUND,
			"Could not find log file");
		return -1;
	}

	/* strtok manipulates a string.  It turns out that message_record_open */
	/* implements a caching mechcanism which means the oiginal string is   */
	/* To avoid that, I will make a copy and mess with that                */
	p = strdup(rec->association);

	if (!p) {
		/* no association string == no associations */
		sd_bus_error_set(error,
			SD_BUS_ERROR_NO_MEMORY,
			"Not enough memory for association");
		return -1;
	}

	token = strtok(p, " ");

	if (token) {

		r = sd_bus_message_open_container(reply, 'a', "(sss)");
		if (r < 0) {
			fprintf(stderr,"Error opening container %s to reply %s\n", token, strerror(-r));
		}

		while(token) {
			r = sd_bus_message_append(reply, "(sss)", "fru", "event", token);
			if (r < 0) {
				fprintf(stderr,"Error adding properties for %s to reply %s\n", token, strerror(-r));
			}

			token = strtok(NULL, " ");
		}

		r = sd_bus_message_close_container(reply);
	}

	free(p);

	return r;
}


static int prop_message(sd_bus *bus,
			const char *path,
			const char *interface,
			const char *property,
			sd_bus_message *reply,
			void *userdata,
			sd_bus_error *error)
{
	int r=0;
	messageEntry_t *m = (messageEntry_t*) userdata;
	char *p;
	struct tm *tm_info;
	char buffer[36];
	event_record_t *rec;

	rec = message_record_open(m->em, m->logid);
	if (!rec) {
		fprintf(stderr,"Warning missing event log for %zx\n", m->logid);
		sd_bus_error_set(error,
			SD_BUS_ERROR_FILE_NOT_FOUND,
			"Could not find log file");
		return -1;
	}

	if (!strncmp("message", property, 7)) {
		p = rec->message;
	} else if (!strncmp("severity", property, 8)) {
		p = rec->severity;
	} else if (!strncmp("reported_by", property, 11)) {
		p = rec->reportedby;
	} else if (!strncmp("time", property, 4)) {
		tm_info = localtime(&rec->timestamp);
		strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
		p = buffer;
	} else {
		p = "";
	}

	r = sd_bus_message_append(reply, "s", p);
	if (r < 0) {
	    fprintf(stderr,"Error adding property to reply %s\n", strerror(-r));
	}

	return r;
}


static int prop_message_dd(sd_bus *bus,
		       const char *path,
		       const char *interface,
		       const char *property,
		       sd_bus_message *reply,
		       void *userdata,
		       sd_bus_error *error)
{

	event_record_t *rec;
	messageEntry_t *m = (messageEntry_t*) userdata;


	rec = message_record_open(m->em, m->logid);

	if (!rec) {
		sd_bus_error_set(error,
				 SD_BUS_ERROR_FILE_NOT_FOUND,
				 "Could not find log file");

		return -1;
	}
	return sd_bus_message_append_array(reply, 'y', rec->p, rec->n);
}

/////////////////////////////////////////////////////////////
// Receives an array of bytes as an esel error log
// returns the messageid in 2 byte format
//  
//  S1 - Message - Simple sentence about the fail
//  S2 - Severity - How bad of a problem is this
//  S3 - Association - sensor path
//  ay - Detailed data - developer debug information
//
/////////////////////////////////////////////////////////////
static int accept_message(sd_bus_message *m,
				      void *userdata,
				      sd_bus_error *ret_error,
				      char *reportedby)
{
	char *message, *severity, *association;
	size_t   n = 4;
	uint8_t *p;
	int r;
	uint16_t logid;
	event_record_t rec;
	event_manager *em = (event_manager *) userdata;

	r = sd_bus_message_read(m, "sss", &message, &severity, &association);
	if (r < 0) {
		fprintf(stderr, "Error parsing strings: %s\n", 	strerror(-r));
		return r;
	}

	r = sd_bus_message_read_array(m, 'y', (const void **)&p, &n);
	if (r < 0) {
		fprintf(stderr, "Error parsing debug data: %s\n", strerror(-r));
		return r;
	}

	rec.message     = (char*) message;
	rec.severity    = (char*) severity;
	rec.association = (char*) association;
	rec.reportedby  = reportedby;
	rec.p           = (uint8_t*) p;
	rec.n           = n;

	syslog(LOG_NOTICE, "%s %s (%s)", rec.severity, rec.message, rec.association);

	logid = message_create_new_log_event(em, &rec);

	if (logid) 
		r = send_log_to_dbus(em, logid, rec.association);

	return sd_bus_reply_method_return(m, "q", logid);
}

static int method_accept_host_message(sd_bus_message *m,
				      void *userdata,
				      sd_bus_error *ret_error)
{
	return accept_message(m, userdata, ret_error, "Host");
}

static int method_accept_bmc_message(sd_bus_message *m,
				      void *userdata,
				      sd_bus_error *ret_error)
{
	return accept_message(m, userdata, ret_error, "BMC");
}
static int method_accept_test_message(sd_bus_message *m,
				      void *userdata,
				      sd_bus_error *ret_error)
{
	//  Random debug data including, ascii, null, >signed int, max
	uint8_t p[] = {0x30, 0x00, 0x13, 0x7F, 0x88, 0xFF};
	uint16_t logid;
	event_record_t rec;
	event_manager *em = (event_manager *) userdata;

	rec.message     = (char*) "A Test event log just happened";
	rec.severity    = (char*) "Info";
	rec.association = (char*) "/org/openbmc/inventory/system/chassis/motherboard/dimm3 " \
				  "/org/openbmc/inventory/system/chassis/motherboard/dimm2";
	rec.reportedby  = (char*) "Test";
	rec.p           = (uint8_t*) p;
	rec.n           = 6;


	syslog(LOG_NOTICE, "%s %s (%s)", rec.severity, rec.message, rec.association);
	logid = message_create_new_log_event(em, &rec);

	if (logid)
		send_log_to_dbus(em, logid, rec.association);

	return sd_bus_reply_method_return(m, "q", logid);
}

static int finish_delete_log(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	return 0;
}

static int method_clearall(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	event_manager *em = (event_manager *) userdata;
	uint16_t logid;
	char buffer[32];
	int r;

	message_refresh_events(em);

	while ((logid = message_next_event(em))) {
		snprintf(buffer, sizeof(buffer),
			"%s/%d", event_path, logid);

		r = sd_bus_call_method_async(bus,
					     NULL,
					     "org.openbmc.records.events",
					     buffer,
					     "org.openbmc.Object.Delete",
					     "delete",
					     finish_delete_log,
					     NULL,
					     NULL);
		if (r < 0) {
			fprintf(stderr,
				"sd_bus_call_method_async Failed : %s\n",
				strerror(-r));
			return -1;
		}
	}

	return sd_bus_reply_method_return(m, "q", 0);
}


static int method_deletelog(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
	messageEntry_t *p = (messageEntry_t *) userdata;

	message_delete_log(p->em, p->logid);
	remove_log_from_dbus(p);
	return sd_bus_reply_method_return(m, "q", 0);
}



static const sd_bus_vtable recordlog_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("acceptHostMessage", "sssay", "q", method_accept_host_message, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("acceptBMCMessage", "sssay", "q", method_accept_bmc_message, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("acceptTestMessage", NULL, "q", method_accept_test_message, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("clear", NULL, "q", method_clearall, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

static const sd_bus_vtable log_vtable[] = {
	SD_BUS_VTABLE_START(0),   
	SD_BUS_PROPERTY("message",     "s",  prop_message,    0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("severity",    "s",  prop_message,    0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("reported_by", "s",  prop_message,    0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("time",        "s",  prop_message,    0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("debug_data",  "ay", prop_message_dd ,0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_VTABLE_END
};


static const sd_bus_vtable recordlog_delete_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("delete", NULL, "q", method_deletelog, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END
};

static const sd_bus_vtable recordlog_association_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_PROPERTY("associations", "a(sss)",  prop_message_assoc, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_VTABLE_END
};

static int remove_log_from_dbus(messageEntry_t *p)
{
	int r;
	char buffer[32];

	snprintf(buffer, sizeof(buffer), "%s/%zu", event_path, p->logid);

	printf("Attempting to delete %s\n", buffer);

	r = sd_bus_emit_object_removed(bus, buffer);
	if (r < 0) {
		fprintf(stderr, "Failed to emit the delete signal %s\n", strerror(-r));
		return -1;
	}	
	sd_bus_slot_unref(p->messageslot);
	sd_bus_slot_unref(p->deleteslot);

	if (p->associationslot)
		sd_bus_slot_unref(p->associationslot);

	message_entry_close(p);

	return 0;
}

int send_log_to_dbus(event_manager *em, const uint16_t logid, const char *association)
{
	char loglocation[64];
	int r;
	messageEntry_t *m;

	snprintf(loglocation, sizeof(loglocation), "%s/%d", event_path, logid);

	message_entry_new(&m, logid, em);

	r = sd_bus_add_object_vtable(bus,
				     &m->messageslot,
				     loglocation,
				     "org.openbmc.record",
				     log_vtable,
				     m);
	if (r < 0) {
		fprintf(stderr, "Failed to acquire service name: %s %s\n",
			loglocation, strerror(-r));
		message_entry_close(m);
		return 0;
	}

	r = sd_bus_add_object_vtable(bus,
				     &m->deleteslot,
				     loglocation,
				     "org.openbmc.Object.Delete",
				     recordlog_delete_vtable,
				     m);

	if (r < 0) {
		fprintf(stderr, "Failed to add delete object for: %s, %s\n",
			loglocation, strerror(-r));
		message_entry_close(m);
		return 0;
	}

	m->associationslot = NULL;
	if (strlen(association) > 0) {
		r = sd_bus_add_object_vtable(bus,
					     &m->associationslot,
					     loglocation,
					     "org.openbmc.Associations",
					     recordlog_association_vtable,
					     m);
		if (r < 0) {
			fprintf(stderr, "Failed to add association object for: %s %s\n",
				loglocation, strerror(-r));
			message_entry_close(m);
			return 0;
		}
	}

	r = sd_bus_emit_object_added(bus, loglocation);
	if (r < 0) {
		fprintf(stderr, "Failed to emit signal %s\n", strerror(-r));
		message_entry_close(m);
		return 0;
	}

	return logid;
}


int start_event_monitor(void)
{
	int r;

	for (;;) {

		r = sd_bus_process(bus, NULL);
		if (r < 0) {
			fprintf(stderr, "Error bus process: %s\n", strerror(-r));
			break;
		}

		if (r > 0)
			continue;

		r = sd_bus_wait(bus, (uint64_t) -1);
		if (r < 0) {
			fprintf(stderr, "Error in sd_bus_wait: %s\n", strerror(-r));
			break;
		}
	}

	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}


/* Only thing we are doing in this function is to get a connection on the dbus */
int build_bus(event_manager *em)
{

	int r = 0;

	/* Connect to the system bus */
	r = sd_bus_open_system(&bus);
	if (r < 0) {
		fprintf(stderr, "Error connecting to system bus: %s\n", strerror(-r));
		goto finish;
	}

	/* Install the object */
	r = sd_bus_add_object_vtable(bus,
				     &slot,
				     "/org/openbmc/records/events",
				     "org.openbmc.recordlog",
				     recordlog_vtable,
				     em);
	if (r < 0) {
		fprintf(stderr, "Error adding vtable: %s\n", strerror(-r));
		goto finish;
	}

	r = sd_bus_request_name(bus, "org.openbmc.records.events", 0);
	if (r < 0) {
		fprintf(stderr, "Error requesting name: %s\n", strerror(-r));
	}	
	
	/* You want to add an object manager to support deleting stuff  */
	/* without it, dbus can show interfaces that no longer exist */
	r = sd_bus_add_object_manager(bus, NULL, event_path);
	if (r < 0) {
		fprintf(stderr, "Object Manager failure  %s\n", strerror(-r));
	}


	finish:
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

void cleanup_event_monitor(void)
{
	sd_bus_slot_unref(slot);
	sd_bus_unref(bus);
}
