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

import os
import sys
import dbus
import dbus.exceptions
import json
from xml.etree import ElementTree
from bottle import Bottle, abort, request, response, JSONPlugin, HTTPError
from bottle import static_file
import obmc.utils.misc
from obmc.dbuslib.introspection import IntrospectionNodeParser
import obmc.mapper
import spwd
import grp
import crypt
import tempfile
import re
import mimetypes
have_wsock = True
try:
    from geventwebsocket import WebSocketError
except ImportError:
    have_wsock = False
if have_wsock:
    from dbus.mainloop.glib import DBusGMainLoop
    DBusGMainLoop(set_as_default=True)
    # TODO: openbmc/openbmc#2994 remove python 2 support
    try:  # python 2
        import gobject
    except ImportError:  # python 3
        from gi.repository import GObject as gobject
    import gevent
    from gevent import socket
    from gevent import Greenlet

DBUS_UNKNOWN_INTERFACE = 'org.freedesktop.DBus.Error.UnknownInterface'
DBUS_UNKNOWN_METHOD = 'org.freedesktop.DBus.Error.UnknownMethod'
DBUS_PROPERTY_READONLY = 'org.freedesktop.DBus.Error.PropertyReadOnly'
DBUS_INVALID_ARGS = 'org.freedesktop.DBus.Error.InvalidArgs'
DBUS_TYPE_ERROR = 'org.freedesktop.DBus.Python.TypeError'
DELETE_IFACE = 'xyz.openbmc_project.Object.Delete'
SOFTWARE_PATH = '/xyz/openbmc_project/software'
WEBSOCKET_TIMEOUT = 45

_4034_msg = "The specified %s cannot be %s: '%s'"

www_base_path = '/usr/share/www/'


def valid_user(session, *a, **kw):
    ''' Authorization plugin callback that checks
    that the user is logged in. '''
    if session is None:
        abort(401, 'Login required')


def get_type_signature_by_introspection(bus, service, object_path,
                                        property_name):
    obj = bus.get_object(service, object_path)
    iface = dbus.Interface(obj, 'org.freedesktop.DBus.Introspectable')
    xml_string = iface.Introspect()
    for child in ElementTree.fromstring(xml_string):
        # Iterate over each interfaces's properties to find
        # matching property_name, and return its signature string
        if child.tag == 'interface':
            for i in child.iter():
                if ('name' in i.attrib) and \
                   (i.attrib['name'] == property_name):
                    type_signature = i.attrib['type']
                    return type_signature


def get_method_signature(bus, service, object_path, interface, method):
    obj = bus.get_object(service, object_path)
    iface = dbus.Interface(obj, 'org.freedesktop.DBus.Introspectable')
    xml_string = iface.Introspect()
    arglist = []

    root = ElementTree.fromstring(xml_string)
    for dbus_intf in root.findall('interface'):
        if (dbus_intf.get('name') == interface):
            for dbus_method in dbus_intf.findall('method'):
                if(dbus_method.get('name') == method):
                    for arg in dbus_method.findall('arg'):
                        if (arg.get('direction') == 'in'):
                            arglist.append(arg.get('type'))
                    return arglist


def split_struct_signature(signature):
    struct_regex = r'(b|y|n|i|x|q|u|t|d|s|a\(.+?\)|\(.+?\))|a\{.+?\}+?'
    struct_matches = re.findall(struct_regex, signature)
    return struct_matches


def convert_type(signature, value):
    # Basic Types
    converted_value = None
    converted_container = None
    # TODO: openbmc/openbmc#2994 remove python 2 support
    try:  # python 2
        basic_types = {'b': bool, 'y': dbus.Byte, 'n': dbus.Int16, 'i': int,
                       'x': long, 'q': dbus.UInt16, 'u': dbus.UInt32,
                       't': dbus.UInt64, 'd': float, 's': str}
    except NameError:  # python 3
        basic_types = {'b': bool, 'y': dbus.Byte, 'n': dbus.Int16, 'i': int,
                       'x': int, 'q': dbus.UInt16, 'u': dbus.UInt32,
                       't': dbus.UInt64, 'd': float, 's': str}
    array_matches = re.match(r'a\((\S+)\)', signature)
    struct_matches = re.match(r'\((\S+)\)', signature)
    dictionary_matches = re.match(r'a{(\S+)}', signature)
    if signature in basic_types:
        converted_value = basic_types[signature](value)
        return converted_value
    # Array
    if array_matches:
        element_type = array_matches.group(1)
        converted_container = list()
        # Test if value is a list
        # to avoid iterating over each character in a string.
        # Iterate over each item and convert type
        if isinstance(value, list):
            for i in value:
                converted_element = convert_type(element_type, i)
                converted_container.append(converted_element)
        # Convert non-sequence to expected type, and append to list
        else:
            converted_element = convert_type(element_type, value)
            converted_container.append(converted_element)
        return converted_container
    # Struct
    if struct_matches:
        element_types = struct_matches.group(1)
        split_element_types = split_struct_signature(element_types)
        converted_container = list()
        # Test if value is a list
        if isinstance(value, list):
            for index, val in enumerate(value):
                converted_element = convert_type(split_element_types[index],
                                                 value[index])
                converted_container.append(converted_element)
        else:
            converted_element = convert_type(element_types, value)
            converted_container.append(converted_element)
        return tuple(converted_container)
    # Dictionary
    if dictionary_matches:
        element_types = dictionary_matches.group(1)
        split_element_types = split_struct_signature(element_types)
        converted_container = dict()
        # Convert each element of dict
        for key, val in value.items():
            converted_key = convert_type(split_element_types[0], key)
            converted_val = convert_type(split_element_types[1], val)
            converted_container[converted_key] = converted_val
        return converted_container


def send_ws_ping(wsock, timeout) :
    # Most webservers close websockets after 60 seconds of
    # inactivity. Make sure to send a ping before that.
    payload = "ping"
    # the ping payload can be anything, the receiver has to just
    # return the same back.
    while True:
        gevent.sleep(timeout)
        try:
            if wsock:
                wsock.send_frame(payload, wsock.OPCODE_PING)
        except Exception as e:
            wsock.close()
            return


class UserInGroup:
    ''' Authorization plugin callback that checks that the user is logged in
    and a member of a group. '''
    def __init__(self, group):
        self.group = group

    def __call__(self, session, *a, **kw):
        valid_user(session, *a, **kw)
        res = False

        try:
            res = session['user'] in grp.getgrnam(self.group)[3]
        except KeyError:
            pass

        if not res:
            abort(403, 'Insufficient access')


