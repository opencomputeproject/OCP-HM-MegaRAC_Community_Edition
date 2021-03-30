#!/usr/bin/env python3

'''Phosphor DBus Monitor YAML parser and code generator.

The parser workflow is broken down as follows:
  1 - Import YAML files as native python type(s) instance(s).
  2 - Create an instance of the Everything class from the
        native python type instance(s) with the Everything.load
        method.
  3 - The Everything class constructor orchestrates conversion of the
        native python type(s) instances(s) to render helper types.
        Each render helper type constructor imports its attributes
        from the native python type(s) instances(s).
  4 - Present the converted YAML to the command processing method
        requested by the script user.
'''

import os
import sys
import yaml
import mako.lookup
from argparse import ArgumentParser
from sdbusplus.renderer import Renderer
from sdbusplus.namedelement import NamedElement
import sdbusplus.property


class InvalidConfigError(BaseException):
    '''General purpose config file parsing error.'''

    def __init__(self, path, msg):
        '''Display configuration file with the syntax
        error and the error message.'''

        self.config = path
        self.msg = msg


class NotUniqueError(InvalidConfigError):
    '''Within a config file names must be unique.
    Display the config file with the duplicate and
    the duplicate itself.'''

    def __init__(self, path, cls, *names):
        fmt = 'Duplicate {0}: "{1}"'
        super(NotUniqueError, self).__init__(
            path, fmt.format(cls, ' '.join(names)))


def get_index(objs, cls, name, config=None):
    '''Items are usually rendered as C++ arrays and as
    such are stored in python lists.  Given an item name
    its class, and an optional config file filter, find
    the item index.'''

    for i, x in enumerate(objs.get(cls, [])):
        if config and x.configfile != config:
            continue
        if x.name != name:
            continue

        return i
    raise InvalidConfigError(config, 'Could not find name: "{0}"'.format(name))


def exists(objs, cls, name, config=None):
    '''Check to see if an item already exists in a list given
    the item name.'''

    try:
        get_index(objs, cls, name, config)
    except:
        return False

    return True


def add_unique(obj, *a, **kw):
    '''Add an item to one or more lists unless already present,
    with an option to constrain the search to a specific config file.'''

    for container in a:
        if not exists(container, obj.cls, obj.name, config=kw.get('config')):
            container.setdefault(obj.cls, []).append(obj)


class Cast(object):
    '''Decorate an argument by casting it.'''

    def __init__(self, cast, target):
        '''cast is the cast type (static, const, etc...).
           target is the cast target type.'''
        self.cast = cast
        self.target = target

    def __call__(self, arg):
        return '{0}_cast<{1}>({2})'.format(self.cast, self.target, arg)


class Literal(object):
    '''Decorate an argument with a literal operator.'''

    integer_types = [
        'int16',
        'int32',
        'int64',
        'uint16',
        'uint32',
        'uint64'
    ]

    def __init__(self, type):
        self.type = type

    def __call__(self, arg):
        if 'uint' in self.type:
            arg = '{0}ull'.format(arg)
        elif 'int' in self.type:
            arg = '{0}ll'.format(arg)

        if self.type in self.integer_types:
            return Cast('static', '{0}_t'.format(self.type))(arg)
        elif self.type == 'byte':
            return Cast('static', 'uint8_t'.format(self.type))(arg)

        if self.type == 'string':
            return '{0}s'.format(arg)

        return arg


class FixBool(object):
    '''Un-capitalize booleans.'''

    def __call__(self, arg):
        return '{0}'.format(arg.lower())


class Quote(object):
    '''Decorate an argument by quoting it.'''

    def __call__(self, arg):
        return '"{0}"'.format(arg)


class Argument(NamedElement, Renderer):
    '''Define argument type interface.'''

    def __init__(self, **kw):
        self.type = kw.pop('type', None)
        super(Argument, self).__init__(**kw)

    def argument(self, loader, indent):
        raise NotImplementedError


class TrivialArgument(Argument):
    '''Non-array type arguments.'''

    def __init__(self, **kw):
        self.value = kw.pop('value')
        self.decorators = kw.pop('decorators', [])
        if kw.get('type', None):
            self.decorators.insert(0, Literal(kw['type']))
        if kw.get('type', None) == 'string':
            self.decorators.insert(0, Quote())
        if kw.get('type', None) == 'boolean':
            self.decorators.insert(0, FixBool())

        super(TrivialArgument, self).__init__(**kw)

    def argument(self, loader, indent):
        a = str(self.value)
        for d in self.decorators:
            a = d(a)

        return a


