/*
Copyright (c) 2019, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "target_handler.h"

#include <fcntl.h>
#include <gpiod.h>
#include <poll.h>
#include <safe_mem_lib.h>
#include <safe_str_lib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "asd_common.h"
#include "gpio.h"
#include "logging.h"

#define JTAG_CLOCK_CYCLE_MILLISECONDS 1000
#define GPIOD_CONSUMER_LABEL "ASD"
#define GPIOD_DEV_ROOT_FOLDER "/dev/"
#define GPIOD_DEV_ROOT_FOLDER_STRLEN strlen(GPIOD_DEV_ROOT_FOLDER)

static inline void read_pin_value(Target_Control_GPIO gpio, int* value,
                                  STATUS* result)
{
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
    if (gpio.type == PIN_GPIO)
    {
        *result = gpio_get_value(gpio.fd, value);
    }
    else
#endif
        if (gpio.type == PIN_GPIOD)
    {
        *value = gpiod_line_get_value(gpio.line);
        if (*value == -1)
            *result = ST_ERR;
        else
            *result = ST_OK;
    }
}

static inline void write_pin_value(Target_Control_GPIO gpio, int value,
                                   STATUS* result)
{
    int rv = 0;
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
    if (gpio.type == PIN_GPIO)
    {
        *result = gpio_set_value(gpio.fd, value);
    }
    else
#endif
        if (gpio.type == PIN_GPIOD)
    {
        rv = gpiod_line_set_value(gpio.line, value);
        if (rv == 0)
            *result = ST_OK;
        else
            *result = ST_ERR;
    }
}

static inline void get_pin_events(Target_Control_GPIO gpio, short* events)
{
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
    if (gpio.type == PIN_GPIO)
    {
        *events = POLL_GPIO;
    }
    else
#endif
        if (gpio.type == PIN_GPIOD)
    {
        *events = POLLIN | POLLPRI;
    }
}

static inline void string_to_enum(char* str, const char* (*enum_strings)[],
                                  int arr_size, int* val)
{
    if ((str != NULL) && (enum_strings != NULL) && (val != NULL))
    {
        for (int index = 0; index < arr_size; index++)
        {
            if (strcmp(str, (*enum_strings)[index]) == 0)
            {
                *val = index;
                return;
            }
        }
    }
}

STATUS initialize_gpios(Target_Control_Handle* state);
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
STATUS initialize_gpio(Target_Control_GPIO* gpio);
STATUS find_gpio_base(char* gpio_name, int* gpio_base);
STATUS find_gpio(char* gpio_name, int* gpio_number);
#endif
STATUS deinitialize_gpios(Target_Control_Handle* state);
STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event);
STATUS initialize_gpiod(Target_Control_GPIO* gpio);
STATUS platform_init(Target_Control_Handle* state);

static const ASD_LogStream stream = ASD_LogStream_Pins;
static const ASD_LogOption option = ASD_LogOption_None;

Target_Control_Handle* TargetHandler()
{
    Target_Control_Handle* state =
        (Target_Control_Handle*)malloc(sizeof(Target_Control_Handle));

    if (state == NULL)
        return NULL;

    state->dbus = dbus_helper();
    if (state->dbus == NULL)
    {
        free(state);
        return NULL;
    }

    state->initialized = false;

    explicit_bzero(&state->gpios, sizeof(state->gpios));

    for (int i = 0; i < NUM_GPIOS; i++)
    {
        state->gpios[i].number = -1;
        state->gpios[i].fd = -1;
        state->gpios[i].line = NULL;
        state->gpios[i].chip = NULL;
        state->gpios[i].handler = NULL;
        state->gpios[i].active_low = false;
        state->gpios[i].type = PIN_GPIOD;
    }

    strcpy_s(state->gpios[BMC_TCK_MUX_SEL].name,
             sizeof(state->gpios[BMC_TCK_MUX_SEL].name), "TCK_MUX_SEL");
    state->gpios[BMC_TCK_MUX_SEL].direction = GPIO_DIRECTION_LOW;
    state->gpios[BMC_TCK_MUX_SEL].edge = GPIO_EDGE_NONE;

    strcpy_s(state->gpios[BMC_PREQ_N].name,
             sizeof(state->gpios[BMC_PREQ_N].name), "PREQ_N");
    state->gpios[BMC_PREQ_N].direction = GPIO_DIRECTION_HIGH;
    state->gpios[BMC_PREQ_N].edge = GPIO_EDGE_NONE;
    state->gpios[BMC_PREQ_N].active_low = true;

    strcpy_s(state->gpios[BMC_PRDY_N].name,
             sizeof(state->gpios[BMC_PRDY_N].name), "PRDY_N");
    state->gpios[BMC_PRDY_N].direction = GPIO_DIRECTION_IN;
    state->gpios[BMC_PRDY_N].edge = GPIO_EDGE_FALLING;
    state->gpios[BMC_PRDY_N].active_low = true;

    strcpy_s(state->gpios[BMC_RSMRST_B].name,
             sizeof(state->gpios[BMC_RSMRST_B].name), "RSMRST_N");
    state->gpios[BMC_RSMRST_B].direction = GPIO_DIRECTION_IN;
    state->gpios[BMC_RSMRST_B].edge = GPIO_EDGE_NONE;

    strcpy_s(state->gpios[BMC_CPU_PWRGD].name,
             sizeof(state->gpios[BMC_CPU_PWRGD].name), "SIO_POWER_GOOD");
    state->gpios[BMC_CPU_PWRGD].direction = GPIO_DIRECTION_IN;
    state->gpios[BMC_CPU_PWRGD].edge = GPIO_EDGE_BOTH;
    state->gpios[BMC_CPU_PWRGD].type = PIN_DBUS;

    strcpy_s(state->gpios[BMC_PLTRST_B].name,
             sizeof(state->gpios[BMC_PLTRST_B].name), "PLTRST_N");
    state->gpios[BMC_PLTRST_B].direction = GPIO_DIRECTION_IN;
    state->gpios[BMC_PLTRST_B].edge = GPIO_EDGE_BOTH;
    state->gpios[BMC_PLTRST_B].active_low = true;

    strcpy_s(state->gpios[BMC_SYSPWROK].name,
             sizeof(state->gpios[BMC_SYSPWROK].name), "SYSPWROK");
    state->gpios[BMC_SYSPWROK].direction = GPIO_DIRECTION_HIGH;
    state->gpios[BMC_SYSPWROK].edge = GPIO_EDGE_NONE;
    state->gpios[BMC_SYSPWROK].active_low = true;

    strcpy_s(state->gpios[BMC_PWR_DEBUG_N].name,
             sizeof(state->gpios[BMC_PWR_DEBUG_N].name), "PWR_DEBUG_N");
    state->gpios[BMC_PWR_DEBUG_N].direction = GPIO_DIRECTION_HIGH;
    state->gpios[BMC_PWR_DEBUG_N].edge = GPIO_EDGE_NONE;
    state->gpios[BMC_PWR_DEBUG_N].active_low = true;

    strcpy_s(state->gpios[BMC_DEBUG_EN_N].name,
             sizeof(state->gpios[BMC_DEBUG_EN_N].name), "DEBUG_EN_N");
    state->gpios[BMC_DEBUG_EN_N].direction = GPIO_DIRECTION_HIGH;
    state->gpios[BMC_DEBUG_EN_N].edge = GPIO_EDGE_NONE;
    state->gpios[BMC_DEBUG_EN_N].active_low = true;

    strcpy_s(state->gpios[BMC_XDP_PRST_IN].name,
             sizeof(state->gpios[BMC_XDP_PRST_IN].name), "XDP_PRST_N");
    state->gpios[BMC_XDP_PRST_IN].direction = GPIO_DIRECTION_IN;
    state->gpios[BMC_XDP_PRST_IN].active_low = true;
    state->gpios[BMC_XDP_PRST_IN].edge = GPIO_EDGE_BOTH;

    platform_init(state);

    state->event_cfg.break_all = false;
    state->event_cfg.report_MBP = false;
    state->event_cfg.report_PLTRST = false;
    state->event_cfg.report_PRDY = false;
    state->event_cfg.reset_break = false;

    initialize_powergood_pin_handler(state);
    state->gpios[BMC_PLTRST_B].handler =
        (TargetHandlerEventFunctionPtr)on_platform_reset_event;
    state->gpios[BMC_PRDY_N].handler =
        (TargetHandlerEventFunctionPtr)on_prdy_event;
    state->gpios[BMC_XDP_PRST_IN].handler =
        (TargetHandlerEventFunctionPtr)on_xdp_present_event;

    // Change is_master_probe accordingly on your BMC implementations.
    // <MODIFY>
    state->is_master_probe = false;
    // </MODIFY>

    return state;
}

STATUS initialize_powergood_pin_handler(Target_Control_Handle* state)
{
    int result = ST_OK;
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
    if (state->gpios[BMC_CPU_PWRGD].type == PIN_GPIO)
    {
        state->gpios[BMC_CPU_PWRGD].handler =
            (TargetHandlerEventFunctionPtr)on_power_event;
    }
    else
#endif
        if (state->gpios[BMC_CPU_PWRGD].type == PIN_GPIOD)
    {
        state->gpios[BMC_CPU_PWRGD].handler =
            (TargetHandlerEventFunctionPtr)on_power_event;
    }
    return result;
}

STATUS platform_override_gpio(const Dbus_Handle* dbus, char* interface,
                              Target_Control_GPIO* gpio)
{
    STATUS result = ST_ERR;
    int* enum_val = NULL;
    bool* bool_val = NULL;
    int match = 0;
    union out_data
    {
        bool bval;
        char str[MAX_PLATFORM_PATH_SIZE];
    } rval;

    static const data_json_map TARGET_JSON_MAP[] = {
        {"PinName", 's', offsetof(Target_Control_GPIO, name), NULL, 0},
        {"PinDirection", 's', offsetof(Target_Control_GPIO, direction),
         &GPIO_DIRECTION_STRINGS,
         sizeof(GPIO_DIRECTION_STRINGS) / sizeof(char*)},
        {"PinEdge", 's', offsetof(Target_Control_GPIO, edge),
         &GPIO_EDGE_STRINGS, sizeof(GPIO_EDGE_STRINGS) / sizeof(char*)},
        {"PinActiveLow", 'b', offsetof(Target_Control_GPIO, active_low), NULL,
         0},
        {"PinType", 's', offsetof(Target_Control_GPIO, type), &PIN_TYPE_STRINGS,
         sizeof(PIN_TYPE_STRINGS) / sizeof(char*)}};

    if ((dbus == NULL) || (interface == NULL))
    {
        return ST_ERR;
    }

    // Search for Target_Control_GPIO settings in the interface
    for (int i = 0; i < sizeof(TARGET_JSON_MAP) / sizeof(data_json_map); i++)
    {
        result =
            dbus_read_asd_config(dbus, interface, TARGET_JSON_MAP[i].fname_json,
                                 TARGET_JSON_MAP[i].ftype, &rval);
        if (result == ST_OK)
        {
            switch (TARGET_JSON_MAP[i].ftype)
            {
                case 'b':
                    // Copy active_low
                    bool_val = (bool*)((char*)gpio + TARGET_JSON_MAP[i].offset);
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Trace, stream, option, "%s = %d",
                            TARGET_JSON_MAP[i].fname_json, rval.bval);
#endif
                    *bool_val = rval.bval;
                    break;
                case 's':
                    // Copy GPIO name
                    match = 0;
                    strcmp_s(TARGET_JSON_MAP[i].fname_json, strlen("PinName"),
                             "PinName", &match);
                    if (match == 0)
                    {
                        // Copy Pin Name from dbus object to
                        memcpy_s((char*)gpio + TARGET_JSON_MAP[i].offset,
                                 MAX_PLATFORM_PATH_SIZE, &rval.str,
                                 strlen(rval.str));
                        break;
                    }
                    // Convert strings to enum values and set values
                    enum_val = (int*)((char*)gpio + TARGET_JSON_MAP[i].offset);
                    string_to_enum(rval.str, TARGET_JSON_MAP[i].enum_strings,
                                   TARGET_JSON_MAP[i].arr_size, enum_val);
                    break;
                default:
                    return ST_ERR;
            }
        }
    }
    return result;
}

STATUS platform_init(Target_Control_Handle* state)
{
    STATUS result = ST_ERR;
    Dbus_Handle* dbus = dbus_helper();
    char interfaces[NUM_GPIOS][MAX_PLATFORM_PATH_SIZE];

    // Read configuration from dbus
    if (dbus)
    {
        // Connect to the system bus
        int retcode = sd_bus_open_system(&dbus->bus);
        if (retcode >= 0)
        {
            dbus->fd = sd_bus_get_fd(dbus->bus);
            if (dbus->fd < 0)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "sd_bus_get_fd failed: %d", retcode);
#endif
                result = ST_ERR;
            }
            else
            {
                // get interface paths
                explicit_bzero(interfaces, sizeof(interfaces));
                dbus_get_asd_interface_paths(dbus, TARGET_CONTROL_GPIO_STRINGS,
                                             interfaces, NUM_GPIOS);
                for (int i = 0; i < NUM_GPIOS; i++)
                {
                    if (strlen(interfaces[i]) != 0)
                    {
                        ASD_log(ASD_LogLevel_Info, stream, option,
                                "interface[%d]: %s - %s", i,
                                TARGET_CONTROL_GPIO_STRINGS[i], interfaces[i]);
                        // Override Target_Control_GPIO settings for pin using
                        // entity mnager ASD settings.
                        platform_override_gpio(dbus, interfaces[i],
                                               &state->gpios[i]);
                    }
                }
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log_buffer(ASD_LogLevel_Debug, stream, option,
                               (char*)&state->gpios, sizeof(state->gpios),
                               "JSON");
#endif
            }
            dbus_deinitialize(dbus);
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "sd_bus_open_system failed: %d", retcode);
#endif
            result = ST_ERR;
        }
    }
    return result;
}

STATUS target_initialize(Target_Control_Handle* state)
{
    STATUS result;
    int value = 0;
    if (state == NULL || state->initialized)
        return ST_ERR;

    result = initialize_gpios(state);

    if (result == ST_OK)
    {
        read_pin_value(state->gpios[BMC_XDP_PRST_IN], &value, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed check XDP state or XDP not available");
        }
        else if (value == 1)
        {
            result = ST_ERR;
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "XDP Presence Detected");
        }
    }

    // specifically drive debug enable to assert
    if (result == ST_OK)
    {
        write_pin_value(state->gpios[BMC_DEBUG_EN_N], 1, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to assert debug enable");
        }
    }

    if (result == ST_OK)
    {
        result = dbus_initialize(state->dbus);
    }

    if (result == ST_OK)
        state->initialized = true;
    else
        deinitialize_gpios(state);

    return result;
}

STATUS initialize_gpios(Target_Control_Handle* state)
{
    STATUS result = ST_OK;
    int i;

    for (i = 0; i < NUM_GPIOS; i++)
    {
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
        if (state->gpios[i].type == PIN_GPIO)
        {
            result = initialize_gpio(&state->gpios[i]);
            if (result != ST_OK)
                break;
            // do a read to clear any bogus events on startup
            int dummy;
            result = gpio_get_value(state->gpios[i].fd, &dummy);
            if (result != ST_OK)
                break;
        }
        else
#endif
            if (state->gpios[i].type == PIN_GPIOD)
        {
            result = initialize_gpiod(&state->gpios[i]);
            if (result != ST_OK)
                break;
        }
    }

    if (result == ST_OK)
        ASD_log(ASD_LogLevel_Info, stream, option,
                "GPIOs initialized successfully");
    else
        ASD_log(ASD_LogLevel_Error, stream, option,
                "GPIOs initialization failed");
    return result;
}

#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
STATUS initialize_gpio(Target_Control_GPIO* gpio)
{
    int num;
    STATUS result = find_gpio(gpio->name, &num);

    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to find gpio for %s", gpio->name);
    }
    else
    {
        result = gpio_export(num, &gpio->fd);
        if (result != ST_OK)
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Gpio export failed for %s", gpio->name);
#ifdef ENABLE_DEBUG_LOGGING
        else
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Gpio export succeeded for %s num %d fd %d", gpio->name,
                    num, gpio->fd);
#endif
    }

    if (result == ST_OK)
    {
        result = gpio_set_active_low(num, gpio->active_low);
        if (result != ST_OK)
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Gpio set active low failed for %s", gpio->name);
    }

    if (result == ST_OK)
    {
        result = gpio_set_direction(num, gpio->direction);
        if (result != ST_OK)
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Gpio set direction failed for %s", gpio->name);
    }

    if (result == ST_OK)
    {
        result = gpio_set_edge(num, gpio->edge);
        if (result != ST_OK)
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Gpio set edge failed for %s", gpio->name);
    }

    if (result == ST_OK)
    {
        gpio->number = num;
        ASD_log(ASD_LogLevel_Info, stream, option, "gpio %s initialized to %d",
                gpio->name, gpio->number);
    }

    return result;
}
#endif

STATUS initialize_gpiod(Target_Control_GPIO* gpio)
{
    int offset = -1;
    uint8_t chip_name[CHIP_BUFFER_SIZE];
    int rv;
    int default_val = 0;
    struct gpiod_line_request_config config;

    if (gpio == NULL)
    {
        return ST_ERR;
    }

    explicit_bzero(chip_name, CHIP_BUFFER_SIZE);
    memcpy_s(chip_name, CHIP_BUFFER_SIZE, GPIOD_DEV_ROOT_FOLDER,
             GPIOD_DEV_ROOT_FOLDER_STRLEN);

    rv = gpiod_ctxless_find_line(
        gpio->name, &chip_name[GPIOD_DEV_ROOT_FOLDER_STRLEN],
        CHIP_BUFFER_SIZE - GPIOD_DEV_ROOT_FOLDER_STRLEN, &offset);
    if (rv < 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option,
                "error performing the line lookup");
#endif
        return ST_ERR;
    }
    else if (rv == 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option, "line %s doesn't exist",
                gpio->name);
#endif
        return ST_ERR;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Info, stream, option,
            "gpio: %s gpio device: %s line offset: %d", gpio->name, chip_name,
            offset);
#endif

    gpio->chip = gpiod_chip_open(chip_name);
    if (!gpio->chip)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option, "Failed to open the chip");
#endif
        return ST_ERR;
    }

    gpio->line = gpiod_chip_get_line(gpio->chip, offset);
    if (!gpio->line)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to get line reference");
#endif
        gpiod_chip_close(gpio->chip);
        return ST_ERR;
    }

    config.consumer = GPIOD_CONSUMER_LABEL;

    switch (gpio->direction)
    {
        case GPIO_DIRECTION_IN:
            switch (gpio->edge)
            {
                case GPIO_EDGE_RISING:
                    config.request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
                    break;
                case GPIO_EDGE_FALLING:
                    config.request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
                    break;
                case GPIO_EDGE_BOTH:
                    config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
                    break;
                case GPIO_EDGE_NONE:
                    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
                default:
                    break;
            }
            break;
        case GPIO_DIRECTION_HIGH:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
            default_val = gpio->active_low ? 0 : 1;
            break;
        case GPIO_DIRECTION_OUT:
        case GPIO_DIRECTION_LOW:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
            default_val = gpio->active_low ? 1 : 0;
            break;
        default:
            break;
    }

    config.flags = gpio->active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0;
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Info, stream, option,
            "default_val = %d request_type = 0x%x flags = 0x%x consumer = %s",
            default_val, config.request_type, config.flags, config.consumer);
#endif

    // Default value have a different behavior in SysFs and gpiod. For
    // SysFs, the setting HIGH or LOW means the pin level while in gpiod
    // the value describes if signal will be active or not. The pin level
    // in gpiod will vary according with the active_low settings.

    rv = gpiod_line_request(gpio->line, &config, default_val);
    if (rv)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to process line request");
#endif
        gpiod_chip_close(gpio->chip);
        return ST_ERR;
    }

    // File Descriptor
    switch (gpio->direction)
    {
        case GPIO_DIRECTION_IN:
            switch (gpio->edge)
            {
                case GPIO_EDGE_RISING:
                case GPIO_EDGE_FALLING:
                case GPIO_EDGE_BOTH:
                    // Get GPIOD line file descriptor
                    gpio->fd = gpiod_line_event_get_fd(gpio->line);
                    if (gpio->fd == -1)
                    {
#ifdef ENABLE_DEBUG_LOGGING
                        ASD_log(ASD_LogLevel_Error, stream, option,
                                "Failed to get file descriptor");
#endif
                        return ST_ERR;
                    }
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Info, stream, option, "%s.fd = 0x%x",
                            gpio->name, gpio->fd);
#endif
                    break;
                case GPIO_EDGE_NONE:
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Info, stream, option, "No event");
#endif
                default:
                    break;
            }
            break;
        case GPIO_DIRECTION_HIGH:
        case GPIO_DIRECTION_OUT:
        case GPIO_DIRECTION_LOW:
        default:
            break;
    }

    gpio->number = offset;
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Info, stream, option, "gpio %s initialized to %d",
            gpio->name, gpio->number);
#endif

    return ST_OK;
}

#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
STATUS find_gpio_base(char* gpio_name, int* gpio_base)
{
    int fd;
    char buf[CHIP_FNAME_BUFF_SIZE];
    char ch;

    *gpio_base = 0;
    if (memcpy_s(buf, sizeof(buf), AST2500_GPIO_BASE_FILE,
                 strlen(AST2500_GPIO_BASE_FILE) + 1))
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "memcpy_s: gpio base filename failed");
#endif
        return ST_ERR;
    }
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option, "open gpio base file %s", buf);
#endif
    fd = open(buf, O_RDONLY);
    if (fd < 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "open gpio base file %s failed", buf);
#endif
        return ST_ERR;
    }
    lseek(fd, 0, SEEK_SET);
    // read all characters in the file
    while (read(fd, &ch, 1))
    {
        if ((ch >= '0') && (ch <= '9'))
            *gpio_base = (*gpio_base * 10) + (ch - '0');
    }
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option, "base is %d", *gpio_base);
#endif
    close(fd);
    return ST_OK;
}

STATUS find_gpio(char* gpio_name, int* gpio_number)
{
    STATUS result = ST_OK;
    int gpio_base = 0;
    result = find_gpio_base(gpio_name, &gpio_base);
    if (result != ST_OK)
        return result;
    if (strcmp(gpio_name, "TCK_MUX_SEL") == 0)
        *gpio_number = gpio_base + 213;
    else if (strcmp(gpio_name, "PREQ_N") == 0)
        *gpio_number = gpio_base + 212;
    else if (strcmp(gpio_name, "PRDY_N") == 0)
        *gpio_number = gpio_base + 47;
    else if (strcmp(gpio_name, "RSMRST_N") == 0)
        *gpio_number = gpio_base + 146;
    else if (strcmp(gpio_name, "SIO_POWER_GOOD") == 0)
        *gpio_number = gpio_base + 201;
    else if (strcmp(gpio_name, "PLTRST_N") == 0)
        *gpio_number = gpio_base + 46;
    else if (strcmp(gpio_name, "SYSPWROK") == 0)
        *gpio_number = gpio_base + 145;
    else if (strcmp(gpio_name, "PWR_DEBUG_N") == 0)
        *gpio_number = gpio_base + 135;
    else if (strcmp(gpio_name, "DEBUG_EN_N") == 0)
        *gpio_number = gpio_base + 37;
    else if (strcmp(gpio_name, "XDP_PRST_N") == 0)
        *gpio_number = gpio_base + 137;
    else
        result = ST_ERR;

    return result;
}
#endif

STATUS target_deinitialize(Target_Control_Handle* state)
{
    if (state == NULL || !state->initialized)
        return ST_ERR;

#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
    for (int i = 0; i < NUM_GPIOS; i++)
    {
        if (state->gpios[i].type == PIN_GPIO)
        {
            if (state->gpios[i].fd != -1)
            {
                close(state->gpios[i].fd);
                state->gpios[i].fd = -1;
            }
        }
    }
#endif

    dbus_deinitialize(state->dbus);

    return deinitialize_gpios(state);
}

STATUS deinitialize_gpios(Target_Control_Handle* state)
{
    STATUS result = ST_OK;
    STATUS retcode = ST_OK;
    int i;

    for (i = 0; i < NUM_GPIOS; i++)
    {
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
        if (state->gpios[i].type == PIN_GPIO)
        {
            retcode =
                gpio_set_direction(state->gpios[i].number, GPIO_DIRECTION_IN);
            if (retcode != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Gpio set direction failed for %s",
                        state->gpios[i].name);
                result = ST_ERR;
            }
            retcode = gpio_unexport(state->gpios[i].number);
            if (retcode != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Gpio export failed for %s", state->gpios[i].name);
                result = ST_ERR;
            }
        }
        else
#endif
            if (state->gpios[i].type == PIN_GPIOD)
        {
            gpiod_chip_close(state->gpios[i].chip);
        }
    }

    ASD_log(ASD_LogLevel_Info, stream, option,
            (result == ST_OK) ? "GPIOs deinitialized successfully"
                              : "GPIOs deinitialized failed");
    return result;
}

STATUS target_event(Target_Control_Handle* state, struct pollfd poll_fd,
                    ASD_EVENT* event)
{
    STATUS result = ST_ERR;
    int i, rv = 0;

    if (state == NULL || !state->initialized || event == NULL)
        return ST_ERR;

    *event = ASD_EVENT_NONE;

    if (state->dbus && state->dbus->fd == poll_fd.fd &&
        (poll_fd.revents & POLLIN) == POLLIN)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Handling dbus event for fd: %d", poll_fd.fd);
#endif
        result = dbus_process_event(state->dbus, event);
    }
    else
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
        if ((poll_fd.revents & POLL_GPIO) == POLL_GPIO)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Handling event for fd: %d",
                poll_fd.fd);
#endif
        for (i = 0; i < NUM_GPIOS; i++)
        {
            if (state->gpios[i].type == PIN_GPIO)
            {
                if (state->gpios[i].fd == poll_fd.fd)
                {
                    // do a read to clear the event
                    int dummy;
                    gpio_get_value(poll_fd.fd, &dummy);
                    result = state->gpios[i].handler(state, event);
                    break;
                }
            }
        }
    }
    else
#endif
        if (poll_fd.revents & (POLLIN | POLLPRI))
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Handling event for fd: %d",
                poll_fd.fd);
#endif
        for (i = 0; i < NUM_GPIOS; i++)
        {
            if (state->gpios[i].type == PIN_GPIOD)
            {
                if (state->gpios[i].fd == poll_fd.fd)
                {
                    // clear the gpiod event
                    struct gpiod_line_event levent;
                    rv = gpiod_line_event_read(state->gpios[i].line, &levent);
                    if (rv != 0)
                    {
                        result = ST_ERR;
                        break;
                    }
                    result = state->gpios[i].handler(state, event);
                    break;
                }
            }
        }
    }
    else
    {
        result = ST_OK;
    }

    return result;
}

STATUS on_power_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result;
    int value;

    read_pin_value(state->gpios[BMC_CPU_PWRGD], &value, &result);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to get gpio data for CPU_PWRGD: %d", result);
    }
    else if (value == 1)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power restored");
#endif
        *event = ASD_EVENT_PWRRESTORE;
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Power fail");
#endif
        *event = ASD_EVENT_PWRFAIL;
    }
    return result;
}

STATUS on_platform_reset_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result;
    int value;

    read_pin_value(state->gpios[BMC_PLTRST_B], &value, &result);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to get event status for PLTRST: %d", result);
    }
    else if (value == 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option, "Platform reset asserted");
#endif
        *event = ASD_EVENT_PLRSTASSERT;
        if (state->event_cfg.reset_break)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "ResetBreak detected PLT_RESET "
                    "assert, asserting PREQ");
#endif
            write_pin_value(state->gpios[BMC_PREQ_N], 1, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to assert PREQ");
            }
        }
    }
    else
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Platform reset de-asserted");
#endif
        *event = ASD_EVENT_PLRSTDEASSRT;
    }

    return result;
}

STATUS on_prdy_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "CPU_PRDY Asserted Event Detected.");
#endif
    *event = ASD_EVENT_PRDY_EVENT;
    if (state->event_cfg.break_all)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "BreakAll detected PRDY, asserting PREQ");
#endif
        write_pin_value(state->gpios[BMC_PREQ_N], 1, &result);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Failed to assert PREQ");
        }
        else if (!state->event_cfg.reset_break)
        {
            usleep(10000);
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "CPU_PRDY, de-asserting PREQ");
#endif
            write_pin_value(state->gpios[BMC_PREQ_N], 0, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to deassert PREQ");
            }
        }
    }

    return result;
}

STATUS on_xdp_present_event(Target_Control_Handle* state, ASD_EVENT* event)
{
    STATUS result = ST_OK;
    (void)state; /* unused */

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "XDP Present state change detected");
#endif
    *event = ASD_EVENT_XDP_PRESENT;

    return result;
}

