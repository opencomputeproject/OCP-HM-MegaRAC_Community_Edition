#!/usr/bin/env python3

'''Phosphor Inventory Manager YAML parser and code generator.

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

import sys
import os
import argparse
import subprocess
import yaml
import mako.lookup
import sdbusplus.property
from sdbusplus.namedelement import NamedElement
from sdbusplus.renderer import Renderer


# Global busname for use within classes where necessary
busname = "xyz.openbmc_project.Inventory.Manager"


def cppTypeName(yaml_type):
    ''' Convert yaml types to cpp types.'''
    return sdbusplus.property.Property(type=yaml_type).cppTypeName


class InterfaceComposite(object):
    '''Compose interface properties.'''

    def __init__(self, dict):
        self.dict = dict

    def interfaces(self):
        return list(self.dict.keys())

    def names(self, interface):
        names = []
        if self.dict[interface]:
            names = [NamedElement(name=x["name"]) for x in self.dict[interface]]
        return names


class Interface(list):
    '''Provide various interface transformations.'''

    def __init__(self, iface):
        super(Interface, self).__init__(iface.split('.'))

    def namespace(self):
        '''Represent as an sdbusplus namespace.'''
        return '::'.join(['sdbusplus'] + self[:-1] + ['server', self[-1]])

    def header(self):
        '''Represent as an sdbusplus server binding header.'''
        return os.sep.join(self + ['server.hpp'])

    def __str__(self):
        return '.'.join(self)


class Indent(object):
    '''Help templates be depth agnostic.'''

    def __init__(self, depth=0):
        self.depth = depth

    def __add__(self, depth):
        return Indent(self.depth + depth)

    def __call__(self, depth):
        '''Render an indent at the current depth plus depth.'''
        return 4*' '*(depth + self.depth)


class Template(NamedElement):
    '''Associate a template name with its namespace.'''

    def __init__(self, **kw):
        self.namespace = kw.pop('namespace', [])
        super(Template, self).__init__(**kw)

    def qualified(self):
        return '::'.join(self.namespace + [self.name])


class FixBool(object):
    '''Un-capitalize booleans.'''

    def __call__(self, arg):
        return '{0}'.format(arg.lower())


class Quote(object):
    '''Decorate an argument by quoting it.'''

    def __call__(self, arg):
        return '"{0}"'.format(arg)


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
        'int8',
        'int16',
        'int32',
        'int64',
        'uint8',
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

        if self.type == 'string':
            return '{0}s'.format(arg)

        return arg


class Argument(NamedElement, Renderer):
    '''Define argument type inteface.'''

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


class InitializerList(Argument):
    '''Initializer list arguments.'''

    def __init__(self, **kw):
        self.values = kw.pop('values')
        super(InitializerList, self).__init__(**kw)

    def argument(self, loader, indent):
        return self.render(
            loader,
            'argument.mako.cpp',
            arg=self,
            indent=indent)


class DbusSignature(Argument):
    '''DBus signature arguments.'''

    def __init__(self, **kw):
        self.sig = {x: y for x, y in kw.items()}
        kw.clear()
        super(DbusSignature, self).__init__(**kw)

    def argument(self, loader, indent):
        return self.render(
            loader,
            'signature.mako.cpp',
            signature=self,
            indent=indent)


class MethodCall(Argument):
    '''Render syntatically correct c++ method calls.'''

    def __init__(self, **kw):
        self.namespace = kw.pop('namespace', [])
        self.templates = kw.pop('templates', [])
        self.args = kw.pop('args', [])
        super(MethodCall, self).__init__(**kw)

    def call(self, loader, indent):
        return self.render(
            loader,
            'method.mako.cpp',
            method=self,
            indent=indent)

    def argument(self, loader, indent):
        return self.call(loader, indent)


class Vector(MethodCall):
    '''Convenience type for vectors.'''

    def __init__(self, **kw):
        kw['name'] = 'vector'
        kw['namespace'] = ['std']
        kw['args'] = [InitializerList(values=kw.pop('args'))]
        super(Vector, self).__init__(**kw)


class Filter(MethodCall):
    '''Convenience type for filters'''

    def __init__(self, **kw):
        kw['name'] = 'make_filter'
        super(Filter, self).__init__(**kw)


class Action(MethodCall):
    '''Convenience type for actions'''

    def __init__(self, **kw):
        kw['name'] = 'make_action'
        super(Action, self).__init__(**kw)


class PathCondition(MethodCall):
    '''Convenience type for path conditions'''

    def __init__(self, **kw):
        kw['name'] = 'make_path_condition'
        super(PathCondition, self).__init__(**kw)


class GetProperty(MethodCall):
    '''Convenience type for getting inventory properties'''

    def __init__(self, **kw):
        kw['name'] = 'make_get_property'
        super(GetProperty, self).__init__(**kw)


class CreateObjects(MethodCall):
    '''Assemble a createObjects functor.'''

    def __init__(self, **kw):
        objs = []

        for path, interfaces in kw.pop('objs').items():
            key_o = TrivialArgument(
                value=path,
                type='string',
                decorators=[Literal('string')])
            value_i = []

            for interface, properties, in interfaces.items():
                key_i = TrivialArgument(value=interface, type='string')
                value_p = []
                if properties:
                    for prop, value in properties.items():
                        key_p = TrivialArgument(value=prop, type='string')
                        value_v = TrivialArgument(
                            decorators=[Literal(value.get('type', None))],
                            **value)
                        value_p.append(InitializerList(values=[key_p, value_v]))

                value_p = InitializerList(values=value_p)
                value_i.append(InitializerList(values=[key_i, value_p]))

            value_i = InitializerList(values=value_i)
            objs.append(InitializerList(values=[key_o, value_i]))

        kw['args'] = [InitializerList(values=objs)]
        kw['namespace'] = ['functor']
        super(CreateObjects, self).__init__(**kw)


class DestroyObjects(MethodCall):
    '''Assemble a destroyObject functor.'''

    def __init__(self, **kw):
        values = [{'value': x, 'type': 'string'} for x in kw.pop('paths')]
        conditions = [
            Event.functor_map[
                x['name']](**x) for x in kw.pop('conditions', [])]
        conditions = [PathCondition(args=[x]) for x in conditions]
        args = [InitializerList(
            values=[TrivialArgument(**x) for x in values])]
        args.append(InitializerList(values=conditions))
        kw['args'] = args
        kw['namespace'] = ['functor']
        super(DestroyObjects, self).__init__(**kw)


class SetProperty(MethodCall):
    '''Assemble a setProperty functor.'''

    def __init__(self, **kw):
        args = []

        value = kw.pop('value')
        prop = kw.pop('property')
        iface = kw.pop('interface')
        iface = Interface(iface)
        namespace = iface.namespace().split('::')[:-1]
        name = iface[-1]
        t = Template(namespace=namespace, name=iface[-1])

        member = '&%s' % '::'.join(
            namespace + [name, NamedElement(name=prop).camelCase])
        member_type = cppTypeName(value['type'])
        member_cast = '{0} ({1}::*)({0})'.format(member_type, t.qualified())

        paths = [{'value': x, 'type': 'string'} for x in kw.pop('paths')]
        args.append(InitializerList(
            values=[TrivialArgument(**x) for x in paths]))

        conditions = [
            Event.functor_map[
                x['name']](**x) for x in kw.pop('conditions', [])]
        conditions = [PathCondition(args=[x]) for x in conditions]

        args.append(InitializerList(values=conditions))
        args.append(TrivialArgument(value=str(iface), type='string'))
        args.append(TrivialArgument(
            value=member, decorators=[Cast('static', member_cast)]))
        args.append(TrivialArgument(**value))

        kw['templates'] = [Template(name=name, namespace=namespace)]
        kw['args'] = args
        kw['namespace'] = ['functor']
        super(SetProperty, self).__init__(**kw)


class PropertyChanged(MethodCall):
    '''Assemble a propertyChanged functor.'''

    def __init__(self, **kw):
        args = []
        args.append(TrivialArgument(value=kw.pop('interface'), type='string'))
        args.append(TrivialArgument(value=kw.pop('property'), type='string'))
        args.append(TrivialArgument(
            decorators=[
                Literal(kw['value'].get('type', None))], **kw.pop('value')))
        kw['args'] = args
        kw['namespace'] = ['functor']
        super(PropertyChanged, self).__init__(**kw)


class PropertyIs(MethodCall):
    '''Assemble a propertyIs functor.'''

    def __init__(self, **kw):
        args = []
        path = kw.pop('path', None)
        if not path:
            path = TrivialArgument(value='nullptr')
        else:
            path = TrivialArgument(value=path, type='string')

        args.append(path)
        iface = TrivialArgument(value=kw.pop('interface'), type='string')
        args.append(iface)
        prop = TrivialArgument(value=kw.pop('property'), type='string')
        args.append(prop)
        args.append(TrivialArgument(
            decorators=[
                Literal(kw['value'].get('type', None))], **kw.pop('value')))

        service = kw.pop('service', None)
        if service:
            args.append(TrivialArgument(value=service, type='string'))

        dbusMember = kw.pop('dbusMember', None)
        if dbusMember:
            # Inventory manager's service name is required
            if not service or service != busname:
                args.append(TrivialArgument(value=busname, type='string'))

            gpArgs = []
            gpArgs.append(path)
            gpArgs.append(iface)
            # Prepend '&' and append 'getPropertyByName' function on dbusMember
            gpArgs.append(TrivialArgument(
                value='&'+dbusMember+'::getPropertyByName'))
            gpArgs.append(prop)
            fArg = MethodCall(
                name='getProperty',
                namespace=['functor'],
                templates=[Template(
                    name=dbusMember,
                    namespace=[])],
                args=gpArgs)

            # Append getProperty functor
            args.append(GetProperty(
                templates=[Template(
                    name=dbusMember+'::PropertiesVariant',
                    namespace=[])],
                    args=[fArg]))

        kw['args'] = args
        kw['namespace'] = ['functor']
        super(PropertyIs, self).__init__(**kw)


class Event(MethodCall):
    '''Assemble an inventory manager event.'''

    functor_map = {
        'destroyObjects': DestroyObjects,
        'createObjects': CreateObjects,
        'propertyChangedTo': PropertyChanged,
        'propertyIs': PropertyIs,
        'setProperty': SetProperty,
    }

    def __init__(self, **kw):
        self.summary = kw.pop('name')

        filters = [
            self.functor_map[x['name']](**x) for x in kw.pop('filters', [])]
        filters = [Filter(args=[x]) for x in filters]
        filters = Vector(
            templates=[Template(name='Filter', namespace=[])],
            args=filters)

        event = MethodCall(
            name='make_shared',
            namespace=['std'],
            templates=[Template(
                name=kw.pop('event'),
                namespace=kw.pop('event_namespace', []))],
            args=kw.pop('event_args', []) + [filters])

        events = Vector(
            templates=[Template(name='EventBasePtr', namespace=[])],
            args=[event])

        action_type = Template(name='Action', namespace=[])
        action_args = [
            self.functor_map[x['name']](**x) for x in kw.pop('actions', [])]
        action_args = [Action(args=[x]) for x in action_args]
        actions = Vector(
            templates=[action_type],
            args=action_args)

        kw['name'] = 'make_tuple'
        kw['namespace'] = ['std']
        kw['args'] = [events, actions]
        super(Event, self).__init__(**kw)


class MatchEvent(Event):
    '''Associate one or more dbus signal match signatures with
    a filter.'''

    def __init__(self, **kw):
        kw['event'] = 'DbusSignal'
        kw['event_namespace'] = []
        kw['event_args'] = [
            DbusSignature(**x) for x in kw.pop('signatures', [])]

        super(MatchEvent, self).__init__(**kw)


class StartupEvent(Event):
    '''Assemble a startup event.'''

    def __init__(self, **kw):
        kw['event'] = 'StartupEvent'
        kw['event_namespace'] = []
        super(StartupEvent, self).__init__(**kw)


class Everything(Renderer):
    '''Parse/render entry point.'''

    class_map = {
        'match': MatchEvent,
        'startup': StartupEvent,
    }

    @staticmethod
    def load(args):
        # Aggregate all the event YAML in the events.d directory
        # into a single list of events.

        events = []
        events_dir = os.path.join(args.inputdir, 'events.d')

        if os.path.exists(events_dir):
            yaml_files = [x for x in os.listdir(events_dir) if
                    x.endswith('.yaml')]

            for x in yaml_files:
                with open(os.path.join(events_dir, x), 'r') as fd:
                    for e in yaml.safe_load(fd.read()).get('events', {}):
                        events.append(e)

        interfaces, interface_composite = Everything.get_interfaces(
            args.ifacesdir)
        extra_interfaces, extra_interface_composite = \
            Everything.get_interfaces(
                os.path.join(args.inputdir, 'extra_interfaces.d'))
        interface_composite.update(extra_interface_composite)
        interface_composite = InterfaceComposite(interface_composite)
        # Update busname if configured differenly than the default
        busname = args.busname

        return Everything(
            *events,
            interfaces=interfaces + extra_interfaces,
            interface_composite=interface_composite)

    @staticmethod
    def get_interfaces(targetdir):
        '''Scan the interfaces directory for interfaces that PIM can create.'''

        yaml_files = []
        interfaces = []
        interface_composite = {}

        if targetdir and os.path.exists(targetdir):
            for directory, _, files in os.walk(targetdir):
                if not files:
                    continue

                yaml_files += [os.path.relpath(
                        os.path.join(directory, f),
                        targetdir) for f in [f for f in files if
                                f.endswith('.interface.yaml')]]

        for y in yaml_files:
            # parse only phosphor dbus related interface files
            if not y.startswith('xyz'):
                continue
            with open(os.path.join(targetdir, y)) as fd:
                i = y.replace('.interface.yaml', '').replace(os.sep, '.')

                # PIM can't create interfaces with methods.
                parsed = yaml.safe_load(fd.read())
                if parsed.get('methods', None):
                    continue
                # Cereal can't understand the type sdbusplus::object_path. This
                # type is a wrapper around std::string. Ignore interfaces having
                # a property of this type for now. The only interface that has a
                # property of this type now is xyz.openbmc_project.Association,
                # which is an unused interface. No inventory objects implement
                # this interface.
                # TODO via openbmc/openbmc#2123 : figure out how to make Cereal
                # understand sdbusplus::object_path.
                properties = parsed.get('properties', None)
                if properties:
                    if any('path' in p['type'] for p in properties):
                        continue
                interface_composite[i] = properties
                interfaces.append(i)

        return interfaces, interface_composite

    def __init__(self, *a, **kw):
        self.interfaces = \
            [Interface(x) for x in kw.pop('interfaces', [])]
        self.interface_composite = \
            kw.pop('interface_composite', {})
        self.events = [
            self.class_map[x['type']](**x) for x in a]
        super(Everything, self).__init__(**kw)

    def generate_cpp(self, loader):
        '''Render the template with the provided events and interfaces.'''
        with open(os.path.join(
                args.outputdir,
                'generated.cpp'), 'w') as fd:
            fd.write(
                self.render(
                    loader,
                    'generated.mako.cpp',
                    events=self.events,
                    interfaces=self.interfaces,
                    indent=Indent()))

    def generate_serialization(self, loader):
        with open(os.path.join(
                args.outputdir,
                'gen_serialization.hpp'), 'w') as fd:
            fd.write(
                self.render(
                    loader,
                    'gen_serialization.mako.hpp',
                    interfaces=self.interfaces,
                    interface_composite=self.interface_composite))


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    valid_commands = {
        'generate-cpp': 'generate_cpp',
        'generate-serialization': 'generate_serialization',
    }

    parser = argparse.ArgumentParser(
        description='Phosphor Inventory Manager (PIM) YAML '
        'scanner and code generator.')
    parser.add_argument(
        '-o', '--output-dir', dest='outputdir',
        default='.', help='Output directory.')
    parser.add_argument(
        '-i', '--interfaces-dir', dest='ifacesdir',
        help='Location of interfaces to be supported.')
    parser.add_argument(
        '-d', '--dir', dest='inputdir',
        default=os.path.join(script_dir, 'example'),
        help='Location of files to process.')
    parser.add_argument(
        '-b', '--bus-name', dest='busname',
        default='xyz.openbmc_project.Inventory.Manager',
        help='Inventory manager busname.')
    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=list(valid_commands.keys()),
        help='%s.' % " | ".join(list(valid_commands.keys())))

    args = parser.parse_args()

    if sys.version_info < (3, 0):
        lookup = mako.lookup.TemplateLookup(
            directories=[script_dir],
            disable_unicode=True)
    else:
        lookup = mako.lookup.TemplateLookup(
            directories=[script_dir])

    function = getattr(
        Everything.load(args),
        valid_commands[args.command])
    function(lookup)


# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
