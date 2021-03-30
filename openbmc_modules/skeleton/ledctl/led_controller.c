#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <systemd/sd-bus.h>

static int led_stable_state_function(const char *, const char *);
static int led_default_blink(const char *, const char *);
static int read_led(const char *, const char *, void *, const size_t);
static int led_custom_blink(const char *, sd_bus_message *);

/*
 * These are control files that are present for each led under
 *'/sys/class/leds/<led_name>/' which are used to trigger action
 * on the respective leds by writing predefined data.
 */
const char *power_ctrl = "brightness";
const char *blink_ctrl = "trigger";
const char *duty_on = "delay_on";
const char *duty_off = "delay_off";

/*
 * --------------------------------------------------
 * Given the dbus path, returns the name of the LED
 * --------------------------------------------------
 */
char *
get_led_name(const char *dbus_path)
{
	char *led_name = NULL;

	/* Get the led name from /org/openbmc/control/led/<name> */
	led_name = strrchr(dbus_path, '/');
	if(led_name)
	{
		led_name++;
	}

	return led_name;
}

/*
 * -------------------------------------------------------------------------
 * Writes the 'on / off / blink' trigger to leds.
 * -------------------------------------------------------------------------
 */
int
write_to_led(const char *name, const char *ctrl_file, const char *value)
{
	/* Generic error reporter. */
	int rc = -1;

	/* To get /sys/class/leds/<name>/<control file> */
	char led_path[128] = {0};

	int len = 0;
	len = snprintf(led_path, sizeof(led_path),
			"/sys/class/leds/%s/%s",name, ctrl_file);
	if(len >= sizeof(led_path))
	{
		fprintf(stderr, "Error. LED path is too long. :[%d]\n",len);
		return rc;
	}

	FILE *fp = fopen(led_path,"w");
	if(fp == NULL)
	{
		fprintf(stderr,"Error:[%s] opening:[%s]\n",strerror(errno),led_path);
		return rc;
	}

	rc = fwrite(value, strlen(value), 1, fp);
	if(rc != 1)
	{
		fprintf(stderr, "Error:[%s] writing to :[%s]\n",strerror(errno),led_path);
	}

	fclose(fp);

	/* When we get here, rc would be what it was from writing to the file */
	return (rc == 1) ? 0 : -1;
}

/*
 * ----------------------------------------------------------------
 * Router function for any LED operations that come via dbus
 *----------------------------------------------------------------
 */
static int
led_function_router(sd_bus_message *msg, void *user_data,
		sd_bus_error *ret_error)
{
	/* Generic error reporter. */
	int rc = -1;

	/* Extract the led name from the full dbus path */
	const char *led_path = sd_bus_message_get_path(msg);
	if(led_path == NULL)
	{
		fprintf(stderr, "Error. LED path is empty");
		return sd_bus_reply_method_return(msg, "i", rc);
	}

	char *led_name = get_led_name(led_path);
	if(led_name == NULL)
	{
		fprintf(stderr, "Invalid LED name for path :[%s]\n",led_path);
		return sd_bus_reply_method_return(msg, "i", rc);
	}

	/* Now that we have the LED name, get the Operation. */
	const char *led_function = sd_bus_message_get_member(msg);
	if(led_function == NULL)
	{
		fprintf(stderr, "Null LED function specified for : [%s]\n",led_name);
		return sd_bus_reply_method_return(msg, "i", rc);
	}

	/* Route the user action to appropriate handlers. */
	if( (strcmp(led_function, "setOn") == 0) ||
			(strcmp(led_function, "setOff") == 0))
	{
		rc = led_stable_state_function(led_name, led_function);
		return sd_bus_reply_method_return(msg, "i", rc);
	}
	else if( (strcmp(led_function, "setBlinkFast") == 0) ||
			(strcmp(led_function, "setBlinkSlow") == 0))
	{
		rc = led_default_blink(led_name, led_function);
		return sd_bus_reply_method_return(msg, "i", rc);
	}
	else if(strcmp(led_function, "BlinkCustom") == 0)
	{
		rc = led_custom_blink(led_name, msg);
		return sd_bus_reply_method_return(msg, "i", rc);
	}
	else if(strcmp(led_function, "GetLedState") == 0)
	{
		char value_str[10] = {0};
		const char *led_state = NULL;

		rc = read_led(led_name, power_ctrl, value_str, sizeof(value_str)-1);
		if(rc >= 0)
		{
			/* LED is active HI */
			led_state = strtoul(value_str, NULL, 0) ? "On" : "Off";
		}
		return sd_bus_reply_method_return(msg, "is", rc, led_state);
	}
	else
	{
		fprintf(stderr,"Invalid LED function:[%s]\n",led_function);
	}

	return sd_bus_reply_method_return(msg, "i", rc);
}