STATUS target_write(Target_Control_Handle* state, const Pin pin,
                    const bool assert)
{
    STATUS result = ST_OK;
    Target_Control_GPIO gpio;
    int value;

    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_write, null or uninitialized state");
        return ST_ERR;
    }
    switch (pin)
    {
        case PIN_RESET_BUTTON:
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "Pin Write: %s reset button",
                    assert ? "assert" : "deassert");
            if (result == ST_OK)
            {
                result = dbus_power_reset(state->dbus);
            }
            break;
        case PIN_POWER_BUTTON:
            ASD_log(ASD_LogLevel_Info, stream, option,
                    "Pin Write: %s power button",
                    assert ? "assert" : "deassert");
            if (assert)
            {
                if (result == ST_OK)
                {
                    gpio = state->gpios[BMC_CPU_PWRGD];
                    read_pin_value(state->gpios[BMC_CPU_PWRGD], &value,
                                   &result);
                    if (state->gpios[BMC_CPU_PWRGD].type == PIN_DBUS)
                        result = dbus_get_powerstate(state->dbus, &value);
                    if (result != ST_OK)
                    {
#ifdef ENABLE_DEBUG_LOGGING
                        ASD_log(ASD_LogLevel_Debug, stream, option,
                                "Failed to read gpio %s %d", gpio.name,
                                gpio.number);
#endif
                    }
                    else
                    {
                        if (value)
                        {
                            result = dbus_power_off(state->dbus);
                        }
                        else
                        {
                            result = dbus_power_on(state->dbus);
                        }
                    }
                }
            }
            break;
        case PIN_PREQ:
        case PIN_TCK_MUX_SELECT:
        case PIN_SYS_PWR_OK:
        case PIN_EARLY_BOOT_STALL:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            ASD_log(ASD_LogLevel_Info, stream, option, "Pin Write: %s %s %d",
                    assert ? "assert" : "deassert", gpio.name, gpio.number);
            write_pin_value(gpio, (uint8_t)(assert ? 1 : 0), &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to set %s %s %d",
                        assert ? "assert" : "deassert", gpio.name, gpio.number);
            }
            break;
        default:
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Pin write: unsupported pin '%s'", gpio.name);
#endif
            result = ST_ERR;
            break;
    }

    return result;
}

