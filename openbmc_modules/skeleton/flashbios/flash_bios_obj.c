#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <openbmc_intf.h>
#include <openbmc.h>

/* ------------------------------------------------------------------------- */
static const gchar* dbus_object_path = "/org/openbmc/control/flash";
static const gchar* dbus_name = "org.openbmc.control.Flash";
static const gchar* FLASHER_BIN = "flasher.exe";

static GDBusObjectManagerServer *manager = NULL;

void
catch_child(int sig_num)
{
	/* when we get here, we know there's a zombie child waiting */
	int child_status;

	wait(&child_status);
	printf("flasher exited.\n");
}

int
update(Flash* flash, const char* obj_path)
{
	pid_t pid;
	int status=-1;
	pid = fork();
	if(pid == 0)
	{
		const gchar* name = flash_get_flasher_name(flash);
		const gchar* inst = flash_get_flasher_instance(flash);
		const gchar* filename = flash_get_filename(flash);
		status = execlp(name, name, inst, filename, obj_path, NULL);
		return status;
	}
	return 0;
}

static gboolean
on_init(Flash *f,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	flash_complete_init(f,invocation);

	//tune flash
	if(strcmp(flash_get_flasher_instance(f),"bios") == 0)
	{
		flash_set_filename(f,"");
		const gchar* obj_path = g_dbus_object_get_object_path((GDBusObject*)user_data);
		int rc = update(f,obj_path);
		if(rc==-1)
		{
			printf("ERROR FlashControl: Unable to init\n");
		}
		sleep(3);
		rc = update(f,obj_path);

	}
	return TRUE;
}

static gboolean
on_lock(SharedResource *lock,
		GDBusMethodInvocation *invocation,
		gchar* name,
		gpointer user_data)
{
	gboolean locked = shared_resource_get_lock(lock);
	if(locked)
	{
		const gchar* name = shared_resource_get_name(lock);
		printf("ERROR: BIOS Flash is already locked: %s\n",name);
	}
	else
	{
		printf("Locking BIOS Flash: %s\n",name);
		shared_resource_set_lock(lock,true);
		shared_resource_set_name(lock,name);
	}
	shared_resource_complete_lock(lock,invocation);
	return TRUE;
}

static gboolean
on_is_locked(SharedResource *lock,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	gboolean locked = shared_resource_get_lock(lock);
	const gchar* name = shared_resource_get_name(lock);
	shared_resource_complete_is_locked(lock,invocation,locked,name);
	return TRUE;
}

static gboolean
on_unlock(SharedResource *lock,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	printf("Unlocking BIOS Flash\n");
	shared_resource_set_lock(lock,false);
	shared_resource_set_name(lock,"");
	shared_resource_complete_unlock(lock,invocation);
	return TRUE;
}

static gboolean
on_update_via_tftp(Flash *flash,
		GDBusMethodInvocation *invocation,
		gchar* url,
		gchar* write_file,
		gpointer user_data)
{
	SharedResource *lock = object_get_shared_resource((Object*)user_data);
	gboolean locked = shared_resource_get_lock(lock);
	flash_complete_update_via_tftp(flash,invocation);
	if(locked)
	{
		const gchar* name = shared_resource_get_name(lock);
		printf("BIOS Flash is locked: %s\n",name);
	}
	else
	{
		printf("Flashing BIOS from TFTP: %s,%s\n",url,write_file);
		flash_set_filename(flash,write_file);
		flash_emit_download(flash,url,write_file);
		flash_set_status(flash,"Downloading");
	}
	return TRUE;
}

static gboolean
on_error(Flash *flash,
		GDBusMethodInvocation *invocation,
		gchar* error_msg,
		gpointer user_data)
{
	SharedResource *lock = object_get_shared_resource((Object*)user_data);
	shared_resource_get_lock(lock);
	flash_set_status(flash, error_msg);
	flash_complete_error(flash,invocation);
	printf("ERROR: %s.  Clearing locks\n",error_msg);
	shared_resource_set_lock(lock,false);
	shared_resource_set_name(lock,"");

	return TRUE;
}

static gboolean
on_done(Flash *flash,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	int rc = 0;
	SharedResource *lock = object_get_shared_resource((Object*)user_data);
	shared_resource_get_lock(lock);
	flash_set_status(flash, "Flash Done");
	flash_complete_done(flash,invocation);
	printf("Flash Done. Clearing locks\n");
	shared_resource_set_lock(lock,false);
	shared_resource_set_name(lock,"");
	const gchar* filename = flash_get_filename(flash);
	rc = unlink(filename);
	if(rc != 0 )
	{
		printf("ERROR: Unable to delete file %s (%d)\n",filename,rc);
	}

	return TRUE;
}