/*
 * --------------------------------------------------------------
 * Turn On or Turn Off the LED
 * --------------------------------------------------------------
 */
static int
led_stable_state_function(const char *led_name, const char *led_function)
{
	/* Generic error reporter. */
	int rc = -1;

	const char *value = NULL;
	if(strcmp(led_function, "setOff") == 0)
	{
		/* LED active low */
		value = "0";
	}
	else if(strcmp(led_function, "setOn") == 0)
	{
		value = "255";
	}
	else
	{
		fprintf(stderr,"Invalid LED stable state operation:[%s] \n",led_function);
		return rc;
	}

	/*
	 * Before doing anything, need to turn off the blinking
	 * if there is one in progress by writing 'none' to trigger
	 */
	rc = write_to_led(led_name, blink_ctrl, "none");
	if(rc < 0)
	{
		fprintf(stderr,"Error disabling blink. Function:[%s]\n", led_function);
		return rc;
	}

	/*
	 * Open the brightness file and write corresponding values.
	 */
	rc = write_to_led(led_name, power_ctrl, value);
	if(rc < 0)
	{
		fprintf(stderr,"Error driving LED. Function:[%s]\n", led_function);
	}

	return rc;
}

//-----------------------------------------------------------------------------------
// Given the on and off duration, applies the action on the specified LED.
//-----------------------------------------------------------------------------------
int
blink_led(const char *led_name, const char *on_duration, const char *off_duration)
{
	/* Generic error reporter */
	int rc = -1;

	/* Protocol demands that 'timer' be echoed to 'trigger' */
	rc = write_to_led(led_name, blink_ctrl, "timer");
	if(rc < 0)
	{
		fprintf(stderr,"Error writing timer to Led:[%s]\n", led_name);
		return rc;
	}

	/*
	 * After writing 'timer to 'trigger', 2 new files get generated namely
	 *'delay_on' and 'delay_off' which are telling the time duration for a
	 * particular LED on and off.
	 */
	rc = write_to_led(led_name, duty_on, on_duration);
	if(rc < 0)
	{
		fprintf(stderr,"Error writing [%s] to delay_on:[%s]\n",on_duration,led_name);
		return rc;
	}

	rc = write_to_led(led_name, duty_off, off_duration);
	if(rc < 0)
	{
		fprintf(stderr,"Error writing [%s] to delay_off:[%s]\n",off_duration,led_name);
	}

	return rc;
}

/*
 * ----------------------------------------------------
 * Default blink action on the LED.
 * ----------------------------------------------------
 */
static int
led_default_blink(const char *led_name, const char *blink_type)
{
	/* Generic error reporter */
	int rc = -1;

	/* How long the LED needs to be in on and off state while blinking */
	const char *on_duration = NULL;
	const char *off_duration = NULL;
	if(strcmp(blink_type, "setBlinkSlow") == 0)
	{
		//*Delay 900 millisec before 'on' and delay 900 millisec before off */
		on_duration = "900";
		off_duration = "900";
	}
	else if(strcmp(blink_type, "setBlinkFast") == 0)
	{
		/* Delay 200 millisec before 'on' and delay 200 millisec before off */
		on_duration = "200";
		off_duration = "200";
	}
	else
	{
		fprintf(stderr,"Invalid blink operation:[%s]\n",blink_type);
		return rc;
	}

	rc = blink_led(led_name, on_duration, off_duration);

	return rc;
}