STATUS target_read(Target_Control_Handle* state, Pin pin, bool* asserted)
{
    STATUS result;
    Target_Control_GPIO gpio;
    int value;
    if (state == NULL || asserted == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_read, null or uninitialized state");
        return ST_ERR;
    }
    *asserted = false;

    switch (pin)
    {
        case PIN_PWRGOOD:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            read_pin_value(state->gpios[BMC_CPU_PWRGD], &value, &result);
            if (state->gpios[BMC_CPU_PWRGD].type == PIN_DBUS)
            {
                result = dbus_get_powerstate(state->dbus, &value);
            }
            if (result != ST_OK)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option,
                        "Failed to read PIN %s %d", gpio.name, gpio.number);
#endif
            }
            else
            {
                *asserted = (value != 0);
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Info, stream, option,
                        "Pin read: %s powergood",
                        *asserted ? "asserted" : "deasserted");
#endif
            }
            break;
        case PIN_PRDY:
        case PIN_PREQ:
        case PIN_SYS_PWR_OK:
        case PIN_EARLY_BOOT_STALL:
            gpio = state->gpios[ASD_PIN_TO_GPIO[pin]];
            read_pin_value(gpio, &value, &result);
            if (result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "Failed to read gpio %s %d", gpio.name, gpio.number);
            }
            else
            {
                *asserted = (value != 0);
                ASD_log(ASD_LogLevel_Info, stream, option, "Pin read: %s %s %d",
                        *asserted ? "asserted" : "deasserted", gpio.name,
                        gpio.number);
            }
            break;
        default:
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, stream, option,
                    "Pin read: unsupported gpio read for pin: %d", pin);