class RouteHandler(object):
    _require_auth = obmc.utils.misc.makelist(valid_user)
    _enable_cors = True

    def __init__(self, app, bus, verbs, rules, content_type=''):
        self.app = app
        self.bus = bus
        self.mapper = obmc.mapper.Mapper(bus)
        self._verbs = obmc.utils.misc.makelist(verbs)
        self._rules = rules
        self._content_type = content_type

        if 'GET' in self._verbs:
            self._verbs = list(set(self._verbs + ['HEAD']))
        if 'OPTIONS' not in self._verbs:
            self._verbs.append('OPTIONS')

    def _setup(self, **kw):
        request.route_data = {}

        if request.method in self._verbs:
            if request.method != 'OPTIONS':
                return self.setup(**kw)

            # Javascript implementations will not send credentials
            # with an OPTIONS request.  Don't help malicious clients
            # by checking the path here and returning a 404 if the
            # path doesn't exist.
            return None

        # Return 405
        raise HTTPError(
            405, "Method not allowed.", Allow=','.join(self._verbs))

    def __call__(self, **kw):
        return getattr(self, 'do_' + request.method.lower())(**kw)

    def do_head(self, **kw):
        return self.do_get(**kw)

    def do_options(self, **kw):
        for v in self._verbs:
            response.set_header(
                'Allow',
                ','.join(self._verbs))
        return None

    def install(self):
        self.app.route(
            self._rules, callback=self,
            method=['OPTIONS', 'GET', 'PUT', 'PATCH', 'POST', 'DELETE'])

    @staticmethod
    def try_mapper_call(f, callback=None, **kw):
        try:
            return f(**kw)
        except dbus.exceptions.DBusException as e:
            if e.get_dbus_name() == \
                    'org.freedesktop.DBus.Error.ObjectPathInUse':
                abort(503, str(e))
            if e.get_dbus_name() != obmc.mapper.MAPPER_NOT_FOUND:
                raise
            if callback is None:
                def callback(e, **kw):
                    abort(404, str(e))

            callback(e, **kw)

    @staticmethod
    def try_properties_interface(f, *a):
        try:
            return f(*a)
        except dbus.exceptions.DBusException as e:
            if DBUS_UNKNOWN_INTERFACE in e.get_dbus_name():
                # interface doesn't have any properties
                return None
            if DBUS_UNKNOWN_METHOD == e.get_dbus_name():
                # properties interface not implemented at all
                return None
            raise


class DirectoryHandler(RouteHandler):
    verbs = 'GET'
    rules = '<path:path>/'
    suppress_logging = True

    def __init__(self, app, bus):
        super(DirectoryHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, path='/'):
        return self.try_mapper_call(
            self.mapper.get_subtree_paths, path=path, depth=1)

    def setup(self, path='/'):
        request.route_data['map'] = self.find(path)

    def do_get(self, path='/'):
        return request.route_data['map']


class ListNamesHandler(RouteHandler):
    verbs = 'GET'
    rules = ['/list', '<path:path>/list']
    suppress_logging = True

    def __init__(self, app, bus):
        super(ListNamesHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, path='/'):
        return list(self.try_mapper_call(
            self.mapper.get_subtree, path=path).keys())

    def setup(self, path='/'):
        request.route_data['map'] = self.find(path)

    def do_get(self, path='/'):
        return request.route_data['map']


class ListHandler(RouteHandler):
    verbs = 'GET'
    rules = ['/enumerate', '<path:path>/enumerate']
    suppress_logging = True

    def __init__(self, app, bus):
        super(ListHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, path='/'):
        return self.try_mapper_call(
            self.mapper.get_subtree, path=path)

    def setup(self, path='/'):
        request.route_data['map'] = self.find(path)

    def do_get(self, path='/'):
        return {x: y for x, y in self.mapper.enumerate_subtree(
                path,
                mapper_data=request.route_data['map']).dataitems()}


class MethodHandler(RouteHandler):
    verbs = 'POST'
    rules = '<path:path>/action/<method>'
    request_type = list
    content_type = 'application/json'

    def __init__(self, app, bus):
        super(MethodHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)
        self.service = ''
        self.interface = ''

    def find(self, path, method):
        method_list = []
        buses = self.try_mapper_call(
            self.mapper.get_object, path=path)
        for items in buses.items():
            m = self.find_method_on_bus(path, method, *items)
            if m:
                method_list.append(m)
        if method_list:
            return method_list

        abort(404, _4034_msg % ('method', 'found', method))

    def setup(self, path, method):
        request.route_data['map'] = self.find(path, method)

    def do_post(self, path, method, retry=True):
        try:
            args = []
            if request.parameter_list:
                args = request.parameter_list
            # To see if the return type is capable of being merged
            if len(request.route_data['map']) > 1:
                results = None
                for item in request.route_data['map']:
                    tmp = item(*args)
                    if not results:
                        if tmp is not None:
                            results = type(tmp)()
                    if isinstance(results, dict):
                        results = results.update(tmp)
                    elif isinstance(results, list):
                        results = results + tmp
                    elif isinstance(results, type(None)):
                        results = None
                    else:
                        abort(501, 'Don\'t know how to merge method call '
                                   'results of {}'.format(type(tmp)))
                return results
            # There is only one method
            return request.route_data['map'][0](*args)

        except dbus.exceptions.DBusException as e:
            paramlist = []
            if e.get_dbus_name() == DBUS_INVALID_ARGS and retry:

                signature_list = get_method_signature(self.bus, self.service,
                                                      path, self.interface,
                                                      method)
                if not signature_list:
                    abort(400, "Failed to get method signature: %s" % str(e))
                if len(signature_list) != len(request.parameter_list):
                    abort(400, "Invalid number of args")
                converted_value = None
                try:
                    for index, expected_type in enumerate(signature_list):
                        value = request.parameter_list[index]
                        converted_value = convert_type(expected_type, value)
                        paramlist.append(converted_value)
                    request.parameter_list = paramlist
                    self.do_post(path, method, False)
                    return
                except Exception as ex:
                    abort(400, "Bad Request/Invalid Args given")
                abort(400, str(e))

            if e.get_dbus_name() == DBUS_TYPE_ERROR:
                abort(400, str(e))
            raise

    @staticmethod
    def find_method_in_interface(method, obj, interface, methods):
        if methods is None:
            return None

        method = obmc.utils.misc.find_case_insensitive(method, list(methods.keys()))
        if method is not None:
            iface = dbus.Interface(obj, interface)
            return iface.get_dbus_method(method)

    def find_method_on_bus(self, path, method, bus, interfaces):
        obj = self.bus.get_object(bus, path, introspect=False)
        iface = dbus.Interface(obj, dbus.INTROSPECTABLE_IFACE)
        data = iface.Introspect()
        parser = IntrospectionNodeParser(
            ElementTree.fromstring(data),
            intf_match=lambda x: x in interfaces)
        for x, y in parser.get_interfaces().items():
            m = self.find_method_in_interface(
                method, obj, x, y.get('method'))
            if m:
                self.service = bus
                self.interface = x
                return m