/*
 * -------------------------------------------------
 * Blinks at user defined 'on' and 'off' intervals.
 * -------------------------------------------------
 */
static int
led_custom_blink(const char *led_name, sd_bus_message *msg)
{
	/* Generic error reporter. */
	int rc = -1;
	int led_len = 0;

	/* User supplied 'on' and 'off' duration converted into string */
	char on_duration[32] = {0};
	char off_duration[32] = {0};

	/* User supplied 'on' and 'off' duration */
	uint32_t user_input_on = 0;
	uint32_t user_input_off = 0;

	/* Extract values into 'ss' ( string, string) */
	rc = sd_bus_message_read(msg, "uu", &user_input_on, &user_input_off);
	if(rc < 0)
	{
		fprintf(stderr, "Failed to read 'on' and 'off' duration.[%s]\n", strerror(-rc));
	}
	else
	{
		/*
		 * Converting user supplied integer arguments into string as required by
		 * sys interface. The top level REST will make sure that an error is
		 * thrown right away on invalid inputs. However, REST is allowing the
		 * unsigned decimal and floating numbers but when its received here, its
		 * received as decimal so no input validation needed.
		 */
		led_len = snprintf(on_duration, sizeof(on_duration),
				"%d",user_input_on);
		if(led_len >= sizeof(on_duration))
		{
			fprintf(stderr, "Error. Blink ON duration is too long. :[%d]\n",led_len);
			return rc;
		}

		led_len = snprintf(off_duration, sizeof(off_duration),
				"%d",user_input_off);
		if(led_len >= sizeof(off_duration))
		{
			fprintf(stderr, "Error. Blink OFF duration is too long. :[%d]\n",led_len);
			return rc;
		}

		/* We are good here.*/
		rc = blink_led(led_name, on_duration, off_duration);
	}
	return rc;
}

/*
 * ---------------------------------------------------------------
 * Gets the current value of passed in LED file
 * Mainly used for reading 'brightness'
 * NOTE : It is the responsibility of the caller to allocate
 * sufficient space for buffer. This will read up to user supplied
 * size -or- entire contents of file whichever is smaller
 * ----------------------------------------------------------------
 */
static int
read_led(const char *name, const char *ctrl_file,
		void *value, const size_t len)
{
	/* Generic error reporter. */
	int rc = -1;
	int count = 0;

	if(value == NULL || len <= 0)
	{
		fprintf(stderr, "Invalid buffer passed to LED read\n");
		return rc;
	}

	/* To get /sys/class/leds/<name>/<control file> */
	char led_path[128] = {0};

	int led_len = 0;
	led_len = snprintf(led_path, sizeof(led_path),
			"/sys/class/leds/%s/%s",name, ctrl_file);
	if(led_len >= sizeof(led_path))
	{
		fprintf(stderr, "Error. LED path is too long. :[%d]\n",led_len);
		return rc;
	}

	FILE *fp = fopen(led_path,"rb");
	if(fp == NULL)
	{
		fprintf(stderr,"Error:[%s] opening:[%s]\n",strerror(errno),led_path);
		return rc;
	}

	char *sysfs_value = (char *)value;
	while(!feof(fp) && (count < len))
	{
		sysfs_value[count++] = fgetc(fp);
	}

	fclose(fp);
	return 0;
}

/*
 * -----------------------------------------------
 * Dbus Services offered by this LED controller
 * -----------------------------------------------
 */