#endif
            result = ST_ERR;
    }

    return result;
}

STATUS target_write_event_config(Target_Control_Handle* state,
                                 const WriteConfig event_cfg, const bool enable)
{
    STATUS status = ST_OK;
    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_write_event_config, null or uninitialized state");
        return ST_ERR;
    }

    switch (event_cfg)
    {
        case WRITE_CONFIG_BREAK_ALL:
        {
            if (state->event_cfg.break_all != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "BREAK_ALL %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.break_all = enable;
            }
            break;
        }
        case WRITE_CONFIG_RESET_BREAK:
        {
            if (state->event_cfg.reset_break != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "RESET_BREAK %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.reset_break = enable;
            }
            break;
        }
        case WRITE_CONFIG_REPORT_PRDY:
        {
#ifdef ENABLE_DEBUG_LOGGING
            if (state->event_cfg.report_PRDY != enable)
            {
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_PRDY %s",
                        enable ? "enabled" : "disabled");
            }
#endif
            // do a read to ensure no outstanding prdys are present before
            // wait for prdy is enabled.
            int dummy = 0;
            STATUS retval = ST_ERR;
            read_pin_value(state->gpios[BMC_PRDY_N], &dummy, &retval);
            state->event_cfg.report_PRDY = enable;
            break;
        }
        case WRITE_CONFIG_REPORT_PLTRST:
        {
            if (state->event_cfg.report_PLTRST != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_PLTRST %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.report_PLTRST = enable;
            }
            break;
        }
        case WRITE_CONFIG_REPORT_MBP:
        {
            if (state->event_cfg.report_MBP != enable)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, stream, option, "REPORT_MBP %s",
                        enable ? "enabled" : "disabled");