class PropertyHandler(RouteHandler):
    verbs = ['PUT', 'GET']
    rules = '<path:path>/attr/<prop>'
    content_type = 'application/json'

    def __init__(self, app, bus):
        super(PropertyHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)

    def find(self, path, prop):
        self.app.instance_handler.setup(path)
        obj = self.app.instance_handler.do_get(path)
        real_name = obmc.utils.misc.find_case_insensitive(
            prop, list(obj.keys()))

        if not real_name:
            if request.method == 'PUT':
                abort(403, _4034_msg % ('property', 'created', prop))
            else:
                abort(404, _4034_msg % ('property', 'found', prop))
        return real_name, {path: obj}

    def setup(self, path, prop):
        name, obj = self.find(path, prop)
        request.route_data['obj'] = obj
        request.route_data['name'] = name

    def do_get(self, path, prop):
        name = request.route_data['name']
        return request.route_data['obj'][path][name]

    def do_put(self, path, prop, value=None, retry=True):
        if value is None:
            value = request.parameter_list

        prop, iface, properties_iface = self.get_host_interface(
            path, prop, request.route_data['map'][path])
        try:
            properties_iface.Set(iface, prop, value)
        except ValueError as e:
            abort(400, str(e))
        except dbus.exceptions.DBusException as e:
            if e.get_dbus_name() == DBUS_PROPERTY_READONLY:
                abort(403, str(e))
            if e.get_dbus_name() == DBUS_INVALID_ARGS and retry:
                bus_name = properties_iface.bus_name
                expected_type = get_type_signature_by_introspection(self.bus,
                                                                    bus_name,
                                                                    path,
                                                                    prop)
                if not expected_type:
                    abort(403, "Failed to get expected type: %s" % str(e))
                converted_value = None
                try:
                    converted_value = convert_type(expected_type, value)
                except Exception as ex:
                    abort(403, "Failed to convert %s to type %s" %
                          (value, expected_type))
                try:
                    self.do_put(path, prop, converted_value, False)
                    return
                except Exception as ex:
                    abort(403, str(ex))

                abort(403, str(e))
            raise

    def get_host_interface(self, path, prop, bus_info):
        for bus, interfaces in bus_info.items():
            obj = self.bus.get_object(bus, path, introspect=True)
            properties_iface = dbus.Interface(
                obj, dbus_interface=dbus.PROPERTIES_IFACE)

            try:
                info = self.get_host_interface_on_bus(
                    path, prop, properties_iface, bus, interfaces)
            except Exception:
                continue
            if info is not None:
                prop, iface = info
                return prop, iface, properties_iface

    def get_host_interface_on_bus(self, path, prop, iface, bus, interfaces):
        for i in interfaces:
            properties = self.try_properties_interface(iface.GetAll, i)
            if not properties:
                continue
            match = obmc.utils.misc.find_case_insensitive(
                prop, list(properties.keys()))
            if match is None:
                continue
            prop = match
            return prop, i


class SchemaHandler(RouteHandler):
    verbs = ['GET']
    rules = '<path:path>/schema'
    suppress_logging = True

    def __init__(self, app, bus):
        super(SchemaHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, path):
        return self.try_mapper_call(
            self.mapper.get_object,
            path=path)

    def setup(self, path):
        request.route_data['map'] = self.find(path)

    def do_get(self, path):
        schema = {}
        for x in request.route_data['map'].keys():
            obj = self.bus.get_object(x, path, introspect=False)
            iface = dbus.Interface(obj, dbus.INTROSPECTABLE_IFACE)
            data = iface.Introspect()
            parser = IntrospectionNodeParser(
                ElementTree.fromstring(data))
            for x, y in parser.get_interfaces().items():
                schema[x] = y

        return schema


class InstanceHandler(RouteHandler):
    verbs = ['GET', 'PUT', 'DELETE']
    rules = '<path:path>'
    request_type = dict

    def __init__(self, app, bus):
        super(InstanceHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, path, callback=None):
        return {path: self.try_mapper_call(
            self.mapper.get_object,
            callback,
            path=path)}

    def setup(self, path):
        callback = None
        if request.method == 'PUT':
            def callback(e, **kw):
                abort(403, _4034_msg % ('resource', 'created', path))

        if request.route_data.get('map') is None:
            request.route_data['map'] = self.find(path, callback)

    def do_get(self, path):
        return self.mapper.enumerate_object(
            path,
            mapper_data=request.route_data['map'])

    def do_put(self, path):
        # make sure all properties exist in the request
        obj = set(self.do_get(path).keys())
        req = set(request.parameter_list.keys())

        diff = list(obj.difference(req))
        if diff:
            abort(403, _4034_msg % (
                'resource', 'removed', '%s/attr/%s' % (path, diff[0])))

        diff = list(req.difference(obj))
        if diff:
            abort(403, _4034_msg % (
                'resource', 'created', '%s/attr/%s' % (path, diff[0])))

        for p, v in request.parameter_list.items():
            self.app.property_handler.do_put(
                path, p, v)

    def do_delete(self, path):
        deleted = False
        for bus, interfaces in request.route_data['map'][path].items():
            if self.bus_has_delete(interfaces):
                self.delete_on_bus(path, bus)
                deleted = True

        #It's OK if some objects didn't have a Delete, but not all
        if not deleted:
            abort(403, _4034_msg % ('resource', 'removed', path))

    def bus_has_delete(self, interfaces):
        return DELETE_IFACE in interfaces

    def delete_on_bus(self, path, bus):
        obj = self.bus.get_object(bus, path, introspect=False)
        delete_iface = dbus.Interface(
            obj, dbus_interface=DELETE_IFACE)
        delete_iface.Delete()


