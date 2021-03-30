#!/usr/bin/env python3

import os
import sys
import yaml
import argparse
from mako.template import Template


def generate_cpp(inventory_yaml, output_dir):
    with open(inventory_yaml, 'r') as f:
        ifile = yaml.safe_load(f)
        if not isinstance(ifile, dict):
            ifile = {}

        # Render the mako template

        t = Template(filename=os.path.join(
                     script_dir,
                     "readfru.mako.cpp"))

        output_hpp = os.path.join(output_dir, "fru-read-gen.cpp")
        with open(output_hpp, 'w') as fd:
            fd.write(t.render(fruDict=ifile))


def main():

    valid_commands = {
        'generate-cpp': generate_cpp
    }
    parser = argparse.ArgumentParser(
        description="IPMI FRU map code generator")

    parser.add_argument(
        '-i', '--inventory_yaml', dest='inventory_yaml',
        default='example.yaml', help='input inventory yaml file to parse')

    parser.add_argument(
        "-o", "--output-dir", dest="outputdir",
        default=".",
        help="output directory")

    parser.add_argument(
        'command', metavar='COMMAND', type=str,
        choices=valid_commands.keys(),
        help='Command to run.')

    args = parser.parse_args()

    if (not (os.path.isfile(args.inventory_yaml))):
        sys.exit("Can not find input yaml file " + args.inventory_yaml)

    function = valid_commands[args.command]
    function(args.inventory_yaml, args.outputdir)

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main()
