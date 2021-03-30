#!/usr/bin/env python3

import os
import yaml
from mako.template import Template
import argparse

def main():
    parser = argparse.ArgumentParser(
        description="OpenPOWER error map code generator")

    parser.add_argument(
        '-i', '--errors_map_yaml', dest='errors_map_yaml',
        default='errors_watch.yaml', help='input errors watch yaml file to parse')
    args = parser.parse_args()

    with open(os.path.join(script_dir, args.errors_map_yaml), 'r') as fd:
        yamlDict = yaml.safe_load(fd)

        # Render the mako template
        template = os.path.join(script_dir, 'errors_map.mako.hpp')
        t = Template(filename=template)
        with open('errors_map.hpp', 'w') as fd:
            fd.write(
                t.render(
                    errDict=yamlDict))


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