class SessionHandler(MethodHandler):
    ''' Handles the /login and /logout routes, manages
    server side session store and session cookies.  '''

    rules = ['/login', '/logout']
    login_str = "User '%s' logged %s"
    bad_passwd_str = "Invalid username or password"
    no_user_str = "No user logged in"
    bad_json_str = "Expecting request format { 'data': " \
        "[<username>, <password>] }, got '%s'"
    bmc_not_ready_str = "BMC is not ready (booting)"
    _require_auth = None
    MAX_SESSIONS = 16
    BMCSTATE_IFACE = 'xyz.openbmc_project.State.BMC'
    BMCSTATE_PATH = '/xyz/openbmc_project/state/bmc0'
    BMCSTATE_PROPERTY = 'CurrentBMCState'
    BMCSTATE_READY = 'xyz.openbmc_project.State.BMC.BMCState.Ready'
    suppress_json_logging = True

    def __init__(self, app, bus):
        super(SessionHandler, self).__init__(
            app, bus)
        self.hmac_key = os.urandom(128)
        self.session_store = []

    @staticmethod
    def authenticate(username, clear):
        try:
            encoded = spwd.getspnam(username)[1]
            return encoded == crypt.crypt(clear, encoded)
        except KeyError:
            return False

    def invalidate_session(self, session):
        try:
            self.session_store.remove(session)
        except ValueError:
            pass

    def new_session(self):
        sid = os.urandom(32)
        if self.MAX_SESSIONS <= len(self.session_store):
            self.session_store.pop()
        self.session_store.insert(0, {'sid': sid})

        return self.session_store[0]

    def get_session(self, sid):
        sids = [x['sid'] for x in self.session_store]
        try:
            return self.session_store[sids.index(sid)]
        except ValueError:
            return None

    def get_session_from_cookie(self):
        return self.get_session(
            request.get_cookie(
                'sid', secret=self.hmac_key))

    def do_post(self, **kw):
        if request.path == '/login':
            return self.do_login(**kw)
        else:
            return self.do_logout(**kw)

    def do_logout(self, **kw):
        session = self.get_session_from_cookie()
        if session is not None:
            user = session['user']
            self.invalidate_session(session)
            response.delete_cookie('sid')
            return self.login_str % (user, 'out')

        return self.no_user_str

    def do_login(self, **kw):
        if len(request.parameter_list) != 2:
            abort(400, self.bad_json_str % (request.json))

        if not self.authenticate(*request.parameter_list):
            abort(401, self.bad_passwd_str)

        force = False
        try:
            force = request.json.get('force')
        except (ValueError, AttributeError, KeyError, TypeError):
            force = False

        if not force and not self.is_bmc_ready():
            abort(503, self.bmc_not_ready_str)

        user = request.parameter_list[0]
        session = self.new_session()
        session['user'] = user
        response.set_cookie(
            'sid', session['sid'], secret=self.hmac_key,
            secure=True,
            httponly=True)
        return self.login_str % (user, 'in')

    def is_bmc_ready(self):
        if not self.app.with_bmc_check:
            return True

        try:
            obj = self.bus.get_object(self.BMCSTATE_IFACE, self.BMCSTATE_PATH)
            iface = dbus.Interface(obj, dbus.PROPERTIES_IFACE)
            state = iface.Get(self.BMCSTATE_IFACE, self.BMCSTATE_PROPERTY)
            if state == self.BMCSTATE_READY:
                return True

        except dbus.exceptions.DBusException:
            pass

        return False

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass


class ImageUploadUtils:
    ''' Provides common utils for image upload. '''

    file_loc = '/tmp/images'
    file_prefix = 'img'
    file_suffix = ''
    signal = None

    @classmethod
    def do_upload(cls, filename=''):
        def cleanup():
            os.close(handle)
            if cls.signal:
                cls.signal.remove()
                cls.signal = None

        def signal_callback(path, a, **kw):
            # Just interested on the first Version interface created which is
            # triggered when the file is uploaded. This helps avoid getting the
            # wrong information for multiple upload requests in a row.
            if "xyz.openbmc_project.Software.Version" in a and \
               "xyz.openbmc_project.Software.Activation" not in a:
                paths.append(path)

        while cls.signal:
            # Serialize uploads by waiting for the signal to be cleared.
            # This makes it easier to ensure that the version information
            # is the right one instead of the data from another upload request.
            gevent.sleep(1)
        if not os.path.exists(cls.file_loc):
            abort(500, "Error Directory not found")
        paths = []
        bus = dbus.SystemBus()
        cls.signal = bus.add_signal_receiver(
            signal_callback,
            dbus_interface=dbus.BUS_DAEMON_IFACE + '.ObjectManager',
            signal_name='InterfacesAdded',
            path=SOFTWARE_PATH)
        if not filename:
            handle, filename = tempfile.mkstemp(cls.file_suffix,
                                                cls.file_prefix, cls.file_loc)
        else:
            filename = os.path.join(cls.file_loc, filename)
            handle = os.open(filename, os.O_WRONLY | os.O_CREAT)
        try:
            file_contents = request.body.read()
            request.body.close()
            os.write(handle, file_contents)
            # Close file after writing, the image manager process watches for
            # the close event to know the upload is complete.
            os.close(handle)
        except (IOError, ValueError) as e:
            cleanup()
            abort(400, str(e))
        except Exception:
            cleanup()
            abort(400, "Unexpected Error")
        loop = gobject.MainLoop()
        gcontext = loop.get_context()
        count = 0
        version_id = ''
        while loop is not None:
            try:
                if gcontext.pending():
                    gcontext.iteration()
                if not paths:
                    gevent.sleep(1)
                else:
                    version_id = os.path.basename(paths.pop())
                    break
                count += 1
                if count == 10:
                    break
            except Exception:
                break
        cls.signal.remove()
        cls.signal = None
        if version_id:
            return version_id
        else:
            abort(400, "Version already exists or failed to be extracted")


class ImagePostHandler(RouteHandler):
    ''' Handles the /upload/image route. '''

    verbs = ['POST']
    rules = ['/upload/image']
    content_type = 'application/octet-stream'

    def __init__(self, app, bus):
        super(ImagePostHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)

    def do_post(self, filename=''):
        return ImageUploadUtils.do_upload()

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass


class CertificateHandler:
    file_suffix = '.pem'
    file_prefix = 'cert_'
    CERT_PATH = '/xyz/openbmc_project/certs'
    CERT_IFACE = 'xyz.openbmc_project.Certs.Install'

    def __init__(self, route_handler, cert_type, service):
        if not service:
            abort(500, "Missing service")
        if not cert_type:
            abort(500, "Missing certificate type")
        bus = dbus.SystemBus()
        certPath = self.CERT_PATH + "/" + cert_type + "/" + service
        intfs = route_handler.try_mapper_call(
            route_handler.mapper.get_object, path=certPath)
        for busName,intf in intfs.items():
            if self.CERT_IFACE in intf:
                self.obj = bus.get_object(busName, certPath)
                return
        abort(404, "Path not found")

    def do_upload(self):
        def cleanup():
            if os.path.exists(temp.name):
                os.remove(temp.name)

        with tempfile.NamedTemporaryFile(
            suffix=self.file_suffix,
            prefix=self.file_prefix,
            delete=False) as temp:
            try:
                file_contents = request.body.read()
                request.body.close()
                temp.write(file_contents)
            except (IOError, ValueError) as e:
                cleanup()
                abort(500, str(e))
            except Exception:
                cleanup()
                abort(500, "Unexpected Error")

        try:
            iface = dbus.Interface(self.obj, self.CERT_IFACE)
            iface.Install(temp.name)
        except Exception as e:
            cleanup()
            abort(400, str(e))
        cleanup()

    def do_delete(self):
        delete_iface = dbus.Interface(
            self.obj, dbus_interface=DELETE_IFACE)
        delete_iface.Delete()