static const sd_bus_vtable led_control_vtable[] =
{
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("setOn", "", "i", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("setOff", "", "i", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("setBlinkFast", "", "i", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("setBlinkSlow", "", "i", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("GetLedState", "", "is", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("BlinkCustom", "uu", "i", &led_function_router, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_VTABLE_END,
};

/*
 * ---------------------------------------------
 * Interested in all files except standard ones
 * ---------------------------------------------
 */
int
led_select(const struct dirent *entry)
{
	if( (strcmp(entry->d_name, ".") == 0) ||
			(strcmp(entry->d_name, "..") == 0))
	{
		return 0;
	}
	return 1;
}

/*
 * ------------------------------------------------
 * Called as part of setting up skeleton services.
 * -----------------------------------------------
 */
int
start_led_services()
{
	static const char *led_dbus_root = "/org/openbmc/control/led";

	/* Generic error reporter. */
	int rc = -1;
	int num_leds = 0;
	int count_leds = 0;

	/* Bus and slot where we are offering the LED dbus service. */
	sd_bus *bus_type = NULL;
	sd_bus_slot *led_slot = NULL;

	/* For walking '/sys/class/leds/' looking for names of LED.*/
	struct dirent **led_list;

	/* Get a hook onto system bus. */
	rc = sd_bus_open_system(&bus_type);
	if(rc < 0)
	{
		fprintf(stderr,"Error opening system bus.\n");
		return rc;
	}

	count_leds = num_leds = scandir("/sys/class/leds/",
			&led_list, led_select, alphasort);
	if(num_leds <= 0)
	{
		fprintf(stderr,"No LEDs present in the system\n");

		sd_bus_slot_unref(led_slot);
		sd_bus_unref(bus_type);
		return rc;
	}

	/* Install a freedesktop object manager */
	rc = sd_bus_add_object_manager(bus_type, NULL, led_dbus_root);
	if(rc < 0) {
		fprintf(stderr, "Failed to add object to dbus: %s\n",
				strerror(-rc));

		sd_bus_slot_unref(led_slot);
		sd_bus_unref(bus_type);
		return rc;
	}

	/* Fully qualified Dbus object for a particular LED */
	char led_object[128] = {0};
	int len = 0;

	/* For each led present, announce the service on dbus. */
	while(num_leds--)
	{
		memset(led_object, 0x0, sizeof(led_object));

		len = snprintf(led_object, sizeof(led_object), "%s%s%s",
				led_dbus_root, "/", led_list[num_leds]->d_name);

		if(len >= sizeof(led_object))
		{
			fprintf(stderr, "Error. LED object is too long:[%d]\n",len);
			rc = -1;
			break;
		}

		/* Install the object */
		rc = sd_bus_add_object_vtable(bus_type,
				&led_slot,
				led_object, /* object path */
				"org.openbmc.Led", /* interface name */
				led_control_vtable,
				NULL);

		if(rc < 0)
		{
			fprintf(stderr, "Failed to add object to dbus: %s\n", strerror(-rc));
			break;
		}

		rc = sd_bus_emit_object_added(bus_type, led_object);

		if(rc < 0)
		{
			fprintf(stderr, "Failed to emit InterfacesAdded "
					"signal: %s\n", strerror(-rc));
			break;
		}
	}

	/* Done with all registration. */
	while(count_leds > 0)
	{
		free(led_list[--count_leds]);
	}
	free(led_list);

	/* If we had success in adding the providers, request for a bus name. */
	if(rc >= 0)
	{
		/* Take one in OpenBmc */
		rc = sd_bus_request_name(bus_type, "org.openbmc.control.led", 0);
		if(rc < 0)
		{
			fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-rc));
		}
		else
		{
			for(;;)
			{
				/* Process requests */
				rc = sd_bus_process(bus_type, NULL);
				if(rc < 0)
				{
					fprintf(stderr, "Failed to process bus: %s\n", strerror(-rc));
					break;
				}
				if(rc > 0)
				{
					continue;
				}

				rc = sd_bus_wait(bus_type, (uint64_t) - 1);
				if(rc < 0)
				{
					fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-rc));
					break;
				}
			}
		}
	}
	sd_bus_slot_unref(led_slot);
	sd_bus_unref(bus_type);

	return rc;
}

int
main(void)
{
	int rc = 0;

	/* This call is not supposed to return. If it does, then an error */
	rc = start_led_services();
	if(rc < 0)
	{
		fprintf(stderr, "Error starting LED Services. Exiting");
	}

	return rc;
}
