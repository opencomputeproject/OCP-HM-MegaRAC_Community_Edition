/* Copyright 2016 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <byteswap.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <arpa/inet.h>
#include <assert.h>
#include <libflash/arch_flash.h>
#include <libflash/libffs.h>
#include <libflash/blocklevel.h>
#include <libflash/errors.h>
#include <openbmc_intf.h>
#include <openbmc.h>

static const gchar* dbus_object_path = "/org/openbmc/control";
static const gchar* dbus_name = "org.openbmc.control.Flasher";

static GDBusObjectManagerServer *manager = NULL;

#define __aligned(x)			__attribute__((aligned(x)))

#define FILE_BUF_SIZE	0x10000
static uint8_t file_buf[FILE_BUF_SIZE] __aligned(0x1000);

static struct blocklevel_device *bl;
static struct ffs_handle	*ffsh;

static uint8_t FLASH_OK = 0;
static uint8_t FLASH_ERROR = 0x01;
static uint8_t FLASH_SETUP_ERROR = 0x02;

static int
erase_chip(void)
{
	int rc = 0;

	printf("Erasing... (may take a while !) ");
	fflush(stdout);

	rc = arch_flash_erase_chip(bl);
	if(rc) {
		fprintf(stderr, "Error %d erasing chip\n", rc);
		return(rc);
	}

	printf("done !\n");
	return(rc);
}

void
flash_message(GDBusConnection* connection,char* obj_path,char* method, char* error_msg)
{
	GDBusProxy *proxy;
	GError *error;
	GVariant *parm = NULL;
	error = NULL;
	proxy = g_dbus_proxy_new_sync(connection,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, /* GDBusInterfaceInfo* */
			"org.openbmc.control.Flash", /* name */
			obj_path, /* object path */
			"org.openbmc.Flash", /* interface name */
			NULL, /* GCancellable */
			&error);
	g_assert_no_error(error);

	error = NULL;
	if(strcmp(method,"error")==0) {
		parm = g_variant_new("(s)",error_msg);
	}
	g_dbus_proxy_call_sync(proxy,
			method,
			parm,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);

	g_assert_no_error(error);
}

static int
program_file(FlashControl* flash_control, const char *file, uint32_t start, uint32_t size)
{
	int fd, rc;
	ssize_t len;
	uint32_t actual_size = 0;

	fd = open(file, O_RDONLY);
	if(fd == -1) {
		perror("Failed to open file");
		return(fd);
	}
	printf("About to program \"%s\" at 0x%08x..0x%08x !\n",
			file, start, size);

	printf("Programming & Verifying...\n");
	//progress_init(size >> 8);
	unsigned int save_size = size;
	uint8_t last_progress = 0;
	while(size) {
		len = read(fd, file_buf, FILE_BUF_SIZE);
		if(len < 0) {
			perror("Error reading file");
			return(1);
		}
		if(len == 0)
			break;
		if(len > size)
			len = size;
		size -= len;
		actual_size += len;
		rc = blocklevel_write(bl, start, file_buf, len);
		if(rc) {
			if(rc == FLASH_ERR_VERIFY_FAILURE)
				fprintf(stderr, "Verification failed for"
						" chunk at 0x%08x\n", start);
			else
				fprintf(stderr, "Flash write error %d for"
						" chunk at 0x%08x\n", rc, start);
			return(rc);
		}
		start += len;
		unsigned int percent = (100*actual_size/save_size);
		uint8_t progress = (uint8_t)(percent);
		if(progress != last_progress) {
			flash_control_emit_progress(flash_control,file,progress);
			last_progress = progress;
		}
	}
	close(fd);

	return(0);
}

static void
flash_access_cleanup(void)
{
	if(ffsh)
		ffs_close(ffsh);
	arch_flash_close(bl, NULL);
}

