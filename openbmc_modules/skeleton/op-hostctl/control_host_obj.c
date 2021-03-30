#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include <openbmc_intf.h>
#include <openbmc.h>

/* ------------------------------------------------------------------------- */
static const gchar* dbus_object_path = "/org/openbmc/control";
static const gchar* instance_name = "host0";
static const gchar* dbus_name = "org.openbmc.control.Host";

static GDBusObjectManagerServer *manager = NULL;

#define PPC_BIT32(bit)          (0x80000000UL >> (bit))

#define FSI_EXTERNAL_MODE_PATH	"/sys/devices/platform/gpio-fsi/external_mode"
#define FSI_SCAN_PATH		"/sys/devices/platform/gpio-fsi/fsi0/rescan"

/* TODO: Change this over to the cfam path once the cfam chardev patches have landed */
#define FSI_RAW_PATH		"/sys/devices/platform/gpio-fsi/fsi0/slave@00:00/raw"

#define FSI_SCAN_DELAY_US	10000

/* Attention registers */
#define FSI_A_SI1S		0x081c
#define TRUE_MASK		0x100d
#define INTERRUPT_STATUS_REG	0x100b

/* SBE boot register and values */
#define SBE_VITAL		0x281c
#define SBE_WARMSTART		PPC_BIT32(0)
#define SBE_HW_TRIGGER		PPC_BIT32(2)
#define SBE_UPDATE_1ST_NIBBLE	PPC_BIT32(3)
#define SBE_IMAGE_SELECT	PPC_BIT32(8)
#define SBE_UPDATE_3RD_NIBBLE	PPC_BIT32(11)

/* Once the side is selected and attention bits are set, this starts the SBE */
#define START_SBE		(SBE_WARMSTART | SBE_HW_TRIGGER | SBE_UPDATE_1ST_NIBBLE)

/* Primary is first side. Golden is second side */
#define PRIMARY_SIDE		(SBE_HW_TRIGGER | SBE_UPDATE_1ST_NIBBLE)
#define GOLDEN_SIDE		(SBE_HW_TRIGGER | SBE_UPDATE_1ST_NIBBLE | \
				 SBE_IMAGE_SELECT | SBE_UPDATE_3RD_NIBBLE)

static gboolean
on_init(Control *control,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	control_complete_init(control,invocation);
	return TRUE;
}

static gint
fsi_putcfam(int fd, uint64_t addr64, uint32_t val_host)
{
	int rc;
	uint32_t val = htobe32(val_host);
	/* Map FSI to FSI_BYTE, as the 'raw' kernel interface expects this */
	uint32_t addr = (addr64 & 0x7ffc00) | ((addr64 & 0x3ff) << 2);

	rc = lseek(fd, addr, SEEK_SET);
	if (rc < 0) {
		g_print("ERROR HostControl: cfam seek failed (0x%08x): %s\n", addr,
				strerror(errno));
		return errno;
	};

	rc = write(fd, &val, sizeof(val));
	if (rc < 0) {
		g_print("ERROR HostControl: cfam write failed: %s\n",
				strerror(errno));
		return errno;
	}

	return 0;
}

static int fsi_rescan(void)
{
	char *one = "1";
	int fd, rc;

	fd = open(FSI_SCAN_PATH, O_WRONLY);
	if (fd < 0) {
		g_print("ERROR HostControl: Failed to open path '%s': %s\n",
				FSI_SCAN_PATH, strerror(errno));
		return errno;
	}
	rc = write(fd, one, sizeof(one));
	close(fd);
	if (rc < 0) {
		g_print("ERROR HostControl: Failed to perform FSI scan: %s\n",
				strerror(errno));
		return errno;
	}
	g_print("HostControl: Performing FSI scan (delay %d us)\n",
			FSI_SCAN_DELAY_US);
	usleep(FSI_SCAN_DELAY_US);

	return 0;
}