class CertificatePutHandler(RouteHandler):
    ''' Handles the /xyz/openbmc_project/certs/<cert_type>/<service> route. '''

    verbs = ['PUT', 'DELETE']
    rules = ['/xyz/openbmc_project/certs/<cert_type>/<service>']
    content_type = 'application/octet-stream'

    def __init__(self, app, bus):
        super(CertificatePutHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)

    def do_put(self, cert_type, service):
        return CertificateHandler(self, cert_type, service).do_upload()

    def do_delete(self, cert_type, service):
        return CertificateHandler(self, cert_type, service).do_delete()

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass


class EventNotifier:
    keyNames = {}
    keyNames['event'] = 'event'
    keyNames['path'] = 'path'
    keyNames['intfMap'] = 'interfaces'
    keyNames['propMap'] = 'properties'
    keyNames['intf'] = 'interface'

    def __init__(self, wsock, filters):
        self.wsock = wsock
        self.paths = filters.get("paths", [])
        self.interfaces = filters.get("interfaces", [])
        self.signals = []
        self.socket_error = False
        if not self.paths:
            self.paths.append(None)
        bus = dbus.SystemBus()
        # Add a signal receiver for every path the client is interested in
        for path in self.paths:
            add_sig = bus.add_signal_receiver(
                self.interfaces_added_handler,
                dbus_interface=dbus.BUS_DAEMON_IFACE + '.ObjectManager',
                signal_name='InterfacesAdded',
                path=path)
            chg_sig = bus.add_signal_receiver(
                self.properties_changed_handler,
                dbus_interface=dbus.PROPERTIES_IFACE,
                signal_name='PropertiesChanged',
                path=path,
                path_keyword='path')
            self.signals.append(add_sig)
            self.signals.append(chg_sig)
        loop = gobject.MainLoop()
        # gobject's mainloop.run() will block the entire process, so the gevent
        # scheduler and hence greenlets won't execute. The while-loop below
        # works around this limitation by using gevent's sleep, instead of
        # calling loop.run()
        gcontext = loop.get_context()
        while loop is not None:
            try:
                if self.socket_error:
                    for signal in self.signals:
                        signal.remove()
                    loop.quit()
                    break;
                if gcontext.pending():
                    gcontext.iteration()
                else:
                    # gevent.sleep puts only the current greenlet to sleep,
                    # not the entire process.
                    gevent.sleep(5)
            except WebSocketError:
                break

    def interfaces_added_handler(self, path, iprops, **kw):
        ''' If the client is interested in these changes, respond to the
            client. This handles d-bus interface additions.'''
        if (not self.interfaces) or \
           (not set(iprops).isdisjoint(self.interfaces)):
            response = {}
            response[self.keyNames['event']] = "InterfacesAdded"
            response[self.keyNames['path']] = path
            response[self.keyNames['intfMap']] = iprops
            try:
                self.wsock.send(json.dumps(response))
            except:
                self.socket_error = True
                return

    def properties_changed_handler(self, interface, new, old, **kw):
        ''' If the client is interested in these changes, respond to the
            client. This handles d-bus property changes. '''
        if (not self.interfaces) or (interface in self.interfaces):
            path = str(kw['path'])
            response = {}
            response[self.keyNames['event']] = "PropertiesChanged"
            response[self.keyNames['path']] = path
            response[self.keyNames['intf']] = interface
            response[self.keyNames['propMap']] = new
            try:
                self.wsock.send(json.dumps(response))
            except:
                self.socket_error = True
                return


