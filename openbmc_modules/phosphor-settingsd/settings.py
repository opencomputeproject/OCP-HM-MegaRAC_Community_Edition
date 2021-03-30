#!/usr/bin/env python3

import os
import yaml
from mako.template import Template
import argparse


def main():
    parser = argparse.ArgumentParser(
        description="Settings YAML parser and code generator")

    parser.add_argument(
        '-i', '--settings_yaml', dest='settings_yaml',
        default='settings_example.yaml', help='settings yaml file to parse')
    args = parser.parse_args()

    with open(os.path.join(script_dir, args.settings_yaml), 'r') as fd:
        yamlDict = yaml.safe_load(fd)

        # Render the mako template
        template = os.path.join(script_dir, 'settings_manager.mako.hpp')
        t = Template(filename=template)
        with open('settings_manager.hpp', 'w') as fd:
            fd.write(
                t.render(
                    settingsDict=yamlDict))


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
