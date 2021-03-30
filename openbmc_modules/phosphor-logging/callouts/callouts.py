#!/usr/bin/env python3

import os
import yaml
from mako.template import Template
import argparse


def main():
    parser = argparse.ArgumentParser(
        description="Callout code generator")

    parser.add_argument(
        '-i', '--callouts_yaml', dest='callouts_yaml',
        default='callouts-example.yaml', help='input callouts yaml')
    args = parser.parse_args()

    with open(os.path.join(script_dir, args.callouts_yaml), 'r') as fd:
        calloutsMap = yaml.safe_load(fd)

        # Render the mako template
        template = os.path.join(script_dir, 'callouts-gen.mako.hpp')
        t = Template(filename=template)
        with open('callouts-gen.hpp', 'w') as fd:
            fd.write(
                t.render(
                    calloutsMap=calloutsMap))


if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