class EventHandler(RouteHandler):
    ''' Handles the /subscribe route, for clients to be able
        to subscribe to BMC events. '''

    verbs = ['GET']
    rules = ['/subscribe']
    suppress_logging = True

    def __init__(self, app, bus):
        super(EventHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass

    def do_get(self):
        wsock = request.environ.get('wsgi.websocket')
        if not wsock:
            abort(400, 'Expected WebSocket request.')
        ping_sender = Greenlet.spawn(send_ws_ping, wsock, WEBSOCKET_TIMEOUT)
        filters = wsock.receive()
        filters = json.loads(filters)
        notifier = EventNotifier(wsock, filters)

class HostConsoleHandler(RouteHandler):
    ''' Handles the /console route, for clients to be able
        read/write the host serial console. The way this is
        done is by exposing a websocket that's mirrored to an
        abstract UNIX domain socket, which is the source for
        the console data. '''

    verbs = ['GET']
    # Naming the route console0, because the numbering will help
    # on multi-bmc/multi-host systems.
    rules = ['/console0']
    suppress_logging = True

    def __init__(self, app, bus):
        super(HostConsoleHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass

    def read_wsock(self, wsock, sock):
        while True:
            try:
                incoming = wsock.receive()
                if incoming:
                    # Read websocket, write to UNIX socket
                    sock.send(incoming)
            except Exception as e:
                sock.close()
                return

    def read_sock(self, sock, wsock):
        max_sock_read_len = 4096
        while True:
            try:
                outgoing = sock.recv(max_sock_read_len)
                if outgoing:
                    # Read UNIX socket, write to websocket
                    wsock.send(outgoing)
            except Exception as e:
                wsock.close()
                return

    def do_get(self):
        wsock = request.environ.get('wsgi.websocket')
        if not wsock:
            abort(400, 'Expected WebSocket based request.')

        # An abstract Unix socket path must be less than or equal to 108 bytes
        # and does not need to be nul-terminated or padded out to 108 bytes
        socket_name = "\0obmc-console"
        socket_path = socket_name
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        try:
            sock.connect(socket_path)
        except Exception as e:
            abort(500, str(e))

        wsock_reader = Greenlet.spawn(self.read_wsock, wsock, sock)
        sock_reader = Greenlet.spawn(self.read_sock, sock, wsock)
        ping_sender = Greenlet.spawn(send_ws_ping, wsock, WEBSOCKET_TIMEOUT)
        gevent.joinall([wsock_reader, sock_reader, ping_sender])


class ImagePutHandler(RouteHandler):
    ''' Handles the /upload/image/<filename> route. '''

    verbs = ['PUT']
    rules = ['/upload/image/<filename>']
    content_type = 'application/octet-stream'

    def __init__(self, app, bus):
        super(ImagePutHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)

    def do_put(self, filename=''):
        return ImageUploadUtils.do_upload(filename)

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass


class DownloadDumpHandler(RouteHandler):
    ''' Handles the /download/dump route. '''

    verbs = 'GET'
    rules = ['/download/dump/<dumpid>']
    content_type = 'application/octet-stream'
    dump_loc = '/var/lib/phosphor-debug-collector/dumps'
    suppress_json_resp = True
    suppress_logging = True

    def __init__(self, app, bus):
        super(DownloadDumpHandler, self).__init__(
            app, bus, self.verbs, self.rules, self.content_type)

    def do_get(self, dumpid):
        return self.do_download(dumpid)

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass

    def do_download(self, dumpid):
        dump_loc = os.path.join(self.dump_loc, dumpid)
        if not os.path.exists(dump_loc):
            abort(404, "Path not found")

        files = os.listdir(dump_loc)
        num_files = len(files)
        if num_files == 0:
            abort(404, "Dump not found")

        return static_file(os.path.basename(files[0]), root=dump_loc,
                           download=True, mimetype=self.content_type)


class WebHandler(RouteHandler):
    ''' Handles the routes for the web UI files. '''

    verbs = 'GET'

    # Match only what we know are web files, so everything else
    # can get routed to the REST handlers.
    rules = ['//', '/<filename:re:.+\.js>', '/<filename:re:.+\.svg>',
             '/<filename:re:.+\.css>', '/<filename:re:.+\.ttf>',
             '/<filename:re:.+\.eot>', '/<filename:re:.+\.woff>',
             '/<filename:re:.+\.woff2>', '/<filename:re:.+\.map>',
             '/<filename:re:.+\.png>', '/<filename:re:.+\.html>',
             '/<filename:re:.+\.ico>']

    # The mimetypes module knows about most types, but not these
    content_types = {
        '.eot': 'application/vnd.ms-fontobject',
        '.woff': 'application/x-font-woff',
        '.woff2': 'application/x-font-woff2',
        '.ttf': 'application/x-font-ttf',
        '.map': 'application/json'
    }

    _require_auth = None
    suppress_json_resp = True
    suppress_logging = True

    def __init__(self, app, bus):
        super(WebHandler, self).__init__(
            app, bus, self.verbs, self.rules)

    def get_type(self, filename):
        ''' Returns the content type and encoding for a file '''

        content_type, encoding = mimetypes.guess_type(filename)

        # Try our own list if mimetypes didn't recognize it
        if content_type is None:
            if filename[-3:] == '.gz':
                filename = filename[:-3]
            extension = filename[filename.rfind('.'):]
            content_type = self.content_types.get(extension, None)

        return content_type, encoding

    def do_get(self, filename='index.html'):

        # If a gzipped version exists, use that instead.
        # Possible future enhancement: if the client doesn't
        # accept compressed files, unzip it ourselves before sending.
        if not os.path.exists(os.path.join(www_base_path, filename)):
            filename = filename + '.gz'

        # Though bottle should protect us, ensure path is valid
        realpath = os.path.realpath(filename)
        if realpath[0] == '/':
            realpath = realpath[1:]
        if not os.path.exists(os.path.join(www_base_path, realpath)):
            abort(404, "Path not found")

        mimetype, encoding = self.get_type(filename)

        # Couldn't find the type - let static_file() deal with it,
        # though this should never happen.
        if mimetype is None:
            print("Can't figure out content-type for %s" % filename)
            mimetype = 'auto'

        # This call will set several header fields for us,
        # including the charset if the type is text.
        response = static_file(filename, www_base_path, mimetype)

        # static_file() will only set the encoding if the
        # mimetype was auto, so set it here.
        if encoding is not None:
            response.set_header('Content-Encoding', encoding)

        return response

    def find(self, **kw):
        pass

    def setup(self, **kw):
        pass


class AuthorizationPlugin(object):
    ''' Invokes an optional list of authorization callbacks. '''

    name = 'authorization'
    api = 2

    class Compose:
        def __init__(self, validators, callback, session_mgr):
            self.validators = validators
            self.callback = callback
            self.session_mgr = session_mgr

        def __call__(self, *a, **kw):
            sid = request.get_cookie('sid', secret=self.session_mgr.hmac_key)
            session = self.session_mgr.get_session(sid)
            if request.method != 'OPTIONS':
                for x in self.validators:
                    x(session, *a, **kw)

            return self.callback(*a, **kw)

    def apply(self, callback, route):
        undecorated = route.get_undecorated_callback()
        if not isinstance(undecorated, RouteHandler):
            return callback

        auth_types = getattr(
            undecorated, '_require_auth', None)
        if not auth_types:
            return callback

        return self.Compose(
            auth_types, callback, undecorated.app.session_handler)


class CorsPlugin(object):
    ''' Add CORS headers. '''

    name = 'cors'
    api = 2

    @staticmethod
    def process_origin():
        origin = request.headers.get('Origin')
        if origin:
            response.add_header('Access-Control-Allow-Origin', origin)
            response.add_header(
                'Access-Control-Allow-Credentials', 'true')

    @staticmethod
    def process_method_and_headers(verbs):
        method = request.headers.get('Access-Control-Request-Method')
        headers = request.headers.get('Access-Control-Request-Headers')
        if headers:
            headers = [x.lower() for x in headers.split(',')]

        if method in verbs \
                and headers == ['content-type']:
            response.add_header('Access-Control-Allow-Methods', method)
            response.add_header(
                'Access-Control-Allow-Headers', 'Content-Type')
            response.add_header('X-Frame-Options', 'deny')
            response.add_header('X-Content-Type-Options', 'nosniff')
            response.add_header('X-XSS-Protection', '1; mode=block')
            response.add_header(
                'Content-Security-Policy', "default-src 'self'")
            response.add_header(
                'Strict-Transport-Security',
                'max-age=31536000; includeSubDomains; preload')

    def __init__(self, app):
        app.install_error_callback(self.error_callback)

    def apply(self, callback, route):
        undecorated = route.get_undecorated_callback()
        if not isinstance(undecorated, RouteHandler):
            return callback

        if not getattr(undecorated, '_enable_cors', None):
            return callback

        def wrap(*a, **kw):
            self.process_origin()
            self.process_method_and_headers(undecorated._verbs)
            return callback(*a, **kw)

        return wrap

    def error_callback(self, **kw):
        self.process_origin()


class JsonApiRequestPlugin(object):
    ''' Ensures request content satisfies the OpenBMC json api format. '''
    name = 'json_api_request'
    api = 2

    error_str = "Expecting request format { 'data': <value> }, got '%s'"
    type_error_str = "Unsupported Content-Type: '%s'"
    json_type = "application/json"
    request_methods = ['PUT', 'POST', 'PATCH']

    @staticmethod
    def content_expected():
        return request.method in JsonApiRequestPlugin.request_methods

    def validate_request(self):
        if request.content_length > 0 and \
                request.content_type != self.json_type:
            abort(415, self.type_error_str % request.content_type)

        try:
            request.parameter_list = request.json.get('data')
        except ValueError as e:
            abort(400, str(e))
        except (AttributeError, KeyError, TypeError):
            abort(400, self.error_str % request.json)

    def apply(self, callback, route):
        content_type = getattr(
            route.get_undecorated_callback(), '_content_type', None)
        if self.json_type != content_type:
            return callback

        verbs = getattr(
            route.get_undecorated_callback(), '_verbs', None)
        if verbs is None:
            return callback

        if not set(self.request_methods).intersection(verbs):
            return callback

        def wrap(*a, **kw):
            if self.content_expected():
                self.validate_request()
            return callback(*a, **kw)

        return wrap


class JsonApiRequestTypePlugin(object):
    ''' Ensures request content type satisfies the OpenBMC json api format. '''
    name = 'json_api_method_request'
    api = 2

    error_str = "Expecting request format { 'data': %s }, got '%s'"
    json_type = "application/json"

    def apply(self, callback, route):
        content_type = getattr(
            route.get_undecorated_callback(), '_content_type', None)
        if self.json_type != content_type:
            return callback

        request_type = getattr(
            route.get_undecorated_callback(), 'request_type', None)
        if request_type is None:
            return callback

        def validate_request():
            if not isinstance(request.parameter_list, request_type):
                abort(400, self.error_str % (str(request_type), request.json))

        def wrap(*a, **kw):
            if JsonApiRequestPlugin.content_expected():
                validate_request()
            return callback(*a, **kw)

        return wrap


class JsonErrorsPlugin(JSONPlugin):
    ''' Extend the Bottle JSONPlugin such that it also encodes error
        responses. '''

    def __init__(self, app, **kw):
        super(JsonErrorsPlugin, self).__init__(**kw)
        self.json_opts = {
            x: y for x, y in kw.items()
            if x in ['indent', 'sort_keys']}
        app.install_error_callback(self.error_callback)

    def error_callback(self, response_object, response_body, **kw):
        response_body['body'] = json.dumps(response_object, **self.json_opts)
        response.content_type = 'application/json'


class JsonApiResponsePlugin(object):
    ''' Emits responses in the OpenBMC json api format. '''
    name = 'json_api_response'
    api = 2

    @staticmethod
    def has_body():
        return request.method not in ['OPTIONS']

    def __init__(self, app):
        app.install_error_callback(self.error_callback)

    @staticmethod
    def dbus_boolean_to_bool(data):
        ''' Convert all dbus.Booleans to true/false instead of 1/0 as
            the JSON encoder thinks they're ints.  Note that unlike
            dicts and lists, tuples (from a dbus.Struct) are immutable
            so they need special handling. '''

        def walkdict(data):
            for key, value in data.items():
                if isinstance(value, dbus.Boolean):
                    data[key] = bool(value)
                elif isinstance(value, tuple):
                    data[key] = walktuple(value)
                else:
                    JsonApiResponsePlugin.dbus_boolean_to_bool(value)

        def walklist(data):
            for i in range(len(data)):
                if isinstance(data[i], dbus.Boolean):
                    data[i] = bool(data[i])
                elif isinstance(data[i], tuple):
                    data[i] = walktuple(data[i])
                else:
                    JsonApiResponsePlugin.dbus_boolean_to_bool(data[i])

        def walktuple(data):
            new = []
            for item in data:
                if isinstance(item, dbus.Boolean):
                    item = bool(item)
                else:
                    JsonApiResponsePlugin.dbus_boolean_to_bool(item)
                new.append(item)
            return tuple(new)

        if isinstance(data, dict):
            walkdict(data)
        elif isinstance(data, list):
            walklist(data)

    def apply(self, callback, route):
        skip = getattr(
            route.get_undecorated_callback(), 'suppress_json_resp', None)
        if skip:
            return callback

        def wrap(*a, **kw):
            data = callback(*a, **kw)
            JsonApiResponsePlugin.dbus_boolean_to_bool(data)
            if self.has_body():
                resp = {'data': data}
                resp['status'] = 'ok'
                resp['message'] = response.status_line
                return resp
        return wrap

    def error_callback(self, error, response_object, **kw):
        response_object['message'] = error.status_line
        response_object['status'] = 'error'
        response_object.setdefault('data', {})['description'] = str(error.body)
        if error.status_code == 500:
            response_object['data']['exception'] = repr(error.exception)
            response_object['data']['traceback'] = error.traceback.splitlines()


class JsonpPlugin(object):
    ''' Json javascript wrapper. '''
    name = 'jsonp'
    api = 2

    def __init__(self, app, **kw):
        app.install_error_callback(self.error_callback)

    @staticmethod
    def to_jsonp(json):
        jwrapper = request.query.callback or None
        if(jwrapper):
            response.set_header('Content-Type', 'application/javascript')
            json = jwrapper + '(' + json + ');'
        return json

    def apply(self, callback, route):
        def wrap(*a, **kw):
            return self.to_jsonp(callback(*a, **kw))
        return wrap

    def error_callback(self, response_body, **kw):
        response_body['body'] = self.to_jsonp(response_body['body'])


class ContentCheckerPlugin(object):
    ''' Ensures that a route is associated with the expected content-type
        header. '''
    name = 'content_checker'
    api = 2

    class Checker:
        def __init__(self, type, callback):
            self.expected_type = type
            self.callback = callback
            self.error_str = "Expecting content type '%s', got '%s'"

        def __call__(self, *a, **kw):
            if request.method in ['PUT', 'POST', 'PATCH'] and \
                    self.expected_type and \
                    self.expected_type != request.content_type:
                abort(415, self.error_str % (self.expected_type,
                      request.content_type))

            return self.callback(*a, **kw)

    def apply(self, callback, route):
        content_type = getattr(
            route.get_undecorated_callback(), '_content_type', None)

        return self.Checker(content_type, callback)


class LoggingPlugin(object):
    ''' Wraps a request in order to emit a log after the request is handled. '''
    name = 'loggingp'
    api = 2

    class Logger:
        def __init__(self, suppress_json_logging, callback, app):
            self.suppress_json_logging = suppress_json_logging
            self.callback = callback
            self.app = app
            self.logging_enabled = None
            self.bus = dbus.SystemBus()
            self.dbus_path = '/xyz/openbmc_project/logging/rest_api_logs'
            self.no_json = [
                '/xyz/openbmc_project/user/ldap/action/CreateConfig'
            ]
            self.bus.add_signal_receiver(
                self.properties_changed_handler,
                dbus_interface=dbus.PROPERTIES_IFACE,
                signal_name='PropertiesChanged',
                path=self.dbus_path)
            Greenlet.spawn(self.dbus_loop)

        def __call__(self, *a, **kw):
            resp = self.callback(*a, **kw)
            if not self.enabled():
                return resp
            if request.method == 'GET':
                return resp
            json = request.json
            if self.suppress_json_logging:
                json = None
            elif any(substring in request.url for substring in self.no_json):
                json = None
            session = self.app.session_handler.get_session_from_cookie()
            user = None
            if "/login" in request.url:
                user = request.parameter_list[0]
            elif session is not None:
                user = session['user']
            print("{remote} user:{user} {method} {url} json:{json} {status}" \
                .format(
                    user=user,
                    remote=request.remote_addr,
                    method=request.method,
                    url=request.url,
                    json=json,
                    status=response.status))
            return resp

        def enabled(self):
            if self.logging_enabled is None:
                try:
                    obj = self.bus.get_object(
                              'xyz.openbmc_project.Settings',
                              self.dbus_path)
                    iface = dbus.Interface(obj, dbus.PROPERTIES_IFACE)
                    logging_enabled = iface.Get(
                                          'xyz.openbmc_project.Object.Enable',
                                          'Enabled')
                    self.logging_enabled = logging_enabled
                except dbus.exceptions.DBusException:
                    self.logging_enabled = False
            return self.logging_enabled

        def dbus_loop(self):
            loop = gobject.MainLoop()
            gcontext = loop.get_context()
            while loop is not None:
                try:
                    if gcontext.pending():
                        gcontext.iteration()
                    else:
                        gevent.sleep(5)
                except Exception as e:
                    break

        def properties_changed_handler(self, interface, new, old, **kw):
            self.logging_enabled = new.values()[0]

    def apply(self, callback, route):
        cb = route.get_undecorated_callback()
        skip = getattr(
            cb, 'suppress_logging', None)
        if skip:
            return callback

        suppress_json_logging = getattr(
            cb, 'suppress_json_logging', None)
        return self.Logger(suppress_json_logging, callback, cb.app)


class App(Bottle):
    def __init__(self, **kw):
        super(App, self).__init__(autojson=False)

        self.have_wsock = kw.get('have_wsock', False)
        self.with_bmc_check = '--with-bmc-check' in sys.argv

        self.bus = dbus.SystemBus()
        self.mapper = obmc.mapper.Mapper(self.bus)
        self.error_callbacks = []

        self.install_hooks()
        self.install_plugins()
        self.create_handlers()
        self.install_handlers()

    def install_plugins(self):
        # install json api plugins
        json_kw = {'indent': 2, 'sort_keys': True}
        self.install(AuthorizationPlugin())
        self.install(CorsPlugin(self))
        self.install(ContentCheckerPlugin())
        self.install(JsonpPlugin(self, **json_kw))
        self.install(JsonErrorsPlugin(self, **json_kw))
        self.install(JsonApiResponsePlugin(self))
        self.install(JsonApiRequestPlugin())
        self.install(JsonApiRequestTypePlugin())
        self.install(LoggingPlugin())

    def install_hooks(self):
        self.error_handler_type = type(self.default_error_handler)
        self.original_error_handler = self.default_error_handler
        self.default_error_handler = self.error_handler_type(
            self.custom_error_handler, self, Bottle)

        self.real_router_match = self.router.match
        self.router.match = self.custom_router_match
        self.add_hook('before_request', self.strip_extra_slashes)

    def create_handlers(self):
        # create route handlers
        self.session_handler = SessionHandler(self, self.bus)
        self.web_handler = WebHandler(self, self.bus)
        self.directory_handler = DirectoryHandler(self, self.bus)
        self.list_names_handler = ListNamesHandler(self, self.bus)
        self.list_handler = ListHandler(self, self.bus)
        self.method_handler = MethodHandler(self, self.bus)
        self.property_handler = PropertyHandler(self, self.bus)
        self.schema_handler = SchemaHandler(self, self.bus)
        self.image_upload_post_handler = ImagePostHandler(self, self.bus)
        self.image_upload_put_handler = ImagePutHandler(self, self.bus)
        self.download_dump_get_handler = DownloadDumpHandler(self, self.bus)
        self.certificate_put_handler = CertificatePutHandler(self, self.bus)
        if self.have_wsock:
            self.event_handler = EventHandler(self, self.bus)
            self.host_console_handler = HostConsoleHandler(self, self.bus)
        self.instance_handler = InstanceHandler(self, self.bus)

    def install_handlers(self):
        self.session_handler.install()
        self.web_handler.install()
        self.directory_handler.install()
        self.list_names_handler.install()
        self.list_handler.install()
        self.method_handler.install()
        self.property_handler.install()
        self.schema_handler.install()
        self.image_upload_post_handler.install()
        self.image_upload_put_handler.install()
        self.download_dump_get_handler.install()
        self.certificate_put_handler.install()
        if self.have_wsock:
            self.event_handler.install()
            self.host_console_handler.install()
        # this has to come last, since it matches everything
        self.instance_handler.install()

    def install_error_callback(self, callback):
        self.error_callbacks.insert(0, callback)

    def custom_router_match(self, environ):
        ''' The built-in Bottle algorithm for figuring out if a 404 or 405 is
            needed doesn't work for us since the instance rules match
            everything. This monkey-patch lets the route handler figure
            out which response is needed.  This could be accomplished
            with a hook but that would require calling the router match
            function twice.
        '''
        route, args = self.real_router_match(environ)
        if isinstance(route.callback, RouteHandler):
            route.callback._setup(**args)

        return route, args

    def custom_error_handler(self, res, error):
        ''' Allow plugins to modify error responses too via this custom
            error handler. '''

        response_object = {}
        response_body = {}
        for x in self.error_callbacks:
            x(error=error,
                response_object=response_object,
                response_body=response_body)

        return response_body.get('body', "")

    @staticmethod
    def strip_extra_slashes():
        path = request.environ['PATH_INFO']
        trailing = ("", "/")[path[-1] == '/']
        parts = list(filter(bool, path.split('/')))
        request.environ['PATH_INFO'] = '/' + '/'.join(parts) + trailing
