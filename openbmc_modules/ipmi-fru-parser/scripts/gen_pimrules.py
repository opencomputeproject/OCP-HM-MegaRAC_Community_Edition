#!/usr/bin/env python3

'''Generate PIM rules for ipmi-fru-parser.
'''

import argparse
import os
import sys
import yaml
from mako.template import Template

tmpl = '''
description: >
    PIM rules for ipmi-fru-parser inventory objects.
events:

    - name: Host off at startup
      description: >
          Mark ipmi-fru-parser inventory items as cached when
          the host is off at startup.
      type: startup
      actions:
          - name: setProperty
            interface: ${cacheable_iface}
            property: ${cacheable_property}
            paths:
%for i in cacheable:
                - ${i}
%endfor
            value:
              type: ${cacheable_type}
              value: ${cacheable_cached}

    - name: Host off event
      description: >
          Mark ipmi-fru-parser inventory items as cached when
          the host goes away.
      type: match
      signatures:
          - type: signal
            interface: org.freedesktop.DBus.Properties
            path: ${off_path}
            member: PropertiesChanged
      filters:
          - name: propertyChangedTo
            interface: ${off_iface}
            property: ${off_property}
            value:
              value: ${off_value}
              type: ${off_type}
      actions:
          - name: setProperty
            interface: ${cacheable_iface}
            property: ${cacheable_property}
            paths:
%for i in cacheable:
                - ${i}
%endfor
            value:
              type: ${cacheable_type}
              value: ${cacheable_cached}

    - name: Host on at startup
      description: >
          Remove ipmi-fru-parser inventory items when the host is finished
          sending inventory items and the item is still marked as cached.
      type: startup
      filters:
          - name: propertyIs
            path: ${on_path}
            interface: ${on_iface}
            property: ${on_property}
            value:
              value: ${on_value}
              type: ${on_type}
      actions:
          - name: destroyObjects
            paths:
%for i in cacheable:
                - ${i}
%endfor
            conditions:
              - name: propertyIs
                interface: ${cacheable_iface}
                property: ${cacheable_property}
                value:
                  type: ${cacheable_type}
                  value: ${cacheable_cached}

    - name: Host on event
      description: >
          Remove ipmi-fru-parser inventory items when the host is finished
          sending inventory items and the item is still marked as cached.
      type: match
      signatures:
          - type: signal
            interface: org.freedesktop.DBus.Properties
            path: ${on_path}
            member: PropertiesChanged
      filters:
          - name: propertyChangedTo
            interface: ${on_iface}
            property: ${on_property}
            value:
              value: ${on_value}
              type: ${on_type}
      actions:
          - name: destroyObjects
            paths:
%for i in cacheable:
                - ${i}
%endfor
            conditions:
              - name: propertyIs
                interface: ${cacheable_iface}
                property: ${cacheable_property}
                value:
                  type: ${cacheable_type}
                  value: ${cacheable_cached}
'''


def get_cacheable_objs(yaml):
    cacheable = []

    for objdata in data.values():
        if not isinstance(objdata, dict):
            continue
        for path, ifaces in objdata.items():
            if not isinstance(ifaces, dict):
                continue

            if cacheable_iface_name in ifaces.keys():
                cacheable.append(path)

    return cacheable


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))

    parser = argparse.ArgumentParser(
        description='ipmi-fru-parser PIM rule generator.')
    parser.add_argument(
        '-o', '--output-dir', dest='outputdir',
        default='.', help='Output directory.')
    parser.add_argument(
        'inventory', metavar='INVENTORY', type=str,
        help='Path to inventory YAML.')

    args = parser.parse_args()

    with open(args.inventory, 'r') as fd:
        data = yaml.safe_load(fd.read())

    cacheable_iface_name = 'xyz.openbmc_project.Inventory.Decorator.Cacheable'
    target_file = os.path.join(args.outputdir, 'ipmi-fru-rules.yaml')
    cacheable = []

    if isinstance(data, dict):
        cacheable = get_cacheable_objs(data)
    if cacheable:
        with open(target_file, 'w') as out:
            out.write(
                Template(tmpl).render(
                    cacheable_iface=cacheable_iface_name,
                    cacheable_property='Cached',
                    cacheable_cached='true',
                    cacheable_type='boolean',
                    on_path='/xyz/openbmc_project/state/host0',
                    on_iface='xyz.openbmc_project.State.Boot.Progress',
                    on_property='BootProgress',
                    on_value='xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart',
                    on_type='string',
                    off_path='/xyz/openbmc_project/state/host0',
                    off_iface='xyz.openbmc_project.State.Host',
                    off_property='CurrentHostState',
                    off_value='xyz.openbmc_project.State.Host.Off',
                    off_type='string',
                    cacheable=cacheable))


# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