class Metadata(Argument):
    '''Metadata type arguments.'''

    def __init__(self, **kw):
        self.value = kw.pop('value')
        self.decorators = kw.pop('decorators', [])
        if kw.get('type', None) == 'string':
            self.decorators.insert(0, Quote())

        super(Metadata, self).__init__(**kw)

    def argument(self, loader, indent):
        a = str(self.value)
        for d in self.decorators:
            a = d(a)

        return a


class OpArgument(Argument):
    '''Operation type arguments.'''

    def __init__(self, **kw):
        self.op = kw.pop('op')
        self.bound = kw.pop('bound')
        self.decorators = kw.pop('decorators', [])
        if kw.get('type', None):
            self.decorators.insert(0, Literal(kw['type']))
        if kw.get('type', None) == 'string':
            self.decorators.insert(0, Quote())
        if kw.get('type', None) == 'boolean':
            self.decorators.insert(0, FixBool())

        super(OpArgument, self).__init__(**kw)

    def argument(self, loader, indent):
        a = str(self.bound)
        for d in self.decorators:
            a = d(a)

        return a


class Indent(object):
    '''Help templates be depth agnostic.'''

    def __init__(self, depth=0):
        self.depth = depth

    def __add__(self, depth):
        return Indent(self.depth + depth)

    def __call__(self, depth):
        '''Render an indent at the current depth plus depth.'''
        return 4*' '*(depth + self.depth)


class ConfigEntry(NamedElement):
    '''Base interface for rendered items.'''

    def __init__(self, *a, **kw):
        '''Pop the configfile/class/subclass keywords.'''

        self.configfile = kw.pop('configfile')
        self.cls = kw.pop('class')
        self.subclass = kw.pop(self.cls)
        super(ConfigEntry, self).__init__(**kw)

    def factory(self, objs):
        ''' Optional factory interface for subclasses to add
        additional items to be rendered.'''

        pass

    def setup(self, objs):
        ''' Optional setup interface for subclasses, invoked
        after all factory methods have been run.'''

        pass


