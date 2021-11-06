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

#ifndef __ASD_DBUS_HELPER_H_
#define __ASD_DBUS_HELPER_H_

#include <stdbool.h>
#include <systemd/sd-bus.h>

#include "asd_common.h"

#define POWER_SERVICE_HOST "xyz.openbmc_project.State.Host"
#define POWER_INTERFACE_NAME_HOST "xyz.openbmc_project.State.Host"
#define POWER_OBJECT_PATH_HOST "/xyz/openbmc_project/state/host0"
#define HOST_TRANSITION_PROPERTY "RequestedHostTransition"
#define RESET_ARGUMENT_HOST                                                    \
    "xyz.openbmc_project.State.Host.Transition.ForceWarmReboot"
#define POWER_SERVICE_CHASSIS "xyz.openbmc_project.State.Chassis"
#define POWER_INTERFACE_NAME_CHASSIS "xyz.openbmc_project.State.Chassis"
#define POWER_OBJECT_PATH_CHASSIS "/xyz/openbmc_project/state/chassis0"
#define GET_POWER_STATE_PROPERTY_CHASSIS "CurrentPowerState"
#define SET_POWER_STATE_METHOD_CHASSIS "RequestedPowerTransition"
#define POWER_OFF_PROPERTY_CHASSIS                                             \
    "xyz.openbmc_project.State.Chassis.PowerState.Off"
#define POWER_ON_PROPERTY_CHASSIS                                              \
    "xyz.openbmc_project.State.Chassis.PowerState.On"
#define POWER_OFF_ARGUMENT_CHASSIS                                             \
    "xyz.openbmc_project.State.Chassis.Transition.Off"
#define POWER_ON_ARGUMENT_CHASSIS                                              \
    "xyz.openbmc_project.State.Chassis.Transition.On"
#define POWER_REBOOT_ARGUMENT_CHASSIS                                          \
    "xyz.openbmc_project.State.Chassis.Transition.Reboot"
#define POWER_RESET_ARGUMENT_CHASSIS                                           \
    "xyz.openbmc_project.State.Chassis.Transition.Reset"

#define DBUS_PROPERTIES "org.freedesktop.DBus.Properties"
#define DBUS_SET_METHOD "Set"
#define MATCH_STRING_CHASSIS                                                   \
    "type='signal',path='/xyz/openbmc_project/state/chassis0',\
member='PropertiesChanged',interface='org.freedesktop.DBus.Properties',\
sender='xyz.openbmc_project.State.Chassis',\
arg0namespace='xyz.openbmc_project.State.Chassis'"

#define OBJECT_MAPPER_SERVICE "xyz.openbmc_project.ObjectMapper"
#define OBJECT_MAPPER_PATH "/xyz/openbmc_project/object_mapper"
#define OBJECT_MAPPER_INTERFACE "xyz.openbmc_project.ObjectMapper"
#define BASEBOARD_PATH "/xyz/openbmc_project/inventory/system/board"
#define MOTHERBOARD_IDENTIFIER                                                 \
    "xyz.openbmc_project.Inventory.Item.Board.Motherboard"

#define ENTITY_MANAGER_SERVICE "xyz.openbmc_project.EntityManager"
#define ENTITY_MANAGER_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"
#define ASD_CONFIG_PATH "xyz.openbmc_project.Configuration.ASD"

#define SD_BUS_ASYNC_TIMEOUT 10000
#define MAX_PLATFORM_PATH_SIZE 120

typedef enum
{
    STATE_UNKNOWN = -1,
    STATE_OFF = 0,
    STATE_ON = 1
} Power_State;

typedef struct Dbus_Handle
{
    sd_bus* bus;
    int fd;
    Power_State power_state;
} Dbus_Handle;

Dbus_Handle* dbus_helper();
STATUS dbus_initialize(Dbus_Handle*);
STATUS dbus_deinitialize(Dbus_Handle*);
STATUS dbus_power_reset(Dbus_Handle*);
STATUS dbus_power_toggle(Dbus_Handle*);
STATUS dbus_power_reboot(Dbus_Handle*);
STATUS dbus_power_on(Dbus_Handle*);
STATUS dbus_power_off(Dbus_Handle*);
int sdbus_callback(sd_bus_message* reply, void* userdata, sd_bus_error* error);
STATUS dbus_process_event(Dbus_Handle* state, ASD_EVENT* event);
STATUS dbus_get_powerstate(Dbus_Handle* state, int* value);
STATUS dbus_get_platform_path(const Dbus_Handle* state, char* path);
STATUS dbus_get_platform_id(const Dbus_Handle* state, uint64_t* pid);
STATUS dbus_read_asd_config(const Dbus_Handle* state, const char* interface,
                            const char* name, char type, void* var);
STATUS dbus_get_asd_interface_paths(const Dbus_Handle* state,
                                    const char* names[],
                                    char interfaces[][MAX_PLATFORM_PATH_SIZE],
                                    int arr_size);
int match_callback(sd_bus_message* m, void* userdata, sd_bus_error* error);

#endif