static gboolean
on_update(Flash *flash,
		GDBusMethodInvocation *invocation,
		gchar* write_file,
		gpointer user_data)
{
	int rc = 0;
	SharedResource *lock = object_get_shared_resource((Object*)user_data);
	gboolean locked = shared_resource_get_lock(lock);
	flash_set_status(flash,"Flashing");
	flash_complete_update(flash,invocation);
	if(locked)
	{
		const gchar* name = shared_resource_get_name(lock);
		printf("BIOS Flash is locked: %s\n",name);
	}
	else
	{
		printf("Flashing BIOS from: %s\n",write_file);
		flash_set_status(flash, "Flashing");
		shared_resource_set_lock(lock,true);
		shared_resource_set_name(lock,dbus_object_path);
		flash_set_filename(flash,write_file);
		const gchar* obj_path = g_dbus_object_get_object_path((GDBusObject*)user_data);
		rc = update(flash,obj_path);
		if(!rc)
		{
			shared_resource_set_lock(lock,false);
			shared_resource_set_name(lock,"");
		}
	}
	return TRUE;
}

static void
on_flash_progress(GDBusConnection* connection,
		const gchar* sender_name,
		const gchar* object_path,
		const gchar* interface_name,
		const gchar* signal_name,
		GVariant* parameters,
		gpointer user_data)
{
	Flash *flash = object_get_flash((Object*)user_data);
	object_get_shared_resource((Object*)user_data);
	GVariantIter *iter = g_variant_iter_new(parameters);
	g_variant_iter_next_value(iter);
	GVariant* v_progress = g_variant_iter_next_value(iter);

	uint8_t progress = g_variant_get_byte(v_progress);

	gchar *s;
	s = g_strdup_printf("Flashing: %d%%",progress);
	flash_set_status(flash,s);
	g_free(s);
}

static void
on_bus_acquired(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	ObjectSkeleton *object;
	cmdline *cmd = user_data;
	manager = g_dbus_object_manager_server_new(dbus_object_path);
	int i=0;

	gchar *flasher_file = g_strdup_printf("%s", FLASHER_BIN);

	const char* inst[] = {"bios"};
	for(i=0;i<1;i++)
	{
		gchar* s;
		s = g_strdup_printf("%s/%s",dbus_object_path,inst[i]);
		object = object_skeleton_new(s);
		g_free(s);

		Flash* flash = flash_skeleton_new();
		object_skeleton_set_flash(object, flash);
		g_object_unref(flash);

		SharedResource* lock = shared_resource_skeleton_new();
		object_skeleton_set_shared_resource(object, lock);
		g_object_unref(lock);

		shared_resource_set_lock(lock,false);
		shared_resource_set_name(lock,"");

		flash_set_flasher_name(flash,FLASHER_BIN);
		flash_set_flasher_instance(flash,inst[i]);
		//g_free (s);


		//define method callbacks here
		g_signal_connect(lock,
				"handle-lock",
				G_CALLBACK(on_lock),
				NULL); /* user_data */
		g_signal_connect(lock,
				"handle-unlock",
				G_CALLBACK(on_unlock),
				NULL); /* user_data */
		g_signal_connect(lock,
				"handle-is-locked",
				G_CALLBACK(on_is_locked),
				NULL); /* user_data */

		g_signal_connect(flash,
				"handle-update",
				G_CALLBACK(on_update),
				object); /* user_data */

		g_signal_connect(flash,
				"handle-error",
				G_CALLBACK(on_error),
				object); /* user_data */

		g_signal_connect(flash,
				"handle-done",
				G_CALLBACK(on_done),
				object); /* user_data */

		g_signal_connect(flash,
				"handle-update-via-tftp",
				G_CALLBACK(on_update_via_tftp),
				object); /* user_data */

		g_signal_connect(flash,
				"handle-init",
				G_CALLBACK(on_init),
				object); /* user_data */

		s = g_strdup_printf("/org/openbmc/control/%s",inst[i]);
		g_dbus_connection_signal_subscribe(connection,
				NULL,
				"org.openbmc.FlashControl",
				"Progress",
				s,
				NULL,
				G_DBUS_SIGNAL_FLAGS_NONE,
				(GDBusSignalCallback) on_flash_progress,
				object,
				NULL );

		g_free(s);


		flash_set_filename(flash,"");
		/* Export the object (@manager takes its own reference to @object) */
		g_dbus_object_manager_server_set_connection(manager, connection);
		g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
		g_object_unref(object);
	}
	g_free(flasher_file);
}

static void
on_name_acquired(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	//  g_print ("Acquired the name %s\n", name);
}

static void
on_name_lost(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	//g_print ("Lost the name %s\n", name);
}

gint
main(gint argc, gchar *argv[])
{
	GMainLoop *loop;
	cmdline cmd;
	cmd.argc = argc;
	cmd.argv = argv;
	guint id;
	loop = g_main_loop_new(NULL, FALSE);

	signal(SIGCHLD, catch_child);
	id = g_bus_own_name(DBUS_TYPE,
			dbus_name,
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
			G_BUS_NAME_OWNER_FLAGS_REPLACE,
			on_bus_acquired,
			on_name_acquired,
			on_name_lost,
			&cmd,
			NULL);

	g_main_loop_run(loop);

	g_bus_unown_name(id);
	g_main_loop_unref(loop);
	return 0;
}