static gboolean
on_boot(ControlHost *host,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	int rc, cfam_fd;
	GDBusProxy *proxy;
	GError *error = NULL;
	GDBusConnection *connection =
		g_dbus_object_manager_server_get_connection(manager);

	if(control_host_get_debug_mode(host)==1) {
		int fd;
		char *one = "1";
		g_print("Enabling debug mode; not booting host\n");
		fd = open(FSI_EXTERNAL_MODE_PATH, O_RDWR);
		if (fd < 0) {
			g_print("ERROR HostControl: Failed to open path '%s'\n",
					FSI_EXTERNAL_MODE_PATH);
			return TRUE;
		}
		rc = write(fd, one, sizeof(one));
		if (rc < 0) {
			g_print("ERROR HostControl: Failed to enable debug mode '%s'\n",
					FSI_EXTERNAL_MODE_PATH);
		}
		close(fd);
		return TRUE;
	}
	g_print("Booting host\n");

	rc = fsi_rescan();
	if (rc < 0)
		return FALSE;

	cfam_fd = open(FSI_RAW_PATH, O_RDWR);
	if (cfam_fd < 0) {
		g_print("ERROR HostControl: Failed to open '%s'\n", FSI_RAW_PATH);
		return FALSE;
	}

	Control* control = object_get_control((Object*)user_data);
	control_host_complete_boot(host,invocation);
	do {
		rc = fsi_putcfam(cfam_fd, FSI_A_SI1S, 0x20000000);
		rc |= fsi_putcfam(cfam_fd, TRUE_MASK, 0x40000000);
		rc |= fsi_putcfam(cfam_fd, INTERRUPT_STATUS_REG, 0xFFFFFFFF);
		if(rc) { break; }

		const gchar* flash_side = control_host_get_flash_side(host);
		g_print("Using %s side of the bios flash\n",flash_side);
		if(strcmp(flash_side,"primary")==0) {
			rc |= fsi_putcfam(cfam_fd, SBE_VITAL, PRIMARY_SIDE);
		} else if(strcmp(flash_side,"golden") == 0) {
			rc |= fsi_putcfam(cfam_fd, SBE_VITAL, GOLDEN_SIDE);
		} else {
			g_print("ERROR: Invalid flash side: %s\n",flash_side);
			rc = 0xff;

		}
		if(rc) { break; }

		rc = fsi_putcfam(cfam_fd, SBE_VITAL, START_SBE);
	} while(0);
	if(rc)
	{
		g_print("ERROR HostControl: SBE sequence failed (rc=%d)\n",rc);
	}
	/* Close file descriptor */
	close(cfam_fd);

	control_host_emit_booted(host);

	return TRUE;
}

static void
on_bus_acquired(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	ObjectSkeleton *object;
	//g_print ("Acquired a message bus connection: %s\n",name);
	manager = g_dbus_object_manager_server_new(dbus_object_path);

	gchar *s;
	s = g_strdup_printf("%s/%s",dbus_object_path,instance_name);
	object = object_skeleton_new(s);
	g_free(s);

	ControlHost* control_host = control_host_skeleton_new();
	object_skeleton_set_control_host(object, control_host);
	g_object_unref(control_host);

	Control* control = control_skeleton_new();
	object_skeleton_set_control(object, control);
	g_object_unref(control);

	//define method callbacks here
	g_signal_connect(control_host,
			"handle-boot",
			G_CALLBACK(on_boot),
			object); /* user_data */
	g_signal_connect(control,
			"handle-init",
			G_CALLBACK(on_init),
			NULL); /* user_data */

	control_host_set_debug_mode(control_host,0);
	control_host_set_flash_side(control_host,"primary");

	/* Export the object (@manager takes its own reference to @object) */
	g_dbus_object_manager_server_set_connection(manager, connection);
	g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
	g_object_unref(object);
}

static void
on_name_acquired(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	// g_print ("Acquired the name %s\n", name);
}

static void
on_name_lost(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	// g_print ("Lost the name %s\n", name);
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
