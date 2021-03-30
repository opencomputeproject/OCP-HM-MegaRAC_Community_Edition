#!/usr/bin/env python3

'''
Phosphor Fan Presence (PFP) YAML parser and code generator.

Parse the provided PFP configuration file and generate C++ code.

The parser workflow is broken down as follows:
  1 - Import the YAML configuration file as native python type(s)
        instance(s).
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
from argparse import ArgumentParser
import mako.lookup
from sdbusplus.renderer import Renderer
from sdbusplus.namedelement import NamedElement


class InvalidConfigError(BaseException):
    '''General purpose config file parsing error.'''

    def __init__(self, path, msg):
        '''Display configuration file with the syntax
        error and the error message.'''

        self.config = path
        self.msg = msg


class NotUniqueError(InvalidConfigError):
    '''Within a config file names must be unique.
    Display the duplicate item.'''

    def __init__(self, path, cls, *names):
        fmt = 'Duplicate {0}: "{1}"'
        super(NotUniqueError, self).__init__(
            path, fmt.format(cls, ' '.join(names)))


def get_index(objs, cls, name):
    '''Items are usually rendered as C++ arrays and as
    such are stored in python lists.  Given an item name
    its class, find the item index.'''

    for i, x in enumerate(objs.get(cls, [])):
        if x.name != name:
            continue

        return i
    raise InvalidConfigError('Could not find name: "{0}"'.format(name))


def exists(objs, cls, name):
    '''Check to see if an item already exists in a list given
    the item name.'''

    try:
        get_index(objs, cls, name)
    except:
        return False

    return True


def add_unique(obj, *a, **kw):
    '''Add an item to one or more lists unless already present.'''

    for container in a:
        if not exists(container, obj.cls, obj.name):
            container.setdefault(obj.cls, []).append(obj)


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
        '''Pop the class keyword.'''

        self.cls = kw.pop('class')
        super(ConfigEntry, self).__init__(**kw)

    def factory(self, objs):
        ''' Optional factory interface for subclasses to add
        additional items to be rendered.'''

        pass

    def setup(self, objs):
        ''' Optional setup interface for subclasses, invoked
        after all factory methods have been run.'''

        pass


class Sensor(ConfigEntry):
    '''Convenience type for config file method:type handlers.'''

    def __init__(self, *a, **kw):
        kw['class'] = 'sensor'
        kw.pop('type')
        self.policy = kw.pop('policy')
        super(Sensor, self).__init__(**kw)

    def setup(self, objs):
        '''All sensors have an associated policy.  Get the policy index.'''

        self.policy = get_index(objs, 'policy', self.policy)


class Gpio(Sensor, Renderer):
    '''Handler for method:type:gpio.'''

    def __init__(self, *a, **kw):
        self.key = kw.pop('key')
        self.physpath = kw.pop('physpath')
        self.devpath = kw.pop('devpath')
        kw['name'] = 'gpio-{}'.format(self.key)
        super(Gpio, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'gpio.mako.hpp',
            g=self,
            indent=indent)

    def setup(self, objs):
        super(Gpio, self).setup(objs)


class Tach(Sensor, Renderer):
    '''Handler for method:type:tach.'''

    def __init__(self, *a, **kw):
        self.sensors = kw.pop('sensors')
        kw['name'] = 'tach-{}'.format('-'.join(self.sensors))
        super(Tach, self).__init__(**kw)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'tach.mako.hpp',
            t=self,
            indent=indent)

    def setup(self, objs):
        super(Tach, self).setup(objs)


class Rpolicy(ConfigEntry):
    '''Convenience type for config file rpolicy:type handlers.'''

    def __init__(self, *a, **kw):
        kw.pop('type', None)
        self.fan = kw.pop('fan')
        self.sensors = []
        kw['class'] = 'policy'
        super(Rpolicy, self).__init__(**kw)

    def setup(self, objs):
        '''All policies have an associated fan and methods.
        Resolve the indices.'''

        sensors = []
        for s in self.sensors:
            sensors.append(get_index(objs, 'sensor', s))

        self.sensors = sensors
        self.fan = get_index(objs, 'fan', self.fan)


class AnyOf(Rpolicy, Renderer):
    '''Default policy handler (policy:type:anyof).'''

    def __init__(self, *a, **kw):
        kw['name'] = 'anyof-{}'.format(kw['fan'])
        super(AnyOf, self).__init__(**kw)

    def setup(self, objs):
        super(AnyOf, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'anyof.mako.hpp',
            f=self,
            indent=indent)


class Fallback(Rpolicy, Renderer):
    '''Fallback policy handler (policy:type:fallback).'''

    def __init__(self, *a, **kw):
        kw['name'] = 'fallback-{}'.format(kw['fan'])
        super(Fallback, self).__init__(**kw)

    def setup(self, objs):
        super(Fallback, self).setup(objs)

    def construct(self, loader, indent):
        return self.render(
            loader,
            'fallback.mako.hpp',
            f=self,
            indent=indent)


class Fan(ConfigEntry):
    '''Fan directive handler.  Fans entries consist of an inventory path,
    optional redundancy policy and associated sensors.'''

    def __init__(self, *a, **kw):
        self.path = kw.pop('path')
        self.methods = kw.pop('methods')
        self.rpolicy = kw.pop('rpolicy', None)
        super(Fan, self).__init__(**kw)

    def factory(self, objs):
        ''' Create rpolicy and sensor(s) objects.'''

        if self.rpolicy:
            self.rpolicy['fan'] = self.name
            factory = Everything.classmap(self.rpolicy['type'])
            rpolicy = factory(**self.rpolicy)
        else:
            rpolicy = AnyOf(fan=self.name)

        for m in self.methods:
            m['policy'] = rpolicy.name
            factory = Everything.classmap(m['type'])
            sensor = factory(**m)
            rpolicy.sensors.append(sensor.name)
            add_unique(sensor, objs)

        add_unique(rpolicy, objs)
        super(Fan, self).factory(objs)


class Everything(Renderer):
    '''Parse/render entry point.'''

    @staticmethod
    def classmap(cls):
        '''Map render item class entries to the appropriate
        handler methods.'''

        class_map = {
            'anyof': AnyOf,
            'fan': Fan,
            'fallback': Fallback,
            'gpio': Gpio,
            'tach': Tach,
        }

        if cls not in class_map:
            raise NotImplementedError('Unknown class: "{0}"'.format(cls))

        return class_map[cls]

    @staticmethod
    def load(args):
        '''Load the configuration file.  Parsing occurs in three phases.
        In the first phase a factory method associated with each
        configuration file directive is invoked.  These factory
        methods generate more factory methods.  In the second
        phase the factory methods created in the first phase
        are invoked.  In the last phase a callback is invoked on
        each object created in phase two.  Typically the callback
        resolves references to other configuration file directives.'''

        factory_objs = {}
        objs = {}
        with open(args.input, 'r') as fd:
            for x in yaml.safe_load(fd.read()) or {}:

                # The top level elements all represent fans.
                x['class'] = 'fan'
                # Create factory object for this config file directive.
                factory = Everything.classmap(x['class'])
                obj = factory(**x)

                # For a given class of directive, validate the file
                # doesn't have any duplicate names.
                if exists(factory_objs, obj.cls, obj.name):
                    raise NotUniqueError(args.input, 'fan', obj.name)

                factory_objs.setdefault('fan', []).append(obj)
                objs.setdefault('fan', []).append(obj)

            for cls, items in list(factory_objs.items()):
                for obj in items:
                    # Add objects for template consumption.
                    obj.factory(objs)

            # Configuration file directives reference each other via
            # the name attribute; however, when rendered the reference
            # is just an array index.
            #
            # At this point all objects have been created but references
            # have not been resolved to array indices.  Instruct objects
            # to do that now.
            for cls, items in list(objs.items()):
                for obj in items:
                    obj.setup(objs)

        return Everything(**objs)

    def __init__(self, *a, **kw):
        self.fans = kw.pop('fan', [])
        self.policies = kw.pop('policy', [])
        self.sensors = kw.pop('sensor', [])
        super(Everything, self).__init__(**kw)

    def generate_cpp(self, loader):
        '''Render the template with the provided data.'''
        sys.stdout.write(
            self.render(
                loader,
                args.template,
                fans=self.fans,
                sensors=self.sensors,
                policies=self.policies,
                indent=Indent()))

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    valid_commands = {
        'generate-cpp': 'generate_cpp',
    }

    parser = ArgumentParser(
        description='Phosphor Fan Presence (PFP) YAML '
        'scanner and code generator.')

    parser.add_argument(
        '-i', '--input', dest='input',
        default=os.path.join(script_dir, 'example', 'example.yaml'),
        help='Location of config file to process.')
    parser.add_argument(
        '-t', '--template', dest='template',
        default='generated.mako.hpp',
        help='The top level template to render.')
    parser.add_argument(
        '-p', '--template-path', dest='template_search',
        default=os.path.join(script_dir, 'templates'),
        help='The space delimited mako template search path.')
    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=list(valid_commands.keys()),
        help='%s.' % ' | '.join(list(valid_commands.keys())))

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
        sys.stderr.write('{0}: {1}\n\n'.format(e.config, e.msg))
        raise