static int
flash_access_setup(enum flash_access chip)
{
	int rc;
	printf("Setting up flash\n");

	rc = arch_flash_access(bl, chip);
	if (rc != chip) {
		fprintf(stderr, "Failed to select flash chip\n");
		return FLASH_SETUP_ERROR;
	}

	rc = arch_flash_init(&bl, NULL, 1);
	if (rc) {
		fprintf(stderr, "Failed to init flash: %d\n", rc);
		return FLASH_SETUP_ERROR;
	}

	/* Setup cleanup function */
	atexit(flash_access_cleanup);
	return FLASH_OK;
}

uint8_t
flash(FlashControl* flash_control, enum flash_access chip, uint32_t address, char* write_file, char* obj_path)
{
	int rc;
	printf("flasher: %s, BMC = %d, address = 0x%x\n", write_file, chip, address);

	/* Prepare for access */
	rc = flash_access_setup(chip);
	if(rc) {
		return FLASH_SETUP_ERROR;
	}

	if(strcmp(write_file,"")!=0)
	{
		// If file specified but not size, get size from file
		struct stat stbuf;
		if(stat(write_file, &stbuf)) {
			perror("Failed to get file size");
			return FLASH_ERROR;
		}
		uint32_t write_size = stbuf.st_size;
		rc = erase_chip();
		if(rc) {
			return FLASH_ERROR;
		}
		rc = program_file(flash_control, write_file, address, write_size);
		if(rc) {
			return FLASH_ERROR;
		}

		printf("Flash done\n");
	}
	return FLASH_OK;
}

static void
on_bus_acquired(GDBusConnection *connection,
		const gchar *name,
		gpointer user_data)
{
	cmdline *cmd = user_data;
	if(cmd->argc < 4)
	{
		g_print("flasher [flash name] [filename] [source object]\n");
		g_main_loop_quit(cmd->loop);
		return;
	}
	printf("Starting flasher: %s,%s,%s,\n",cmd->argv[1],cmd->argv[2],cmd->argv[3]);
	ObjectSkeleton *object;
	manager = g_dbus_object_manager_server_new(dbus_object_path);
	gchar *s;
	s = g_strdup_printf("%s/%s",dbus_object_path,cmd->argv[1]);

	object = object_skeleton_new(s);
	g_free(s);

	FlashControl* flash_control = flash_control_skeleton_new();
	object_skeleton_set_flash_control(object, flash_control);
	g_object_unref(flash_control);

	/* Export the object (@manager takes its own reference to @object) */
	g_dbus_object_manager_server_export(manager, G_DBUS_OBJECT_SKELETON(object));
	g_object_unref(object);

	/* Export all objects */
	g_dbus_object_manager_server_set_connection(manager, connection);
	enum flash_access chip = PNOR_MTD;
	uint32_t address = 0;

	/* TODO: Look up all partitions from the device tree or /proc/mtd */
	if(strcmp(cmd->argv[1],"bmc")==0) {
		chip = BMC_MTD;
		address = 0;
	}
	if(strcmp(cmd->argv[1],"bmc_ramdisk")==0) {
		chip = BMC_MTD;
		/* TODO: Look up from device tree or similar */
		address = 0x300000;
	}
	if(strcmp(cmd->argv[1],"bmc_kernel")==0) {
		chip = BMC_MTD;
		/* TODO: Look up from device tree or similar */
		address = 0x80000;
	}

	int rc = flash(flash_control, chip, address, cmd->argv[2], cmd->argv[3]);
	if(rc) {
		flash_message(connection,cmd->argv[3],"error","Flash Error");
	} else {
		flash_message(connection,cmd->argv[3],"done","");
	}

	//Object exits when done flashing
	g_main_loop_quit(cmd->loop);
}

int
main(int argc, char *argv[])
{
	GMainLoop *loop;
	cmdline cmd;
	cmd.argc = argc;
	cmd.argv = argv;

	guint id;
	loop = g_main_loop_new(NULL, FALSE);
	cmd.loop = loop;

	id = g_bus_own_name(DBUS_TYPE,
			dbus_name,
			G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
			G_BUS_NAME_OWNER_FLAGS_REPLACE,
			on_bus_acquired,
			NULL,
			NULL,
			&cmd,
			NULL);

	g_main_loop_run(loop);

	g_bus_unown_name(id);
	g_main_loop_unref(loop);

	return 0;
}
