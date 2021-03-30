#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <openbmc_intf.h>
#include <openbmc.h>

/* ------------------------------------------------------------------------- */
static const gchar* dbus_object_path = "/org/openbmc/control";
static const gchar* instance_name = "bmc0";
static const gchar* dbus_name = "org.openbmc.control.Bmc";

static GDBusObjectManagerServer *manager = NULL;

static gboolean on_init(Control *control, GDBusMethodInvocation *invocation,
                        gpointer user_data)
{
    control_complete_init(control, invocation);
    return TRUE;
}

static gboolean on_warm_reset(ControlBmc *bmc,
                              GDBusMethodInvocation *invocation,
                              gpointer user_data)
{
    /* Wait a while before reboot, so the caller can be responded.
     */
    const char *reboot_command = "/bin/sh -c 'sleep 3;reboot'&";
    system(reboot_command);

    control_bmc_complete_warm_reset(bmc, invocation);
    return TRUE;
}

static gboolean on_cold_reset(ControlBmc *bmc,
                              GDBusMethodInvocation *invocation,
                              gpointer user_data)
{
    GError *err = NULL;
    /* Wait a while before reboot, so the caller can be responded.
     * Note that g_spawn_command_line_async() cannot parse ';' as
     * a command separator. Need to use 'sh -c' to let shell parse it.
     */
    gchar *reboot_command = "/bin/sh -c 'sleep 3;reboot'";

    g_spawn_command_line_async(reboot_command, &err);
    if(err != NULL) {
       fprintf(stderr, "coldReset() error: %s\n", err->message);
       g_error_free(err);
    }

    control_bmc_complete_cold_reset(bmc, invocation);
    return TRUE;
}

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
                            gpointer user_data)
{
    ObjectSkeleton *object;
    cmdline *cmd = user_data;
    manager = g_dbus_object_manager_server_new(dbus_object_path);

    gchar *s;
    s = g_strdup_printf("%s/%s", dbus_object_path, instance_name);
    object = object_skeleton_new(s);
    g_free(s);

    ControlBmc* control_bmc = control_bmc_skeleton_new();
    object_skeleton_set_control_bmc(object, control_bmc);
    g_object_unref(control_bmc);

    Control* control = control_skeleton_new();
    object_skeleton_set_control(object, control);
    g_object_unref(control);

    //define method callbacks here
    g_signal_connect(control, "handle-init", G_CALLBACK(on_init), NULL); /* user_data */

    g_signal_connect(control_bmc, "handle-warm-reset",
                     G_CALLBACK(on_warm_reset), NULL); /* user_data */

    g_signal_connect(control_bmc, "handle-cold-reset",
                     G_CALLBACK(on_cold_reset), NULL); /* user_data */

    /* Export the object (@manager takes its own reference to @object) */
    g_dbus_object_manager_server_export(manager,
                                        G_DBUS_OBJECT_SKELETON(object));
    g_object_unref(object);

    /* Export all objects */
    g_dbus_object_manager_server_set_connection(manager, connection);

    cmd->user_data = object;
}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
                             gpointer user_data)
{
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
                         gpointer user_data)
{
}

/*----------------------------------------------------------------*/
/* Main Event Loop                                                */

gint main(gint argc, gchar *argv[])
{
    GMainLoop *loop;
    cmdline cmd;
    cmd.argc = argc;
    cmd.argv = argv;

    guint id;
    loop = g_main_loop_new(NULL, FALSE);
    cmd.loop = loop;

    id = g_bus_own_name(
            DBUS_TYPE,
            dbus_name,
            G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT
            | G_BUS_NAME_OWNER_FLAGS_REPLACE,
            on_bus_acquired, on_name_acquired, on_name_lost, &cmd, NULL);

    g_main_loop_run(loop);

    g_bus_unown_name(id);
    g_main_loop_unref(loop);
    return 0;
}