#endif
                state->event_cfg.report_MBP = enable;
            }
            break;
        }
        default:
        {
            ASD_log(ASD_LogLevel_Error, stream, option,
                    "Invalid event config %d", event_cfg);
            status = ST_ERR;
        }
    }
    return status;
}

STATUS target_wait_PRDY(Target_Control_Handle* state, const uint8_t log2time)
{
    // The design for this calls for waiting for PRDY or until a timeout
    // occurs. The timeout is computed using the PRDY timeout setting
    // (log2time) and the JTAG TCLK.

    int timeout_ms = 0;
    struct pollfd pfd = {0};
    int poll_result = 0;
    STATUS result = ST_OK;
    short events = 0;

    if (state == NULL || !state->initialized)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_wait_PRDY, null or uninitialized state");
        return ST_ERR;
    }

    // The timeout for commands that wait for a PRDY pulse is defined to be in
    // uSec, we need to convert to mSec, so we divide by 1000. For
    // values less than 1 ms that get rounded to 0 we need to wait 1ms.
    timeout_ms = (1 << log2time) / JTAG_CLOCK_CYCLE_MILLISECONDS;
    if (timeout_ms <= 0)
    {
        timeout_ms = 1;
    }
    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    pfd.events = events;
    pfd.fd = state->gpios[BMC_PRDY_N].fd;
    poll_result = poll(&pfd, 1, timeout_ms);
    if (poll_result == 0)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, stream, option,
                "Wait PRDY timed out occurred");
