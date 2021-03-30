# Contributors Listed Below - COPYRIGHT 2016
# [+] International Business Machines Corp.
#
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import dbus

DBUS_OBJMGR_IFACE = dbus.BUS_DAEMON_IFACE + '.ObjectManager'
DBUS_UNKNOWN_INTERFACE = 'org.freedesktop.DBus.Error.UnknownInterface'
DBUS_UNKNOWN_SERVICE = 'org.freedesktop.DBus.Error.ServiceUnknown'
DBUS_UNKNOWN_PROPERTY = 'org.freedesktop.DBus.Error.UnknownProperty'
DBUS_UNKNOWN_METHOD = 'org.freedesktop.DBus.Error.UnknownMethod'
DBUS_INVALID_ARGS = 'org.freedesktop.DBus.Error.InvalidArgs'
DBUS_NO_REPLY = 'org.freedesktop.DBus.Error.NoReply'
DBUS_TYPE_ERROR = 'org.freedesktop.DBus.Python.TypeError'
OBMC_ASSOCIATIONS_IFACE = 'org.openbmc.Associations'
OBMC_ASSOC_IFACE = 'org.openbmc.Association'
OBMC_DELETE_IFACE = 'org.openbmc.Object.Delete'
OBMC_PROPERTIES_IFACE = "org.openbmc.Object.Properties"
OBMC_ENUMERATE_IFACE = "org.openbmc.Object.Enumerate"