class Path(ConfigEntry):
    '''Path/metadata association.'''

    def __init__(self, *a, **kw):
        super(Path, self).__init__(**kw)

        if self.name['meta'].upper() != self.name['meta']:
            raise InvalidConfigError(
                self.configfile,
                'Metadata tag "{0}" must be upper case.'.format(
                    self.name['meta']))

    def factory(self, objs):
        '''Create path and metadata elements.'''

        args = {
            'class': 'pathname',
            'pathname': 'element',
            'name': self.name['path']
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'meta',
            'meta': 'element',
            'name': self.name['meta']
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        super(Path, self).factory(objs)

    def setup(self, objs):
        '''Resolve path and metadata names to indices.'''

        self.path = get_index(
            objs, 'pathname', self.name['path'])
        self.meta = get_index(
            objs, 'meta', self.name['meta'])

        super(Path, self).setup(objs)


class Property(ConfigEntry):
    '''Property/interface/metadata association.'''

    def __init__(self, *a, **kw):
        super(Property, self).__init__(**kw)

        if self.name['meta'].upper() != self.name['meta']:
            raise InvalidConfigError(
                self.configfile,
                'Metadata tag "{0}" must be upper case.'.format(
                    self.name['meta']))

    def factory(self, objs):
        '''Create interface, property name and metadata elements.'''

        args = {
            'class': 'interface',
            'interface': 'element',
            'name': self.name['interface']
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'propertyname',
            'propertyname': 'element',
            'name': self.name['property']
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'meta',
            'meta': 'element',
            'name': self.name['meta']
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        super(Property, self).factory(objs)

    def setup(self, objs):
        '''Resolve interface, property and metadata to indices.'''

        self.interface = get_index(
            objs, 'interface', self.name['interface'])
        self.prop = get_index(
            objs, 'propertyname', self.name['property'])
        self.meta = get_index(
            objs, 'meta', self.name['meta'])

        super(Property, self).setup(objs)


class Instance(ConfigEntry):
    '''Property/Path association.'''

    def __init__(self, *a, **kw):
        super(Instance, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve elements to indices.'''

        self.interface = get_index(
            objs, 'interface', self.name['property']['interface'])
        self.prop = get_index(
            objs, 'propertyname', self.name['property']['property'])
        self.propmeta = get_index(
            objs, 'meta', self.name['property']['meta'])
        self.path = get_index(
            objs, 'pathname', self.name['path']['path'])
        self.pathmeta = get_index(
            objs, 'meta', self.name['path']['meta'])

        super(Instance, self).setup(objs)

class PathInstance(ConfigEntry):
    '''Path association.'''

    def __init__(self, *a, **kw):
        super(PathInstance, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve elements to indices.'''
        self.path = self.name['path']['path']
        self.pathmeta = self.name['path']['meta']
        super(PathInstance, self).setup(objs)

class Group(ConfigEntry):
    '''Pop the members keyword for groups.'''

    def __init__(self, *a, **kw):
        self.members = kw.pop('members')
        super(Group, self).__init__(**kw)


class ImplicitGroup(Group):
    '''Provide a factory method for groups whose members are
    not explicitly declared in the config files.'''

    def __init__(self, *a, **kw):
        super(ImplicitGroup, self).__init__(**kw)

    def factory(self, objs):
        '''Create group members.'''

        factory = Everything.classmap(self.subclass, 'element')
        for m in self.members:
            args = {
                'class': self.subclass,
                self.subclass: 'element',
                'name': m
            }

            obj = factory(configfile=self.configfile, **args)
            add_unique(obj, objs)
            obj.factory(objs)

        super(ImplicitGroup, self).factory(objs)


class GroupOfPaths(ImplicitGroup):
    '''Path group config file directive.'''

    def __init__(self, *a, **kw):
        super(GroupOfPaths, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            path = get_index(
                objs, 'pathname', x['path'])
            meta = get_index(
                objs, 'meta', x['meta'])
            return (path, meta)

        self.members = map(
            map_member,
            self.members)

        super(GroupOfPaths, self).setup(objs)


class GroupOfProperties(ImplicitGroup):
    '''Property group config file directive.'''

    def __init__(self, *a, **kw):
        self.type = kw.pop('type')
        self.datatype = sdbusplus.property.Property(
            name=kw.get('name'),
            type=self.type).cppTypeName

        super(GroupOfProperties, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            iface = get_index(
                objs, 'interface', x['interface'])
            prop = get_index(
                objs, 'propertyname', x['property'])
            meta = get_index(
                objs, 'meta', x['meta'])

            return (iface, prop, meta)

        self.members = map(
            map_member,
            self.members)

        super(GroupOfProperties, self).setup(objs)


class GroupOfInstances(ImplicitGroup):
    '''A group of property instances.'''

    def __init__(self, *a, **kw):
        super(GroupOfInstances, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            path = get_index(objs, 'pathname', x['path']['path'])
            pathmeta = get_index(objs, 'meta', x['path']['meta'])
            interface = get_index(
                objs, 'interface', x['property']['interface'])
            prop = get_index(objs, 'propertyname', x['property']['property'])
            propmeta = get_index(objs, 'meta', x['property']['meta'])
            instance = get_index(objs, 'instance', x)

            return (path, pathmeta, interface, prop, propmeta, instance)

        self.members = map(
            map_member,
            self.members)

        super(GroupOfInstances, self).setup(objs)

class GroupOfPathInstances(ImplicitGroup):
    '''A group of path instances.'''

    def __init__(self, *a, **kw):
        super(GroupOfPathInstances, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            path = get_index(objs, 'pathname', x['path']['path'])
            pathmeta = get_index(objs, 'meta', x['path']['meta'])
            pathinstance = get_index(objs, 'pathinstance', x)
            return (path, pathmeta, pathinstance)

        self.members = map(
            map_member,
            self.members)

        super(GroupOfPathInstances, self).setup(objs)


class HasPropertyIndex(ConfigEntry):
    '''Handle config file directives that require an index to be
    constructed.'''

    def __init__(self, *a, **kw):
        self.paths = kw.pop('paths')
        self.properties = kw.pop('properties')
        super(HasPropertyIndex, self).__init__(**kw)

    def factory(self, objs):
        '''Create a group of instances for this index.'''

        members = []
        path_group = get_index(
            objs, 'pathgroup', self.paths, config=self.configfile)
        property_group = get_index(
            objs, 'propertygroup', self.properties, config=self.configfile)

        for path in objs['pathgroup'][path_group].members:
            for prop in objs['propertygroup'][property_group].members:
                member = {
                    'path': path,
                    'property': prop,
                }
                members.append(member)

        args = {
            'members': members,
            'class': 'instancegroup',
            'instancegroup': 'instance',
            'name': '{0} {1}'.format(self.paths, self.properties)
        }

        group = GroupOfInstances(configfile=self.configfile, **args)
        add_unique(group, objs, config=self.configfile)
        group.factory(objs)

        super(HasPropertyIndex, self).factory(objs)

    def setup(self, objs):
        '''Resolve path, property, and instance groups.'''

        self.instances = get_index(
            objs,
            'instancegroup',
            '{0} {1}'.format(self.paths, self.properties),
            config=self.configfile)
        self.paths = get_index(
            objs,
            'pathgroup',
            self.paths,
            config=self.configfile)
        self.properties = get_index(
            objs,
            'propertygroup',
            self.properties,
            config=self.configfile)
        self.datatype = objs['propertygroup'][self.properties].datatype
        self.type = objs['propertygroup'][self.properties].type

        super(HasPropertyIndex, self).setup(objs)

class HasPathIndex(ConfigEntry):
    '''Handle config file directives that require an index to be
    constructed.'''

    def __init__(self, *a, **kw):
        self.paths = kw.pop('paths')
        super(HasPathIndex, self).__init__(**kw)

    def factory(self, objs):
        '''Create a group of instances for this index.'''

        members = []
        path_group = get_index(
            objs, 'pathgroup', self.paths, config=self.configfile)

        for path in objs['pathgroup'][path_group].members:
            member = {
                'path': path,
            }
            members.append(member)

        args = {
            'members': members,
            'class': 'pathinstancegroup',
            'pathinstancegroup': 'pathinstance',
            'name': '{0}'.format(self.paths)
        }

        group = GroupOfPathInstances(configfile=self.configfile, **args)
        add_unique(group, objs, config=self.configfile)
        group.factory(objs)

        super(HasPathIndex, self).factory(objs)

    def setup(self, objs):
        '''Resolve path and instance groups.'''

        self.pathinstances = get_index(
            objs,
            'pathinstancegroup',
            '{0}'.format(self.paths),
            config=self.configfile)
        self.paths = get_index(
            objs,
            'pathgroup',
            self.paths,
            config=self.configfile)
        super(HasPathIndex, self).setup(objs)

class GroupOfFilters(ConfigEntry):
    '''Handle config file directives that require an index for filters.'''

    def __init__(self, *a, **kw):
        # Pop filters data for adding to the available filters array
        self.type = kw.pop('type')
        self.datatype = kw.pop('datatype', None)
        self.filters = kw.pop('filters', None)

        super(GroupOfFilters, self).__init__(**kw)

    def factory(self, objs):
        '''Modify filters to add the property value type and
        make them of operation argument type.'''
        if self.filters:
            # 'type' used within OpArgument to generate filter
            # argument values so add to each filter
            for f in self.filters:
                f['type'] = self.type
            self.filters = [OpArgument(**x) for x in self.filters]

        super(GroupOfFilters, self).factory(objs)

class PropertyWatch(HasPropertyIndex):
    '''Handle the property watch config file directive.'''

    def __init__(self, *a, **kw):
        # Pop optional filters for the properties being watched
        self.filters = kw.pop('filters', None)
        self.callback = kw.pop('callback', None)
        super(PropertyWatch, self).__init__(**kw)

    def factory(self, objs):
        '''Create any filters for this property watch.'''

        if self.filters:
            # Get the datatype(i.e. "int64_t") of the properties in this watch
            # (Made available after all `super` classes init'd)
            datatype = objs['propertygroup'][get_index(
                objs,
                'propertygroup',
                self.properties,
                config=self.configfile)].datatype
            # Get the type(i.e. "int64") of the properties in this watch
            # (Made available after all `super` classes init'd)
            type = objs['propertygroup'][get_index(
                objs,
                'propertygroup',
                self.properties,
                config=self.configfile)].type
            # Construct the data needed to make the filters for
            # this watch available.
            # *Note: 'class', 'subclass', 'name' are required for
            # storing the filter data(i.e. 'type', 'datatype', & 'filters')
            args = {
                'type': type,
                'datatype': datatype,
                'filters': self.filters,
                'class': 'filtersgroup',
                'filtersgroup': 'filters',
                'name': self.name,
            }
            # Init GroupOfFilters class with this watch's filters' arguments
            group = GroupOfFilters(configfile=self.configfile, **args)
            # Store this group of filters so it can be indexed later
            add_unique(group, objs, config=self.configfile)
            group.factory(objs)

        super(PropertyWatch, self).factory(objs)

    def setup(self, objs):
        '''Resolve optional filters and callback.'''

        if self.filters:
            # Watch has filters, provide array index to access them
            self.filters = get_index(
                objs,
                'filtersgroup',
                self.name,
                config=self.configfile)

        if self.callback:
            self.callback = get_index(
                objs,
                'callback',
                self.callback,
                config=self.configfile)

        super(PropertyWatch, self).setup(objs)

class PathWatch(HasPathIndex):
    '''Handle the path watch config file directive.'''

    def __init__(self, *a, **kw):
        self.pathcallback = kw.pop('pathcallback', None)
        super(PathWatch, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve optional callback.'''
        if self.pathcallback:
            self.pathcallback = get_index(
                objs,
                'pathcallback',
                self.pathcallback,
                config=self.configfile)
        super(PathWatch, self).setup(objs)

class Callback(HasPropertyIndex):
    '''Interface and common logic for callbacks.'''

    def __init__(self, *a, **kw):
        super(Callback, self).__init__(**kw)

class PathCallback(HasPathIndex):
    '''Interface and common logic for callbacks.'''

    def __init__(self, *a, **kw):
        super(PathCallback, self).__init__(**kw)

class ConditionCallback(ConfigEntry, Renderer):
    '''Handle the journal callback config file directive.'''

    def __init__(self, *a, **kw):
        self.condition = kw.pop('condition')
        self.instance = kw.pop('instance')
        self.defer = kw.pop('defer', None)
        super(ConditionCallback, self).__init__(**kw)

    def factory(self, objs):
        '''Create a graph instance for this callback.'''

        args = {
            'configfile': self.configfile,
            'members': [self.instance],
            'class': 'callbackgroup',
            'callbackgroup': 'callback',
            'name': [self.instance]
        }

        entry = CallbackGraphEntry(**args)
        add_unique(entry, objs, config=self.configfile)

        super(ConditionCallback, self).factory(objs)

    def setup(self, objs):
        '''Resolve condition and graph entry.'''

        self.graph = get_index(
            objs,
            'callbackgroup',
            [self.instance],
            config=self.configfile)

        self.condition = get_index(
            objs,
            'condition',
            self.name,
            config=self.configfile)

        super(ConditionCallback, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'conditional.mako.cpp',
            c=self,
            indent=indent)


class Condition(HasPropertyIndex):
    '''Interface and common logic for conditions.'''

    def __init__(self, *a, **kw):
        self.callback = kw.pop('callback')
        self.defer = kw.pop('defer', None)
        super(Condition, self).__init__(**kw)

    def factory(self, objs):
        '''Create a callback instance for this conditional.'''

        args = {
            'configfile': self.configfile,
            'condition': self.name,
            'class': 'callback',
            'callback': 'conditional',
            'instance': self.callback,
            'name': self.name,
            'defer': self.defer
        }

        callback = ConditionCallback(**args)
        add_unique(callback, objs, config=self.configfile)
        callback.factory(objs)

        super(Condition, self).factory(objs)


class CountCondition(Condition, Renderer):
    '''Handle the count condition config file directive.'''

    def __init__(self, *a, **kw):
        self.countop = kw.pop('countop')
        self.countbound = kw.pop('countbound')
        self.op = kw.pop('op')
        self.bound = kw.pop('bound')
        self.oneshot = TrivialArgument(
            type='boolean',
            value=kw.pop('oneshot', False))
        super(CountCondition, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve type.'''

        super(CountCondition, self).setup(objs)
        self.bound = TrivialArgument(
            type=self.type,
            value=self.bound)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'count.mako.cpp',
            c=self,
            indent=indent)


class MedianCondition(Condition, Renderer):
    '''Handle the median condition config file directive.'''

    def __init__(self, *a, **kw):
        self.op = kw.pop('op')
        self.bound = kw.pop('bound')
        self.oneshot = TrivialArgument(
            type='boolean',
            value=kw.pop('oneshot', False))
        super(MedianCondition, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve type.'''

        super(MedianCondition, self).setup(objs)
        self.bound = TrivialArgument(
            type=self.type,
            value=self.bound)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'median.mako.cpp',
            c=self,
            indent=indent)


class Journal(Callback, Renderer):
    '''Handle the journal callback config file directive.'''

    def __init__(self, *a, **kw):
        self.severity = kw.pop('severity')
        self.message = kw.pop('message')
        super(Journal, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'journal.mako.cpp',
            c=self,
            indent=indent)


class Elog(Callback, Renderer):
    '''Handle the elog callback config file directive.'''

    def __init__(self, *a, **kw):
        self.error = kw.pop('error')
        self.metadata = [Metadata(**x) for x in kw.pop('metadata', {})]
        super(Elog, self).__init__(**kw)

    def construct(self, loader, indent):
        with open('errors.hpp', 'a') as fd:
            fd.write(
                self.render(
                    loader,
                    'errors.mako.hpp',
                    c=self))
        return self.render(
            loader,
            'elog.mako.cpp',
            c=self,
            indent=indent)

class Event(Callback, Renderer):
    '''Handle the event callback config file directive.'''

    def __init__(self, *a, **kw):
        self.eventName = kw.pop('eventName')
        self.eventMessage = kw.pop('eventMessage')
        super(Event, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'event.mako.cpp',
            c=self,
            indent=indent)

class EventPath(PathCallback, Renderer):
    '''Handle the event path callback config file directive.'''

    def __init__(self, *a, **kw):
        self.eventType = kw.pop('eventType')
        super(EventPath, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'eventpath.mako.cpp',
            c=self,
            indent=indent)

class ElogWithMetadata(Callback, Renderer):
    '''Handle the elog_with_metadata callback config file directive.'''

    def __init__(self, *a, **kw):
        self.error = kw.pop('error')
        self.metadata = kw.pop('metadata')
        super(ElogWithMetadata, self).__init__(**kw)

    def construct(self, loader, indent):
        with open('errors.hpp', 'a') as fd:
            fd.write(
                self.render(
                    loader,
                    'errors.mako.hpp',
                    c=self))
        return self.render(
            loader,
            'elog_with_metadata.mako.cpp',
            c=self,
            indent=indent)


class ResolveCallout(Callback, Renderer):
    '''Handle the 'resolve callout' callback config file directive.'''

    def __init__(self, *a, **kw):
        self.callout = kw.pop('callout')
        super(ResolveCallout, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'resolve_errors.mako.cpp',
            c=self,
            indent=indent)


class Method(ConfigEntry, Renderer):
    '''Handle the method callback config file directive.'''

    def __init__(self, *a, **kw):
        self.service = kw.pop('service')
        self.path = kw.pop('path')
        self.interface = kw.pop('interface')
        self.method = kw.pop('method')
        self.args = [TrivialArgument(**x) for x in kw.pop('args', {})]
        super(Method, self).__init__(**kw)

    def factory(self, objs):
        args = {
            'class': 'interface',
            'interface': 'element',
            'name': self.service
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'pathname',
            'pathname': 'element',
            'name': self.path
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'interface',
            'interface': 'element',
            'name': self.interface
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        args = {
            'class': 'propertyname',
            'propertyname': 'element',
            'name': self.method
        }
        add_unique(ConfigEntry(
            configfile=self.configfile, **args), objs)

        super(Method, self).factory(objs)

    def setup(self, objs):
        '''Resolve elements.'''

        self.service = get_index(
            objs,
            'interface',
            self.service)

        self.path = get_index(
            objs,
            'pathname',
            self.path)

        self.interface = get_index(
            objs,
            'interface',
            self.interface)

        self.method = get_index(
            objs,
            'propertyname',
            self.method)

        super(Method, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'method.mako.cpp',
            c=self,
            indent=indent)


class CallbackGraphEntry(Group):
    '''An entry in a traversal list for groups of callbacks.'''

    def __init__(self, *a, **kw):
        super(CallbackGraphEntry, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            return get_index(
                objs, 'callback', x, config=self.configfile)

        self.members = map(
            map_member,
            self.members)

        super(CallbackGraphEntry, self).setup(objs)

class PathCallbackGraphEntry(Group):
    '''An entry in a traversal list for groups of callbacks.'''

    def __init__(self, *a, **kw):
        super(PathCallbackGraphEntry, self).__init__(**kw)

    def setup(self, objs):
        '''Resolve group members.'''

        def map_member(x):
            return get_index(
                objs, 'pathcallback', x, config=self.configfile)

        self.members = map(
            map_member,
            self.members)

        super(PathCallbackGraphEntry, self).setup(objs)

class GroupOfCallbacks(ConfigEntry, Renderer):
    '''Handle the callback group config file directive.'''

    def __init__(self, *a, **kw):
        self.members = kw.pop('members')
        super(GroupOfCallbacks, self).__init__(**kw)

    def factory(self, objs):
        '''Create a graph instance for this group of callbacks.'''

        args = {
            'configfile': self.configfile,
            'members': self.members,
            'class': 'callbackgroup',
            'callbackgroup': 'callback',
            'name': self.members
        }

        entry = CallbackGraphEntry(**args)
        add_unique(entry, objs, config=self.configfile)

        super(GroupOfCallbacks, self).factory(objs)

    def setup(self, objs):
        '''Resolve graph entry.'''

        self.graph = get_index(
            objs, 'callbackgroup', self.members, config=self.configfile)

        super(GroupOfCallbacks, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'callbackgroup.mako.cpp',
            c=self,
            indent=indent)

class GroupOfPathCallbacks(ConfigEntry, Renderer):
    '''Handle the callback group config file directive.'''

    def __init__(self, *a, **kw):
        self.members = kw.pop('members')
        super(GroupOfPathCallbacks, self).__init__(**kw)

    def factory(self, objs):
        '''Create a graph instance for this group of callbacks.'''

        args = {
            'configfile': self.configfile,
            'members': self.members,
            'class': 'pathcallbackgroup',
            'pathcallbackgroup': 'pathcallback',
            'name': self.members
        }

        entry = PathCallbackGraphEntry(**args)
        add_unique(entry, objs, config=self.configfile)
        super(GroupOfPathCallbacks, self).factory(objs)

    def setup(self, objs):
        '''Resolve graph entry.'''

        self.graph = get_index(
            objs, 'callbackpathgroup', self.members, config=self.configfile)

        super(GroupOfPathCallbacks, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'callbackpathgroup.mako.cpp',
            c=self,
            indent=indent)

class Everything(Renderer):
    '''Parse/render entry point.'''

    @staticmethod
    def classmap(cls, sub=None):
        '''Map render item class and subclass entries to the appropriate
        handler methods.'''
        class_map = {
            'path': {
                'element': Path,
            },
            'pathgroup': {
                'path': GroupOfPaths,
            },
            'propertygroup': {
                'property': GroupOfProperties,
            },
            'property': {
                'element': Property,
            },
            'watch': {
                'property': PropertyWatch,
            },
            'pathwatch': {
                'path': PathWatch,
            },
            'instance': {
                'element': Instance,
            },
            'pathinstance': {
                'element': PathInstance,
            },
            'callback': {
                'journal': Journal,
                'elog': Elog,
                'elog_with_metadata': ElogWithMetadata,
                'event': Event,
                'group': GroupOfCallbacks,
                'method': Method,
                'resolve callout': ResolveCallout,
            },
            'pathcallback': {
                'eventpath': EventPath,
                'grouppath': GroupOfPathCallbacks,
            },
            'condition': {
                'count': CountCondition,
                'median': MedianCondition,
            },
        }

        if cls not in class_map:
            raise NotImplementedError('Unknown class: "{0}"'.format(cls))
        if sub not in class_map[cls]:
            raise NotImplementedError('Unknown {0} type: "{1}"'.format(
                cls, sub))

        return class_map[cls][sub]

    @staticmethod
    def load_one_yaml(path, fd, objs):
        '''Parse a single YAML file.  Parsing occurs in three phases.
        In the first phase a factory method associated with each
        configuration file directive is invoked.  These factory
        methods generate more factory methods.  In the second
        phase the factory methods created in the first phase
        are invoked.  In the last phase a callback is invoked on
        each object created in phase two.  Typically the callback
        resolves references to other configuration file directives.'''

        factory_objs = {}
        for x in yaml.safe_load(fd.read()) or {}:

            # Create factory object for this config file directive.
            cls = x['class']
            sub = x.get(cls)
            if cls == 'group':
                cls = '{0}group'.format(sub)

            factory = Everything.classmap(cls, sub)
            obj = factory(configfile=path, **x)

            # For a given class of directive, validate the file
            # doesn't have any duplicate names (duplicates are
            # ok across config files).
            if exists(factory_objs, obj.cls, obj.name, config=path):
                raise NotUniqueError(path, cls, obj.name)

            factory_objs.setdefault(cls, []).append(obj)
            objs.setdefault(cls, []).append(obj)

        for cls, items in factory_objs.items():
            for obj in items:
                # Add objects for template consumption.
                obj.factory(objs)

    @staticmethod
    def load(args):
        '''Aggregate all the YAML in the input directory
        into a single aggregate.'''

        objs = {}
        yaml_files = filter(
            lambda x: x.endswith('.yaml'),
            os.listdir(args.inputdir))

        for x in sorted(yaml_files):
            path = os.path.join(args.inputdir, x)
            with open(path, 'r') as fd:
                Everything.load_one_yaml(path, fd, objs)

        # Configuration file directives reference each other via
        # the name attribute; however, when rendered the reference
        # is just an array index.
        #
        # At this point all objects have been created but references
        # have not been resolved to array indices.  Instruct objects
        # to do that now.
        for cls, items in objs.items():
            for obj in items:
                obj.setup(objs)

        return Everything(**objs)

    def __init__(self, *a, **kw):
        self.pathmeta = kw.pop('path', [])
        self.paths = kw.pop('pathname', [])
        self.meta = kw.pop('meta', [])
        self.pathgroups = kw.pop('pathgroup', [])
        self.interfaces = kw.pop('interface', [])
        self.properties = kw.pop('property', [])
        self.propertynames = kw.pop('propertyname', [])
        self.propertygroups = kw.pop('propertygroup', [])
        self.instances = kw.pop('instance', [])
        self.pathinstances = kw.pop('pathinstance', [])
        self.instancegroups = kw.pop('instancegroup', [])
        self.pathinstancegroups = kw.pop('pathinstancegroup', [])
        self.watches = kw.pop('watch', [])
        self.pathwatches = kw.pop('pathwatch', [])
        self.callbacks = kw.pop('callback', [])
        self.pathcallbacks = kw.pop('pathcallback', [])
        self.callbackgroups = kw.pop('callbackgroup', [])
        self.pathcallbackgroups = kw.pop('pathcallbackgroup', [])
        self.conditions = kw.pop('condition', [])
        self.filters = kw.pop('filtersgroup', [])

        super(Everything, self).__init__(**kw)

    def generate_cpp(self, loader):
        '''Render the template with the provided data.'''
        # errors.hpp is used by generated.hpp to included any error.hpp files
        open('errors.hpp', 'w+')

        with open(args.output, 'w') as fd:
            fd.write(
                self.render(
                    loader,
                    args.template,
                    meta=self.meta,
                    properties=self.properties,
                    propertynames=self.propertynames,
                    interfaces=self.interfaces,
                    paths=self.paths,
                    pathmeta=self.pathmeta,
                    pathgroups=self.pathgroups,
                    propertygroups=self.propertygroups,
                    instances=self.instances,
                    pathinstances=self.pathinstances,
                    watches=self.watches,
                    pathwatches=self.pathwatches,
                    instancegroups=self.instancegroups,
                    pathinstancegroups=self.pathinstancegroups,
                    callbacks=self.callbacks,
                    pathcallbacks=self.pathcallbacks,
                    callbackgroups=self.callbackgroups,
                    pathcallbackgroups=self.pathcallbackgroups,
                    conditions=self.conditions,
                    filters=self.filters,
                    indent=Indent()))

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    valid_commands = {
        'generate-cpp': 'generate_cpp',
    }

    parser = ArgumentParser(
        description='Phosphor DBus Monitor (PDM) YAML '
        'scanner and code generator.')

    parser.add_argument(
        "-o", "--out", dest="output",
        default='generated.cpp',
        help="Generated output file name and path.")
    parser.add_argument(
        '-t', '--template', dest='template',
        default='generated.mako.hpp',
        help='The top level template to render.')
    parser.add_argument(
        '-p', '--template-path', dest='template_search',
        default=script_dir,
        help='The space delimited mako template search path.')
    parser.add_argument(
        '-d', '--dir', dest='inputdir',
        default=os.path.join(script_dir, 'example'),
        help='Location of files to process.')
    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=valid_commands.keys(),
        help='%s.' % " | ".join(valid_commands.keys()))

    args = parser.parse_args()

    if sys.version_info < (3, 0):
        lookup = mako.lookup.TemplateLookup(
            directories=args.template_search.split(),
            disable_unicode=True)
    else:
        lookup = mako.lookup.TemplateLookup(
            directories=args.template_search.split())
    try:
        function = getattr(
            Everything.load(args),
            valid_commands[args.command])
        function(lookup)
    except InvalidConfigError as e:
        sys.stdout.write('{0}: {1}\n\n'.format(e.config, e.msg))
        raise