#endif
        // future: we should return something to indicate a timeout
    }
    else if (poll_result > 0)
    {
        if (pfd.revents & events)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Trace, stream, option,
                    "Wait PRDY complete, detected PRDY");
#endif
        }
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_wait_PRDY poll failed: %d.", poll_result);
        result = ST_ERR;
    }
    return result;
}

STATUS target_get_fds(Target_Control_Handle* state, target_fdarr_t* fds,
                      int* num_fds)
{
    int index = 0;
    short events = 0;

    if (state == NULL || !state->initialized || fds == NULL || num_fds == NULL)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Error, stream, option,
                "target_get_fds, null or uninitialized state");
#endif
        return ST_ERR;
    }

    get_pin_events(state->gpios[BMC_PRDY_N], &events);
    if (state->event_cfg.report_PRDY && state->gpios[BMC_PRDY_N].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PRDY_N].fd;
        (*fds)[index].events = events;
        index++;
    }

    get_pin_events(state->gpios[BMC_PLTRST_B], &events);
    if (state->gpios[BMC_PLTRST_B].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_PLTRST_B].fd;
        (*fds)[index].events = events;
        index++;
    }

    get_pin_events(state->gpios[BMC_CPU_PWRGD], &events);
    if (state->gpios[BMC_CPU_PWRGD].type == PIN_GPIOD
#ifdef GPIO_SYSFS_SUPPORT_DEPRECATED
        || state->gpios[BMC_CPU_PWRGD].type == PIN_GPIO
#endif
    )
    {
        if (state->gpios[BMC_CPU_PWRGD].fd != -1)
        {
            (*fds)[index].fd = state->gpios[BMC_CPU_PWRGD].fd;
            (*fds)[index].events = events;
            index++;
        }
    }

    get_pin_events(state->gpios[BMC_XDP_PRST_IN], &events);
    if (state->gpios[BMC_XDP_PRST_IN].fd != -1)
    {
        (*fds)[index].fd = state->gpios[BMC_XDP_PRST_IN].fd;
        (*fds)[index].events = events;
        index++;
    }

    if (state->dbus && state->dbus->fd != -1)
    {
        (*fds)[index].fd = state->dbus->fd;
        (*fds)[index].events = POLLIN;
        index++;
    }

    *num_fds = index;

    return ST_OK;
}

// target_wait_sync - This command will only be issued in a multiple
//   probe configuration where there are two or more TAP masters. The
//   WaitSync command is used to tell all TAP masters to wait until a
//   sync indication is received. The exact flow of sync signaling is
//   implementation specific. Command processing will continue after
//   either the Sync indication is received or the SyncTimeout is
//   reached. The SyncDelay is intended to be used in an implementation
//   where there is a single sync signal routed from a single designated
//   TAP Master to all other TAP Masters. The SyncDelay is used as an
//   implicit means to ensure that all other TAP Masters have reached
//   the WaitSync before the Sync signal is asserted.
//
// Parameters:
//  timeout - the SyncTimeout provides a timeout value to all slave
//    probes. If a slave probe does not receive the sync signal during
//    this timeout period, then a timeout occurs. The value is in
//    milliseconds (Range 0ms - 65s).
//  delay - the SyncDelay is meant to be used in a single master probe
//    sync singal implmentation. Upon receiving the WaitSync command,
//    the probe will delay for the Sync Delay Value before sending
//    the sync signal. This is to ensure that all slave probes
//    have reached WaitSync state prior to the sync being sent.
//    The value is in milliseconds (Range 0ms - 65s).
//
// Returns:
//  ST_OK if operation completed successfully.
//  ST_ERR if operation failed.
//  ST_TIMEOUT if failed to detect sync signal
STATUS target_wait_sync(Target_Control_Handle* state, const uint16_t timeout,
                        const uint16_t delay)
{
    STATUS result = ST_OK;
    if (state == NULL || !state->initialized)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Trace, stream, option,
                "target_wait_sync, null or uninitialized state");
#endif
        return ST_ERR;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "WaitSync(%s) - delay=%u ms - timeout=%u ms",
            state->is_master_probe ? "master" : "slave", delay, timeout);
#endif

    if (state->is_master_probe)
    {
        usleep((__useconds_t)(delay * 1000)); // convert from us to ms
        // Once delay has occurred, send out the sync signal.

        // <MODIFY>
        // hard code a error until code is implemented
        result = ST_ERR;
        // </MODIFY>
    }
    else
    {
        // Wait for sync signal to arrive.
        // timeout if sync signal is not received in the
        // milliseconds provided by the timeout parameter

        // <MODIFY>
        usleep((__useconds_t)(timeout * 1000)); // convert from us to ms
        // hard code a error/timeout until code is implemented
        result = ST_TIMEOUT;
        // when sync is detected, set result to ST_OK
        // </MODIFY>
    }

    return result;
}

STATUS target_get_i2c_config(i2c_options* i2c)
{
    STATUS result = ST_ERR;
    Dbus_Handle* dbus = dbus_helper();

    if (dbus)
    {
        // Connect to the system bus
        int retcode = sd_bus_open_system(&dbus->bus);
        if (retcode >= 0)
        {
            dbus->fd = sd_bus_get_fd(dbus->bus);
            if (dbus->fd < 0)
            {
                ASD_log(ASD_LogLevel_Error, stream, option,
                        "sd_bus_get_fd failed");
            }
            else
            {
                uint64_t pid = 0;
                result = dbus_get_platform_id(dbus, &pid);
                if (result == ST_OK)
                {
                    switch (pid)
                    {
                        case WOLF_PASS_PLATFORM_ID:
                            i2c->bus = 4;
                            i2c->enable = true;
                            break;
                        case COOPER_CITY_PLATFORM_ID:
                        case WILSON_CITY_PLATFORM_ID:
                        case WILSON_POINT_PLATFORM_ID:
                            i2c->bus = 9;
                            i2c->enable = true;
                            break;
                        default:
                            i2c->enable = false;
                            break;
                    }
                }
                else
                {
                    ASD_log(ASD_LogLevel_Error, stream, option,
                            "dbus_get_platform_id failed");
                }
            }
            dbus_deinitialize(dbus);
        }
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "failed to get dbus handle");
    }
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, stream, option, "i2c.enable: %d i2c.bus: %d",
            i2c->enable, i2c->bus);
#endif
    return result;
}